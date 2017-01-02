#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <math.h>

#include <pluginapi.h>
#include <io.h>
#include <plugins.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/nanohttp.h>

#include <sqlite3.h>
#include "../commands/interface.h"
#include "../users/interface.h"
#include <toolbox/tb_string.h>

#include <curl/curl.h>

#ifndef PLUGIN_NAME
# define PLUGIN_NAME "gpxtell"
#endif

typedef struct {
	PluginInfo *info;
	int socket;
	sqlite3 *db;
	sqlite3_stmt *stmtFilterSelect;
	sqlite3_stmt *stmtInsertReviewer;
	sqlite3_stmt *stmtInsertState;
	sqlite3_stmt *stmtInsertPattern;
	EVENT_HANDLER *onignore;
} GpxTellPluginData;

typedef struct {
	GpxTellPluginData *dt;
	xmlParserCtxtPtr ctxt;
	xmlDocPtr doc;
} GpxTellGpx;

xmlNodePtr gpxtell_findNode(xmlNodePtr node, ...) {
        va_list ap;
        char *arg;
        int i = 50; /**< Recursion depth */

        va_start(ap, node);
                // Just for case, limit recursion depth.
                while (i-- > 0) {
                        arg = va_arg(ap, char *);
                        if (arg) {
                                // Find childs.
                                xmlNodePtr child = node->children;
                                while (child) {
                                        if (child->type == XML_ELEMENT_NODE && strcmp((char *)child->name, arg) == 0) {
                                                node = child;
                                                break;
                                        }

                                        child = child->next;
                                }

                                if (!child) {
                                        // Not found
                                        node = NULL;
                                        break;
                                }
                        } else {
                                // arg is NULL, end.
                                break;
                        }
                }
        va_end(ap);

        return node;
}

char *gpxtell_getNodeContent(xmlNodePtr node) {
        if (node) {
                if (node->children && node->children->type == XML_TEXT_NODE) {
                        return strdup((char *)node->children->content);
                } else {
                        return NULL;
                }
        } else {
                return NULL;
        }
}


char *gpxtell_format_coords(char *in_lat, char *in_lon) {
	double lat = atof(in_lat);
	double lon = atof(in_lon);

	char ns = (lat < 0)?'S':'N';
	char we = (lon < 0)?'W':'E';

	lat = fabs(lat);
	lon = fabs(lon);

	int lat_deg = lat;
	int lon_deg = lon;

	lat = (lat - lat_deg) * 60;
	lon = (lon - lon_deg) * 60; 

	char *out;
	asprintf(&out, "%c %02d°%02.3lf' %c %02d°%02.3lf'",
		ns, lat_deg, lat,
		we, lon_deg, lon
	);
	return out;
}

char *gpxtell_format_dt(double dt) {
	int need = (dt + 1) * 6 + 1;
	char *out = malloc(need * sizeof(char));
	char *ret = out;
	
	while (dt >= 1) {
		*out++ = '\xe2';
		*out++ = '\x97';
		*out++ = '\x8f';
		dt -= 1;
	}

	if (dt > 0) {
		*out++ = '\xe2';
		*out++ = '\x97';
		*out++ = '\x96';
	}

	*out = '\0';
	return ret;
}

struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;

	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);

	if (mem->memory == NULL) {
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = '\0';

	return realsize;
}

void gpxtell_process_gpx(GpxTellGpx *cli) {
	xmlNodePtr root = cli->doc->children;

	char *cache_name = gpxtell_getNodeContent(gpxtell_findNode(root, "wpt", "cache", "name", NULL));
	char *author = gpxtell_getNodeContent(gpxtell_findNode(root, "wpt", "cache", "placed_by", NULL));
	char *type = gpxtell_getNodeContent(gpxtell_findNode(root, "wpt", "cache", "type", NULL));
	char *diff = gpxtell_getNodeContent(gpxtell_findNode(root, "wpt", "cache", "difficulty", NULL));
	char *terr = gpxtell_getNodeContent(gpxtell_findNode(root, "wpt", "cache", "terrain", NULL));
	char *gcid = gpxtell_getNodeContent(gpxtell_findNode(root, "wpt", "name", NULL));
	char *reviewer = NULL;
	
	xmlNodePtr log = gpxtell_findNode(root, "wpt", "cache", "logs", "log", NULL);
	while (log) {
		if (log->type == XML_ELEMENT_NODE) {
			char *type = gpxtell_getNodeContent(gpxtell_findNode(log, "type", NULL));
			if (type && strcmp(type, "Publish Listing") == 0) {
				reviewer = gpxtell_getNodeContent(gpxtell_findNode(log, "finder", NULL));
				free(type);
				break;
			}
			free(type);
		}
		log = log->next;
	}

	char *lat = NULL;
	char *lon = NULL;

	struct MemoryStruct geoloc;
	geoloc.memory = NULL;
	geoloc.size = 0;

	// And finally, coordinates.
	xmlNodePtr node = gpxtell_findNode(root, "wpt", NULL);
	if (node) {
		lat = strdup((char *)xmlGetProp(node, BAD_CAST "lat"));
		lon = strdup((char *)xmlGetProp(node, BAD_CAST "lon"));

		// Geolocation
	        char url[255];
	        snprintf(url, 255, "https://gc.gcm.cz/geoloc.php?lat=%s&lon=%s", lat, lon);
		CURL *curl = curl_easy_init();
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&geoloc);
			curl_easy_perform(curl);
			curl_easy_cleanup(curl);
		}
	}

	char *fmt_coords = gpxtell_format_coords(lat, lon);
	//char *fmt_diff = gpxtell_format_dt(atof(diff));
	//char *fmt_terr = gpxtell_format_dt(atof(terr));
	
	// Try to match against filter...
	sqlite3_bind_text(cli->dt->stmtFilterSelect, 1, reviewer, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(cli->dt->stmtFilterSelect, 2, geoloc.memory, geoloc.size, SQLITE_TRANSIENT);
	sqlite3_bind_text(cli->dt->stmtFilterSelect, 3, cache_name, -1, SQLITE_TRANSIENT);

	if (sqlite3_step(cli->dt->stmtFilterSelect) != SQLITE_ROW) {
		irclib_message(cli->dt->info->irc, "#geocaching.cz",
			"Notify: %s (by %s) :: %s :: D %s T %s :: %s :: %s :: %s :: http://coord.info/%s",
			cache_name,
			author,
			type,
			diff,
			terr,
			fmt_coords,
			geoloc.memory,
			reviewer,
			gcid
		);
	} else {
		irclib_message(cli->dt->info->irc, "#despe",
			"Notify: %s (by %s) :: %s :: D %s T %s :: %s :: %s :: %s :: http://coord.info/%s",
			cache_name,
			author,
			type,
			diff,
			terr,
			fmt_coords,
			geoloc.memory,
			reviewer,
			gcid
		);
	}

	if (geoloc.memory) {
		free(geoloc.memory);
	}

	sqlite3_reset(cli->dt->stmtFilterSelect);

	if (cache_name) free(cache_name);
	if (author) free(author);
	if (type) free(type);
	if (diff) free(diff);
	if (terr) free(terr);
	//if (fmt_diff) free(fmt_diff);
	//if (fmt_terr) free(fmt_terr);
	if (gcid) free(gcid);
	if (lat) free(lat);
	if (lon) free(lon);
	if (fmt_coords) free(fmt_coords);
	if (reviewer) free(reviewer);
}

void gpxtell_read(Socket socket) {
#define BUFF_SIZE 4096
	char buffer[BUFF_SIZE];
	
	GpxTellGpx *cli = (GpxTellGpx *)socket->customData;

	int readed;
	while ((readed = read(socket->socketfd, buffer, BUFF_SIZE)) > 0) {
		write(0, buffer, readed);
		xmlParseChunk(cli->ctxt, buffer, readed, 0);
	}

	if (readed == 0) {
		socketpool_close(socket->pool, socket->socketfd);
	}
}

void gpxtell_closed(Socket socket) {
	GpxTellGpx *cli = (GpxTellGpx *)socket->customData;

	// Here, we have all the data we need.
	xmlParseChunk(cli->ctxt, NULL, 0, 1);

	cli->doc = cli->ctxt->myDoc;
	xmlFreeParserCtxt(cli->ctxt);

	if (cli->doc) {
		gpxtell_process_gpx(cli);
		xmlFreeDoc(cli->doc);
	}

	free(cli);
}

void gpxtell_accept_client(Socket socket) {
	GpxTellPluginData *dt = (GpxTellPluginData *)socket->customData;

	struct sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);

	int fd = accept(socket->socketfd, (struct sockaddr *)&addr, &addrlen);
	if (fd < 0) {
		printError(PLUGIN_NAME, "Unable to accept client: %s", strerror(errno));
		return;
	}

	GpxTellGpx *cli = malloc(sizeof(GpxTellGpx));
	cli->dt = dt;
	cli->ctxt = xmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL);
	cli->doc = NULL;

	if (!cli->ctxt) {
		printError(PLUGIN_NAME, "Unable to create libxml2 parser.\n");
		close(fd);
	} else {
		socketpool_add(dt->info->socketpool, fd, gpxtell_read, NULL, gpxtell_closed, cli);
	}
}

void gpxtell_ignore(EVENT *event) {
	GpxTellPluginData *dt = (GpxTellPluginData *)event->handlerData;
	Commands_Event *cmd = (Commands_Event *)event->customData;
	IRCEvent_Message *message = cmd->replyData;

	if (!eq(cmd->command, "gpxignore")) return;

	char *source = message->channel != NULL ?
		message->channel : message->address->nick;

	PluginInfo *usersPlugin = plugins_getinfo("users");

	char *host = irclib_construct_addr(message->address);
	UsersList list;

	if (usersPlugin != NULL) {
		UsersPluginData *uData = (UsersPluginData *)usersPlugin->customData;
		list = users_match_host(uData->usersdb, host);
	}

	if (usersPlugin == NULL || users_get_priv(list, NULL, "gpxignore") > 0) {
		if (strncmp(cmd->params, "reviewer ", strlen("reviewer ")) == 0) {
			char *pattern = cmd->params + strlen("reviewer ");

			if (!eq(pattern, "")) {
				sqlite3_bind_text(dt->stmtInsertReviewer, 1, pattern, -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(dt->stmtInsertReviewer, 2, message->address->nick, -1, SQLITE_TRANSIENT);

				if (sqlite3_step(dt->stmtInsertReviewer) == SQLITE_DONE) {
					irclib_message(message->sender, source, "%s: Ignoruji vse od reviewera %s.", message->address->nick, pattern);
				} else {
					printError(PLUGIN_NAME, "Query error: %s", sqlite3_errmsg(dt->db));
				}

				sqlite3_reset(dt->stmtInsertReviewer);
			}
		}

		if (strncmp(cmd->params, "state ", strlen("state ")) == 0) {
			char *pattern = cmd->params + strlen("state ");

			if (!eq(pattern, "")) {
				sqlite3_bind_text(dt->stmtInsertState, 1, pattern, -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(dt->stmtInsertState, 2, message->address->nick, -1, SQLITE_TRANSIENT);

				if (sqlite3_step(dt->stmtInsertState) == SQLITE_DONE) {
					irclib_message(message->sender, source, "%s: Ignoruji vse ze statu %s.", message->address->nick, pattern);
				} else {
					printError(PLUGIN_NAME, "Query error: %s", sqlite3_errmsg(dt->db));
				}
				sqlite3_reset(dt->stmtInsertReviewer);
			}
		}

		if (strncmp(cmd->params, "name ", strlen("name ")) == 0) {
			char *pattern = cmd->params + strlen("name ");

			if (!eq(pattern, "")) {
				sqlite3_bind_text(dt->stmtInsertPattern, 1, pattern, -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(dt->stmtInsertPattern, 2, message->address->nick, -1, SQLITE_TRANSIENT);

				if (sqlite3_step(dt->stmtInsertPattern) == SQLITE_DONE) {
					irclib_message(message->sender, source, "%s: Ignoruji vse odpovidajici nazvu %s.", message->address->nick, pattern);
				} else {
					printError(PLUGIN_NAME, "Query error: %s", sqlite3_errmsg(dt->db));
				}
				sqlite3_reset(dt->stmtInsertPattern);
			}
		}

	} // users_get_priv
	else {
		printError(PLUGIN_NAME, "User %s does not have privilege gpxignore.", message->address->nick);
	}

	if (usersPlugin != NULL) {
		users_free_list(list);
	}

	if (host != NULL) {
		free(host);
	}
}

void PluginInit(PluginInfo *info) {
	info->name = "GPX parser for #geocaching.cz";
	info->author = "Niximor";
	info->version = "1.0.0";

	GpxTellPluginData *dt = malloc(sizeof(GpxTellPluginData));
	memset(dt, 0, sizeof(GpxTellPluginData));
	dt->info = info;
	info->customData = dt;

	dt->socket = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);

	int val = 1;
	setsockopt(dt->socket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	xmlNanoHTTPInit();

	int port = config_getvalue_int(info->config, PLUGIN_NAME":port", 6666);
	struct sockaddr_in6 bindAddress = {
		.sin6_family = AF_INET6,
		.sin6_addr = in6addr_any,
		.sin6_port = htons(port)
	};

	if (bind(dt->socket, (struct sockaddr *)&bindAddress, sizeof(bindAddress)) < 0) {
		printError(PLUGIN_NAME, "Unable to bind: %s", strerror(errno));
	} else {
		if (listen(dt->socket, 10) < 0) {
			printError(PLUGIN_NAME, "Unable to start listening: %s", strerror(errno));
		} else {
			socketpool_add(info->socketpool, dt->socket, gpxtell_accept_client, NULL, NULL, dt);
			printError(PLUGIN_NAME, "Server startup successfull.");
		}
	}

	int result;
	if ((result = sqlite3_open_v2(config_getvalue_string(info->config, PLUGIN_NAME":dbfile", "gpxtell.db"), &dt->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL)) == SQLITE_OK) {
		if (sqlite3_exec(dt->db, "CREATE TABLE IF NOT EXISTS `filter` (`reviewer` VARCHAR(255), `state` VARCHAR(255), `pattern` VARCHAR(255), `date` INT, `nick` VARCHAR(255))", NULL, NULL, NULL) != SQLITE_OK) {
			printError(PLUGIN_NAME, "Query exception: %s", sqlite3_errmsg(dt->db));
		}

		if (sqlite3_prepare(
				dt->db,
				"SELECT 1 FROM `filter` WHERE `reviewer` = ? OR ? LIKE `state` OR ? LIKE `pattern`",
				-1,
				&dt->stmtFilterSelect,
				NULL
			) != SQLITE_OK)
		{
			printError(PLUGIN_NAME, "Query exception: %s", sqlite3_errmsg(dt->db));
		}

		if (sqlite3_prepare(
				dt->db,
				"INSERT INTO `filter` (`reviewer`, `nick`, `date`) VALUES (?, ?, date('now'))",
				-1,
				&dt->stmtInsertReviewer,
				NULL
			) != SQLITE_OK)
		{
			printError(PLUGIN_NAME, "Query exception: %s", sqlite3_errmsg(dt->db));
		}

		if (sqlite3_prepare(
				dt->db,
				"INSERT INTO `filter` (`state`, `nick`, `date`) VALUES (?, ?, date('now'))",
				-1,
				&dt->stmtInsertState,
				NULL
			) != SQLITE_OK)
		{
			printError(PLUGIN_NAME, "Query exception: %s", sqlite3_errmsg(dt->db));
		}

		if (sqlite3_prepare(
				dt->db,
				"INSERT INTO `filter` (`pattern`, `nick`, `date`) VALUES (?, ?, date('now'))",
				-1,
				&dt->stmtInsertPattern,
				NULL
			) != SQLITE_OK)
		{
			printError(PLUGIN_NAME, "Query exception: %s", sqlite3_errmsg(dt->db));
		}

		dt->onignore = events_addEventListener(info->events, "oncommand", gpxtell_ignore, dt);
		printError(PLUGIN_NAME, "Bound ignore command.");
	} else {
		printError(PLUGIN_NAME, "Unable to open SQLite database: %s", sqlite3_errmsg(dt->db));
	}

}

void PluginDone(PluginInfo *info) {
	GpxTellPluginData *dt = (GpxTellPluginData *)info->customData;
	
	if (dt->onignore) {
		events_removeEventListener(dt->onignore);
	}

	socketpool_close(info->socketpool, dt->socket);

	if (dt->stmtFilterSelect) {
		sqlite3_finalize(dt->stmtFilterSelect);
	}

	if (dt->stmtInsertReviewer) {
		sqlite3_finalize(dt->stmtInsertReviewer);
	}

	if (dt->stmtInsertState) {
		sqlite3_finalize(dt->stmtInsertState);
	}

	if (dt->stmtInsertPattern) {
		sqlite3_finalize(dt->stmtInsertPattern);
	}

	if (dt->db) {
		sqlite3_close(dt->db);
	}
	free(dt);

	xmlNanoHTTPCleanup();
}

void PluginDeps(char **deps) {
	*deps = NULL;
}

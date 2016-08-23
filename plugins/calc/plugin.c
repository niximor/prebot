#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <pluginapi.h>
#include <sqlite3.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "../users/interface.h"
#include <plugins.h>

#define PLUG_NAME "Calc"

typedef struct {
	EVENT_HANDLER *onmessage;
	sqlite3 *db;
	sqlite3_stmt *stmtQuery;
	sqlite3_stmt *stmtInsert;
	sqlite3_stmt *stmtUpdate;
	sqlite3_stmt *stmtDelete;
} CalcPluginData;

void calc_message(EVENT *event) {
	IRCEvent_Message *message = (IRCEvent_Message *)event->customData;
	CalcPluginData *plugData = event->handlerData;

	int result;

	if (strncmp(message->message, "??", 2) == 0) {
		// It is calc request...
		
		// Trim leading spaces
		char *msgtrim = message->message + 2;

		while (*msgtrim != '\0' && isspace(*msgtrim)) {
			msgtrim++;
		}

		// Do not accept empty calc
		if (*msgtrim == '\0') return;

		// Need to create copy to be able to modify the msg.
		msgtrim = strdup(msgtrim);

		// Trim trailing spaces
		for (int x = strlen(msgtrim) - 1; x >= 0; x--) {
			if (isspace(msgtrim[x])) {
				msgtrim[x] = '\0';
			} else {
				break;
			}
		}


		// Assignment
		char *valueBegin;
		if ((valueBegin = strstr(msgtrim, "=")) != NULL) {
			char *name;
			char *value;

			int canUpdate = 0; // Noone can update by default.

			// If user has calcadmin privilege, allow it to modify
			// the entrys.
			PluginInfo *usersPlugin = plugins_getinfo("users");
			if (usersPlugin != NULL) {
				UsersPluginData *uData = (UsersPluginData *)usersPlugin->customData;
				char *host = irclib_construct_addr(message->address);
			    UsersList list = users_match_host(uData->usersdb, host);
				if (users_get_priv(list, NULL, "calcadmin")) {
					canUpdate = 1;
				}
				users_free_list(list);
				free(host);
			}

			size_t namelen = valueBegin - msgtrim;

			// Trim right name. From left it is already trimmed from initial trim.
			for (int i = namelen - 1; i >= 0; i--) {
				if (isspace(msgtrim[i])) {
					namelen--;
				} else {
					break;
				}
			}
			name = msgtrim;
			name[namelen] = '\0';

			// Trim left the value. From right it is already trimmed from initial trim.
			value = valueBegin + 1;
			while (*value != '\0' && isspace(*value)) {
				value++;
			}

			sqlite3_bind_text(plugData->stmtQuery, 1, name, -1, SQLITE_TRANSIENT);
			if (sqlite3_step(plugData->stmtQuery) == SQLITE_ROW) {
				if (canUpdate) {
					// Already exists, update or delete.
					
					if (*value == '\0') {
						// Delete the value.
						sqlite3_bind_text(plugData->stmtDelete, 1, name, -1, SQLITE_TRANSIENT);
						if (sqlite3_step(plugData->stmtDelete) == SQLITE_DONE) {
							irclib_message(
								message->sender,
								message->channel,
								"%s: Netusim, co %s je.",
								message->address->nick,
								name
							);
						} else {
							irclib_message(
								message->sender,
								message->channel,
								"%s: %s neumim zapomenout. :(",
								message->address->nick,
								name
							);
							printError(PLUG_NAME, "SQLite: %s", sqlite3_errmsg(plugData->db));
						}
						sqlite3_reset(plugData->stmtDelete);
					} else {
						// Update it.
						sqlite3_bind_text(plugData->stmtUpdate, 1, value, -1, SQLITE_TRANSIENT);
						sqlite3_bind_text(plugData->stmtUpdate, 2, message->address->nick, -1, SQLITE_TRANSIENT);
						sqlite3_bind_text(plugData->stmtUpdate, 3, message->channel, -1, SQLITE_TRANSIENT);
						sqlite3_bind_int64(plugData->stmtUpdate, 4, time(NULL));
						sqlite3_bind_text(plugData->stmtUpdate, 5, name, -1, SQLITE_TRANSIENT);

						if (sqlite3_step(plugData->stmtUpdate) == SQLITE_DONE) {
							irclib_message(
								message->sender,
								message->channel,
								"%s: Ok, %s je nyni %s.",
								message->address->nick,
								name,
								value
							);
						} else {
							irclib_message(
								message->sender,
								message->channel,
								"%s: Tohle si zapamatovat odmitam :(",
								message->address->nick
							);
							printError(PLUG_NAME, "SQLite: %s", sqlite3_errmsg(plugData->db));
						}

						sqlite3_reset(plugData->stmtUpdate);
					}
				} else {
					irclib_message(
						message->sender,
						message->channel,
						"%s: %s uz znam :(",
						message->address->nick,
						name
					);
				}
			} else {
				if (*name != '\0' && *value != '\0') {
					// Does not exists, create.
					sqlite3_bind_text(plugData->stmtInsert, 1, name, -1, SQLITE_TRANSIENT);
					sqlite3_bind_text(plugData->stmtInsert, 2, value, -1, SQLITE_TRANSIENT);
					sqlite3_bind_text(plugData->stmtInsert, 3, message->address->nick, -1, SQLITE_TRANSIENT);
					sqlite3_bind_text(plugData->stmtInsert, 4, message->channel, -1, SQLITE_TRANSIENT);
					sqlite3_bind_int64(plugData->stmtInsert, 5, time(NULL));

					if ((result = sqlite3_step(plugData->stmtInsert)) == SQLITE_DONE) {
						irclib_message(
							message->sender,
							message->channel,
							"%s: Ok, %s je nyni %s.",
							message->address->nick,
							name,
							value
						);
					} else {
						irclib_message(
							message->sender,
							message->channel,
							"%s: Tohle si zapamatovat odmitam :(",
							message->address->nick
						);
					}

					sqlite3_reset(plugData->stmtInsert);
				} else {
					// Do not insert empty value
					irclib_message(
						message->sender,
						message->channel,
						"%s: Tohle si zapamatovat odmitam :(",
						message->address->nick
					);
				}
			}
			sqlite3_reset(plugData->stmtQuery);

		// Query
		} else {
			sqlite3_bind_text(plugData->stmtQuery, 1, msgtrim, -1, SQLITE_TRANSIENT);
			if (sqlite3_step(plugData->stmtQuery) == SQLITE_ROW) {
				// Got data!
				irclib_message(
					message->sender,
					message->channel,
					"%s: %s je %s",
					message->address->nick,
					sqlite3_column_text(plugData->stmtQuery, 0),
					sqlite3_column_text(plugData->stmtQuery, 1)
				);
			} else {
				irclib_message(
					message->sender,
					message->channel,
					"%s: Netusim, co %s je.",
					message->address->nick,
					msgtrim
				);
			}

			// Prepare the statement for next use.
			sqlite3_reset(plugData->stmtQuery);
		}

		free(msgtrim);
	}
}

void PluginInit(PluginInfo *info) {
	CalcPluginData *plugData = malloc(sizeof(CalcPluginData));
	info->name = PLUG_NAME;
	info->author = "Niximor";
	info->version = "1.0.1";

	int result;
	if ((result = sqlite3_open_v2(config_getvalue_string(info->config, "calc:dbfile", "calc.db"), &plugData->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL)) == SQLITE_OK) {
		// Create the tables
		if (sqlite3_exec(plugData->db, "CREATE TABLE IF NOT EXISTS \"calc\" (\"name\" TEXT PRIMARY KEY, \"value\" TEXT, \"user\" TEXT, \"channel\" TEXT, \"date\" INT)", NULL, NULL, NULL) != SQLITE_OK) {
			printError(info->name, "Cannot create calc table: %s", sqlite3_errmsg(plugData->db));
		}

		if (sqlite3_exec(plugData->db, "CREATE INDEX IF NOT EXISTS \"calc_name_idx\" ON \"calc\" (\"name\")", NULL, NULL, NULL)) {
			printError(info->name, "Cannot create calc index: %s", sqlite3_errmsg(plugData->db));
		}

		// Prepare the statements
		if (sqlite3_prepare(
				plugData->db,
				"SELECT \"name\", \"value\", \"user\", \"channel\", \"date\" FROM \"calc\" WHERE \"name\" = ? COLLATE NOCASE",
				-1,
				&plugData->stmtQuery,
				NULL
			) != SQLITE_OK) {

			printError(info->name, "Query exception: %s", sqlite3_errmsg(plugData->db));
		}

		if (sqlite3_prepare(plugData->db, "INSERT INTO \"calc\" (\"name\", \"value\", \"user\", \"channel\", \"date\") VALUES (?, ?, ?, ?, ?)", -1, &plugData->stmtInsert, NULL) != SQLITE_OK) {
			printError(info->name, "Query exception: %s", sqlite3_errmsg(plugData->db));
		}

		if (sqlite3_prepare(plugData->db, "UPDATE \"calc\" SET \"value\" = ?, \"user\" = ?, \"channel\" = ?, \"date\" = ? WHERE \"name\" = ? COLLATE NOCASE", -1, &plugData->stmtUpdate, NULL) != SQLITE_OK) {
			printError(info->name, "Query exception: %s", sqlite3_errmsg(plugData->db));
		}

		if (sqlite3_prepare(plugData->db, "DELETE FROM \"calc\" WHERE \"name\" = ? COLLATE NOCASE", -1, &plugData->stmtDelete, NULL) != SQLITE_OK) {
			printError(info->name, "Query exception: %s", sqlite3_errmsg(plugData->db));
		}

		plugData->onmessage = events_addEventListener(info->events, "onchannelmessage", calc_message, plugData);
		info->customData = plugData;
	} else {
		printError(info->name, "Unable to open database: %s (code: %d)", sqlite3_errmsg(plugData->db), result);
		free(plugData);
		info->customData = NULL;
	}
}

void PluginDone(PluginInfo *info) {
	if (info->customData != NULL) {
		CalcPluginData *plugData = (CalcPluginData *)info->customData;

		events_removeEventListener(plugData->onmessage);

		sqlite3_finalize(plugData->stmtQuery);
		sqlite3_finalize(plugData->stmtInsert);
		sqlite3_finalize(plugData->stmtUpdate);
		sqlite3_finalize(plugData->stmtDelete);

		sqlite3_close(plugData->db);

		free(plugData);
	}
}

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#define PLUG_NAME "Last seen"

#include <pluginapi.h>
#include <toolbox/tb_string.h>
#include <sqlite3.h>
#include <time.h>

#include "../commands/interface.h"

typedef struct {
	sqlite3 *db;
	sqlite3_stmt *stmtQuery;
	sqlite3_stmt *stmtInsert;
	sqlite3_stmt *stmtUpdate;

	EVENT_HANDLER *onjoin;
	EVENT_HANDLER *onpart;
	EVENT_HANDLER *onquit;
	EVENT_HANDLER *oncommand;
} SeenCustomData;

void updateLastSeen(SeenCustomData *plugData, IRCLib_Host *address, char *channel, char *action, char *reason) {
	sqlite3_bind_text(plugData->stmtQuery, 1, address->nick, -1, SQLITE_TRANSIENT);

	char *host = irclib_construct_addr(address);

	int result;
	if ((result = sqlite3_step(plugData->stmtQuery)) == SQLITE_ROW) {
		// Got row, update it.
		sqlite3_bind_int(plugData->stmtUpdate, 7, sqlite3_column_int(plugData->stmtQuery, 0)); // ID
		sqlite3_bind_text(plugData->stmtUpdate, 1, address->nick, -1, SQLITE_TRANSIENT);

		sqlite3_bind_text(plugData->stmtUpdate, 2, host, -1, SQLITE_TRANSIENT);

		if (channel != NULL) {
			sqlite3_bind_text(plugData->stmtUpdate, 3, channel, -1, SQLITE_TRANSIENT);
		} else {
			// Bind last channel
			sqlite3_bind_text(plugData->stmtUpdate, 3, (const char *)sqlite3_column_text(plugData->stmtQuery, 3), -1, SQLITE_TRANSIENT);
		}

		sqlite3_bind_text(plugData->stmtUpdate, 4, action, -1, SQLITE_TRANSIENT);
		sqlite3_bind_int64(plugData->stmtUpdate, 5, time(NULL));
		sqlite3_bind_text(plugData->stmtUpdate, 6, reason, -1, SQLITE_TRANSIENT);

		if (sqlite3_step(plugData->stmtUpdate) != SQLITE_DONE) {
			printError(PLUG_NAME, "SQLite: %s", sqlite3_errmsg(plugData->db));
		}
		sqlite3_reset(plugData->stmtUpdate);
	} else if (result == SQLITE_DONE) {
		// No row, insert new entry.
		sqlite3_bind_text(plugData->stmtInsert, 1, address->nick, -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(plugData->stmtInsert, 2, host, -1, SQLITE_TRANSIENT);

		if (channel != NULL) {
			sqlite3_bind_text(plugData->stmtInsert, 3, channel, -1, SQLITE_TRANSIENT);
		} else {
			sqlite3_bind_null(plugData->stmtInsert, 3);
		}

		sqlite3_bind_text(plugData->stmtInsert, 4, action, -1, SQLITE_TRANSIENT);
		sqlite3_bind_int64(plugData->stmtInsert, 5, time(NULL));
		sqlite3_bind_text(plugData->stmtInsert, 6, reason, -1, SQLITE_TRANSIENT);

		if (sqlite3_step(plugData->stmtInsert) != SQLITE_DONE) {
			printError(PLUG_NAME, "SQLite: %s", sqlite3_errmsg(plugData->db));
		}
		sqlite3_reset(plugData->stmtInsert);
	} else {
		printError(PLUG_NAME, "SQLite: %s", sqlite3_errmsg(plugData->db));
	}

	sqlite3_reset(plugData->stmtQuery);
	free(host);
}

// User joined
void seen_join(EVENT *event) {
	SeenCustomData *plugData = (SeenCustomData *)event->handlerData;
	IRCEvent_JoinPart *join = (IRCEvent_JoinPart *)event->customData;

	updateLastSeen(plugData, join->address, join->channel, "is on", "");
}

// User parted
void seen_part(EVENT *event) {
	SeenCustomData *plugData = (SeenCustomData *)event->handlerData;
	IRCEvent_JoinPart *join = (IRCEvent_JoinPart *)event->customData;

	// Scan if user is not in any other channel...
	IRCLib_User user = irclib_find_user(join->sender->userStorage, join->address->nick);	
	if (user != NULL) {
		IRCLib_UserChannel chan = user->first;
		if (chan != NULL) {
			while (chan != NULL) {
				if (!eq(chan->channel->name, join->channel)) {
					updateLastSeen(plugData, join->address, chan->channel->name, "is on", "");
					break;
				}

				chan = chan->next;
			}

			// No other channel was found for the user.
			if (chan == NULL) {
				updateLastSeen(plugData, join->address, join->channel, "part", join->reason);
			}
		} else {
			printError(PLUG_NAME, "User is lost (no channel info, but still in db.");
			updateLastSeen(plugData, join->address, join->channel, "part", join->reason);
		}
	} else {
		// User is not on channel.
		updateLastSeen(plugData, join->address, join->channel, "part", join->reason);
	}
}

// User quited
void seen_quit(EVENT *event) {
	SeenCustomData *plugData = (SeenCustomData *)event->handlerData;
	IRCEvent_Quit *quit = (IRCEvent_Quit *)event->customData;

	IRCLib_User user = irclib_find_user(quit->sender->userStorage, quit->address->nick);
	if (user != NULL && user->first != NULL) {
		updateLastSeen(plugData, quit->address, user->first->channel->name, "quit", quit->message);
	} else {
		// User has quit, keep channel from last info.
		updateLastSeen(plugData, quit->address, NULL, "quit", quit->message);
	}
}

void seen_command(EVENT *event) {
	SeenCustomData *plugData = (SeenCustomData *)event->handlerData;
	Commands_Event *cmd = (Commands_Event *)event->customData;
	IRCEvent_Message *message = cmd->replyData;

	if (eq(cmd->command, "seen")) {
		if (cmd->source != CMD_CHANNEL) return;

		char *nick = cmd->params;
		char *space = strstr(nick, " ");
		if (space != NULL) *space = '\0';

		// User asks for himself :)
		if (eq(nick, message->address->nick)) {
			irclib_message(
				message->sender,
				message->channel,
				"%s: Yep, I can see you.",
				message->address->nick
			);
			return;
		}

		// Try to find user in active users.
		IRCLib_User user = irclib_find_user(message->sender->userStorage, nick);
		if (user != NULL && user->first != NULL) {
			// User is online somewhere...
			IRCLib_UserChannel chan = user->first;
			irclib_message(
				message->sender,
				message->channel,
				"%s: %s is on %s right now.",
				message->address->nick,
				nick,
				chan->channel->name
			);

		// User is not known on any channel. So query db.
		} else {
			sqlite3_bind_text(plugData->stmtQuery, 1, nick, -1, SQLITE_TRANSIENT);
			if (sqlite3_step(plugData->stmtQuery) == SQLITE_ROW) {
				const char *reason = (const char *)sqlite3_column_text(plugData->stmtQuery, 4);
				if (eq(reason, "is on")) {
					// This should not happen...
					irclib_message(
						message->sender,
						message->channel,
						"%s: %s should be on %s right now, but I cannot see him there.",
						message->address->nick,
						nick,
						sqlite3_column_text(plugData->stmtQuery, 3)
					);
				} else if (eq(reason, "quit") || eq(reason, "part")) {
					time_t tajm = sqlite3_column_int64(plugData->stmtQuery, 5);
					struct tm *event_tm = localtime(&tajm);

					char *msg;
					if (eq(reason, "quit")) {
						msg = "quiting from";
					} else {
						msg = "parting";
					}

					irclib_message(
						message->sender,
						message->channel,
						"%s: %s was last seen %s %s stating \"%s\", %d.%d.%d %d:%02d:%02d.",
						message->address->nick,
						nick,
						msg,
						sqlite3_column_text(plugData->stmtQuery, 3),
						sqlite3_column_text(plugData->stmtQuery, 6),
						event_tm->tm_mday,
						event_tm->tm_mon + 1,
						event_tm->tm_year + 1900,
						event_tm->tm_hour,
						event_tm->tm_min,
						event_tm->tm_sec

					);
				} else {
					irclib_message(
						message->sender,
						message->channel,
						"%s: I remember %s, but don't have an idea how he'd left (aka this should not happen).",
						message->address->nick,
						nick
					);
				}
			} else {
				irclib_message(
					message->sender,
					message->channel,
					"%s: I cannot remember seeing %s.",
					message->address->nick,
					nick
				);
			}
			sqlite3_reset(plugData->stmtQuery);
		}
	}
}

void PluginInit(PluginInfo *info) {
	info->name = PLUG_NAME;
	info->author = "Niximor";
	info->version = "1.0.0";

	SeenCustomData *plugData = malloc(sizeof(SeenCustomData));

	if (sqlite3_open_v2(config_getvalue_string(info->config, "seen:dbfile", "seen.db"), &plugData->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) == SQLITE_OK) {
		// Create the db
		if (sqlite3_exec(plugData->db, "CREATE TABLE IF NOT EXISTS \"seen\" (\"id\" INTEGER PRIMARY KEY AUTOINCREMENT, \"nick\" TEXT, \"address\" TEXT, \"channel\" TEXT NULL, \"action\" TEXT, \"reason\" TEXT, \"time\" INT)", NULL, NULL, NULL) != SQLITE_OK) {
			printError(PLUG_NAME, "Cannot create table seen: %s", sqlite3_errmsg(plugData->db));
		};

		if (sqlite3_exec(plugData->db, "CREATE INDEX IF NOT EXISTS \"seen_nick_idx\" ON \"seen\" (\"nick\")", NULL, NULL, NULL) != SQLITE_OK) {
			printError(PLUG_NAME, "Cannot create index on seen: %s", sqlite3_errmsg(plugData->db));
		}

		if (sqlite3_prepare(plugData->db, "SELECT \"id\", \"nick\", \"address\", \"channel\", \"action\", \"time\", \"reason\" FROM \"seen\" WHERE \"nick\" = ? COLLATE NOCASE", -1, &plugData->stmtQuery, NULL) != SQLITE_OK) {
			printError(PLUG_NAME, "Query exception: %s", sqlite3_errmsg(plugData->db));
		}

		if (sqlite3_prepare(plugData->db, "INSERT INTO \"seen\" (\"nick\", \"address\", \"channel\", \"action\", \"time\", \"reason\") VALUES (?, ?, ?, ?, ?, ?)", -1, &plugData->stmtInsert, NULL) != SQLITE_OK) {
			printError(PLUG_NAME, "Query exception: %s", sqlite3_errmsg(plugData->db));
		}

		if (sqlite3_prepare(plugData->db, "UPDATE \"seen\" SET \"nick\" = ?, \"address\" = ?, \"channel\" = ?, \"action\" = ?, \"time\" = ?, \"reason\" = ? WHERE \"id\" = ?", -1, &plugData->stmtUpdate, NULL) != SQLITE_OK) {
			printError(PLUG_NAME, "Query exception: %s", sqlite3_errmsg(plugData->db));
		}

		// Bind events
		plugData->onjoin = events_addEventListener(info->events, "onjoin", seen_join, plugData);
		plugData->onpart = events_addEventListener(info->events, "onpart", seen_part, plugData);
		plugData->onquit = events_addEventListener(info->events, "onquited", seen_quit, plugData);
		plugData->oncommand = events_addEventListener(info->events, "oncommand", seen_command, plugData);
		
		info->customData = plugData;
	} else {
		free(plugData);
		info->customData = NULL;
	}
}

void PluginDone(PluginInfo *info) {
	if (info->customData != NULL) {
		SeenCustomData *plugData = info->customData;

		sqlite3_finalize(plugData->stmtQuery);
		sqlite3_finalize(plugData->stmtInsert);
		sqlite3_finalize(plugData->stmtUpdate);
		sqlite3_close(plugData->db);

		events_removeEventListener(plugData->onjoin);
		events_removeEventListener(plugData->onpart);
		events_removeEventListener(plugData->onquit);
		events_removeEventListener(plugData->oncommand);

		free(plugData);
	}
}

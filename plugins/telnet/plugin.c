/**
 *  This file is part of the IRCbot project.
 *  Copyright (C) 2007  Michal Kuchta
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _GNU_SOURCE

// Standard libraries
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

// My includes
#include <io.h>
#include <plugins.h>
#include <tokenizer.h>
#include <toolbox/wordwrap.h>
#include <toolbox/tb_string.h>

// Plugins API
#include <pluginapi.h>

// This plugin interface
#include "interface.h"

#ifndef PLUGIN_NAME
# define PLUGIN_NAME "telnet"
#endif

/**
 * Sends message to telnet client.
 * @param client Telnet client
 * @param format Format of message
 * @param ap Variable arguments list
 */
void telnet_vsend(TelnetClient client, char *format, va_list ap) {
	va_list myap;
	char *buffer;

	va_copy(myap, ap);
	vasprintf(&buffer, format, myap);
	va_end(myap);

	// If client is currently on prompt, clear it
	if (client->state == TCS_PROMPT) {
		telnet_action(client, TA_ClearLine, 0, 0);
	}

	// Write message
	if (client->windowWidth > 0) {
		ww_start(buffer, client->windowWidth, outputOffset, outputLength);
			client->send(client, buffer + outputOffset, outputLength);
			client->send(client, "\r\n", 2);
		ww_end();
	} else {
		client->send(client, buffer, strlen(buffer));
		client->send(client, "\r\n", 2);
	}

	// If client was in prompt, send the prompt back
	if (client->state == TCS_PROMPT) {
		telnet_prompt(client);
	}

	if (buffer) {
		free(buffer);
	}
} // telnet_vsend

/**
 * Sends message to telnet client. Variable arguments list are formated by
 * format parameter.
 * @param client Telnet client
 * @param format Format of message.
 */
void telnet_send(TelnetClient client, char *format, ...) {
	va_list ap;
	va_start(ap, format);
	telnet_vsend(client, format, ap);
	va_end(ap);
} // telnet_send

/**
 * Broadcast message to all telnet clients. Variable arguments are formatted
 * by format, which uses standard printf formatting.
 * @param format Format of message.
 */
void telnet_broadcast(char *format, ...) {
	va_list ap;
	va_start(ap, format);

	// ToDo: Broadcast to all clients
	PluginInfo *info = plugins_getinfo(PLUGIN_NAME);
	if (info != NULL) {
		TelnetPluginData *plugData = (TelnetPluginData *)info->customData;

		TelnetClient client = plugData->firstClient;
		while (client != NULL) {
			telnet_vsend(client, format, ap);
			client = client->next;
		}
	}

	va_end(ap);
} // telnet_broadcast

/**
 * Perform an terminal action
 * @param client Telnet client
 * @param action Action to be performed (see Telnet_Action enum for details
 * @param param1 First parameter of action
 * @param param2 Second parameter of action
 * @return Return true if action is supported and was performed by driver,
 *   false otherwise.
 */
bool telnet_action(TelnetClient client, Telnet_Action action, int param1,
	int param2) {

	if (client->action != NULL) {
		return client->action(client, action, param1, param2);
	} else {
		// Client has no action callback defined.
		return false;
	}
} // telnet_action

/**
 * Displays prompt and sets client to "awaiting command" state.
 * @param client Telnet client
 */
void telnet_prompt(TelnetClient client) {
	if (!(client->opts & TC_NOPROMPT)) {
		char *buffer;

		// Get current directory.
		HTVAL hCD = kvp_get(client->kvp, "cd");
		char *cd = NULL;
		if (hCD != NULL) {
			cd = htval_get_string(hCD);
			if (*cd == '\0') {
				cd = NULL;
			}
		}

		asprintf(&buffer, "%s%s%s> %s", client->plugData->info->irc->nickname,
			(cd != NULL) ? " " : "",
			(cd != NULL) ? cd  : "",
			dynastring_getstring(client->recvbuffer));

		if (buffer) {
			client->send(client, buffer, strlen(buffer));
			free(buffer);
		}
	}
	client->state = TCS_PROMPT;
} // telnet_prompt

/**
 * Sends MOTD to client
 * @param client Telnet client
 */
void telnet_motd(TelnetClient client) {
	client->state = TCS_PROCESSING;
	if (kvp_isset(client->kvp, "user")) {
		telnet_send(client, "Welcome to %s telnet service, %s!",
			client->plugData->info->irc->nickname,
			htval_get_string(kvp_get(client->kvp, "user")));
	} else {
		telnet_send(client, "Welcome to %s telnet service!",
			client->plugData->info->irc->nickname);
	}
	telnet_prompt(client);
} // telnet_motd

/**
 * Disconnect client and free client's structure.
 * @param client Telnet client
 */
void telnet_disconnect(TelnetClient client) {
	if (client->state == TCS_PROCESSING) {
		client->state = TCS_DISCONNECTED;
		return;
	}

	client->disconnected(client);

	PluginInfo *info = plugins_getinfo(PLUGIN_NAME);
	TelnetPluginData *plugData = (TelnetPluginData *)info->customData;

	// Remove client from chain of clients
	if (client->prev != NULL) {
		client->prev->next = client->next;
	} else {
		plugData->firstClient = client->next;
	}
	if (client->next != NULL) {
		client->next->prev = client->prev;
	} else {
		plugData->lastClient = client->prev;
	}

	dynastring_free(client->recvbuffer);

	// Double free protection
	if (client->kvp) {
		kvp_free(client->kvp);
		client->kvp = NULL;
	}

	free(client);
} // telnet_disconnect

/**
 * Process telnet command from string that is in receive buffer.
 * @param client Telnet client
 */
void telnet_process_command(TelnetClient client) {
	client->state = TCS_PROCESSING;

	string cmd = dynastring_init();
	HTVAL hCD = kvp_get(client->kvp, "cd");
	if (hCD) {
		dynastring_appendstring(cmd, htval_get_string(hCD));

		// If we are at top level directory.
		if (htval_get_string(hCD)[0] != '\0') {
			dynastring_appendstring(cmd, " ");
		}
	}
	dynastring_append(cmd, client->recvbuffer);

	TOKENS tok = tokenizer_tokenize(
		dynastring_getstring(cmd), ' ');

	// Go to top level directory and don't process the command...
	if (eq(dynastring_getstring(client->recvbuffer), "..")) {
		// Go only one level up.
		if (hCD) {
			// Can go up... probably.
			TOKENS tok2 = tokenizer_tokenize(htval_get_string(hCD), ' ');
			string newcd = dynastring_init();
			for (size_t i = 0; i < tok2->count - 1; i++) {
				if (i != 0) {
					dynastring_appendchar(newcd, ' ');
				}

				dynastring_appendstring(newcd, tokenizer_gettok(tok2, i));
			}

			telnet_cd(client, dynastring_getstring(newcd));
			dynastring_free(newcd);
			tokenizer_free(tok2);
		}

		tokenizer_free(tok);
		dynastring_clear(client->recvbuffer);
		dynastring_free(cmd);
		telnet_prompt(client);
		return;
	}

	Telnet_Command evt = {
		.command = strdup(tokenizer_gettok(tok, 0)),
		.params = strdup(tokenizer_gettok_skipleft(tok, 1)),
		.client = client,
		.setInteractive = false,
		.callback = NULL,
		.handled = false
	};

	events_fireEvent(client->plugData->info->events, "ontelnetcmd", &evt);

	if (!evt.handled) {
		telnet_send(client, "Unknown command: `%s`",
			dynastring_getstring(cmd));
	}

	if (evt.setInteractive && evt.callback != NULL) {
		client->state = TCS_INTERACTIVE;
		client->dataCallback = evt.callback;
	}

	free(evt.command);
	free(evt.params);
	tokenizer_free(tok);

	dynastring_clear(client->recvbuffer);
	dynastring_free(cmd);

	if (client->state == TCS_DISCONNECTED) {
		telnet_disconnect(client);
		return;
	}

	if (client->state != TCS_INTERACTIVE) {
		telnet_prompt(client);
	}
} // telnet_process_command

/**
 * Process telnet key without echo
 * @param client Telnet client
 * @param ch Received key
 */
void telnet_receiveWithoutEcho(TelnetClient client, KeyCode ch) {
	switch (ch) {
		case KEY_UP:
			// ToDo: History
			break;

		case KEY_DOWN:
			// ToDo: History
			break;

		case KEY_LEFT:
			dynastring_seek(client->recvbuffer, -1, SEEK_CUR);
			break;

		case KEY_RIGHT:
			dynastring_seek(client->recvbuffer, 1, SEEK_CUR);
			break;

		case KEY_HOME:
			dynastring_seek(client->recvbuffer, 0, SEEK_SET);
			break;

		case KEY_END:
			dynastring_seek(client->recvbuffer, 0, SEEK_END);
			break;

		case KEY_BACKSPACE:
			dynastring_delete(client->recvbuffer, -1);
			break;

		case KEY_DELETE:
			dynastring_delete(client->recvbuffer, 1);;
			break;

		default:
			if (ch >= 32 && ch <= 255) {
				dynastring_appendchar(client->recvbuffer, ch);
			}
			break;
	}
} // telnet_receiveWithoutEcho

/**
 * Process telnet key with echo
 * @param client Telnet client
 * @param ch Received key
 */
void telnet_receiveWithEcho(TelnetClient client, KeyCode ch) {
	int seek = 0;
	int rewrite = 0;

	switch (ch) {
		case KEY_UP:
			// ToDo: History
			break;

		case KEY_DOWN:
			// ToDo: History
			break;

		case KEY_LEFT:
			seek = dynastring_seek(client->recvbuffer, -1, SEEK_CUR);
			break;

		case KEY_RIGHT:
			seek = dynastring_seek(client->recvbuffer, 1, SEEK_CUR);
			break;

		case KEY_HOME:
			seek = dynastring_seek(client->recvbuffer, 0, SEEK_SET);
			break;

		case KEY_END:
			seek = dynastring_seek(client->recvbuffer, 0, SEEK_END);
			break;

		case KEY_BACKSPACE:
			seek = -1 * dynastring_delete(client->recvbuffer, -1);
			rewrite = -1 * seek;
			break;

		case KEY_DELETE:
			rewrite = dynastring_delete(client->recvbuffer, 1);;
			break;

		default:
			if (ch >= 32 && ch <= 255) {
				dynastring_appendchar(client->recvbuffer, ch);
				if (client->opts & TC_ECHO) {
					client->send(client, &ch, 1);
					string s = client->recvbuffer;
					if (dynastring_getpos(s) != dynastring_getlength(s)) {
						size_t write_len =
							dynastring_getlength(s) - dynastring_getpos(s);
						client->send(client,
							dynastring_getstring(s) + dynastring_getpos(s),
							write_len);
						char *move = malloc(write_len * sizeof(char));
						memset(move, 8, write_len);
						client->send(client, move, write_len);
						free(move);
					}
				}
			}
			break;
	}

	if (seek < 0) {
		char *move = malloc(-1 * seek * sizeof(char));
		memset(move, 8, -1 * seek);
		client->send(client, move, -1 * seek);
		free(move);
	}
	if (seek > 0) {
		string s = client->recvbuffer;
		client->send(client,
			dynastring_getstring(s) + dynastring_getpos(s) - seek, seek);
	}

	if (rewrite) {
		string s = client->recvbuffer;

		int movedChars = dynastring_getlength(s) - dynastring_getpos(s);

		// Rewrite string from current position to end.
		client->send(client, dynastring_getstring(s) + dynastring_getpos(s),
			movedChars);

		// Fill deleted chars with space.
		char *move = malloc(rewrite * sizeof(char));
		memset(move, ' ', rewrite);
		client->send(client, move, rewrite);
		movedChars += rewrite;
		free(move);

		// Move cursor back to position
		move = malloc(movedChars * sizeof(char));
		memset(move, 8, movedChars);
		client->send(client, move, movedChars);
		free(move);
	}
} // telnet_receivewithecho

/**
 * Handles incomming data from driver
 * @param client Telnet client
 * @param ch Received char
 */
void telnet_received(TelnetClient client, KeyCode ch) {
	if (client->state == TCS_INTERACTIVE) {
		if (client->dataCallback != NULL) {
			// If data callback returns true, set client to normal state.
			if (client->dataCallback(client, ch)) {
				client->state = TCS_PROCESSING;
				telnet_prompt(client);
			}
		}
		return;
	}

	if (ch == KEY_ENTER) {
		client->send(client, "\r\n", 2);
		telnet_process_command(client);
	} else {
		telnet_receiveWithEcho(client, ch);
	}
} // telnet_received

/**
 * Add new client to list of telnet clients
 * @param socketdata Client socket data
 * @param send Send function
 * @param disconnected Function to properly close client's connection, which
 *   must do the driver.
 */
TelnetClient telnet_add_client(void *socketdata, TelnetSendData send,
	TelnetPerformAction action, TelnetDisconnected disconnected) {

	PluginInfo *info = plugins_getinfo(PLUGIN_NAME);
	if (info) {
		TelnetPluginData *plugData = (TelnetPluginData *)info->customData;

		TelnetClient client = malloc(sizeof(struct sTelnetClient));
		client->plugData = plugData;
		client->id = ++plugData->lastId;

		// Driver data
		client->socketdata = socketdata;

		// Callbacks
		client->send = send;
		client->action = action;
		client->disconnected = disconnected;

		client->opts = TC_NORMAL;
		client->recvbuffer = dynastring_init();

		// Add client to chain
		client->next = NULL;
		client->prev = plugData->lastClient;
		plugData->lastClient = client;

		if (client->prev != NULL) {
			client->prev->next = client;
		} else {
			plugData->firstClient = client;
		}

		client->windowWidth = 0;
		client->windowHeight = 0;

		client->kvp = kvp_init();

		Telnet_Command evt = {
			.client = client,
			.command = NULL,
			.params = NULL,
			.setInteractive = false,
			.callback = NULL,
			.handled = false
		};
		events_fireEvent(info->events, "ontelnetconnected", &evt);

		if (evt.setInteractive && evt.callback != NULL) {
			client->state = TCS_INTERACTIVE;
			client->dataCallback = evt.callback;
		} else {
			telnet_motd(client);
		}

		return client;
	} else {
		return NULL;
	}
} // telnet_add_client

/**
 * Change current context to another directory.
 */
void telnet_cd(TelnetClient client, char *tree) {
	kvp_set(client->kvp, "cd", htval_string(tree));
} // telnet_cd

/**
 * Initialize plugin.
 * @param info Plugin info, where this function must fill in some informations
 */
void PluginInit(PluginInfo *info) {
	info->name = "Telnet service";
	info->author = "Niximor";
	info->version = "1.0.0";

	events_addEvent(info->events, "ontelnetcmd");
	events_addEvent(info->events, "ontelnetconnected");

	TelnetPluginData *plugData = malloc(sizeof(TelnetPluginData));
	plugData->info = info;
	plugData->firstClient = NULL;
	plugData->lastClient = NULL;
	plugData->lastId = 0;
	info->customData = plugData;
} // PluginInit

/**
 * Close plugin
 * @param info Plugin info, which this function may use to get some
 * informations it may need.
 */
void PluginDone(PluginInfo *info) {
	TelnetPluginData *plugData = (TelnetPluginData *)info->customData;
	free(plugData);
} // PluginDone

/**
 * Sends bye-bye message to all clients, and closes their connections
 * @param info Plugin info
 */
void PluginBeforeUnload(PluginInfo *info) {
	telnet_broadcast("Telnet server shutdown.");

	// Disconnect all clients
	TelnetPluginData *plugData = (TelnetPluginData *)info->customData;
	TelnetClient client = plugData->firstClient;
	TelnetClient next;
	while (client != NULL) {
		next = client->next;
		telnet_disconnect(client);
		client = next;
	}
} // PluginBeforeUnload

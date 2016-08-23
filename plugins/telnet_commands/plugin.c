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

// Standard libraries
#include <stdlib.h>
#include <string.h>

// Linux includes
#include <time.h>

// Plugins API
#include <pluginapi.h>

// My includes
#include <tokenizer.h>
#include <version.h>
#include <plugins.h>
#include <main.h>
#include <toolbox/linkedlist.h>
#include <toolbox/tb_string.h>
#include <htable/keyvalpair.h>
#include <htable/htval.h>

// This plugin interface
#include "interface.h"

#ifndef PLUGIN_NAME
# define PLUGIN_NAME "telnet_commands"
#endif

/**
 * Handles `system *` commands
 * @param client Telnet client that sent the command
 * @param params Parameters of command
 */
bool telnet_commands_system(TelnetClient client, char *params) {
	TOKENS tok = tokenizer_tokenize(params, ' ');
	char *subcommand = tokenizer_gettok(tok, 0);

	// system version
	// Get system version
	if (strcmp(subcommand, "version") == 0) {
		telnet_send(client, "%s v%s", APP_NAME, APP_VERSION);
		goto _telnet_commands_system_handled;
	}

	// system uptime
	// Print system uptime
	if (strcmp(subcommand, "uptime") == 0) {
		telnet_send(client, "Uptime: %d seconds",
			time(NULL) - system_boottime());
		goto _telnet_commands_system_handled;
	}

	// system quit
	// Quit application
	if (strcmp(subcommand, "quit") == 0) {
		system_quit();
		goto _telnet_commands_system_handled;
	}

	// system lsmod
	// List loaded modules
	if (strcmp(subcommand, "lsmod") == 0) {
		Plugin plugin = loadedPlugins->first;
		while (plugin != NULL) {
			telnet_send(client, "- %s v%s by %s (%s)", plugin->info->name,
				plugin->info->version, plugin->info->author, plugin->name);

			plugin = plugin->next;
		}

		goto _telnet_commands_system_handled;
	}

	// system load
	// Load plugin by name
	if (strcmp(subcommand, "load") == 0) {
		char *pluginName = tokenizer_gettok(tok, 1);
		switch (plugins_load(pluginName)) {
			case PLUG_OK:
				telnet_send(client, "Plugin %s has been successfully loaded.",
					pluginName);
				break;

			case PLUG_FILE_NOT_FOUND:
				telnet_send(client, "Plugin %s was not found in plugin "
					"directories.", pluginName);
				break;

			case PLUG_DEPENDENCY:
				telnet_send(client, "Plugin %s depends on plugin that doesn't "
					"exists.", pluginName);
				break;

			case PLUG_ALREADY_LOADED:
				telnet_send(client, "Plugin %s is already loaded.",
					pluginName);
				break;

			default:
				telnet_send(client, "Plugin load error.");
				break;
		}
		goto _telnet_commands_system_handled;
	}

	// system unload
	// Unload plugin by name
	if (strcmp(subcommand, "unload") == 0) {
		char *pluginName = tokenizer_gettok(tok, 1);
		switch (plugins_unload(pluginName)) {
			case PLUG_OK:
				telnet_send(client, "Plugin %s was successfully unloaded.",
					pluginName);
				break;

			case PLUG_FILE_NOT_FOUND:
				telnet_send(client, "Plugin %s is not loaded.", pluginName);
				break;

			default:
				telnet_send(client, "Plugin unload error.");
				break;
		}
		goto _telnet_commands_system_handled;
	}

	// system
	// Print supported `system *` commands
	if (strcmp(subcommand, "help") == 0) {
		telnet_send(client, "- system version ..................... "
			"Print system version");
		telnet_send(client, "- system uptime ...................... "
			"Print system uptime");
		telnet_send(client, "- system quit ........................ "
			"Quit application");
		telnet_send(client, "- system lsmod ....................... "
			"List loaded modules");
		telnet_send(client, "- system load <module> ............... "
			"Load module by name");
		telnet_send(client, "- system unload <module> ............. "
			"Unload module by name");

		goto _telnet_commands_system_handled;
	}

	if (eq(subcommand, "")) {
		telnet_cd(client, "system");
		goto _telnet_commands_system_handled;
	}

	// Unhandled command
	tokenizer_free(tok);
	return false;

	// Handled command
	_telnet_commands_system_handled:
	tokenizer_free(tok);
	return true;
} // telnet_commands_system

/**
 * Handles `telnet *` commands
 * @param client Telnet client that sent the command
 * @param params Parameters of command
 */
bool telnet_commands_telnet(TelnetClient client, char *params) {
	TOKENS tok = tokenizer_tokenize(params, ' ');
	char *subcommand = tokenizer_gettok(tok, 0);

	// telnet quit
	// Disconnects user from telnet session.
	if (strcmp(subcommand, "quit") == 0) {
		telnet_send(client, "Bye.");
		telnet_disconnect(client);
		goto _telnet_commands_telnet_handled;
	}

	// telnet who
	// List users connected to telnet.
	if (strcmp(subcommand, "who") == 0) {
		TelnetClient cli = client->plugData->firstClient;
		while (cli != NULL) {
			telnet_send(client, "- Client ID %d, driver %s", cli->id,
				htval_get_string(kvp_get(cli->kvp, "driver")));
			ll_loop(cli->kvp, kv) {
				if (!eq(kv->key, "driver")) {
					switch (htval_type(kv->value)) {
						case HT_INT:
							telnet_send(client, "   - %s: %d", kv->key,
								htval_get_int(kv->value));
							break;

						case HT_FLOAT:
							telnet_send(client, "   - %s: %d", kv->key,
								htval_get_float(kv->value));
							break;

						case HT_STRING:
							telnet_send(client, "   - %s: %s", kv->key,
								htval_get_string(kv->value));
							break;

						default:
							break;
					}
				}
			}

			cli = cli->next;
		}
		goto _telnet_commands_telnet_handled;
	}

	// telnet
	// Print help
	if (eq(subcommand, "help")) {
		telnet_send(client, "telnet quit .......................... "
			"Disconnects you from telnet.");
		telnet_send(client, "telnet who ........................... "
			"List users connected to telnet.");
		goto _telnet_commands_telnet_handled;
	}

	if (eq(subcommand, "")) {
		telnet_cd(client, "telnet");
		goto _telnet_commands_telnet_handled;
	}

	// Unhandled command
	tokenizer_free(tok);
	return false;

	// Handled command
	_telnet_commands_telnet_handled:
	tokenizer_free(tok);
	return true;
} // telnet_commands_system

/**
 * Handles `irc *` commands
 * @param client Telnet client that sent the command
 * @param params Parameters of command
 */
bool telnet_commands_irc(TelnetClient client, char *params) {
	TOKENS tok = tokenizer_tokenize(params, ' ');
	char *subcommand = tokenizer_gettok(tok, 0);
	IRCLib_Connection *irc = client->plugData->info->irc;

	// irc quit [<reason>]
	// Disconnect from IRC.
	if (strcmp(subcommand, "quit") == 0) {
		irclib_quit(irc, "%s", tokenizer_gettok_skipleft(tok, 1));
		goto _telnet_commands_irc_handled;
	}

	// irc connect [<server> [<port>]]
	if (strcmp(subcommand, "connect") == 0) {
		// Modify server and port if necessary.
		if (tok->count >= 2) {
			// First, disconnect from old server.
			if (irc->status == IRC_CONNECTED || irc->status == IRC_CONNECTING) {
				irclib_quit(irc, "changing servers...");
				irclib_close(irc);
			}

			// Have server
			free(irc->hostname);
			irc->hostname = strdup(tokenizer_gettok(tok, 1));
			
			if (tok->count >= 3) {
				// Have port
				irc->port = atoi(tokenizer_gettok(tok, 2));
			}
		}

		// Connect back to server.
		if (irc->status == IRC_DISCONNECTED || irc->status == IRC_QUITED) {
			irclib_connect(irc);
		} else {
			telnet_send(client, "Already connected to IRC.");
		}

		goto _telnet_commands_irc_handled;
	}

	if (strcmp(subcommand, "status") == 0) {
		switch (irc->status) {
			case IRC_DISCONNECTED:
				telnet_send(client, "IRC is disconnected.");
				break;

			case IRC_CONNECTING:
				telnet_send(client, "Connecting...");
				break;

			case IRC_CONNECTED:
				telnet_send(client, "Connected.");
				telnet_send(client, "Server: %s", irc->hostname);
				telnet_send(client, "Port: %d", irc->port);
				break;

			case IRC_QUITED:
				telnet_send(client, "Quited.");
				break;
		}

		goto _telnet_commands_irc_handled;
	}

	// irc send <channel> <message>
	// Send message to user or channel
	if (strcmp(subcommand, "send") == 0) {
		if (tok->count >= 3) {
			irclib_message(irc, tokenizer_gettok(tok, 1), "%s",
				tokenizer_gettok_skipleft(tok, 2));
		} else {
			telnet_send(client, "Usage: irc send <channel> <message>");
		}
		goto _telnet_commands_irc_handled;
	}

	// irc join <channel>
	// Join to channel
	if (strcmp(subcommand, "join") == 0) {
		if (tok->count == 2) {
			irclib_join(irc, tokenizer_gettok(tok, 1));
		} else {
			telnet_send(client, "Usage: irc join <channel>");
		}
		goto _telnet_commands_irc_handled;
	}

	// irc part <channel> [<reason>]
	// Part from channel
	if (strcmp(subcommand, "part") == 0) {
		if (tok->count >= 2) {
			irclib_part(irc, tokenizer_gettok(tok, 1), "%s",
				tokenizer_gettok_skipleft(tok, 2));
		} else {
			telnet_send(client, "Usage: irc part <channel> [<reason>]");
		}
		goto _telnet_commands_irc_handled;
	}

	// irc kick <channel> <nick> [<reason>]
	// Kick user from channel
	if (strcmp(subcommand, "kick") == 0) {
		if (tok->count >= 3) {
			irclib_kick(irc, tokenizer_gettok(tok, 1), tokenizer_gettok(tok, 2),
				"%s", tokenizer_gettok_skipleft(tok, 3));
		} else {
			telnet_send(client, "Usage: irc kick <channel> <nick> [<reason>]");
		}
		goto _telnet_commands_irc_handled;
	}

	// irc mode [<channel>] <mode string>
	// Sets mode
	if (strcmp(subcommand, "mode") == 0) {
		if (tok->count >= 2) {
			irclib_mode(irc, "%s", tokenizer_gettok_skipleft(tok, 1));
		} else {
			telnet_send(client, "Usage: irc mode [<channel>] <mode string>");
		}
		goto _telnet_commands_irc_handled;
	}

	// irc raw <message>
	// Send raw message
	if (strcmp(subcommand, "raw") == 0) {
		if (tok->count >= 2) {
			irclib_sendraw(irc, "%s", tokenizer_gettok_skipleft(tok, 1));
		} else {
			telnet_send(client, "Usage: irc raw <message>");
		}
		goto _telnet_commands_irc_handled;
	}

	// irc channels
	// List joined channels
	if (strcmp(subcommand, "channels") == 0) {
		ll_loop(irc->channelStorage, channel) {
			telnet_send(client, "- %s (%d users)", channel->name,
				irclib_count_users(channel));
		}

		goto _telnet_commands_irc_handled;
	}

	// irc users <channel/"global">
	// List users on channel or in global storage
	if (strcmp(subcommand, "users") == 0) {
		if (tok->count == 2) {
			char *channel = tokenizer_gettok(tok, 1);
			if (strcmp(channel, "global") == 0) {
				telnet_send(client, "Global users storage:");
				string channellist = dynastring_init();
				ll_loop(irc->userStorage, user) {
					dynastring_clear(channellist);
					ll_loop(user, uchan) {
						if (uchan != user->first) {
							dynastring_appendstring(channellist, ", ");
						}
						dynastring_appendstring(channellist,
							uchan->channel->name);
					}

					telnet_send(client, "- %s (%s!%s@%s) on %s",
						user->host->nick, user->host->nick, user->host->user,
						user->host->host, dynastring_getstring(channellist));
				}
				dynastring_free(channellist);
			} else {
				IRCLib_Channel chan = irclib_find_channel(irc->channelStorage,
					channel);
				if (chan != NULL) {
					telnet_send(client, "Users on channel %s", channel);
					string channellist = dynastring_init();
					ll_loop(chan, user) {
						dynastring_clear(channellist);
						ll_loop(user->userinfo, uchan) {
							if (uchan != user->userinfo->first) {
								dynastring_appendstring(channellist, ", ");
							}
							dynastring_appendstring(channellist,
								uchan->channel->name);
						}

						telnet_send(client, "- %s (%s!%s@%s) on %s",
							user->userinfo->host->nick,
							user->userinfo->host->nick,
							user->userinfo->host->user,
							user->userinfo->host->host,
							dynastring_getstring(channellist));
					}
					dynastring_free(channellist);
				} else {
					telnet_send(client, "You are not on channel %s.", channel);
				}
			}
		} else {
			telnet_send(client, "Usage: irc users <channel/\"global\">");
		}

		goto _telnet_commands_irc_handled;
	}

	// irc
	// Print help message
	if (strcmp(subcommand, "help") == 0) {
		telnet_send(client, "- irc quit [<reason>] ................ "
			"Disconnect from IRC.");
		telnet_send(client, "- irc connect [<server> [<port>]] .... "
			"Connect to IRC, optionally modifying current server.");
		telnet_send(client, "- irc status ......................... "
			"Print current connection info.");
		telnet_send(client, "- irc send <channel/user> <message> .. "
			"Send message to channel or user.");
		telnet_send(client, "- irc join <channel> ................. "
			"Join to channel.");
		telnet_send(client, "- irc part <channel> [<reason>] ...... "
			"Part from channel.");
		telnet_send(client, "- irc kick <channel> <nick> [<reason>] "
			"Kick user from channel.");
		telnet_send(client, "- irc mode <mode string> ............. "
			"Send mode string");
		telnet_send(client, "- irc raw <message> .................. "
			"Send raw message to server.");
		telnet_send(client, "- irc channels ....................... "
			"List of joined channels.");
		telnet_send(client, "- irc users <channel> ................ "
			"List users on channel.");
		telnet_send(client, "- irc users global ................... "
			"List all users that I know");
		goto _telnet_commands_irc_handled;
	}

	if (strcmp(subcommand, "") == 0) {
		telnet_cd(client, "irc");
		goto _telnet_commands_irc_handled;
	}

	tokenizer_free(tok);
	return false;

	_telnet_commands_irc_handled:
	tokenizer_free(tok);
	return true;
} // telnet_commands_irc

/**
 * Telnet basic commands handler
 * @param event Event data
 */
void telnet_commands_handler(EVENT *event) {
	Telnet_Command *command_event = (Telnet_Command *)event->customData;
	char *command = command_event->command;
	char *params = command_event->params;

	// System commands
	if (eq(command, "system")) {
		if (telnet_commands_system(command_event->client, params)) {
			command_event->handled = true;
		}
	}

	// Telnet commands
	if (eq(command, "telnet")) {
		if (telnet_commands_telnet(command_event->client, params)) {
			command_event->handled = true;
		}
	}

	// IRC commands
	if (eq(command, "irc")) {
		if (telnet_commands_irc(command_event->client, params)) {
			command_event->handled = true;
		}
	}

	// Clear screen
	if (eq(command, "cls")) {
		telnet_action(command_event->client, TA_ClearScreen, 0, 0);
		command_event->handled = true;
	}
} // telnet_commands_handler

/**
 * Initialize plugin.
 * @param info Plugin info, where this function must fill in some informations
 */
void PluginInit(PluginInfo *info) {
	info->name = "Telnet basic commands";
	info->author = "Niximor";
	info->version = "1.0.0";

	TelnetCommandsPluginData *plugData =
		malloc(sizeof(TelnetCommandsPluginData));

	plugData->info = info;
	info->customData = plugData;

	plugData->ontelnetcmd = events_addEventListener(info->events,
		"ontelnetcmd", telnet_commands_handler, NULL);
} // PluginInit

/**
 * Close plugin
 * @param info Plugin info, which this function may use to get some
 *   informations it may need.
 */
void PluginDone(PluginInfo *info) {
	TelnetCommandsPluginData *plugData =
		(TelnetCommandsPluginData *)info->customData;

	events_removeEventListener(plugData->ontelnetcmd);

	free(plugData);
} // PluginDone

/**
 * Get list of dependencies
 * @param deps Dependencies
 */
void PluginDeps(char **deps) {
	*deps = "telnet"; // Depends on telnet
} // PluginDeps

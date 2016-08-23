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
#include <string.h>

// Plugins API
#include <pluginapi.h>

// This plugin interface
#include "interface.h"

// My libraries
#include <irclib/irclib.h>

#ifndef PLUGIN_NAME
# define PLUGIN_NAME "users"
#endif

/**
 * Event handler triggered when user joins channel
 * @param event Event data
 */
void users_ircjoin(EVENT *event) {
	UsersPluginData *plugData = (UsersPluginData *)event->handlerData;
	IRCEvent_JoinPart *join = (IRCEvent_JoinPart *)event->customData;

	char *host = irclib_construct_addr(join->address);
	UsersList list = users_match_host(plugData->usersdb, host);

	size_t plen = strlen(join->sender->userPrefixes);
	for (size_t p = 0; p < plen; p++) {
		char privname[2] = { join->sender->userPrefixes[p], '\0' };
		if (users_get_priv(list, join->channel, privname) > 0) {
			irclib_mode(join->sender, "%s +%c %s", join->channel,
				join->sender->userPrefixes[p],
				join->address->nick);
			break;
		}
	}

	users_free_list(list);
	free(host);
} // users_ircjoin

/**
 * Initialize plugin.
 * @param info Plugin info, where this function must fill in some informations
 */
void PluginInit(PluginInfo *info) {
	info->name = "Users";
	info->author = "Niximor";
	info->version = "1.0.0";

	UsersPluginData *plugData = malloc(sizeof(UsersPluginData));
	plugData->info = info;

	plugData->usersdb = config_parse(
		config_getvalue_string(info->config, "users:dbfile", "./users.db"));

	if (plugData->usersdb != NULL) {
		plugData->ontelnetcmd = events_addEventListener(info->events,
			"ontelnetcmd", users_telnetcommands, plugData);
		plugData->ontelnetconnected = events_addEventListener(info->events,
			"ontelnetconnected", users_telnetconnected, plugData);
		plugData->onjoin = events_addEventListener(info->events,
			"onjoin", users_ircjoin, plugData);
	}

	events_addEvent(info->events, "onusersdbchanged");

	info->customData = plugData;
} // PluginInit

/**
 * Close plugin
 * @param info Plugin info, which this function may use to get some
 *   informations it may need.
 */
void PluginDone(PluginInfo *info) {
	UsersPluginData *plugData = (UsersPluginData *)info->customData;

	events_removeEventListener(plugData->ontelnetcmd);
	events_removeEventListener(plugData->ontelnetconnected);
	events_removeEventListener(plugData->onjoin);
	config_free(plugData->usersdb);

	free(plugData);
} // PluginDone

/**
 * Get list of dependencies
 * @param deps Dependencies
 */
void PluginDeps(char **deps) {
	*deps = "telnet";
} // PluginDeps

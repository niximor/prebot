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

// Plugins API
#include <pluginapi.h>

// This plugin interface
#include "interface.h"

#ifndef PLUGIN_NAME
# define PLUGIN_NAME "autojoin"
#endif

/**
 * Event handler fired when connected to IRC.
 * @param event onconnected event data
 */
void autojoin_onconnected(EVENT *event) {
	AutojoinPluginData *plugData = (AutojoinPluginData *)event->handlerData;
	IRCEvent_Notify *eventData = (IRCEvent_Notify *)event->customData;

	size_t count = config_getvalue_count(plugData->info->config,
		PLUGIN_NAME":channel");

	for (size_t i = 0; i < count; i++) {
		char *channel = config_getvalue_array_string(plugData->info->config,
			PLUGIN_NAME":channel", i, NULL);

		if (channel != NULL && channel[0] != '\0') {
			printError(PLUGIN_NAME, "Joining channel %s", channel);
			irclib_join(eventData->sender, channel);
		}
	}
} // autojoin_onconnected

/**
 * Event handler that rejoins the channel after being kicked.
 */
void autojoin_onkicked(EVENT *event) {
	IRCEvent_Kick *eventData = (IRCEvent_Kick *)event->customData;
	irclib_join(eventData->sender, eventData->channel);
} // autojoin_onkicked

/**
 * Initialize plugin.
 * @param info Plugin info, where this function must fill in some informations
 */
void PluginInit(PluginInfo *info) {
	info->name = "Autojoin";
	info->author = "Niximor";
	info->version = "1.0.0";

	AutojoinPluginData *plugData = malloc(sizeof(AutojoinPluginData));
	plugData->info = info;

	plugData->onconnected = events_addEventListener(info->events,
		"onconnected", autojoin_onconnected, plugData);
	plugData->onkicked = events_addEventListener(info->events,
		"onkicked", autojoin_onkicked, plugData);

	info->customData = plugData;
} // PluginInit

/**
 * Close plugin
 * @param info Plugin info, which this function may use to get some
 *   informations it may need.
 */
void PluginDone(PluginInfo *info) {
	AutojoinPluginData *plugData = (AutojoinPluginData *)info->customData;

	// Remove onconnect event
	events_removeEventListener(plugData->onconnected);

	free(plugData);
} // PluginDone

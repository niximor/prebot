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
 *
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

// Plugins API
#include <pluginapi.h>

// This plugin interface
#include "interface.h"

// My libraries
#include <irclib/irclib.h>
#include <toolbox/tb_rand.h>
#include <toolbox/tb_string.h>
#include <toolbox/tb_num.h>
#include <config/config.h>

#ifndef PLUGIN_NAME
# define PLUGIN_NAME "dummytalk"
#endif

bool dummytalk_frog_remkiss(Timer tm) {
	DummytalkPluginData *data = (DummytalkPluginData *)tm->customData;

	if (data->kissNick != NULL) {
		free(data->kissNick);
		data->kissNick = NULL;
	}

	return false;
	(void)tm;
}

/**
 * Handle frog special case
 */
void dummytalk_handle_frog(IRCEvent_Message *message, DummytalkPluginData *data) {
	int currentStatus = 0;
	char *recipient = (message->channel != NULL)?
		message->channel:
		message->address->nick;

	data->threshold += data->threshold;

	if (data->threshold > 1024) {
		if (data->threshold > 65535) {
			irclib_kick(message->sender, message->channel, message->address->nick, "spam warning!");
		}
		return;
	}

	// Does not work on query
	if (message->channel == NULL) return;

	FILE *f = fopen(config_getvalue_string(data->info->config, "dummytalk:frogstamp", "frog.txt"), "r");
	if (f) {
		fscanf(f, "%d", &currentStatus);
		fclose(f);
	}

	currentStatus = max(currentStatus + tb_rand(-2, 10), 0);
	if (currentStatus < 0) currentStatus = 0;

	if (currentStatus > config_getvalue_int(data->info->config, "frog:threshold", 100)) {
		// Got frog
		irclib_message(message->sender, recipient, "\x03" "0,2  ~   \x03" "8,2" "\\**/"  "\x03" "0,2"  "    ~  \x03  Polib me...");
		irclib_message(message->sender, recipient, "\x03" "0,2    ~  \x03" "0,03"  "00"   "\x03" "0,2" "   ~    \x03  ... a splnim");
		irclib_message(message->sender, recipient, "\x03" "0,2 ~    \x03" "0,3"  "(--)"  "\x03" "0,2"  "     ~ \x03  ti jedno prani...");
		irclib_message(message->sender, recipient, "\x03" "0,2   ~ \x03" "0,3"  "( || )" "\x03" "0,2"   "  ~   \x03  ... dle me...");
		irclib_message(message->sender, recipient, "\x03" "0,2     \x03" "0,3"  "^^~~^^" "\x03" "0,2"   "      \x03  vlastni volby...");

		if (data->kissNick != NULL) {
			free(data->kissNick);
		}
		data->kissNick = strdup(message->address->nick);

		timers_add(TM_TIMEOUT, 60, dummytalk_frog_remkiss, data);

		currentStatus = 0;
	} else {
		irclib_message(message->sender, recipient, "%s: %s \x03" "15(%d)",
			message->address->nick, "kvak!", currentStatus);
	}

	f = fopen("frog.txt", "w");
	if (f) {
		fprintf(f, "%d", currentStatus);
		fclose(f);
	}
}

bool dummytalk_frog_remmod(Timer tm) {
	DummytalkTimerData *data = (DummytalkTimerData *)tm->customData;

	irclib_mode(data->irclib, "%s -m", data->channel);

	free(data->channel);
	free(data);

	return false;
}

void dummytalk_frog_kiss(IRCEvent_Message *message, DummytalkPluginData *data) {
	// Ignore channel
	if (message->channel == NULL) return;

	if (data->kissNick != NULL) {
		switch (tb_rand(0, 9)) {
			case 0: // Kick
				irclib_kick(message->sender, message->channel, message->address->nick, "Sorry, dnes mam blbou naladu.");
				printError("frog", "Random choice goes to kick");
				break;

			case 1:
			case 2: // Voice
			case 3:
			case 4:
				irclib_mode(message->sender, "%s +v %s", message->channel, message->address->nick);
				printError("frog", "Random choice goes to voice");
				break;

			case 5: // Op
			case 6:
			case 7:
				irclib_mode(message->sender, "%s +o %s", message->channel, message->address->nick);
				printError("frog", "Random choice goes to op");
				break;

			case 8: // +m, voice
			case 9:
				printError("frog", "Random choice goes to +m");
				irclib_mode(message->sender, "%s +mv %s", message->channel, message->address->nick);
				irclib_message(message->sender, message->channel, "Cha cha!");

				DummytalkTimerData *dt = malloc(sizeof(DummytalkTimerData));
				dt->irclib = message->sender;
				dt->channel = strdup(message->channel);

				timers_add(
					TM_TIMEOUT,
					config_getvalue_int(data->info->config, "frog:modtimeout", 45),
					dummytalk_frog_remmod,
					dt
				);

				break;
		}

		free(data->kissNick);
		data->kissNick = NULL;
	}
}

/**
 * Handles user messages
 * @param event Event data
 */
void dummytalk_message(EVENT *event) {
	IRCEvent_Message *message = (IRCEvent_Message *)event->customData;
	DummytalkPluginData *data = (DummytalkPluginData *)event->handlerData;

	char *recipient = (message->channel != NULL)?
		message->channel:
		message->address->nick;

	char *msgcpy = strdup(message->message);

	char *firstword = strtok(msgcpy, " ");
	if (firstword == NULL) return;

	if (eq(firstword, "haf") || eq(firstword, "mnau")) {
		// It generates mnau or haf with more probable reply the same
		// as request.
		char *reply = (tb_rand(0,2) == 1)?
			((eq(firstword, "mnau"))?"haf":"mnau"):	// Less probable variant
			((eq(firstword, "mnau"))?"mnau":"haf");	// More probable variant

		irclib_message(message->sender, recipient, "%s: %s",
			message->address->nick, reply);
	}

	if (eq(firstword, "baf")) {
		irclib_message(message->sender, recipient, "%s: %s",
			message->address->nick, "lek!");
	}

	if (eq(firstword, "chro")) {
		irclib_message(message->sender, recipient, "%s: %s",
			message->address->nick, "chro!");
	}

	if (eq(firstword, "kvak")) {
		dummytalk_handle_frog(message, data);
	}

	char cmpBuf[128];
	char cmpBuf2[128];
	sprintf(cmpBuf, "%s: :*", message->sender->nickname);
	sprintf(cmpBuf2, "%s: :-*", message->sender->nickname);

	if (strcmp(message->message, cmpBuf) == 0 || strcmp(message->message, cmpBuf2) == 0) {
		if (data->kissNick != NULL && strcmp(data->kissNick, message->address->nick) == 0) {
			dummytalk_frog_kiss(message, data);
		}
	}

	free(msgcpy);
} // dummytalk_message

bool dummytalk_clear_threshold(Timer timer) {
	PluginInfo *info = timer->customData;
	DummytalkPluginData *data = info->customData;

	if (data->threshold > 1024) {
		data->threshold -= 1024;
	} else if (data->threshold > 1) {
		data->threshold /= 2;
	}

	if (data->threshold < 1) {
		data->threshold = 1;
	}

	return true;
}

/**
 * Initialize plugin.
 * @param info Plugin info, where this function must fill in some informations
 */
void PluginInit(PluginInfo *info) {
	info->name = "Dummytalk";
	info->author = "Niximor";
	info->version = "1.2.0";

	DummytalkPluginData *plugData = malloc(sizeof(DummytalkPluginData));
	plugData->info = info;
	plugData->threshold = 1;

	plugData->onchannelmessage = events_addEventListener(info->events,
		"onchannelmessage", dummytalk_message, plugData);
	plugData->onquerymessage = events_addEventListener(info->events,
		"onquerymessage", dummytalk_message, plugData);

	plugData->timerThreshold = timers_add(
		TM_TIMEOUT, 60, dummytalk_clear_threshold, info);

	info->customData = plugData;
} // PluginInit

/**
 * Close plugin
 * @param info Plugin info, which this function may use to get some
 *   informations it may need.
 */
void PluginDone(PluginInfo *info) {
	DummytalkPluginData *plugData = (DummytalkPluginData *)info->customData;

	// Remove events
	events_removeEventListener(plugData->onchannelmessage);
	events_removeEventListener(plugData->onquerymessage);

	timers_remove(plugData->timerThreshold);

	free(plugData);
} // PluginDone

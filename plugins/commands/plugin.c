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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Linux libraries
#include <time.h>

// Plugins API
#include <pluginapi.h>

// This plugin interface
#include "interface.h"

// My includes
#include <irclib/irclib.h>
#include <tokenizer.h>
#include <main.h>

#ifndef PLUGIN_NAME
# define PLUGIN_NAME "commands"
#endif

/**
 * Sends reply to command. Variable arguments are formatted by
 * format parameter.
 * @param event Commands event data
 * @param format Reply message format
 */
void commands_reply(Commands_Event *event, char *format, ...) {
	va_list ap;
	va_start(ap, format);

	switch (event->source) {
		case CMD_CHANNEL: {
			IRCEvent_Message *message = (IRCEvent_Message *)event->replyData;
			irclib_vmessage(message->sender, message->channel, format, ap);
			break;
		}

		case CMD_QUERY: {
			IRCEvent_Message *message = (IRCEvent_Message *)event->replyData;
			irclib_vmessage(message->sender, message->address->nick,
				format, ap);
			break;
		}
	}

	va_end(ap);
} // commands_reply

/**
 * Parses message from user on IRC and call command event.
 * @param event Event data
 */
void commands_ircmessage(EVENT *event) {
	CommandsPluginData *plugData = (CommandsPluginData *)event->handlerData;
	IRCEvent_Message *message = (IRCEvent_Message *)event->customData;

	char *prefix = config_getvalue_string(plugData->info->config,
		PLUGIN_NAME ":prefix", "!");

	if (strncmp(message->message, prefix, strlen(prefix)) == 0) {
		TOKENS tok = tokenizer_tokenize(message->message, ' ');
		char *command = tokenizer_gettok(tok, 0);
		if (strlen(command) > strlen(prefix)) {
			Commands_Event evt = {
				.plugData = plugData,
				.source = (message->channel != NULL)?CMD_CHANNEL:CMD_QUERY,
				.replyData = message,
				.command = strdup(command + strlen(prefix)),
				.params = strdup(tokenizer_gettok_skipleft(tok, 1))
			};

			events_fireEvent(plugData->info->events, "oncommand", &evt);

			free(evt.command);
			free(evt.params);
		}
		tokenizer_free(tok);
	}
} // commands_ircmessage

/**
 * Read system uptime and return it.
 */
time_t commands_get_uptime() {
	FILE *f = fopen("/proc/uptime", "r");
	time_t uptime = 0;
	if (f) {
		fscanf(f, "%ld", &uptime);
		fclose(f);
	}
	return uptime;
} // commands_get_uptime

/**
 * Format the time to human readable form. Returns allocated string, which must be freed.
 * @param value Time value to format
 * @return Allocated string containing the value of the time in human-readable form.
 */
char *commands_format_time(time_t value) {
	long days = value / 86400;
	value -= days * 86400;
	long hours = value / 3600;
	value -= hours * 3600;
	long minutes = value / 60;
	value -= minutes * 60;

	char *output;

	if (days > 0) {
		asprintf(&output, "%ld days, %ld hours, %ld minutes, %ld seconds", days, hours, minutes, value);
	} else if (hours > 0) {
		asprintf(&output, "%ld hours, %ld minutes, %ld seconds", hours, minutes, value);
	} else if (minutes > 0) {
		asprintf(&output, "%ld minutes, %ld seconds", minutes, value);
	} else {
		asprintf(&output, "%ld seconds", value);
	}

	return output;
} // commands_format_time

/**
 * Basic commands handler
 * @param event Event data
 */
void commands_basic_commands(EVENT *event) {
	Commands_Event *command = (Commands_Event *)event->customData;
	if (strcmp(command->command, "uptime") == 0) {
		char *format_bot = commands_format_time(time(NULL) - system_boottime());
		char *format_system = commands_format_time(commands_get_uptime());

		commands_reply(command, "Uptime: Bot: %s, System: %s",
			format_bot, format_system);

		if (format_bot) free(format_bot);
		if (format_system) free(format_system);
	}
} // commands_basic_commands

/**
 * Initialize plugin.
 * @param info Plugin info, where this function must fill in some informations
 */
void PluginInit(PluginInfo *info) {
	info->name = "Commands";
	info->author = "Niximor";
	info->version = "1.0.0";

	CommandsPluginData *plugData = malloc(sizeof(CommandsPluginData));
	plugData->info = info;

	events_addEvent(info->events, "oncommand");

	plugData->onchannelmessage = events_addEventListener(
		info->events, "onchannelmessage", commands_ircmessage, plugData);
	plugData->onquerymessage = events_addEventListener(
		info->events, "onquerymessage", commands_ircmessage, plugData);
	plugData->oncommand = events_addEventListener(
		info->events, "oncommand", commands_basic_commands, NULL);

	info->customData = plugData;
} // PluginInit

/**
 * Close plugin
 * @param info Plugin info, which this function may use to get some
 *   informations it may need.
 */
void PluginDone(PluginInfo *info) {
	CommandsPluginData *plugData = (CommandsPluginData *)info->customData;

	events_removeEventListener(plugData->onchannelmessage);
	events_removeEventListener(plugData->onquerymessage);

	free(plugData);
} // PluginDone

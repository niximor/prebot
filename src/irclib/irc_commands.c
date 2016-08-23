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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

// Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// This library interface
#include "irclib.h"

/**
 * Quit from IRC
 * @param connection IRCLib_Connection structure
 * @param format Quit reason format.
 * @param ap Variable arguments formatted by format parameter.
 */
void irclib_vquit(IRCLib_Connection *connection, const char *format,
	va_list ap) {

	va_list my;
	va_copy(my, ap);

	char *reason;
	vasprintf(&reason, format, my);

	irclib_sendraw(connection, "QUIT :%s", reason);
	connection->status = IRC_QUITED;

	if (reason) {
		free(reason);
	}

	va_end(ap);
} // irclib_vquit

/**
 * Quit from IRC. Variable arguments are formatted by format parameter.
 * @param connection IRCLib_Connection structure
 * @param format Quit reason format.
 */
void irclib_quit(IRCLib_Connection *connection,
	const char *format, ...) {

	va_list ap;
	va_start(ap, format);

	irclib_vquit(connection, format, ap);

	va_end(ap);
}

/**
 * Join to channel
 * @param connection IRCLib_Connection structure
 * @param channel Channel name to join
 */
void irclib_join(IRCLib_Connection *connection, char *channel) {
	irclib_sendraw(connection, "JOIN %s", channel);
} // irclib_join

/**
 * Part from channel.
 * @param connection IRCLib_Connection structures
 * @param channel Channel to part
 * @param format Part reason format
 * @param ap Variable arguments list formatted by format parameter.
 */
void irclib_vpart(IRCLib_Connection *connection, char *channel,
	const char *format, va_list ap) {

	va_list my;
	va_copy(my, ap);

	char *reason;
	vasprintf(&reason, format, my);

	irclib_sendraw(connection, "PART %s :%s", channel, reason);

	if (reason) {
		free(reason);
	}

	va_end(my);
} // irclib_vpart

/**
 * Part from channel. Variable arguments are formated by format parameter.
 * @param connection IRCLib_Connection structures
 * @param channel Channel to part
 * @param format Part reason format
 */
void irclib_part(IRCLib_Connection *connection, char *channel,
	const char *format, ...) {

	va_list ap;
	va_start(ap, format);
	irclib_vpart(connection, channel, format, ap);
	va_end(ap);
} // irclib_part

/**
 * Post message to channel/user.
 * @param connection IRCLib_Connection structure
 * @param channel Channel to post message to
 * @param format Message format
 * @param ap Variable arguments list formatted by format parameter.
 */
void irclib_vmessage(IRCLib_Connection *connection, char *recipient,
	const char *format, va_list ap) {

	va_list my;
	va_copy(my, ap);

	char *message;
	vasprintf(&message, format, ap);

	irclib_sendraw(connection, "PRIVMSG %s :%s", recipient, message);

	if (message) {
		free(message);
	}

	va_end(my);
} // irclib_vmessage

/**
 * Post message to channel/user. Variable arguments are formatted by format
 * parameter.
 * @param connection IRCLib_Connection structure
 * @param recipient Channel to post message to
 * @param format Message format
 */
void irclib_message(IRCLib_Connection *connection, char *recipient,
	const char *format, ...) {

	va_list ap;
	va_start(ap, format);
	irclib_vmessage(connection, recipient, format, ap);
	va_end(ap);
} // irclib_message

/**
 * Post ACTION message to channel/user.
 * format parameter.
 * @param connection IRCLib_Connection structure
 * @param recipient Channel / user to post action to
 * @param format Message format
 * @param ap Variable arguments list formatted by format parametter.
 */
void irclib_vaction(IRCLib_Connection *connection, char *recipient,
	const char *format, va_list ap) {

	va_list my;
	va_copy(my, ap);

	char *message;
	vasprintf(&message, format, ap);

	irclib_sendraw(connection, "PRIVMSG %s :\001ACTION %s\001", recipient, message);

	if (message) {
		free(message);
	}

	va_end(my);
} // irclib_vaction

/**
 * Post ACTION message to channel/user. Variable arguments are formatted by
 * format parameter.
 * @param connection IRCLib_Connection structure
 * @param recipient Channel / user to post action to
 * @param format Message format
 */
void irclib_action(IRCLib_Connection *connection, char *recipient,
	const char *format, ...) {

	va_list ap;
	va_start(ap, format);
	irclib_vaction(connection, recipient, format, ap);
	va_end(ap);
} // irclib_action

/**
 * Post NOTICE to channel/user.
 * format parameter.
 * @param connection IRCLib_Connection structure
 * @param recipient Channel / user to post action to
 * @param format Message format
 * @param ap Variable arguments list formatted by format parametter.
 */
void irclib_vnotice(IRCLib_Connection *connection, char *recipient,
	const char *format, va_list ap) {

	va_list my;
	va_copy(my, ap);

	char *message;
	vasprintf(&message, format, ap);

	irclib_sendraw(connection, "NOTICE %s :%s", recipient, message);

	if (message) {
		free(message);
	}

	va_end(my);
} // irclib_vnotice

/**
 * Post NOTICE to channel/user. Variable arguments are formatted by
 * format parameter.
 * @param connection IRCLib_Connection structure
 * @param recipient Channel / user to post action to
 * @param format Message format
 */
void irclib_notice(IRCLib_Connection *connection, char *recipient,
	const char *format, ...) {

	va_list ap;
	va_start(ap, format);
	irclib_vnotice(connection, recipient, format, ap);
	va_end(ap);
} // irclib_notice

/**
 * Kick user from channel
 * @param connection IRCLib_Connection structure
 * @param channel Channel to kick user from
 * @param nick Nick of user
 * @param format Kick reason format
 * @param ap Variable arguments fromatted by format parameter.
 */
void irclib_vkick(IRCLib_Connection *connection, char *channel, char *nick,
	const char *format, va_list ap) {

	va_list my;
	va_copy(my, ap);

	char *reason;
	vasprintf(&reason, format, my);
	irclib_sendraw(connection, "KICK %s %s :%s", channel, nick, reason);
	if (reason) {
		free(reason);
	}

	va_end(my);
} // irclib_vkick

/**
 * Kick user from channel. Variable arguments are formatted by format
 * parameter.
 * @param connection IRCLib_Connection structure
 * @param channel Channel to kick user from
 * @param nick Nick of user
 * @param format Kick reason format
 */
void irclib_kick(IRCLib_Connection *connection, char *channel, char *nick,
	const char *format, ...) {

	va_list ap;
	va_start(ap, format);

	irclib_vkick(connection, channel, nick, format, ap);

	va_end(ap);
} // irclib_kick

/**
 * Sends mode string
 * @param connection IRCLib_Connection structure
 * @param format Mode string format
 * @param ap Variable arguments formatted by format parameter.
 */
void irclib_vmode(IRCLib_Connection *connection, const char *format,
	va_list ap) {

	va_list my;
	va_copy(my, ap);

	char *mode;
	vasprintf(&mode, format, my);
	irclib_sendraw(connection, "MODE %s", mode);
	if (mode) {
		free(mode);
	}

	va_end(my);
} // irclib_vmode

/**
 * Sends mode string. Variable arguments are formatted by format parameter.
 * @param connection IRCLib_Connection structure
 * @param format Mode string format
 */
void irclib_mode(IRCLib_Connection *connection, const char *format, ...) {
	va_list ap;
	va_start(ap, format);

	irclib_vmode(connection, format, ap);

	va_end(ap);
} // irclib_mode

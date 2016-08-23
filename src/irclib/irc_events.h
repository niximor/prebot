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

#ifndef _IRC_EVENTS_H
# define _IRC_EVENTS_H 1

#include "irclib.h"

/**
 * IRClib event data structure for basic "notify" events
 */
typedef struct {
	IRCLib_Connection *sender;	/**< IRCLib_Connection structure that
								     triggered the event */
} IRCEvent_Notify;

/**
 * IRClib event data structure for raw message events
 */
typedef struct {
	IRCLib_Connection *sender;	/**< IRCLib_Connection structure that
								     triggered the event */
	char *message;				/**< Message that has been received
								     or is going to be send */
} IRCEvent_RawData;

/**
 * IRClib event data structure for server messages
 */
typedef struct {
	IRCLib_Connection *sender;	/**< IRCLib_Connection structure that
								     triggered the event */
	char *server;				/**< Name of server that sent the
								     message */
	int messageCode;			/**< Numeric code of the message
								     (see RFC for details */
	char *message;				/**< Message details */
} IRCEvent_ServerMessage;

/**
 * IRClib event data for join message
 */
typedef struct {
	IRCLib_Connection *sender;	/**< IRCLib_Connection structure that
								     triggered the event */
	IRCLib_Host *address;		/**< Nick, user and host of user that
								     has joined the channel */
	char *channel;				/**< Channel name */
	char *reason;				/**< Part season, NULL if join */
} IRCEvent_JoinPart;

/**
 * IRClib event data for channel/private message
 */
typedef struct {
	IRCLib_Connection *sender;	/**< IRCLib_Connection structure that
								     triggered the event */
	IRCLib_Host *address;		/**< User that sent the message */
	char *channel;				/**< Channel that the message has been
								     sent to. It is null when receiving
								     private message. */
	char *message;				/**< Message text */
} IRCEvent_Message;

/**
 * IRClib event data triggered when some mode has been changed
 */
typedef struct {
	IRCLib_Connection *sender;	/**< IRCLib_Connection strcture that
								     triggered the event */
	char name;					/**< Name of mode */
	int set;					/**< True if mode is being set, false
								     if mode is being removed */
	IRCLib_Host *address;		/**< nick!user@host address of user who changed
								     the mode. */
	char *channel;				/**< Channel that is being affected */
	char *target;				/**< Nick / address or parameter value for
								     the mode*/
} IRCEvent_Mode;

/**
 * IRClib event data triggered when someone has been kicked
 */
typedef struct {
	IRCLib_Connection *sender;	/**< IRCLib_Connection strcture that
								     triggered the event */
	IRCLib_Host *address;		/**< nick!user@host address of user who changed
								     the mode. */
	char *channel;				/**< Channel that is being affected. */
	char *nick;					/**< Nick of person who has been kicked. */
	char *reason;				/**< Kick reason */
} IRCEvent_Kick;

/**
 * IRClib event data triggeded when someone's nick has been changed.
 */
typedef struct {
	IRCLib_Connection *sender;	/**< IRCLib_Connection strcture that
								     triggered the event */
	IRCLib_Host *address;		/**< nick!user@host address of user who changed
								     his nick. */
	char *newnick;				/**< New nick of user */
} IRCEvent_NickChange;

/**
 * IRClib event data triggered when someone quits IRC.
 */
typedef struct {
	IRCLib_Connection *sender;	/**< IRCLib_Connection structure that triggered
									 the event */
	IRCLib_Host *address;		/**< nick!user@host address of user who
									 quited */
	char *message;				/**< Quit message */
} IRCEvent_Quit;

#endif

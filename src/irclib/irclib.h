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

#ifndef _IRCLIB_H
#define _IRCLIB_H 1

// Standard includes
#include <stdarg.h>

// Linux includes
#include <sys/types.h>
#include <sys/socket.h>

// My includes
#include <events.h>
#include <dynastring.h>
#include <socketpool.h>
#include <timers.h>
#include <io.h>

static const int IRCMSG_MAXLEN = 512;	/**< Maximum number of IRC message -
											 this constant should be used to
											 allocate memory for strings. */

typedef enum {
	ERR_NOSUCHNICK = 401,
	ERR_NOSUCHSERVER = 402,
	ERR_NOSUCHCHANNEL = 403,
	ERR_CANNOTSENDTOCHAN = 404,
	ERR_TOOMANYCHANNELS = 405,
	ERR_WASNOSUCHNICK = 406,
	ERR_TOOMANYTARGETS = 407,
	ERR_NOORIGIN = 409,
	ERR_NORECIPIENT = 411,
	ERR_NOTEXTTOSEND = 412,
	ERR_NOTOPLEVEL = 413,
	ERR_WILDTOPLEVEL = 414,
	ERR_UNKNOWNCOMMAND = 421,
	ERR_NOMOTD = 422,
	ERR_NOADMININFO = 423,
	ERR_FILEERROR = 424,
	ERR_NONICKNAMEGIVEN = 431,
	ERR_ERRONUSNICKNAME = 432,
	ERR_NICKNAMEINUSE = 433,
	ERR_NICKCOLLISION = 436,
	ERR_USERNOTINCHANNEL = 441,
	ERR_NOTONCHANNEL = 442,
	ERR_USERONCHANNEL = 443,
	ERR_NOLOGIN = 444,
	ERR_SUMMONDISABLED = 445,
	ERR_USERDISABLED = 446,
	ERR_NOTREGISTERED = 451,
	ERR_NEEDMOREPARAMS = 461,
	ERR_ALREADYREGISTERED = 462,
	ERR_NOPERMFORHOST = 463,
	ERR_PASSWDMISMATCH = 464,
	ERR_YOUREBANNEDCREEP = 465,
	ERR_KEYSET = 467,
	ERR_CHANNELISFULL = 471,
	ERR_UNKNOWNMODE = 472,
	ERR_INVITEONLYCHAN = 473,
	ERR_BANNEDFROMCHAN = 474,
	ERR_BADCHANNELKEY = 475,
	ERR_NOPRIVILEGES = 481,
	ERR_CHANOPRIVSNEEDED = 482,
	ERR_CANTKILLSERVER = 483,
	ERR_NOOPERHOST = 491,
	ERR_UMODEUNKNOWNFLAG = 501,
	ERR_USERSDONTMATCH = 502
} IRCLib_ErrorCode;

typedef enum {
	RPL_NONE = 300,
	RPL_USERHOST = 302,
	RPL_ISON = 303,
	RPL_AWAY = 301,
	RPL_UNAWAY = 305,
	RPL_NOWAWAY = 306,
	RPL_WHOISUSER = 311,
	RPL_WHOISSERVER = 312,
	RPL_WHOISOPERATOR = 313,
	RPL_WHOISIDLE = 317,
	RPL_ENDOFWHOIS = 318,
	RPL_WHOISCHANNELS = 319,
	RPL_WHOWASUSER = 314,
	RPL_ENDOFWHOWAS = 369,
	RPL_LISTSTART = 321,
	RPL_LIST = 322,
	RPL_LISTEND = 323,
	RPL_CHANNELMODEIS = 324,
	RPL_NOTOPIC = 331,
	RPL_TOPIC = 332,
	RPL_INVITING = 341,
	RPL_SUMMONING = 342,
	RPL_VERSION = 351,
	RPL_WHOREPLY = 352,
	RPL_ENDOFWHO = 315,
	RPL_NAMREPLY = 353,
	RPL_ENDOFNAMES = 366,
	RPL_LINKS = 364,
	RPL_ENDOFLINKS = 365,
	RPL_BANLIST = 367,
	RPL_ENDOFBANLIST = 368,
	RPL_INFO = 371,
	RPL_ENDOFINFO = 374,
	RPL_MOTDSTART = 375,
	RPL_MOTD = 372,
	RPL_ENDOFMOTD = 376,
	RPL_YOUREOPER = 381,
	RPL_REHASHING = 382,
	RPL_TIME = 391,
	RPL_USERSTART = 392,
	RPL_USERS = 393,
	RPL_ENDOFUSERS = 394,
	RPL_NOUSERS = 395,
	RPL_TRACELINK = 200,
	RPL_TRACECONNECTING = 201,
	RPL_TRACEHANDSHAKE = 202,
	RPL_TRACEUNKNOWN = 203,
	RPL_TRACEOPERATOR = 204,
	RPL_TRACEUSER = 205,
	RPL_TRACESERVER = 206,
	RPL_TRACENEWTYPE = 208,
	RPL_TRACELOG = 261,
	RPL_STATSLINKINFO = 211,
	RPL_STATSCOMMANDS = 212,
	RPL_STATSCLINE = 213,
	RPL_STATSNLINE = 214,
	RPL_STATSILINE = 215,
	RPL_STATSKLINE = 216,
	RPL_STATSYLINE = 218,
	RPL_ENDOFSTATS = 219,
	RPL_STATSLLINE = 241,
	RPL_STATSUPTIME = 242,
	RPL_STATSOLINE = 243,
	RPL_STATSHLINE = 244,
	RPL_UMODEIS = 221,
	RPL_LUSERCLIENT = 251,
	RPL_LUSEROP = 252,
	RPL_LUSERUNKNOWN = 253,
	RPL_LUSERCHANNELS = 254,
	RPL_LUSERME = 255,
	RPL_ADMINME = 256,
	RPL_ADMINLOC1 = 257,
	RPL_ADMINLOC2 = 258,
	RPL_ADMINEMAIL = 259,
	RPL_ISUPPORT = 5
} IRCLib_ReplyCode;

/**
 * Status of IRCLib
 */
typedef enum {
	IRC_DISCONNECTED = 0,			/**< Not connected to server */
	IRC_CONNECTING,					/**< Connecting to server */
	IRC_CONNECTED,					/**< Successfully connected to server. */
	IRC_QUITED						/**< Disconnected and do not reconnect. */
} IRCLib_ConnectionStatus;

/**
 * IRCLib_Host struct contains parsed nick, user and host from nick!user@host
 * notation
 */
typedef struct {
	char *nick;		/**< Nick */
	char *user;		/**< User including user mode (~, + and so on) */
	char *host;		/**< Hostname */
} IRCLib_Host;

// Forward
typedef struct sIRCLib_ChannelStorage *IRCLib_ChannelStorage;
typedef struct sIRCLib_Channel *IRCLib_Channel;
typedef struct sIRCLib_ChannelUser *IRCLib_ChannelUser;
typedef struct sIRCLib_User *IRCLib_User;
typedef struct sIRCLib_UserChannel *IRCLib_UserChannel;
typedef struct sIRCLib_UserStorage *IRCLib_UserStorage;

/**
 * User information in global user storage
 */
struct sIRCLib_User {
	IRCLib_User prev;				/**< Previous user in storage */
	IRCLib_User next;				/**< Next user in storage */

	IRCLib_Host *host;				/**< User's nick, user, host */
	IRCLib_UserChannel first;		/**< First channel that user is joined
										 to */
	IRCLib_UserChannel last;		/**< Last channel that user is joined to */
}; // sIRCLib_User

struct sIRCLib_UserChannel {
	IRCLib_UserChannel prev;		/**< Previous channel in chain */
	IRCLib_UserChannel next;		/**< Next channel in chain */

	IRCLib_Channel channel;			/**< Channel information */
}; // sIRCLib_UserChannel

/**
 * User information storage
 */
struct sIRCLib_UserStorage {
	IRCLib_User first;
	IRCLib_User last;
}; // sIRCLib_UserStorage

/**
 * User on channel information
 */
struct sIRCLib_ChannelUser {
	IRCLib_ChannelUser prev;		/**< Previous user in channel */
	IRCLib_ChannelUser next;		/**< Next user in channel */

	char prefix;					/**< User's prefix on this channel */
	char prefixSymbol;				/**< User's prefix symbol (@+%) */
	IRCLib_User userinfo;			/**< Pointer to global users info
										 storage */
}; // sIRCLib_ChannelUser

/**
 * Joined channel information
 */
struct sIRCLib_Channel {
	IRCLib_Channel prev;			/**< Previous channel in chain */
	IRCLib_Channel next;			/**< Next channel in chain */

	char *name;						/**< Channel name */
	IRCLib_ChannelUser first;		/**< First user in channel */
	IRCLib_ChannelUser last;		/**< Last user in channel */
}; // sIRCLib_Channel

/**
 * Storage for joined channels
 */
struct sIRCLib_ChannelStorage {
	IRCLib_Channel first;			/**< First channel in chain */
	IRCLib_Channel last;			/**< Last channel in chain */
}; // sIRCLib_ChannelStorage

/**
 * IRCLib_Connection structure is used by IRCLib to identify connection to
 * IRC server.
 */
typedef struct {
	char *hostname;					/**< Hostname of IRC server */
	int port;						/**< Port of IRC server */
	char *bind;						/**< Hostname to bind outgoing socket to. */
    bool force_ipv4;                /**< Force IPv4 connection */
    bool force_ipv6;                /**< Force IPv6 connection */

	char *nickname;					/**< A nickname */
	char *username; 				/**< A username */
	char *realname;					/**< A real name */
	char *password;					/**< Password to be used to connect to IRC
										 server */

	int socket;						/**< Client socket that is used to
										 communicate with server. Don't fill
										 this up, it will be filled
										 automatically by irclib. */
	EVENTS *events;					/**< Events instance */
	SocketPool socketpool;			/**< Socketpool instance */
	IRCLib_ConnectionStatus status;	/**< Status of connection (connected,
										 connecting, disconnected, ...) */

	char *networkName;				/**< Name of network that bot is connected
										 to. */
	char *userPrefixes;				/**< List of user prefixes that server
										 supports */
	char *userPrefixesSymbols;		/**< List of supported user prefixes as
										 symbols */

	char *chanModesAddress;			/**< List of channel modes that requires
										 address as parameter. */
	char *chanModesAlwaysParam;		/**< List of channel modes that always
										 requires parameter */
	char *chanModesSetParam; 		/**< List of channel modes that requires
										 parameter only when being set. */
	char *chanModesNeverParam; 		/**< List of channel modes that never
										 requires parameter. */

	string recvbuffer;				/**< Receive buffer */
	time_t lastActivity;			/**< Last time something has been
										 received. */

	IRCLib_ChannelStorage channelStorage; /**< Joined channels storage */
	IRCLib_UserStorage userStorage;	/**< Global users storage */

	bool reconnect;					/**< Set to true if you want to perform
										 auto reconnect */
	Timer reconnecttimer;			/**< Reconnect timer */
	Timer testalivetimer;			/**< Timer that tests if connection is
										 still alive. */
	unsigned int aliveCheckTimeout; /**< How many seconds must be connection
										 inactive (no received data) before
										 irclib tries to PING server to test
										 if connection is still alive. */
} IRCLib_Connection;

#include "irc_events.h"
#include "irc_functions.h"

#endif

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

#ifndef _IRC_FUNCTIONS_H
# define _IRC_FUNCTIONS_H 1

#include <stdbool.h>
#include "irclib.h"

/**
 * Initialize the IRC library
 * @param connection IRCLib_Connection structure with existing events instance.
 */
extern void irclib_init(IRCLib_Connection *connection);

/**
 * Connects to IRC server
 * @param connection Filled in IRCLib_Connection structure with hostname,
 *   port, username, nickname and realname.
 */
extern int irclib_connect(IRCLib_Connection *connection);

/**
 * Close connection to IRC server
 * @param connection IRCLib_Connection structure
 */
extern void irclib_close(IRCLib_Connection *connection);

/**
 * Shut down IRCLib - free structures allocated after connection
 * @param connection IRCLib_Connection structure
 */
extern void irclib_shutdown(IRCLib_Connection *connection);

/**
 * Socketpool handler to receive data from IRC server
 * @param socket Socketpool socket
 */
extern void irclib_receive(Socket socket);

/**
 * Parses incomming data and fire event, if any is assigned.
 * @param connection IRCLib_Connection structure
 * @param message One message received from IRC server
 * @return 1 on successful parsing, 0 if error.
 */
extern void irclib_parse(IRCLib_Connection *connection, char *message);

/**
 * Sends raw data to IRC server
 * @param connection IRCLib_Connection structure that is connected to IRC
 *   server.
 * @param format Format of data to send to server
 */
extern void irclib_sendraw(IRCLib_Connection *connection,
	const char *format, ...);

/**
 * Quit from IRC
 * @param connection IRCLib_Connection structure
 * @param format Quit reason format.
 * @param ap Variable arguments formatted by format parameter.
 */
extern void irclib_vquit(IRCLib_Connection *connection, const char *format,
	va_list ap);

/**
 * Quit from IRC. Variable arguments are formatted by format parameter.
 * @param connection IRCLib_Connection structure
 * @param format Quit reason format.
 */
extern void irclib_quit(IRCLib_Connection *connection,
	const char *format, ...);

/**
 * Join to channel
 * @param connection IRCLib_Connection structure
 * @param channel Channel name to join
 */
extern void irclib_join(IRCLib_Connection *connection, char *channel);

/**
 * Part from channel.
 * @param connection IRCLib_Connection structures
 * @param channel Channel to part
 * @param format Part reason format
 * @param ap Variable arguments list formatted by format parameter.
 */
extern void irclib_vpart(IRCLib_Connection *connection, char *channel,
	const char *format, va_list ap);

/**
 * Part from channel. Variable arguments are formated by format parameter.
 * @param connection IRCLib_Connection structures
 * @param channel Channel to part
 * @param format Part reason format
 */
extern void irclib_part(IRCLib_Connection *connection, char *channel,
	const char *format, ...);

/**
 * Post message to channel/user.
 * @param connection IRCLib_Connection structure
 * @param channel Channel to post message to
 * @param format Message format
 * @param ap Variable arguments list formatted by format parameter.
 */
extern void irclib_vmessage(IRCLib_Connection *connection, char *recipient,
	const char *format, va_list ap);

/**
 * Post message to channel/user. Variable arguments are formatted by format
 * parameter.
 * @param connection IRCLib_Connection structure
 * @param recipient Channel to post message to
 * @param format Message format
 */
extern void irclib_message(IRCLib_Connection *connection, char *recipient,
	const char *format, ...);

/**
 * Post ACTION message to channel/user.
 * format parameter.
 * @param connection IRCLib_Connection structure
 * @param recipient Channel / user to post action to
 * @param format Message format
 * @param ap Variable arguments list formatted by format parametter.
 */
extern void irclib_vaction(IRCLib_Connection *connection, char *recipient,
	const char *format, va_list ap);

/**
 * Post ACTION message to channel/user. Variable arguments are formatted by
 * format parameter.
 * @param connection IRCLib_Connection structure
 * @param recipient Channel / user to post action to
 * @param format Message format
 */
extern void irclib_action(IRCLib_Connection *connection, char *recipient,
	const char *format, ...);

/**
 * Post NOTICE to channel/user.
 * format parameter.
 * @param connection IRCLib_Connection structure
 * @param recipient Channel / user to post action to
 * @param format Message format
 * @param ap Variable arguments list formatted by format parametter.
 */
extern void irclib_vnotice(IRCLib_Connection *connection, char *recipient,
	const char *format, va_list ap);

/**
 * Post NOTICE to channel/user. Variable arguments are formatted by
 * format parameter.
 * @param connection IRCLib_Connection structure
 * @param recipient Channel / user to post action to
 * @param format Message format
 */
extern void irclib_notice(IRCLib_Connection *connection, char *recipient,
	const char *format, ...);

/**
 * Kick user from channel
 * @param connection IRCLib_Connection structure
 * @param channel Channel to kick user from
 * @param nick Nick of user
 * @param format Kick reason format
 * @param ap Variable arguments fromatted by format parameter.
 */
extern void irclib_vkick(IRCLib_Connection *connection, char *channel,
	char *nick, const char *format, va_list ap);

/**
 * Kick user from channel. Variable arguments are formatted by format
 * parameter.
 * @param connection IRCLib_Connection structure
 * @param channel Channel to kick user from
 * @param nick Nick of user
 * @param format Kick reason format
 */
extern void irclib_kick(IRCLib_Connection *connection, char *channel,
	char *nick, const char *format, ...);

/**
 * Sends mode string
 * @param connection IRCLib_Connection structure
 * @param format Mode string format
 * @param ap Variable arguments formatted by format parameter.
 */
extern void irclib_vmode(IRCLib_Connection *connection, const char *format,
	va_list ap);

/**
 * Sends mode string. Variable arguments are formatted by format parameter.
 * @param connection IRCLib_Connection structure
 * @param format Mode string format
 */
extern void irclib_mode(IRCLib_Connection *connection,
	const char *format, ...);

/**
 * Parse address to nick, username and host structure
 * @param address Address in format nick!user@host
 * @return Pointer to structure IRCLib_Host
 */
extern IRCLib_Host *irclib_parse_addr(char *address);

/**
 * Free IRCLib_Host structure
 * @param host Pointer to IRCLib_Host structure
 */
extern void irclib_free_addr(IRCLib_Host *host);

/**
 * Construct nick!user@host address from IRCLib_Host structure
 * @param host IRCLib_Host
 * @return Allocated string with nick!user@host format
 */
extern char *irclib_construct_addr(IRCLib_Host *host);

/**
 * Convert user prefix to symbol
 * @param connection IRCLib_Connection structure with filled userPrefixes and
 *   userPrefixesSymbols items
 * @param prefix Prefix that should be converted to symbol
 * @return Symbol corresponding to specified prefix. Return space (' ') if
 *   prefix was not found.
 */
extern char irclib_prefix2sym(IRCLib_Connection *connection, char prefix);

/**
 * Convert to symbol prefix
 * @param connection IRCLib_Connection structure with filled userPrefixes and
 *   userPrefixesSymbols items
 * @param sym Symbol that should be converted to prefix.
 * @return Symbol corresponding to specified prefix. Return space (' ') if
 *   symbol was not found.
 */
extern char irclib_sym2prefix(IRCLib_Connection *connection, char sym);

/**
 * Determines whether nickname contains prefix
 * @param connection IRCLib_Connection structure with filled
 *   userPrefixesSymbols items
 * @param nick Nickname that can have prefix.
 * @return true if nickname contains prefix, false otherwise
 */
extern bool irclib_nickHasPrefix(IRCLib_Connection *connection, char *nick);

/**
 * Init channel storage
 * @return Initialized channel storage
 */
extern IRCLib_ChannelStorage irclib_init_channels();

/**
 * Clear channels
 * @param storage Channel storage to clear
 */
extern void irclib_clear_channels(IRCLib_ChannelStorage storage);

/**
 * Free channel storage
 * @param storage Channel storage to be freed
 */
extern void irclib_free_channels(IRCLib_ChannelStorage storage);

/**
 * Add channel to channel storage
 * @param storage Channel storage
 * @param channel Channel name
 * @return Created channel structure, or NULL if error has occured.
 */
extern IRCLib_Channel irclib_add_channel(IRCLib_ChannelStorage storage,
	char *channel);

/**
 * Add new user to channel
 * @param connection Connection with valid prefix information
 * @param channel Channel to add user to
 * @param nick Nickname with optional prefix
 * @return Channel user structure of added user
 */
extern IRCLib_ChannelUser irclib_add_channel_user(
	IRCLib_Connection *connection, IRCLib_Channel channel, char *nick);

/**
 * Remove channel from storage and properly free all it's structures
 * @param storage Channel storage
 * @param channel Channel to remove from storage
 */
extern void irclib_remove_channel(IRCLib_ChannelStorage storage,
	IRCLib_Channel channel);

/**
 * Remove user from channel and free user's structures
 * @param channel Channel to remove user from
 * @param nick Nick of user that should be removed
 */
extern void irclib_remove_channel_user(IRCLib_Channel channel, char *nick);

/**
 * Change user's prefix on channel
 * @param connection IRCLib_Connection
 * @param channel Channel that has been affected
 * @param nick Nickname of affected user
 * @param prefix New user's prefix (as prefix name, not symbol).
 */
extern void irclib_change_user_prefix(IRCLib_Connection *connection,
	IRCLib_Channel channel, char *nick, char prefix);

/**
 * Try to find channel in storage
 * @param storage Channel storage
 * @param channel Channel name
 * @return Channel structure if found, NULL if not found.
 */
extern IRCLib_Channel irclib_find_channel(IRCLib_ChannelStorage storage,
	char *channel);

/**
 * Finds whether user is on channel.
 * @param channel Channel to scan.
 * @param nick Nickname to try to find.
 * @return true if user is on specified channel, false otherwise.
 */
extern bool irclib_is_user_on(IRCLib_Channel channel, char *nick);

/**
 * Retrun number of users on channel
 * @param channel Channel structure
 * @return Number of users on specified channel
 */
extern int irclib_count_users(IRCLib_Channel channel);

/**
 * Init users storage
 * @return Initialized user storage or NULL if error
 */
extern IRCLib_UserStorage irclib_init_userstorage();

/**
 * Clear users storage, keeping storage usable again
 * @param storage Users storage
 */
extern void irclib_clear_userstorage(IRCLib_UserStorage storage);

/**
 * Free user storage structures
 * @param storage User storage
 */
extern void irclib_free_userstorage(IRCLib_UserStorage storage);

/**
 * Add user to storage
 * @param storage User storage
 * @param nick Nickname
 * @param user User name
 * @param host Hostname
 */
extern IRCLib_User irclib_add_user(IRCLib_UserStorage storage, char *nick,
	char *user, char *host);

/**
 * Add new user using it's IRCLib_Host structure
 * @param storage User storage
 * @param host IRCLib_Host structure
 */
extern IRCLib_User irclib_add_usera(IRCLib_UserStorage storage,
	IRCLib_Host *host);

/**
 * Add channel to user's personal channel list
 * @param user User to add channel to
 * @param channel Channel to add
 */
extern void irclib_add_user_channel(IRCLib_User user, IRCLib_Channel channel);

/**
 * Remove channel from user's personal channel list
 * @param user User to remove channel from
 * @param channel Channel to remove
 */
extern void irclib_remove_user_channel(IRCLib_User user,
	IRCLib_Channel channel);

/**
 * Find user in storage and return it's structure
 * @param storage User storage
 * @param nick Nick to find
 * @return IRCLib_User structure of corresponding user, or NULL if user was
 *   not found.
 */
extern IRCLib_User irclib_find_user(IRCLib_UserStorage storage, char *nick);

/**
 * Remove user from storage
 * @param storage User storage
 * @param user User to be removed from that storage
 */
extern void irclib_remove_user(IRCLib_UserStorage storage, IRCLib_User user);

/**
 * Change nick of user
 * @param user User whos nick will be changed
 * @param newnick New nick
 */
extern void irclib_rename_user(IRCLib_User user, char *newnick);

/**
 * Triggered when connection to IRC server has been closed.
 * @param socket Socket from socketpool that has been closed.
 */
extern void irclib_closed(Socket socket);

/**
 * Test if irclib is connected, and if not, try to reconnect.
 * @param timer Timer data
 * @return Always returns true, because the timer should be always resetted.
 */
extern bool irclib_timerreconnect(Timer timer);

/**
 * Test if irclib is still connected to IRC. Sends PING to myself.
 * @param timer Timer data
 * @return Always return true, because timer should be always resetted.
 */
extern bool irclib_timercheckalive(Timer timer);

#endif

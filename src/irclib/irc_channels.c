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
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// This library interface
#include "irclib.h"

// My libraries
#include <toolbox/linkedlist.h>
#include <toolbox/tb_string.h>
#include <io.h>

/**
 * Init channel storage
 * @return Initialized channel storage
 */
IRCLib_ChannelStorage irclib_init_channels() {
	IRCLib_ChannelStorage result =
		malloc(sizeof(struct sIRCLib_ChannelStorage));

	if (result != NULL) {
		ll_init(result);
	}

	return result;
} // irclib_init_channels

/**
 * Clear channels
 * @param storage Channel storage to clear
 */
void irclib_clear_channels(IRCLib_ChannelStorage storage) {
	ll_loop(storage, channel) {
		irclib_remove_channel(storage, channel);
	}
} // irclib_clear_channels

/**
 * Free channel storage
 * @param storage Channel storage to be freed
 */
void irclib_free_channels(IRCLib_ChannelStorage storage) {
	// Free each channel information
	irclib_clear_channels(storage);

	free(storage);
} // irclib_free_channels

/**
 * Add channel to channel storage
 * @param storage Channel storage
 * @param channel Channel name
 * @return Created channel structure, or NULL if error has occured.
 */
IRCLib_Channel irclib_add_channel(IRCLib_ChannelStorage storage,
	char *channel) {

	// Try to find channel in storage first
	IRCLib_Channel newchan = irclib_find_channel(storage, channel);

	// If channel was not found, append it.
	if (newchan == NULL) {
		newchan = malloc(sizeof(struct sIRCLib_Channel));
		if (newchan != NULL) {
			ll_init(newchan);
			newchan->name = strdup(channel);
			ll_append(storage, newchan);
		}
	}

	return newchan;
} // irclib_channel_add

/**
 * Add new user to channel
 * @param connection Connection with valid prefix information
 * @param channel Channel to add user to
 * @param nick Nickname with optional prefix
 * @return Channel user structure of added user or NULL if error occured.
 */
IRCLib_ChannelUser irclib_add_channel_user(IRCLib_Connection *connection,
	IRCLib_Channel channel, char *nick) {

	if (channel == NULL) return NULL;

	IRCLib_ChannelUser newuser = NULL;
	ll_loop(channel, user) {
		if (eq(user->userinfo->host->nick, nick)) {
			newuser = user;
			break;
		}
	}

	// Scan fist nick's char whether it is prefix or not
	char prefix = ' ';
	if (irclib_nickHasPrefix(connection, nick)) {
		prefix = nick[0];
		nick++;
	}

	// User isn't on channel.
	if (newuser == NULL) {
		newuser = malloc(sizeof(struct sIRCLib_ChannelUser));
		if (newuser == NULL) return NULL;

		// Set user info properly
		newuser->userinfo = irclib_add_user(connection->userStorage,
			nick, NULL, NULL);

		ll_append(channel, newuser);
		irclib_add_user_channel(newuser->userinfo, channel);
	}

	newuser->prefix = irclib_sym2prefix(connection, prefix);
	newuser->prefixSymbol = prefix;

	return newuser;
} // irclib_add_channel_user

/**
 * Remove channel from storage and properly free all it's structures
 * @param storage Channel storage
 * @param channel Channel to remove from storage
 */
void irclib_remove_channel(IRCLib_ChannelStorage storage,
	IRCLib_Channel channel) {

	if (channel == NULL) return;

	// Free users-on-channel information
	ll_remove(storage, channel);

	ll_loop(channel, user) {
		irclib_remove_user_channel(user->userinfo, channel);
		free(user);
	}

	free(channel->name);
	free(channel);
} // irclib_remove_channel

/**
 * Remove user from channel and free user's structures
 * @param channel Channel to remove user from
 * @param nick Nick of user that should be removed
 */
void irclib_remove_channel_user(IRCLib_Channel channel, char *nick) {
	if (channel == NULL) return;

	ll_loop(channel, user) {
		if (eq(user->userinfo->host->nick, nick)) {
			irclib_remove_user_channel(user->userinfo, channel);
			ll_remove(channel, user);
			free(user);
			break;
		}
	}
} // irclib_remove_channel_user

/**
 * Change user's prefix on channel
 * @param connection IRCLib_Connection
 * @param channel Channel that has been affected
 * @param nick Nickname of affected user
 * @param prefix New user's prefix (as prefix name, not symbol).
 */
void irclib_change_user_prefix(IRCLib_Connection *connection,
	IRCLib_Channel channel, char *nick, char prefix) {

	if (channel == NULL) return;

	ll_loop(channel, user) {
		if (eq(user->userinfo->host->nick, nick)) {
			user->prefix = prefix;
			user->prefixSymbol = irclib_prefix2sym(connection, prefix);
			break;
		}
	}
} // irclib_change_user_prefix

/**
 * Try to find channel in storage
 * @param storage Channel storage
 * @param channel Channel name
 * @return Channel structure if found, NULL if not found.
 */
IRCLib_Channel irclib_find_channel(IRCLib_ChannelStorage storage,
	char *channel) {

	ll_loop(storage, schannel) {
		if (eq(schannel->name, channel)) {
			return schannel;
		}
	}
	return NULL;
} // irclib_find_channel

/**
 * Finds whether user is on channel.
 * @param channel Channel to scan.
 * @param nick Nickname to try to find.
 * @return true if user is on specified channel, false otherwise.
 */
bool irclib_is_user_on(IRCLib_Channel channel, char *nick) {
	ll_loop(channel, user) {
		if (eq(user->userinfo->host->nick, nick)) {
			return true;
		}
	}
	return false;
} // irclib_is_user_on

/**
 * Retrun number of users on channel
 * @param channel Channel structure
 * @return Number of users on specified channel
 */
int irclib_count_users(IRCLib_Channel channel) {
	int i = 0;
	ll_loop(channel, user) {
		i++;
	}
	return i;
} // irclib_count_users

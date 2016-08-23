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

// This library interface
#include "irclib.h"

// My libraries
#include <toolbox/linkedlist.h>
#include <toolbox/tb_string.h>

/**
 * Init users storage
 * @return Initialized user storage or NULL if error
 */
IRCLib_UserStorage irclib_init_userstorage() {
	IRCLib_UserStorage result = malloc(sizeof(struct sIRCLib_UserStorage));

	if (result != NULL) {
		ll_init(result);
	}

	return result;
} // irclib_init_userstorage

/**
 * Clear users storage, keeping storage usable again
 * @param storage Users storage
 */
void irclib_clear_userstorage(IRCLib_UserStorage storage) {
	ll_loop(storage, user) {
		irclib_remove_user(storage, user);
	}
} // irclib_clear_userstorage

/**
 * Free user storage structures
 * @param storage User storage
 */
void irclib_free_userstorage(IRCLib_UserStorage storage) {
	// Clear users
	irclib_clear_userstorage(storage);

	free(storage);
} // irclib_free_userstorage

/**
 * Add user to storage
 * @param storage User storage
 * @param nick Nickname
 * @param user User name
 * @param host Hostname
 */
IRCLib_User irclib_add_user(IRCLib_UserStorage storage, char *nick,
	char *user, char *host) {

	IRCLib_User newuser = irclib_find_user(storage, nick);
	if (newuser == NULL) {
		newuser = malloc(sizeof(struct sIRCLib_User));
		ll_append(storage, newuser);
		ll_init(newuser);

		newuser->host = malloc(sizeof(IRCLib_Host));
		newuser->host->nick = NULL;
		newuser->host->user = NULL;
		newuser->host->host = NULL;
	} else {
		// Free values only if they aren't null already, and if we have
		// replacement, and if they are not same as already setted ones.

		if (newuser->host->nick != NULL && nick != NULL &&
			!eq(newuser->host->nick, nick)) {

			free(newuser->host->nick);
			newuser->host->nick = NULL;
		}
		if (newuser->host->user != NULL && user != NULL &&
			!eq(newuser->host->user, user)) {

			free(newuser->host->user);
			newuser->host->user = NULL;
		}
		if (newuser->host->host != NULL && host != NULL &&
			!eq(newuser->host->host, host)) {

			free(newuser->host->host);
			newuser->host->host = NULL;
		}
	}

	if (newuser->host->nick == NULL && nick != NULL) {
		newuser->host->nick = strdup(nick);
	}
	if (newuser->host->user == NULL && user != NULL) {
		newuser->host->user = strdup(user);
	}
	if (newuser->host->host == NULL && host != NULL) {
		newuser->host->host = strdup(host);
	}

	return newuser;
} // irclib_add_user

/**
 * Add new user using it's IRCLib_Host structure
 * @param storage User storage
 * @param host IRCLib_Host structure
 */
IRCLib_User irclib_add_usera(IRCLib_UserStorage storage, IRCLib_Host *host) {
	return irclib_add_user(storage, host->nick, host->user, host->host);
} // irclib_add_usera

/**
 * Add channel to user's personal channel list
 * @param user User to add channel to
 * @param channel Channel to add
 */
void irclib_add_user_channel(IRCLib_User user, IRCLib_Channel channel) {
	ll_loop(user, uchan) {
		if (uchan->channel == channel) {
			// User already has this channel in his personal list
			return;
		}
	}

	IRCLib_UserChannel newchan = malloc(sizeof(struct sIRCLib_UserChannel));
	newchan->channel = channel;
	ll_append(user, newchan);
} // irclib_add_user_channel

/**
 * Remove channel from user's personal channel list
 * @param user User to remove channel from
 * @param channel Channel to remove
 */
void irclib_remove_user_channel(IRCLib_User user, IRCLib_Channel channel) {
	ll_loop(user, uchan) {
		if (uchan->channel == channel) {
			ll_remove(user, uchan);
			free(uchan);
			return;
		}
	}
} // irclib_remove_user_channel

/**
 * Find user in storage and return it's structure
 * @param storage User storage
 * @param nick Nick to find
 * @return IRCLib_User structure of corresponding user, or NULL if user was
 *   not found.
 */
IRCLib_User irclib_find_user(IRCLib_UserStorage storage, char *nick) {
	ll_loop(storage, user) {
		if (eq(user->host->nick, nick)) {
			return user;
		}
	}
	// User was not found
	return NULL;
} // irclib_find_user

/**
 * Remove user from storage
 * @param storage User storage
 * @param user User to be removed from that storage
 */
void irclib_remove_user(IRCLib_UserStorage storage, IRCLib_User user) {
	// Remove user from storage
	ll_remove(storage, user);

	// Remove user from channels
	ll_loop(user, uchan) {
		irclib_remove_channel_user(uchan->channel, user->host->nick);
	}

	irclib_free_addr(user->host);
	free(user);
} // irclib_remove_user

/**
 * Change nick of user
 * @param user User whos nick will be changed
 * @param newnick New nick
 */
void irclib_rename_user(IRCLib_User user, char *newnick) {
	if (!eq(user->host->nick, newnick)) {
		free(user->host->nick);
		user->host->nick = strdup(newnick);
	}
} // irclib_rename_user

/**
 * Count number of channels that user is on
 */
int irclib_count_channels(IRCLib_User user) {
	int i = 0;
	ll_loop(user, channel) {
		i++;
	}
	return i;
} // irclib_count_channels

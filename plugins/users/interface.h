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

#ifndef _AUTOJOIN_INTERFACE
#define _AUTOJOIN_INTERFACE 1

#include <pluginapi.h>

#include <stdbool.h>
#include <events.h>
#include <config/config.h>
#include "../telnet/interface.h"

// Forward
typedef struct sUsersList *UsersList;
typedef struct sUsersListUser *UsersListUser;

/**
 * Users plugin data
 */
typedef struct {
	PluginInfo *info;			/**< Plugin info */
	CONF_SECTION *usersdb;		/**< Users database */
	EVENT_HANDLER *ontelnetcmd;	/**< Event handler */
	EVENT_HANDLER *ontelnetconnected; /**< Event handler */
	EVENT_HANDLER *onjoin;		/**< Event handler */
} UsersPluginData;

/**
 * Event data for onusersdbchanged event.
 */
typedef struct {
	CONF_SECTION *usersdb;		/**< Users database */
} Users_DbChangedEvent;

/**
 * Structure to hold user's login data to telnet session.
 */
typedef struct {
	char *login;				/**< Login name */
	char *password;				/**< Password */
	CONF_SECTION *usersdb;		/**< Users database */
} Users_TelnetLoginData;

/**
 * User in users list
 */
struct sUsersListUser {
	CONF_SECTION *user;			/**< User's entry in users db */
	UsersListUser prev;			/**< Previous user in list */
	UsersListUser next;			/**< Last user in list */
};

/**
 * Users list
 */
struct sUsersList {
	UsersListUser first;		/**< First user in list */
	UsersListUser last;			/**< Last user in list */
}; // sUsersList

/**
 * Handles ontelnetcmd event
 * @param event Event data
 */
extern void users_telnetcommands(EVENT *event);

/**
 * Process telnet password
 * @param client Client that sent key
 * @param key Key code
 */
extern bool users_telnetpasswreceive(TelnetClient client, KeyCode key);

/**
 * Process telnet login
 * @param client Client that sent key
 * @param key Key code
 */
extern bool users_telnetloginreceive(TelnetClient client, KeyCode key);

/**
 * Event handler to require user login on telnet session
 * @param event Event
 */
extern void users_telnetconnected(EVENT *event);

/**
 * Verify username and password against users database and return true if
 * username is verified, false if not.
 * @param usersdb Users database to match against
 * @param username Username
 * @param password Password
 * @return True if username and password match users database, false if not.
 */
extern bool users_login(CONF_SECTION *usersdb, char *username, char *password);

/**
 * Match host against entry in users database and returns true if the user
 * matches.
 * @param user Users database entry
 * @param host Hostname in nick!user@host form
 */
extern bool users_match_user_host(CONF_SECTION *user, char *host);

/**
 * Match host against users database and returns list of users that matches.
 * @param usersdb Users db
 * @param host Hostname in nick!user@host form
 */
extern UsersList users_match_host(CONF_SECTION *usersdb, char *host);

/**
 * Get user's privilege value. If users list contains more than one user, it
 * scans all users in that list. The priority of privilege value is:
 * 1. First user in list, global privilege
 * 2. First user in list, section specific privilege (if section isn't NULL)
 * 3. Second user in list, global privilege
 * 4. Second user in list, section specific privilege (if section isn't NULL)
 * and so on till the end of list.
 * @param list Users list
 * @param section Name of section, if NULL, only global privileges will be
 *   scanned.
 * @param privname Name of privilege
 */
extern int users_get_priv(UsersList list, char *section, char *privname);

/**
 * Init users list
 * @return Initialized user's list
 */
extern UsersList users_init_list();

/**
 * Free users list
 * @param list Users list to free
 */
extern void users_free_list(UsersList list);

/**
 * Add another user into list
 * @param list Users list in which to add new user
 * @param dbuser User from users database
 */
extern void users_add_to_list(UsersList list, CONF_SECTION *dbuser);

#endif

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
#include <stdbool.h>
#include <string.h>

// Plugin interface
#include <interface.h>

// My libraries
#include <toolbox/tb_string.h>
#include <toolbox/wildcard.h>
#include <toolbox/linkedlist.h>
#include <config/config.h>

/**
 * Verify username and password against users database and return true if
 * username is verified, false if not.
 * @param usersdb Users database to match against
 * @param username Username
 * @param password Password
 * @return True if username and password match users database, false if not.
 */
bool users_login(CONF_SECTION *usersdb, char *username, char *password) {
	CONF_SECTION *user = config_lookup_section(usersdb, username, false);
	if (user != NULL) {
		char *setpass = config_getvalue_string(user, "password", "");
		if (!eq(setpass, "") && eq(setpass, password)) {
			return true;
		}
	}
	return false;
} // users_login

/**
 * Match host against entry in users database and returns true if the user
 * matches.
 * @param user Users database entry
 * @param host Hostname in nick!user@host form
 */
bool users_match_user_host(CONF_SECTION *user, char *host) {
	for (size_t i = 0; i < user->valuesCount; i++) {
		if (eq(user->values[i]->name, "host") &&
			wildmatch_ci(htval_get_string(user->values[i]->value), host)) {

			return true;
		}
	}

	return false;
} // users_match_user_host

/**
 * Match host against users database and returns list of users that matches.
 * @param usersdb Users db
 * @param host Hostname in nick!user@host form
 */
UsersList users_match_host(CONF_SECTION *usersdb, char *host) {
	UsersList result = users_init_list();

	for (size_t i = 0; i < usersdb->subSectionsCount; i++) {
		if (users_match_user_host(usersdb->subSections[i], host)) {
			users_add_to_list(result, usersdb->subSections[i]);
		}
	}

	return result;
} // users_match_host

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
int users_get_priv(UsersList list, char *section, char *privname) {
	int privValue = 0;
	if (list == NULL) return privValue;

	char *globalname = NULL;
	char *sectionname = NULL;
	asprintf(&globalname, "privileges:%s", privname);
	if (section != NULL) {
		asprintf(&sectionname, "privileges:%s:%s", section, privname);
	}

	ll_loop(list, user) {
		// First, try user's section privilege, if exists, use it, otherwise
		// try global value.
		if (sectionname != NULL && config_lookup(user->user, sectionname, false)) {
			privValue = config_getvalue_int(user->user, sectionname, 0);
		} else if (config_lookup(user->user, globalname, false)) {
			privValue = config_getvalue_int(user->user, globalname, 0);
		}
	}

	if (globalname) {
		free(globalname);
	}
	if (sectionname) {
		free(sectionname);
	}

	return privValue;
} // users_get_priv

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

// My libraries
#include <toolbox/linkedlist.h>
#include <config/config.h>

// This plugin interface
#include "interface.h"

/**
 * Init users list
 * @return Initialized user's list
 */
UsersList users_init_list() {
	UsersList result = malloc(sizeof(struct sUsersList));
	ll_init(result);
	return result;
} // users_init_list

/**
 * Free users list
 * @param list Users list to free
 */
void users_free_list(UsersList list) {
	if (list == NULL) return;

	ll_loop(list, user) {
		free(user);
	}
	free(list);
} // users_free_list

/**
 * Add another user into list
 * @param list Users list in which to add new user
 * @param dbuser User from users database
 */
void users_add_to_list(UsersList list, CONF_SECTION *dbuser) {
	UsersListUser user = malloc(sizeof(struct sUsersListUser));
	user->user = dbuser;
	ll_append(list, user);
} // users_add_to_list

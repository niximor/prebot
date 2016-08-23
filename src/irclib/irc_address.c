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
#include <string.h>

// My libraries
#include <dynastring.h>

// This library interface
#include "irclib.h"

/**
 * Parse address to nick, username and host structure
 * @param address Address in format nick!user@host
 * @return Pointer to structure IRCLib_Host
 */
IRCLib_Host *irclib_parse_addr(char *address) {
	string nick = dynastring_init();
	string user = dynastring_init();
	string host = dynastring_init();

	enum {
		IRCADDR_NICK,
		IRCADDR_USER,
		IRCADDR_HOST
	} whereToAdd = IRCADDR_NICK;
	for (size_t i = 0; i < strlen(address); i++) {
		switch (whereToAdd) {
			case IRCADDR_NICK:
				if (address[i] == '!') {
					whereToAdd = IRCADDR_USER;
				} else {
					dynastring_appendchar(nick,
						address[i]);
				}
				break;

			case IRCADDR_USER:
				if (address[i] == '@') {
					whereToAdd = IRCADDR_HOST;
				} else {
					dynastring_appendchar(user,
						address[i]);
				}
				break;

			case IRCADDR_HOST:
				dynastring_appendchar(host, address[i]);
				break;
		}
	}

	// Just to be sure, none of this three strings should be empty...
	dynastring_appendchar(nick, '\0');
	dynastring_appendchar(user, '\0');
	dynastring_appendchar(host, '\0');

	IRCLib_Host *result = malloc(sizeof(IRCLib_Host));
	if (result != NULL) {
		result->nick = strdup(nick->data);
		result->user = strdup(user->data);
		result->host = strdup(host->data);
	}

	dynastring_free(nick);
	dynastring_free(user);
	dynastring_free(host);

	return result;
} // irclib_parse_addr

/**
 * Free IRCLib_Host structure
 * @param host Pointer to IRCLib_Host structure
 */
void irclib_free_addr(IRCLib_Host *host) {
	if (host != NULL) {
		free(host->nick);
		free(host->user);
		free(host->host);
		free(host);
	}
} // irclib_free_addr

/**
 * Construct nick!user@host address from IRCLib_Host structure
 * @param host IRCLib_Host
 * @return Allocated string with nick!user@host format
 */
char *irclib_construct_addr(IRCLib_Host *host) {
	string result = dynastring_init();
	dynastring_appendstring(result, host->nick);
	dynastring_appendchar(result, '!');
	dynastring_appendstring(result, host->user);
	dynastring_appendchar(result, '@');
	dynastring_appendstring(result, host->host);

	char *out = strdup(dynastring_getstring(result));
	dynastring_free(result);
	return out;
}

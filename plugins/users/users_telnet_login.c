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
#include <ctype.h>

// Plugins API
#include <pluginapi.h>

// This plugin interface
#include "interface.h"

// My libraries
#include "../telnet/interface.h"
#include <toolbox/tb_string.h>
#include <toolbox/tb_num.h>
#include <config/config.h>
#include <htable/keyvalpair.h>

/**
 * Process telnet password
 * @param client Client that sent key
 * @param key Key code
 */
bool users_telnetpasswreceive(TelnetClient client, KeyCode key) {
	if (key == KEY_ENTER) {
		client->send(client, "\r\n", 2);

		// Verify user name and password
		Users_TelnetLoginData *loginData =
			(Users_TelnetLoginData *)client->interactiveData;
		loginData->password = strdup(dynastring_getstring(client->recvbuffer));
		dynastring_clear(client->recvbuffer);

		if (users_login(loginData->usersdb,
			loginData->login, loginData->password)) {

			// ToDo: Check user's telnet privilege

			kvp_set(client->kvp, "user", htval_string(loginData->login));

			free(loginData->login);
			free(loginData->password);
			free(loginData);
			client->interactiveData = NULL;

			// If username and password is valid...
			telnet_motd(client);
		} else {
			free(loginData->login);
			free(loginData->password);
			free(loginData);
			client->interactiveData = NULL;

			// ...and Invalid
			telnet_send(client, "Unknown username or password. Bye.");
			telnet_disconnect(client);
		}
	} else {
		telnet_receiveWithoutEcho(client, key);
	}

	return false; // Do not leave interactive mode
} // users_telnetpasswreceive

/**
 * Process telnet login
 * @param client Client that sent key
 * @param key Key code
 */
bool users_telnetloginreceive(TelnetClient client, KeyCode key) {
	if (key == KEY_ENTER) {
		client->dataCallback = users_telnetpasswreceive;
		Users_TelnetLoginData *loginData =
			(Users_TelnetLoginData *)client->interactiveData;
		loginData->login = strdup(dynastring_getstring(client->recvbuffer));
		dynastring_clear(client->recvbuffer);

		client->send(client, "\r\nPassword: ", 12);
	} else {
		// Accept only printable characters as username, and ignore the rest.
		if (inrange(key, 33, 126)) {
			telnet_receiveWithEcho(client, key);
		}
	}

	return false; // Do not leave interactive mode
} // users_telnetloginreceive

/**
 * Event handler to require user login on telnet session
 * @param event Event
 */
void users_telnetconnected(EVENT *event) {
	Telnet_Command *eventData = (Telnet_Command *)event->customData;
	TelnetClient client = eventData->client;

	// If user isn't already logged in.
	if (!kvp_isset(client->kvp, "user")) {
		UsersPluginData *plugData = (UsersPluginData *)event->handlerData;

		eventData->setInteractive = true;
		eventData->callback = users_telnetloginreceive;

		Users_TelnetLoginData *loginData = malloc(sizeof(Users_TelnetLoginData));
		loginData->login = NULL;
		loginData->password = NULL;
		loginData->usersdb = plugData->usersdb;

		client->interactiveData = loginData;
		client->send(client, "Login: ", 7);
		event->cancelBubble = true;
	}
} // users_telnetconnected

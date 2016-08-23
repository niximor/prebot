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

// Standard includes
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

// Linux includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

// This library interface
#include "irclib.h"

// My includes
#include <io.h>
#include <events.h>
#include <dynastring.h>
#include <socketpool.h>
#include <timers.h>

/**
 * Initialize the IRC library. For documentation about events see
 * irclib_events.txt
 * @param connection IRCLib_Connection structure with existing events instance.
 */
void irclib_init(IRCLib_Connection *connection) {
	if (connection->events != NULL) {
		events_addEvent(connection->events, "onconnected");
		events_addEvent(connection->events, "ondisconnected");

		events_addEvent(connection->events, "onrawreceive");
		events_addEvent(connection->events, "onrawsend");

		events_addEvent(connection->events, "onping");
		events_addEvent(connection->events, "onservermessage");

		events_addEvent(connection->events, "onjoin");
		events_addEvent(connection->events, "onjoined");
		events_addEvent(connection->events, "onpart");
		events_addEvent(connection->events, "onparted");

		events_addEvent(connection->events, "onprivatemessage");
		events_addEvent(connection->events, "onchannelmessage");
		events_addEvent(connection->events, "onprivatenotice");
		events_addEvent(connection->events, "onchannelnotice");

		events_addEvent(connection->events, "onchangeprefix");
		events_addEvent(connection->events, "onop");
		events_addEvent(connection->events, "ondeop");
		events_addEvent(connection->events, "onvoice");
		events_addEvent(connection->events, "ondevoice");
		events_addEvent(connection->events, "onhalfop");
		events_addEvent(connection->events, "ondehalfop");
		events_addEvent(connection->events, "onmode");

		events_addEvent(connection->events, "onchangelist");
		events_addEvent(connection->events, "onban");
		events_addEvent(connection->events, "onunban");

		events_addEvent(connection->events, "onkick");
		events_addEvent(connection->events, "onkicked");

		events_addEvent(connection->events, "onnickchanged");
		events_addEvent(connection->events, "onnick");

		events_addEvent(connection->events, "onquited");
	}

	connection->status = IRC_DISCONNECTED;
	connection->socket = 0;
	connection->recvbuffer = dynastring_init();

	connection->networkName = NULL;
	connection->userPrefixes = NULL;

	connection->chanModesAddress = NULL;
	connection->chanModesAlwaysParam = NULL;
	connection->chanModesSetParam = NULL;
	connection->chanModesNeverParam = NULL;

	// Init channel list
	connection->channelStorage = irclib_init_channels();

	// Init user storage
	connection->userStorage = irclib_init_userstorage();
} // irclib_init

/**
 * Connect to IRC server
 * @param connection Filled in IRCLib_Connection structure with hostname,
 *   port, username, nickname and realname.
 * @return 1 if connection succeded and irclib is prepared for data receive,
 *   false otherwise.
 */
int irclib_connect(IRCLib_Connection *connection) {
	if (connection->status == IRC_CONNECTING ||
		connection->status == IRC_CONNECTED) return 0;

	// Add reconnect callback
	if (connection->reconnecttimer == NULL) {
		connection->reconnecttimer = timers_add(TM_TIMEOUT, 2,
			irclib_timerreconnect, connection);
	}

	printError("irclib", "Connecting to %s:%d",
		connection->hostname, connection->port);

	connection->status = IRC_CONNECTING;

	struct addrinfo *result;

    int family = AF_UNSPEC;

    if (connection->force_ipv4) {
        family = AF_INET;
    } else if (connection->force_ipv6) {
        family = AF_INET6;
    }

    struct addrinfo hints = {
        .ai_family = family,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
        .ai_flags = 0,
    };

	int error = getaddrinfo(connection->hostname, NULL, &hints, &result);
	if (error != 0) {
		printError("irclib", "Unable to resolve hostname: %s",
			gai_strerror(error));
		connection->status = IRC_DISCONNECTED;
		return 0;
	}

	connection->socket = socket(result->ai_family, SOCK_STREAM, 0);
	//connection->socket = socket(PF_INET6, SOCK_STREAM, 0);

	int val = 1;
	setsockopt(connection->socket, IPPROTO_TCP, SO_KEEPALIVE, &val, sizeof(val));

	if (connection->socket < 0) {
		printError("irclib", "Socket creation failed: %s",
			strerror(errno));
		connection->status = IRC_DISCONNECTED;
		return 0;
	}

	if (result->ai_family == AF_INET) {
		printError("irclib", "Will use IPv4.");
		((struct sockaddr_in *)result->ai_addr)->sin_port =
			htons(connection->port);
	} else if (result->ai_family == AF_INET6) {
		printError("irclib", "Will use IPv6.");
		((struct sockaddr_in6 *)result->ai_addr)->sin6_port =
			htons(connection->port);
	}

	// Try to bind to outgoing interface address, if specified in irc:bind config item.
	if (connection->bind) {
		struct addrinfo *result, *res;
		int error = getaddrinfo(connection->bind, NULL, &hints, &result);
		if (error == 0) {
			res = result;
			while (res) {
				if (bind(connection->socket, res->ai_addr, res->ai_addrlen) == 0) {
					break;
				}
				res = res->ai_next;
			}
			if (res == NULL) {
				printError("irclib", "Unable to bind to specified address.");
			}
			freeaddrinfo(result);
		} else {
			if (result->ai_family == AF_INET6) {
				struct sockaddr_in6 addr;
				if (inet_pton(result->ai_family, connection->bind, &addr.sin6_addr) > 0) {
					addr.sin6_family = AF_INET6;
					addr.sin6_port = 0;
					addr.sin6_flowinfo = 0;
				} else {
					printError("irclib", "Specified address is not a valid IPv6 address.");
					addr.sin6_addr = in6addr_any;
				}
				bind(connection->socket, &addr, sizeof(addr));
			} else {
				struct sockaddr_in addr;
				if (inet_pton(result->ai_family, connection->bind, &addr.sin_addr) > 0) {
					addr.sin_family = AF_INET;
					addr.sin_port = 0;
				} else {
					printError("irclib", "Specified address is not a valid IPv4 address.");
					addr.sin_addr.s_addr = INADDR_ANY;
				}
				bind(connection->socket, &addr, sizeof(addr));
			}
		}
	}

	/*struct in_addr addr = { .s_addr = 0 };
	inet_aton(connection->hostname, &addr);

	struct sockaddr_in address = {
		.sin_family = AF_INET,
		.sin_port = htons(connection->port),
		.sin_addr = addr
	};

	if (connect(connection->socket, (struct sockaddr *)&address,
		sizeof(address)) < 0) {

		printError("irclib", "Unable to connect to server: %s",
			strerror(errno));
		return 0;
	}*/

	if (connect(connection->socket, result->ai_addr,
		result->ai_addrlen) < 0) {

		printError("irclib", "Unable to connect to server: %s",
			strerror(errno));
		connection->status = IRC_DISCONNECTED;
		freeaddrinfo(result);
		return 0;
	}

	freeaddrinfo(result);

	// Add socket to socketpool
	socketpool_add(connection->socketpool, connection->socket, irclib_receive,
		NULL, irclib_closed, connection);

	printError("irclib", "Connected.");

	// Register client to IRC server
	if (connection->password != NULL && *(connection->password) != '\0') {
		irclib_sendraw(connection, "PASS %s", connection->password);
	}
	irclib_sendraw(connection, "NICK %s", connection->nickname);
	irclib_sendraw(connection, "USER %s * * :%s", connection->username,
		connection->realname);

	// ToDo: Configurable
        if (connection->testalivetimer == NULL) {
		connection->testalivetimer = timers_add(TM_TIMEOUT,
			connection->aliveCheckTimeout,
			irclib_timercheckalive, connection);
        }

	return 1;
} // irclib_connect

/**
 * Try to receive data from IRC server
 * @param connection IRCLib_Connection structure that is connected to IRC
 *   server.
 * @param data Pointer to 512-byte length buffer where each message can be
 *   stored.
 * @return Return 1 if message was received, 0 otherwise or -1 on error.
 */
void irclib_receive(Socket socket) {
	IRCLib_Connection *connection = (IRCLib_Connection *)socket->customData;

	int ch = 0;
	int readed = 0;

	// Just to explain this: read returns 0 when it has no data
	// to read from, and -1 on error. If some data has been read,
	// >0 is returned. Fine. 0 is when there are no data, but we
	// won't go there if we don't have data because of select in socketpool.
	// So only one thing can end up with read returning 0 -
	// connection was terminated by the other side.
	while ((readed = read(connection->socket, &ch, 1)) > 0) {
		if (ch == EOF) {
			readed = -1;
			break;
		}

		// Skip \r char
		if (ch == '\r') continue;

		// \n ends the reading
		if (ch == '\n') {
			// Fire rawreceive event and if event chain wasn't cancelled,
			// process to parsing the message.
			connection->lastActivity = time(NULL);

			IRCEvent_RawData evt = {
				.sender = connection,
				.message = strdup(dynastring_getstring(connection->recvbuffer))
			};
			if (events_fireEvent(connection->events, "onrawreceive", &evt)) {
				irclib_parse(connection, evt.message);
			}
			free(evt.message);
			dynastring_clear(connection->recvbuffer);

			// Break execution, so that when receiving much IRC data
			// won't slow down other bot functions.
			break;
		}

		dynastring_appendchar(connection->recvbuffer, ch);
	}

	if (readed <= 0) {
		// Connection was interrupted...
		socketpool_close(connection->socketpool, socket->socketfd);
	}
} // irclib_receive

/**
 * Close connection to IRC server
 * @param connection IRCLib_Connection structure
 */
void irclib_close(IRCLib_Connection *connection) {
	// Close socket
	connection->status = IRC_QUITED;
	socketpool_close(connection->socketpool, connection->socket);

	free(connection->nickname);
	dynastring_free(connection->recvbuffer);

	irclib_shutdown(connection);

	// Free channels
	irclib_free_channels(connection->channelStorage);

	// Free user storage
	irclib_free_userstorage(connection->userStorage);
} // irclib_close

/**
 * Sends raw data to IRC server
 * @param connection IRCLib_Connection structure that is connected to IRC
 *   server.
 * @param format Format of data to send to server
 */
void irclib_sendraw(IRCLib_Connection *connection, const char *format, ...) {
	char *buffer;

	// If not connected to server...
	if (connection->status != IRC_CONNECTED &&
		connection->status != IRC_CONNECTING) {

		return;
	}

	va_list ap;
	va_start(ap, format);
	vasprintf(&buffer, format, ap);
	va_end(ap);

	// Fire the rawsend event, and if it wasn't cancelled, send
	// data to server.
	IRCEvent_RawData evt = {
		.sender = connection,
		.message = buffer
	};
	if (events_fireEvent(connection->events, "onrawsend", &evt)) {
		if (evt.message != NULL) {
			// Append \r\n to message
			evt.message = realloc(evt.message,
				(strlen(evt.message) + 3) * sizeof(char));
			if (evt.message != NULL) {
				strcat(evt.message, "\r\n");

				socketpool_send(connection->socketpool, connection->socket,
					evt.message, strlen(evt.message));
			}
		}
	}

	if (evt.message != NULL) {
		free(evt.message);
	}
} // irclib_sendraw

/**
 * Shut down IRCLib - free structures allocated after connection
 * @param connection IRCLib_Connection structure
 */
void irclib_shutdown(IRCLib_Connection *connection) {
	// Free memory allocated during running
	if (connection->networkName != NULL) {
		free(connection->networkName);
		connection->networkName = NULL;
	}

	if (connection->userPrefixes != NULL) {
		free(connection->userPrefixes);
		connection->userPrefixes = NULL;
	}

	if (connection->userPrefixesSymbols != NULL) {
		free(connection->userPrefixesSymbols);
		connection->userPrefixesSymbols = NULL;
	}

	if (connection->chanModesAddress != NULL) {
		free(connection->chanModesAddress);
		connection->chanModesAddress = NULL;
	}

	if (connection->chanModesAlwaysParam != NULL) {
		free(connection->chanModesAlwaysParam);
		connection->chanModesAlwaysParam = NULL;
	}

	if (connection->chanModesSetParam != NULL) {
		free(connection->chanModesSetParam);
		connection->chanModesSetParam = NULL;
	}

	if (connection->chanModesNeverParam != NULL) {
		free(connection->chanModesNeverParam);
		connection->chanModesNeverParam = NULL;
	}

	// This timer gets reset each time the connection is made.
	if (connection->testalivetimer) {
		timers_remove(connection->testalivetimer);
		connection->testalivetimer = NULL;
	}

	// Free channels
	irclib_clear_channels(connection->channelStorage);

	// Free user storage
	irclib_clear_userstorage(connection->userStorage);
} // irclib_shutdown

/**
 * Triggered when connection to IRC server has been closed.
 * @param socket Socket from socketpool that has been closed.
 */
void irclib_closed(Socket socket) {
	IRCLib_Connection *connection = (IRCLib_Connection *)socket->customData;

	printError("irclib", "Got disconnected from IRC.");

	IRCEvent_Notify evt = { .sender = connection };
	events_fireEvent(connection->events, "ondisconnected", &evt);

	irclib_shutdown(connection);

	if (connection->status != IRC_QUITED) {
		connection->status = IRC_DISCONNECTED;
		if (connection->reconnect) {
			irclib_shutdown(connection);
			irclib_connect(connection);
		}
	}
} // irclib_closed

/**
 * Test if irclib is connected, and if not, try to reconnect.
 * @param timer Timer data
 * @return Always returns true, because the timer should be always resetted.
 */
bool irclib_timerreconnect(Timer timer) {
	IRCLib_Connection *connection = (IRCLib_Connection *)timer->customData;
	if (connection->status == IRC_DISCONNECTED && connection->reconnect) {
		irclib_connect(connection);
	}

	return true;
} // irclib_timerreconnect

/**
 * Test if irclib is still connected to IRC. Sends PING to myself.
 * @param timer Timer data
 * @return Always return true, because timer should be always resetted.
 */
bool irclib_timercheckalive(Timer timer) {
	IRCLib_Connection *connection = (IRCLib_Connection *)timer->customData;

	if (connection->lastActivity < time(NULL) - timer->setTimeout * 4) {
		// No message has been received to our PING request, do the reconnect.
		printError("irclib", "Reconnecting to stoned server...");
		irclib_sendraw(connection, "QUIT :Reconnecting...");
		socketpool_close(connection->socketpool, connection->socket);

		// Need to set this, because when returning false from here,
		// timer is removed, but reference to it remains. So at another
		// connect, it is not recreated.
		connection->testalivetimer = NULL;
		return false;
	}

	if (connection->lastActivity < time(NULL) - timer->setTimeout) {
		irclib_sendraw(connection, "PING :%s", connection->nickname);

		// Debug, will not trigger server reply:
		//irclib_sendraw(connection, "PRIVMSG #rls.rct.cz :Blabla");
	}

	return true;
} // irclib_timercheckalive

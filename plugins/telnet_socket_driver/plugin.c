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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Sockets-related includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

// Plugins API
#include <pluginapi.h>

// My includes
#include <io.h>
#include <plugins.h>
#include <htable/keyvalpair.h>

// Include telnet interface
#include "../telnet/interface.h"

// This plugin interface
#include "interface.h"

#ifndef PLUGIN_NAME
# define PLUGIN_NAME "telnet_socket_driver"
#endif

// Define this to see telnet client communication in console
//#define TS_DEBUG 1
#ifdef TS_DEBUG
# define ts_debug(...) fprintf(stderr, __VA_ARGS__)
#else
# define ts_debug(...)
#endif

/**
 * Sets non-blocking flag to socket.
 * Idea: http://www.lowtek.com/sockets/select.html
 * @param sock Socket file descriptor
 */
void telnet_socket_setnonblocking(int sock) {
	int opts = fcntl(sock,F_GETFL);
	if (opts >= 0) {
		opts = (opts | O_NONBLOCK);
		if (fcntl(sock,F_SETFL,opts) < 0) {
			printError("socketpool", "Unable to set socket to non-blocking "
				"state: %s", strerror(errno));
		}
	}
	return;
} // telnet_setnonblocking

/**
 * Send data over telnet client socket (telnet plugin callback)
 * @param client Telnet client which is connected using socket
 * @param buffer Data buffer
 * @param buffersize Data buffer size
 */
void telnet_socket_send(TelnetClient client, void *buffer,
	size_t buffersize) {

	TelnetSocketData socketdata = (TelnetSocketData)client->socketdata;
	socketpool_send(client->plugData->info->socketpool, socketdata->socketfd,
		buffer, buffersize);
} // telnet_send_over_socket

/**
 * Perform action on client's terminal screen. Only does something when client
 * accepted VT100 terminal.
 */
bool telnet_socket_action(TelnetClient client, Telnet_Action action,
	int param1, int param2) {

	TelnetSocketData socketdata = (TelnetSocketData)client->socketdata;

	switch (action) {
		case TA_ClearScreen: {
			if (socketdata->hasTerm) {
				// If terminal is available, it is easy :)
				char clear[4] = { 27, '[', '2', 'J' };
				client->send(client, clear, 4);

				// Move cursor to top left corner of screen
				client->action(client, TA_SetCursorPosition, 0, 0);
				return true;
			} else {
				// Not supported when terminal isn't available
				return false;
			}
			break;
		}

		case TA_ClearLine: {
			if (socketdata->hasTerm) {
				// If terminal is available, it is easy :)
				char clear[4] = { 27, '[', '2', 'K' };
				client->send(client, clear, 4);
				return true;
			} else {
				// Not supported, so far, so at least send line break
				client->send(client, "\r\n", 2);
				return false;
			}
			break;
		}

		case TA_SetCursorPosition: {
			if (socketdata->hasTerm) {
				char *setPos;
				// Order of params is x,y, but terminal wants rows first.
				asprintf(&setPos, "%c[%d;%df", 27, param2, param1);
				if (setPos) {
					client->send(client, setPos, strlen(setPos));
					free(setPos);
				}
				return true;
			} else {
				// This cannot be supported without terminal
				return false;
			}
			break;
		}

		case TA_MoveUp: {
			if (socketdata->hasTerm) {
				char *setPos;
				asprintf(&setPos, "%c[%dA", 27, param1);
				if (setPos) {
					client->send(client, setPos, strlen(setPos));
					free(setPos);
				}
				return true;
			} else {
				// This cannot be supported without terminal
				return false;
			}
			break;
		}

		case TA_MoveDown: {
			if (socketdata->hasTerm) {
				char *setPos;
				asprintf(&setPos, "%c[%dB", 27, param1);
				if (setPos) {
					client->send(client, setPos, strlen(setPos));
					free(setPos);
				}
				return true;
			} else {
				// This cannot be supported without terminal
				return false;
			}
			break;
		}

		case TA_MoveLeft: {
			if (socketdata->hasTerm) {
				char *setPos;
				asprintf(&setPos, "%c[%dD", 27, param1);
				if (setPos) {
					client->send(client, setPos, strlen(setPos));
					free(setPos);
				}
				return true;
			} else {
				// This cannot be supported without terminal
				return false;
			}
			break;
		}

		case TA_MoveRight: {
			if (socketdata->hasTerm) {
				char *setPos;
				asprintf(&setPos, "%c[%dC", 27, param1);
				if (setPos) {
					client->send(client, setPos, strlen(setPos));
					free(setPos);
				}
				return true;
			} else {
				// This cannot be supported without terminal
				return false;
			}
			break;
		}

		case TA_SetColor: {
			// Not supported yet.
			return false;
			break;
		}

		default: {
			return false;
		}
	}
}

/**
 * Disconnect telnet client.
 * @param client Telnet client
 */
void telnet_socket_disconnect(TelnetClient client) {
	// Free this client in my clients and remove it.
	PluginInfo *info = plugins_getinfo(PLUGIN_NAME);
	if (info != NULL) {
		TelnetSocketDriverPluginData *plugData =
			(TelnetSocketDriverPluginData *)info->customData;

		TelnetSocketData sockdata =
			(TelnetSocketData)client->socketdata;

		socketpool_close(plugData->info->socketpool, sockdata->socketfd);

		// Remove data handlers from socket - don't receive any data anymore.
		socketpool_add(plugData->info->socketpool, sockdata->socketfd,
			NULL, NULL, NULL, NULL);

		free(sockdata);

		TelnetSocketClient sclient = plugData->firstClient;
		TelnetSocketClient next;
		while (sclient != NULL) {
			next = sclient->next;

			if (sclient->client == client) {
				// Remove client from chain
				if (sclient->prev != NULL) {
					sclient->prev->next = sclient->next;
				} else {
					plugData->firstClient = sclient->next;
				}
				if (sclient->next != NULL) {
					sclient->next->prev = sclient->prev;
				} else {
					plugData->lastClient = sclient->prev;
				}

				free(sclient);
				break;
			}

			sclient = next;
		}
	}
} // telnet_socket_disconnect

/**
 * Socketpool callback indicating that client sent some data that we need
 * to receive.
 * @param socket Socketpool socket
 */
void telnet_socket_client_receive(Socket socket) {
	unsigned char ch;

	TelnetClient client = (TelnetClient)socket->customData;
	TelnetSocketData socketdata = (TelnetSocketData)client->socketdata;

	enum {
		ESC_CODE = 0,
		ESC_PARAM1 = 1,
		ESC_PARAM2 = 2
	} escState = ESC_CODE;
	unsigned long int escParam1 = 0;
	unsigned long int escParam2 = 0;
	char escCode = 0;

	while (read(socketdata->socketfd, &ch, 1) > 0) {
		// After each enter, 0 is send, which we don't need.
		if (ch == 0) continue;

		ts_debug("%d %x (%c) ", ch, ch, (ch >= 32 && ch < 127)?ch:' ');

		switch (socketdata->state) {
			// Normal state, receiving user input
			case TS_NORMAL:
				switch (ch) {
					case TS_IAC:
						ts_debug("IAC\n");
						socketdata->state = TS_IAC;
						continue;

					case TS_ESC:
						ts_debug("ESC\n");
						socketdata->state = TS_ESC;
						escState = ESC_CODE;
						escParam1 = 0;
						escParam2 = 0;
						escCode = 0;
						continue;

					// No special char was entered, proceed normal processing
					// by telnet.
					default:
						break;
				}
				break;

			case TS_ESC: {
				bool doContinue = true;
				switch (escState) {
					// Awaiting [ char
					case ESC_CODE: {
						if (ch == '[') {
							escState = ESC_PARAM1;
						} else {
							doContinue = false;
						}
						break;
					}

					// Awaiting number or char
					case ESC_PARAM1: {
						if (ch >= '0' && ch <= '9') {
							// Is digit, so it is parameter
							escParam1 *= 10;
							escParam1 += ch - '0';
						} else if (ch == ';') {
							escState = ESC_PARAM2;
						} else {
							// It is not digit, it is end of command
							escCode = ch;
						}
						break;
					}

					case ESC_PARAM2: {
						if (ch >= '0' && ch <= '9') {
							// It is digit
							escParam2 *= 10;
							escParam2 += ch - '0';
						} else {
							// Not digit, must be escape code
							escCode = ch;
						}
					}
				}

				if (escCode > 0) {
					switch (escCode) {
						case 'A':
							telnet_received(client, KEY_UP);
							break;

						case 'B':
							telnet_received(client, KEY_DOWN);
							break;

						case 'C':
							telnet_received(client, KEY_RIGHT);
							break;

						case 'D':
							telnet_received(client, KEY_LEFT);
							break;



						// Unknown code
						default:
							break;
					}
				}

				if (doContinue) {
					continue;
				} else {
					socketdata->state = TS_NORMAL;
					break;
				}
			}

			case TS_IAC:
				switch (ch) {
					case TS_SB:
						ts_debug("SB\n");
						socketdata->state = TS_SB;
						break;

					case TS_WILL:
						ts_debug("WILL\n");
						socketdata->state = TS_WILL;
						break;

					case TS_WONT:
						ts_debug("WONT\n");
						socketdata->state = TS_WONT;
						break;

					case TS_DO:
						ts_debug("DO\n");
						socketdata->state = TS_DO;
						break;

					case TS_DONT:
						ts_debug("DONT\n");
						socketdata->state = TS_DONT;
						break;
				}
				continue;

			// IAC DO
			case TS_DO:
				switch (ch) {
					case TS_ECHO: {
						// Client accepted our local echo request, and will
						// not do local echo.
						ts_debug("ECHO\n");
						client->opts |= TC_ECHO;
						break;
					}

					case TS_GOAHEAD: {
						ts_debug("GOAHEAD\n");
						client->opts |= TC_UNBUFFERED;
						break;
					}

					default:
						ts_debug("IGNORE\n");
						break;
				}
				socketdata->state = TS_NORMAL;
				continue;

			// IAC DONT
			case TS_DONT:
				ts_debug("IGNORE\n");
				socketdata->state = TS_NORMAL;
				continue;

			// IAC WILL
			case TS_WILL:
				switch (ch) {
					case TS_NAWS: {
						// Client will send window size
						ts_debug("NAWS\n");
						break;
					}

					case TS_TERMTYPE: {
						// Client allowes us to send terminal type
						ts_debug("TERMTYPE\n");

						// Send VT100 terminal type
						char termtype[11] = { TS_IAC, TS_SB, TS_TERMTYPE, 0,
							'V', 'T', '1', '0', '0', TS_IAC, TS_SE };
						socketpool_send(socket->pool, socket->socketfd,
							termtype, 11);
						socketdata->hasTerm = true;
						break;
					}

					default: {
						ts_debug("IGNORE\n");
						break;
					}
				}
				socketdata->state = TS_NORMAL;
				continue;

			// IAC WONT
			case TS_WONT:
				ts_debug("IGNORE\n");
				socketdata->state = TS_NORMAL;
				continue;

			// IAC SB - Subnegotiation of parameters
			case TS_SB:
				socketdata->state = TS_IGNORE_TILL_SE;

				switch (ch) {
					case TS_NAWS:
						ts_debug("NAWS\n");
						client->windowWidth = 0;
						client->windowHeight = 0;
						socketdata->state = TS_NAWS_WIDTH_LSB;
						break;

					default:
						break;
				}
				continue;

			case TS_NAWS_WIDTH_MSB:
				client->windowWidth = 0xFF * ch;
				socketdata->state = TS_NAWS_WIDTH_LSB;
				continue;

			case TS_NAWS_WIDTH_LSB:
				client->windowWidth += ch;
				socketdata->state = TS_NAWS_HEIGHT_LSB;
				continue;

			case TS_NAWS_HEIGHT_MSB:
				client->windowHeight = 0xFF * ch;
				socketdata->state = TS_NAWS_HEIGHT_LSB;
				continue;

			case TS_NAWS_HEIGHT_LSB:
				client->windowHeight += ch;
				client->opts |= TC_WINDOWSIZE;
				socketdata->state = TS_IGNORE_TILL_SE;
				continue;

			case TS_IGNORE:
				ts_debug("IGNORE\n");
				socketdata->state = TS_NORMAL;
				continue;

			case TS_IGNORE_TILL_SE:
				switch (ch) {
					case TS_SE:
						ts_debug("SE\n");
						socketdata->state = TS_NORMAL;
						break;

					default:
						ts_debug("IGNORE TILL SE\n");
						break;
				}
				continue;

			case TS_KEYCODE1:
				if (ch == '[') {
					ts_debug("CONTROL\n");
					socketdata->keycode = ch << 8;
					socketdata->state = TS_KEYCODE2;
				} else {
					ts_debug("NORMAL\n");
				}
				continue;

			case TS_KEYCODE2:
				ts_debug("KEYCODE2\n");
				socketdata->keycode |= ch;

				// 0x5b3* has 3 bytes (??)
				if ((socketdata->keycode & 0x7ff0) == 0x5b30) {
					socketdata->keycode = socketdata->keycode << 8;
					socketdata->state = TS_KEYCODE3;
				} else {
					//telnet_socket_keycode(client);
					socketdata->state = TS_NORMAL;
				}
				continue;

			case TS_KEYCODE3:
				ts_debug("KEYCODE3\n");
				socketdata->keycode |= ch;
				//telnet_socket_keycode(client);
				socketdata->state = TS_NORMAL;
				continue;

			default:
				// Nothing special, unknown state...
				socketdata->state = TS_NORMAL;
				break;
		}

		if (ch == 127) {
			telnet_received(client, KEY_BACKSPACE);
		} else if (ch == TS_ESC) {
			telnet_received(client, KEY_ESC);
		} else {
			telnet_received(client, ch);

			// Don't read any more data, if command was completed.
			if (ch == KEY_ENTER) {
				break;
			}
		}
	}

	// If we are inside escape sequence, cancel it and treat this as escape
	// key. And don't care about sockets marked for closing, because they
	// should not receive any more data.
	if (!socket->shouldBeClosed) {
		if (socketdata->state == TS_ESC) {
			if (escState == ESC_CODE) {
				telnet_received(client, KEY_ESC);
			}
			socketdata->state = TS_NORMAL;
		}
	}
} // telnet_client_receive

/**
 * Socketpool callback indicating that server has client that wants to be
 * connected.
 * @param socket Socketpool socket
 */
void telnet_socket_accept_client(Socket socket) {
	TelnetSocketDriverPluginData *plugData =
		(TelnetSocketDriverPluginData *)socket->customData;

	TelnetSocketData socketdata = malloc(sizeof(struct sTelnetSocketData));
	socklen_t addrsize = sizeof(socketdata->address);
	memset(&socketdata->address, 0, addrsize);

	if ((socketdata->socketfd = accept(socket->socketfd,
		(struct sockaddr *)&socketdata->address, &addrsize)) < 0) {

		printError(PLUGIN_NAME, "Failed to accept new client: %s",
			strerror(errno));
		free(socketdata);
		return;
	}

	// No terminal by default
	socketdata->hasTerm = false;

	// New client is connected.
	printError(PLUGIN_NAME, "New client connected from IP %s.",
		inet_ntoa(socketdata->address.sin_addr));

	// Sets client socket to non-blocking mode to prevent stuck in
	// receive function.
	telnet_socket_setnonblocking(socketdata->socketfd);

	socketpool_add(plugData->info->socketpool, socketdata->socketfd,
		NULL, NULL, NULL, NULL);

	TelnetClient client = telnet_add_client(
		socketdata, telnet_socket_send, telnet_socket_action,
		telnet_socket_disconnect);

	socketpool_add(plugData->info->socketpool, socketdata->socketfd,
		telnet_socket_client_receive, NULL, NULL, client);

	if (client == NULL) {
		socketpool_send(plugData->info->socketpool, socketdata->socketfd,
			"Sorry, telnet server has refused to accept you.\r\n", 45);
		socketpool_close(plugData->info->socketpool, socketdata->socketfd);

		free(socketdata);
		printError(PLUGIN_NAME, "Telnet has refused our client.");
		return;
	} else {
		kvp_set(client->kvp, "driver", htval_string("socket"));
		kvp_set(client->kvp, "ip",
			htval_string(inet_ntoa(socketdata->address.sin_addr)));
		client->opts = TC_NORMAL;
	}

	// Send telnet "handshake"
	socketdata->state = TS_NORMAL;

	char handshake[] = {
		TS_IAC, TS_WILL, TS_GOAHEAD,
		TS_IAC, TS_WILL, TS_ECHO,
		TS_IAC, TS_DO, TS_NAWS,
		TS_IAC, TS_DO, TS_GOAHEAD,
		TS_IAC, TS_DO, TS_TERMTYPE
	};
	socketpool_send(plugData->info->socketpool, socketdata->socketfd,
		handshake, 15);

	TelnetSocketClient socketclient =
		malloc(sizeof(struct sTelnetSocketClient));

	socketclient->client = client;

	// Add socketclient to linked list of socket clients
	socketclient->next = NULL;
	socketclient->prev = plugData->lastClient;
	plugData->lastClient = socketclient;

	if (socketclient->prev != NULL) {
		socketclient->prev->next = socketclient;
	} else {
		plugData->firstClient = socketclient;
	}
} // telnet_accept_client

/**
 * Initialize plugin.
 * @param info Plugin info, where this function must fill in some informations
 */
void PluginInit(PluginInfo *info) {
	info->name = "Telnet socket driver";
	info->author = "Niximor";
	info->version = "1.0.0";

	TelnetSocketDriverPluginData *plugData =
		malloc(sizeof(TelnetSocketDriverPluginData));

	plugData->info = info;
	info->customData = plugData;

	plugData->lastClient = NULL;
	plugData->firstClient = NULL;

	// Start telnet server
	plugData->serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (plugData->serverSocket < 0) {
		printError(PLUGIN_NAME, "Unable to create socket: %s",
			strerror(errno));
		return;
	}
	telnet_socket_setnonblocking(plugData->serverSocket);

	int val = 1;
	setsockopt(plugData->serverSocket, SOL_SOCKET, SO_REUSEADDR,
		&val, sizeof(val));

	int port = config_getvalue_int(info->config, PLUGIN_NAME":port", 12345);
	struct sockaddr_in bindAddress = {
		.sin_family = AF_INET,
		.sin_addr = {
			.s_addr = INADDR_ANY
		},
		.sin_port = htons(port)
	};
	if (bind(plugData->serverSocket, (struct sockaddr *)&bindAddress,
		sizeof(bindAddress)) < 0) {

		printError(PLUGIN_NAME, "Unable to bind socket to 0.0.0.0:%d: %s",
			port, strerror(errno));
		return;
	}

	if (listen(plugData->serverSocket, 10) < 0) {
		printError(PLUGIN_NAME, "Unable to start listening: %s",
			strerror(errno));
		return;
	}

	socketpool_add(info->socketpool, plugData->serverSocket,
		telnet_socket_accept_client, NULL, NULL, plugData);
	printError(PLUGIN_NAME, "Server startup successful. Listening on 0.0.0.0:%d", port);
} // PluginInit

/**
 * Close plugin
 * @param info Plugin info, which this function may use to get some
 *   informations it may need.
 */
void PluginDone(PluginInfo *info) {
	// Close server socket
	TelnetSocketDriverPluginData *plugData =
		(TelnetSocketDriverPluginData *)info->customData;
	socketpool_close(plugData->info->socketpool, plugData->serverSocket);

	// Close client's sockets.
	TelnetSocketClient client = plugData->firstClient;
	TelnetSocketClient next;
	while (client != NULL) {
		next = client->next;

		telnet_disconnect(client->client);

		client = next;
	}

	free(plugData);
} // PluginDone

/**
 * Get list of dependencies
 * @param deps Dependencies
 */
void PluginDeps(char **deps) {
	*deps = "telnet"; // Depends on telnet
} // PluginDeps

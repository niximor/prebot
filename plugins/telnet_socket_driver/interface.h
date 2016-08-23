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

#ifndef _TELNET_SOCKET_DRIVER_INTERFACE
#define _TELNET_SOCKET_DRIVER_INTERFACE 1

// Standard headers
#include <stdint.h>

// Include telnet interface
#include "../telnet/interface.h"

// Forward
typedef struct sTelnetSocketData *TelnetSocketData;
typedef struct sTelnetSocketClient *TelnetSocketClient;

struct sTelnetSocketClient {
	TelnetSocketClient next;
	TelnetSocketClient prev;

	TelnetClient client;
}; // sTelnetSocketClient

typedef enum {
	TS_NORMAL = 0,				/**< Normal data, nothing special */
	TS_ESC = 27,				/**< Escaped data */

	// Telnet protocol
	TS_ECHO = 1,				/**< Controls echoing of typed-in characters */
	TS_GOAHEAD = 3,				/**< Go-ahead flow control for half-duplex
									 lines */
	TS_TERMTYPE = 24,			/**< Terminal typ */
	TS_NAWS = 31,				/**< Negotiate about window size */
	TS_LINEMODE = 34,			/**< Flow control (on/off) on client's side */
	TS_SE = 240,				/**< End of subnegotiation */
	TS_SB = 250,				/**< Subnegotiation of parameters */
	TS_WILL = 251,				/**< I'm willing to... */
	TS_WONT = 252,				/**< I'm not willing to... */
	TS_DO = 253,				/**< I will do ... */
	TS_DONT = 254,				/**< I won't do ... */
	TS_IAC = 255,				/**< Interpret as command */

	TS_IGNORE = 1000,			/**< Ignore next char */
	TS_IGNORE_TILL_SE,			/**< Ignore everything until TS_SE */

	TS_NAWS_WIDTH_MSB,			/**< Window size, width MSB */
	TS_NAWS_WIDTH_LSB,			/**< Window size, width LSB */
	TS_NAWS_HEIGHT_MSB,			/**< Window size, height MSB */
	TS_NAWS_HEIGHT_LSB,			/**< Window size, height LSB */

	TS_KEYCODE1,
	TS_KEYCODE2,
	TS_KEYCODE3
} TelnetSocketRecvState;

/**
 * To use by telnet clients to hold information about connected client
 */
struct sTelnetSocketData {
	int socketfd;				/**< Client socket file descriptor */
	struct sockaddr_in address;	/**< Client address */
	TelnetSocketRecvState state; /**< State of FSM for parsing incomming
									 data */
	uint32_t keycode;			/**< Key code from client */
	bool hasTerm;				/**< Set to true if we forced client to use
									 VT100 terminal. */
}; // sTelnetSocketData

/**
 * Data structures for telnet socket driver
 */
typedef struct {
	PluginInfo *info;			/**< PluginInfo */
	int serverSocket;			/**< Telnet server socket file descriptor */
	TelnetSocketClient firstClient; /**< First client connected to server */
	TelnetSocketClient lastClient; /**< Last client connected to server */
} TelnetSocketDriverPluginData;

#endif

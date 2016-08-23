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

#ifndef _TELNET_INTERFACE
#define _TELNET_INTERFACE 1

#include <pluginapi.h>

// Standard headers
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

// My libraries
#include <htable/keyvalpair.h>

// Forward
typedef struct sTelnetClient *TelnetClient;

/**
 * Key codes for special keys
 */
typedef enum {
	// Lower 256 chars are for normal keys
	KEY_TAB = 9,				/**< Tab */
	KEY_ENTER = 13,				/**< Enter */
	KEY_ESC = 27,				/**< Escape */
	KEY_SPACE = 32,				/**< Space key */
	KEY_UP = 1000,				/**< Up arrow */
	KEY_DOWN,					/**< Down arrow */
	KEY_LEFT,					/**< Left arrow */
	KEY_RIGHT,					/**< Right arrow */
	KEY_PGUP,					/**< Page up */
	KEY_PGDOWN,					/**< Page down */
	KEY_HOME,					/**< Home */
	KEY_END,					/**< End */
	KEY_DELETE,					/**< Delete */
	KEY_INSERT,					/**< Insert */
	KEY_F1,						/**< F1 */
	KEY_F2,						/**< F2 */
	KEY_F3,						/**< F3 */
	KEY_F4,						/**< F4 */
	KEY_F5,						/**< F5 */
	KEY_F6,						/**< F6 */
	KEY_F7,						/**< F7 */
	KEY_F8,						/**< F8 */
	KEY_F9,						/**< F9 */
	KEY_F10,					/**< F10 */
	KEY_F11,					/**< F11 */
	KEY_F12,					/**< F12 */
	KEY_BACKSPACE,				/**< Backspace key */
} KeyCode;

/**
 * Telnet data callback used by interactive mode to catch data from client
 * void Telnet_DataCallback(TelnetClient client, KeyCode key);
 * @param client Telnet client
 * @param ch Received key from client
 * @return True if interactive mode should be laved, false otherwise.
 */
typedef bool (*Telnet_DataCallback)(TelnetClient, KeyCode);

/**
 * Telnet ontelnetcmd event data
 */
typedef struct {
	TelnetClient client;		/**< Client that has sent the command */
	char *command;				/**< Telnet command */
	char *params;				/**< Command parameters */
	bool setInteractive;			/**< Set interactive mode? */
	Telnet_DataCallback callback; /**< Interactive mode data receive
									 callback */
	bool handled;
} Telnet_Command;

/**
 * Telnet plugin data
 */
typedef struct {
	PluginInfo *info;			/**< PluginInfo structure */
	size_t lastId;				/**< Last client ID */
	TelnetClient firstClient;	/**< First client in client chain */
	TelnetClient lastClient;	/**< Last client in client chain */
} TelnetPluginData;

/**
 * Telnet color constants for Telnet_Action commands.
 */
typedef enum {
	TC_NoChange = -1,			/**< Do not change this color */
	TC_Black = 0,
	TC_Red,
	TC_Green,
	TC_Yellow,
	TC_Blue,
	TC_Magenta,
	TC_Cyan,
	TC_White
} Telnet_Colors;

/**
 * Telnet action that should be performed on client. Client isn't required
 * to support any of these actions.
 */
typedef enum {
	TA_ClearScreen,				/**< Clear entire client's screen */
	TA_ClearLine,				/**< Clear current line and move the cursor to
									 the begining of line. */
	TA_SetCursorPosition,		/**< Set cursor position on screen. Param1 is x
									 position, param2 is y position */
	TA_MoveUp,					/**< Move cursor up n rows, where n is
									 param1 */
	TA_MoveDown,				/**< Move cursor down n rows, where n is
									 param1 */
	TA_MoveLeft,				/**< Move cursor left n columns, where n is
									 param1 */
	TA_MoveRight,				/**< Move cursor right n columns, where n is
									 param1 */
	TA_SetColor,				/**< Set foreground and background color of
									 output. Param1 is foreground color, param2
									 is background color. See Telnet_Colors
									 enum for details. */
} Telnet_Action;

/**
 * Function prototype used by telnet_(v)send and telnet_(v)broadcast to send
 * data to client.
 * void TelnetSendData(TelnetClient client, void *buffer, size_t buffersize)
 * @param client Telnet client structure
 * @param buffer Buffer to be send
 * @param buffersize Size of data in buffer
 */
typedef void (*TelnetSendData)(TelnetClient, void *, size_t);

/**
 * Function prototype used by telnet_action to perform some terminal action
 * on host.
 * bool TelnetPerformAction(TelnetClient client, Telnet_Action action,
 *   int param1, int param2);
 * @param client Telnet client structure
 * @param action Action to be performed
 * @param param1 Action parameter 1
 * @param param2 Action parameter 2
 * @return True if action was performed, false if action isn't supported.
 */
typedef bool (*TelnetPerformAction)(TelnetClient, Telnet_Action action,
	int param1, int param2);

/**
 * Function prototype used by telnet_disconnect to properly close client's
 * connection.
 * void TelnetDisconnected(TelnetClient client)
 * @param client Telnet client that should be disconnected.
 */
typedef void (*TelnetDisconnected)(TelnetClient);

static const int TC_NORMAL = 0x00;		/**< Nothing special about client. */
static const int TC_ECHO = 0x01;		/**< Client allowed us to do local
											 echo. */
static const int TC_UNBUFFERED = 0x02;	/**< Client sends all data without
											 processing, so telnet must
											 do the editing stuff. */
static const int TC_NOPROMPT = 0x04;	/**< Client cannot use prompt. */
static const int TC_WINDOWSIZE = 0x08;	/**< Has windowsize information. */

typedef enum {
	TCS_PROMPT = 0,						/**< We are at prompt, waiting
											 to command */
	TCS_PROCESSING,						/**< Processing command */
	TCS_INTERACTIVE,					/**< In interactive mode */
	TCS_DISCONNECTED					/**< Client has been disconnected and
											 it's structure should be freed. */
} TelnetClientState;

/**
 * Struct that holds information about a connected client.
 */
struct sTelnetClient {
	size_t id;
	TelnetClient prev;			/**< Previous connected client */
	TelnetClient next;			/**< Next connected client */
	TelnetPluginData *plugData;	/**< Telnet plugin data */

	void *socketdata;			/**< For other clients, this may be used to
									 store specific informations. */

	// Callbacks
	TelnetSendData send;		/**< Function to send data to client */
	TelnetPerformAction action;	/**< Function to perform action on client */
	TelnetDisconnected disconnected; /**< Function to close client's
								     connection */
	Telnet_DataCallback dataCallback; /**< Callback for interactive mode */
	void *interactiveData;		/**< Interactive mode data pointer */

	string recvbuffer;			/**< Receive buffer for client */
	unsigned int opts;			/**< Client abilities */

	TelnetClientState state;	/**< Client state */

	uint16_t windowWidth;		/**< Terminal window width */
	uint16_t windowHeight;		/**< Terminal window height */

	KVPArray kvp;
}; // sTelnetClient

/**
 * Sends message to telnet client.
 * @param client Telnet client
 * @param format Format of message
 * @param ap Variable arguments list
 */
extern void telnet_vsend(TelnetClient client, char *format, va_list ap);

/**
 * Sends message to telnet client. Variable arguments list are formated by
 * format parameter.
 * @param client Telnet client
 * @param format Format of message.
 */
extern void telnet_send(TelnetClient client, char *format, ...);

/**
 * Broadcast message to all telnet clients. Variable arguments are formatted
 * by format, which uses standard printf formatting.
 * @param format Format of message.
 */
extern void telnet_broadcast(char *format, ...);

/**
 * Perform an terminal action
 * @param client Telnet client
 * @param action Action to be performed (see Telnet_Action enum for details
 * @param param1 First parameter of action
 * @param param2 Second parameter of action
 * @return Return true if action is supported and was performed by driver,
 *   false otherwise.
 */
extern bool telnet_action(TelnetClient client, Telnet_Action action, int param1,
	int param2);

/**
 * Disconnect client and free client's structure.
 * @param client Telnet client
 */
extern void telnet_disconnect(TelnetClient client);

/**
 * Sends MOTD to client
 * @param client Telnet client
 */
extern void telnet_motd(TelnetClient client);

/**
 * Displays prompt and sets client to "awaiting command" state.
 * @param client Telnet client
 */
extern void telnet_prompt(TelnetClient client);

/**
 * Add new client to list of telnet clients
 * @param socketdata Client socket data
 * @param send Send function
 */
extern TelnetClient telnet_add_client(void *socketdata, TelnetSendData send,
	TelnetPerformAction action, TelnetDisconnected disconnected);

/**
 * Handles incomming data from driver
 * @param client Telnet client
 * @param ch Received char
 */
extern void telnet_received(TelnetClient client, KeyCode ch);

/**
 * Process telnet key without echo
 * @param client Telnet client
 * @param ch Received key
 */
extern void telnet_receiveWithoutEcho(TelnetClient client, KeyCode ch);

/**
 * Process telnet key with echo
 * @param client Telnet client
 * @param ch Received key
 */
extern void telnet_receiveWithEcho(TelnetClient client, KeyCode ch);

/**
 * CD to subcommand tree.
 * @param client Telnet client
 * @param tree Subtree to CD to.
 */
extern void telnet_cd(TelnetClient client, char *tree);

#endif

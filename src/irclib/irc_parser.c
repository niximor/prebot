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
#include <stdbool.h>

// My libraries
#include <tokenizer.h>
#include <events.h>
#include <toolbox/tb_string.h>

// This library interface
#include "irclib.h"

/**
 * Removes first : from string. It allocates new memory for new string, so
 * don't forget to free it up.
 * @param str Original string
 * @return New string with first : stripped out.
 */
char *irclib_stripcolon(char *str) {
	// This is necessary, because we don't know if we can modify
	// original string.
	char *result = strdup(str);

	int colonfound = 0;
	for (size_t i = 0; i < strlen(result); i++) {
		if (colonfound) {
			result[i] = result[i+1];
		} else if (result[i] == ':') {
			colonfound = 1;
			result[i] = result[i+1];
		}
	}

	return result;
} // irclib_stripcolon

/**
 * Parse message no. 5
 * @param connection IRCLib_Connection structure
 * @param tokenized Tokenized message
 */
void irclib_parse_isupport(IRCLib_Connection *connection, TOKENS tokenized) {
	// :eurix.lan 005 PReBoT MAP KNOCK SAFELIST HCN MAXCHANNELS=10
	// MAXBANS=60 NICKLEN=30 TOPICLEN=307 KICKLEN=307 MAXTARGETS=20
	// AWAYLEN=307 :are supported by this server
	// :eurix.lan 005 PReBoT WALLCHOPS WATCH=128 SILENCE=5 MODES=12
	// CHANTYPES=# PREFIX=(ohv)@%+
	// CHANMODES=beqa,kfL,l,psmntirRcOAQKVGCuzNSM NETWORK=HomeNET
	// CASEMAPPING=ascii :are supported by this server

	for (size_t i = 3; i < tokenized->count; i++) {
		char *tok = strdup(tokenizer_gettok(tokenized, i));

		// We come to "are supported by this server".
		if (tok[0] == ':') {
			free(tok);
			break;
		}

		char *value = strchr(tok, '=');
		if (value != NULL) {
			// Split string to two...
			*value = '\0';

			// Value begins after '=' char
			value++;

			// Get network name
			if (strcmp(tok, "NETWORK") == 0) {
				connection->networkName = strdup(value);
			}

			// Get user mode prefixes (needed for mode parsing)
			// PREFIX=(ovh)@+%
			if (strcmp(tok, "PREFIX") == 0) {
				string s = dynastring_init();
				string sym = dynastring_init();

				enum {
					IRCLib_STATE_BEGIN = 0,
					IRCLib_STATE_MODES,
					IRCLib_STATE_SYMBOLS,
					IRCLib_STATE_SKIP
				} state = IRCLib_STATE_BEGIN;

				for (size_t i = 0; i < strlen(value); i++) {
					switch (state) {
						case IRCLib_STATE_BEGIN:
							if (value[i] == '(') {
								state =	IRCLib_STATE_MODES;
							}
							break;

						case IRCLib_STATE_MODES:
							if (value[i] == ')') {
								state = IRCLib_STATE_SYMBOLS;
							} else {
								dynastring_appendchar(s, value[i]);
							}
							break;

						case IRCLib_STATE_SYMBOLS:
							dynastring_appendchar(sym, value[i]);
							break;

						case IRCLib_STATE_SKIP:
							break;
					}

					if (state == IRCLib_STATE_SKIP) break;
				}
				// Just to be sure, this shouldn't be necessary now...
				dynastring_appendchar(s, '\0');
				dynastring_appendchar(sym, '\0');

				connection->userPrefixes =
					strdup(dynastring_getstring(s));
				connection->userPrefixesSymbols =
					strdup(dynastring_getstring(sym));

				dynastring_free(s);
				dynastring_free(sym);
			}

			// CHANMODES=beIR,k,l,imnpstaqr
			if (strcmp(tok, "CHANMODES") == 0) {
				string chanModesAddress = dynastring_init();
				string chanModesAlwaysParam = dynastring_init();
				string chanModesSetParam = dynastring_init();
				string chanModesNeverParam = dynastring_init();

				enum {
					// Has always parameter which is
					// address of user to be added or
					// removed from list
					IRCLib_STATE_ADDRESS = 0,

					// Has always parameter
					IRCLib_STATE_ALWAYSPARAM,

					// Has parameter only if being set
					IRCLib_STATE_SETPARAM,

					// Never has parameter
					IRCLib_STATE_NEVERPARAM
				} state = IRCLib_STATE_ADDRESS;

				for (size_t i = 0; i < strlen(value); i++) {
					if (value[i] == ',') {
						state++;
						continue;
					}

					switch (state) {
						case IRCLib_STATE_ADDRESS:
							dynastring_appendchar(chanModesAddress,
								value[i]);
							break;

						case IRCLib_STATE_ALWAYSPARAM:
							dynastring_appendchar(chanModesAlwaysParam,
								value[i]);
							break;

						case IRCLib_STATE_SETPARAM:
							dynastring_appendchar(chanModesSetParam,
								value[i]);
							break;

						case IRCLib_STATE_NEVERPARAM:
							dynastring_appendchar(chanModesNeverParam,
								value[i]);
							break;
					}
				}
				dynastring_appendchar(chanModesAddress, '\0');
				dynastring_appendchar(chanModesAlwaysParam, '\0');
				dynastring_appendchar(chanModesSetParam, '\0');
				dynastring_appendchar(chanModesNeverParam, '\0');

				connection->chanModesAddress =
					strdup(chanModesAddress->data);
				connection->chanModesAlwaysParam =
					strdup(chanModesAlwaysParam->data);
				connection->chanModesSetParam =
					strdup(chanModesSetParam->data);
				connection->chanModesNeverParam =
					strdup(chanModesNeverParam->data);

				dynastring_free(chanModesAddress);
				dynastring_free(chanModesAlwaysParam);
				dynastring_free(chanModesSetParam);
				dynastring_free(chanModesNeverParam);
			}
		}

		free(tok);
	}

} // irclib_parseno5message

/**
 * Parses incomming data and fire event, if any is assigned.
 * @param connection IRCLib_Connection structure
 * @param message One message received from IRC server
 * @return 1 on successful parsing, 0 if error.
 */
void irclib_parse(IRCLib_Connection *connection, char *message) {
	TOKENS tok = tokenizer_tokenize(message, ' ');

	// --- PING -----------------------------------------------------------
	// PING :server
	if (strcmp(tokenizer_gettok(tok, 0), "PING") == 0) {
		if (events_fireEvent(connection->events, "onping", tok)) {

			irclib_sendraw(connection, "PONG %s",
				tokenizer_gettok(tok, 1));

		}
		goto _irclib_parse_end;
	}

	// --- Server message -------------------------------------------------
	// :server <NUM> <me> params
	// The only way how to detect server message is to detect if 2nd
	// parameter is number.
	// ToDo: It would be great to distinguish between reply messages and
	// error messages and have another event for errors.
	char *second = tokenizer_gettok(tok, 1), *restOfString;
	if (strlen(second) != 0) {
		// We have second parameter

		long int numericMessage = strtol(second, &restOfString, 10);
		if (strlen(restOfString) == 0) {
			// It is numeric message from server
			if (connection->status == IRC_CONNECTING) {
				printError("irclib", "%s", tokenizer_gettok_skipleft(tok, 3));
			}

			IRCEvent_ServerMessage evt = {
				.sender = connection,
				// +1 skips the : char at the begining of
				// message
				.server = tokenizer_gettok(tok, 0) + 1,
				.messageCode = numericMessage,
				.message = irclib_stripcolon(
					tokenizer_gettok_skipleft(tok, 3))
			};
			events_fireEvent(connection->events, "onservermessage",
				&evt);

			if (numericMessage == ERR_NICKNAMEINUSE && connection->status == IRC_CONNECTING) {
				// Try new nick...
				char *newnick = malloc(strlen(connection->nickname + 2));
				sprintf(newnick, "%s_", connection->nickname);
				
				free(connection->nickname);
				connection->nickname = newnick;

				irclib_sendraw(connection, "NICK %s", connection->nickname);
			}

			// Parse message number 5, which contains some
			// important information for us
			if (numericMessage == RPL_ISUPPORT) {
				irclib_parse_isupport(connection, tok);
			}

			free(evt.message);

			if (numericMessage == RPL_ENDOFMOTD &&
				connection->status == IRC_CONNECTING) {
				connection->status = IRC_CONNECTED;

				IRCEvent_Notify evt = {
					.sender = connection
				};
				events_fireEvent(connection->events,
					"onconnected", &evt);
			}

			// Add nicks to channel
			// :eurix.lan 353 Amonet = #rls.rct.cz :Amonet niximor
			if (numericMessage == RPL_NAMREPLY) {
				char *channel = tokenizer_gettok(tok, 4);
				char *users = irclib_stripcolon(
					tokenizer_gettok_skipleft(tok, 5));

				strtolower(channel);

				IRCLib_Channel ircchannel =
					irclib_find_channel(connection->channelStorage, channel);

				if (ircchannel != NULL) {
					TOKENS chusers = tokenizer_tokenize(users, ' ');
					for (size_t i = 0; i < chusers->count; i++) {
						char *user = tokenizer_gettok(chusers, i);

						// Test if first char is not mode.
						if (connection->userPrefixesSymbols != NULL && user != NULL) {
							for (size_t pi = 0; pi < strlen(connection->userPrefixesSymbols); pi++) {
								if (user[0] == connection->userPrefixesSymbols[pi]) {
									user++;
									break;
								}
							}
						}

						irclib_add_channel_user(connection, ircchannel, user);
					}
					tokenizer_free(chusers);
				}

				free(users);
			}

			goto _irclib_parse_end;
		}
	} // Server message

	// All messages following has syntax :address ACTION params, so we can
	// use it to globally fill users storage.
	IRCLib_Host *address = irclib_parse_addr(tokenizer_gettok(tok, 0) + 1);
	IRCLib_User sender = irclib_add_usera(connection->userStorage, address);
	irclib_free_addr(address);

	// --- Joined to channel ----------------------------------------------
	// :PReBoT!prebot@eurix.lan JOIN :#rct.cz
	// We must test if second token is JOIN, and then if nick from first
	// token is my nick, fire "joined" event, else fire "join" event.
	if (strcmp(second, "JOIN") == 0) {
		IRCEvent_JoinPart evt = {
			.sender = connection,
			.address = sender->host,
			.channel = irclib_stripcolon(tokenizer_gettok(tok, 2)),
			.reason = NULL
		};

		strtolower(evt.channel);

		// If joined nick is me, fire joined event
		if (strcmp(sender->host->nick,connection->nickname) == 0) {
			irclib_add_channel(connection->channelStorage, evt.channel);

			events_fireEvent(connection->events, "onjoined", &evt);
		// Fire join event
		} else {
			irclib_add_channel_user(
				connection,
				irclib_find_channel(
					connection->channelStorage,
					evt.channel),
				sender->host->nick);
			events_fireEvent(connection->events, "onjoin", &evt);
		}

		// Free event data
		free(evt.channel);

		goto _irclib_parse_end;
	} // Join

	// --- Message to channel or to me ------------------------------------
	// :niximor!niximor@station3.lan PRIVMSG #rct.cz :asdgasdgdsag
	// :niximor!niximor@station3.lan PRIVMSG PReBoT :asdg
	if (strcmp(second, "PRIVMSG") == 0 || strcmp(second, "NOTICE") == 0) {
		IRCEvent_Message evt = {
			.sender = connection,
			.address = sender->host,
			.message = irclib_stripcolon(
				tokenizer_gettok_skipleft(tok, 3))
		};

		// Private message
		if (strcmp(tokenizer_gettok(tok, 2),
			connection->nickname) == 0) {

			evt.channel = NULL;

			if (strcmp(second, "PRIVMSG") == 0) {
				events_fireEvent(connection->events,
					"onprivatemessage", &evt);
			} else {
				events_fireEvent(connection->events,
					"onprivatenotice", &evt);
			}


		// Channel message
		} else {
			evt.channel = strdup(tokenizer_gettok(tok, 2));
			strtolower(evt.channel);
			if (strcmp(second, "PRIVMSG") == 0) {
				events_fireEvent(connection->events,
					"onchannelmessage", &evt);
			} else {
				events_fireEvent(connection->events,
					"onchannelnotice", &evt);
			}
		}

		// Free event data
		if (evt.channel != NULL) {
			free(evt.channel);
		}
		free(evt.message);

		goto _irclib_parse_end;
	} // Message, notice

	// --- Mode change ----------------------------------------------------
	// :test!niximor@station3.lan MODE #rls.rct.cz -s+o nix`PC3
	if (strcmp(second, "MODE") == 0) {
		char *modes = tokenizer_gettok(tok, 3);

		int setMode = 1;
		int tokPos = 4;
		for (size_t i = 0; i < strlen(modes); i++) {
			if (modes[i] == '+') {
				setMode = true;
			} else if (modes[i] == '-') {
				setMode = false;
			} else {
				char modeChar = modes[i];

				// Find which mode is this

				// User prefix change (op, halfop, voice, ...)
				if (strchr(connection->userPrefixes, modeChar)
					!= NULL) {

					IRCEvent_Mode evt = {
						.sender = connection,
						.name = modeChar,
						.set = setMode,
						.address = sender->host,
						.channel = strdup(tokenizer_gettok(tok, 2)),
						.target = strdup(tokenizer_gettok(tok, tokPos))
					};
					tokPos++;

					strtolower(evt.channel);

					irclib_change_user_prefix(
						connection,
						irclib_find_channel(
							connection->channelStorage,
							evt.channel),
						evt.target,
						(evt.set)?evt.name:' ');

					switch (modeChar) {
						case 'o':
							if (setMode) {
								events_fireEvent(connection->events,
									"onop", &evt);
							} else {
								events_fireEvent(connection->events,
									"ondeop", &evt);
							}
							break;

						case 'v':
							if (setMode) {
								events_fireEvent(connection->events,
									"onvoice", &evt);
							} else {
								events_fireEvent(connection->events,
									"ondevoice", &evt);
							}
							break;

						case 'h':
							if (setMode) {
								events_fireEvent(connection->events,
									"onhalfop", &evt);
							} else {
								events_fireEvent(connection->events,
									"ondehalfop", &evt);
							}
							break;

						default:
							events_fireEvent(connection->events,
								"onchangeprefix", &evt);
							break;
					}

					free(evt.channel);
					free(evt.target);

				// Channel mode that requires address
				} else if (strchr(connection->chanModesAddress,
					modeChar) != NULL) {

					IRCEvent_Mode evt = {
						.sender = connection,
						.name = modeChar,
						.set = setMode,
						.address = sender->host,
						.channel = strdup(tokenizer_gettok(tok, 2)),
						.target = strdup(tokenizer_gettok(tok, tokPos))
					};
					tokPos++;

					strtolower(evt.channel);

					// Bans have special event
					if (modeChar == 'b') {
						if (setMode) {
							events_fireEvent(connection->events, "onban",
								&evt);
						} else {
							events_fireEvent(connection->events, "onunban",
								&evt);
						}
					} else {
						// All other list changes have common
						// event
						events_fireEvent(connection->events, "onchangelist",
							&evt);
					}

					free(evt.channel);
					free(evt.target);

				// Channel mode that always require parameter
				} else if (strchr(connection->chanModesAlwaysParam,
					modeChar) != NULL) {

					IRCEvent_Mode evt = {
						.sender = connection,
						.name = modeChar,
						.set = setMode,
						.address = sender->host,
						.channel = strdup(tokenizer_gettok(tok, 2)),
						.target = strdup(tokenizer_gettok(tok, tokPos))
					};
					tokPos++;

					strtolower(evt.channel);

					// Common event on mode change
					events_fireEvent(connection->events, "onmode", &evt);

					free(evt.channel);
					free(evt.target);

				} else if (strchr(connection->chanModesSetParam,
					modeChar) != NULL) {

					IRCEvent_Mode evt = {
						.sender = connection,
						.name = modeChar,
						.set = setMode,
						.address = sender->host,
						.channel = strdup(tokenizer_gettok(tok, 2)),
						.target = ((setMode)?
							strdup(tokenizer_gettok(tok, tokPos)):
							NULL)
					};

					strtolower(evt.channel);

					if (setMode) {
						tokPos++;
					}

					// Common event on mode change
					events_fireEvent(connection->events, "onmode", &evt);

					free(evt.channel);
					if (evt.target != NULL) {
							free(evt.target);
					}

				} else if (strchr(connection->chanModesNeverParam,
					modeChar) != NULL) {

					IRCEvent_Mode evt = {
						.sender = connection,
						.name = modeChar,
						.set = setMode,
						.address = sender->host,
						.channel = strdup(tokenizer_gettok(tok, 2)),
						.target = NULL
					};

					strtolower(evt.channel);

					// Common event on mode change
					events_fireEvent(connection->events, "onmode", &evt);

					free(evt.channel);
				}
			} // char is mode char, not sign
		} // for

		goto _irclib_parse_end;
	} // Mode change

	// --- Kick message -------------------------------------------------------
	// :test!niximor@station3.lan KICK #rls.rct.cz nix`PC3 :blabla
	// :test!niximor@station3.lan KICK #rls.rct.cz PReBoT :reason
	if (strcmp(second, "KICK") == 0) {
		IRCEvent_Kick evt = {
			.sender = connection,
			.address = sender->host,
			.channel = strdup(tokenizer_gettok(tok, 2)),
			.nick = strdup(tokenizer_gettok(tok, 3)),
			.reason = irclib_stripcolon(tokenizer_gettok_skipleft(tok, 4))
		};

		strtolower(evt.channel);

		if (strcmp(tokenizer_gettok(tok, 3), connection->nickname) == 0) {
			// I've been kicked!!
			events_fireEvent(connection->events, "onkicked", &evt);
			irclib_remove_channel(connection->channelStorage,
				irclib_find_channel(connection->channelStorage, evt.channel));
		} else {
			// Someone has been kicked.
			events_fireEvent(connection->events, "onkick", &evt);
			irclib_remove_channel_user(
				irclib_find_channel(connection->channelStorage, evt.channel),
				evt.nick);
		}

		free(evt.channel);
		free(evt.nick);
		free(evt.reason);

		goto _irclib_parse_end;
	} // Kick

	// --- Nick change --------------------------------------------------------
	// :niximor!niximor@station3.lan NICK :nix
	if (strcmp(second, "NICK") == 0) {
		IRCLib_Host *address = irclib_parse_addr(tokenizer_gettok(tok, 0)+1);
		IRCEvent_NickChange evt = {
			.sender = connection,
			.address = sender->host,
			.newnick = irclib_stripcolon(tokenizer_gettok(tok, 2))
		};

		// If my nickname was changed
		if (strcmp(address->nick, connection->nickname) == 0) {
			free(connection->nickname);
			connection->nickname = strdup(evt.newnick);
			events_fireEvent(connection->events, "onnickchanged", &evt);
		// If other user's nickname was changed
		} else {
			events_fireEvent(connection->events, "onnick", &evt);
		}

		irclib_rename_user(sender, evt.newnick);

		free(evt.newnick);
		goto _irclib_parse_end;
	} // Nick

	// --- Part channel -------------------------------------------------------
	// :Amonet!Amonet@station3.lan PART #rls.rct.cz :blabla
	if (strcmp(second, "PART") == 0) {
		IRCEvent_JoinPart evt = {
			.sender = connection,
			.address = sender->host,
			.channel = strdup(tokenizer_gettok(tok, 2)),
			.reason = irclib_stripcolon(tokenizer_gettok_skipleft(tok, 3))
		};

		strtolower(evt.channel);

		if (strcmp(sender->host->nick, connection->nickname) == 0) {
			events_fireEvent(connection->events, "onparted", &evt);
			irclib_remove_channel(connection->channelStorage,
				irclib_find_channel(connection->channelStorage, evt.channel));
		} else {
			events_fireEvent(connection->events, "onpart", &evt);
			irclib_remove_channel_user(
				irclib_find_channel(connection->channelStorage, evt.channel),
				sender->host->nick);
		}

		free(evt.channel);
		free(evt.reason);

		goto _irclib_parse_end;
	} // Part

	// --- Quit from IRC ------------------------------------------------------
	// :Amonet!Amonet@d213.dhcp.lan QUIT :Client exited
	if (strcmp(second, "QUIT") == 0) {
		IRCEvent_Quit evt = {
			.sender = connection,
			.address = sender->host,
			.message = irclib_stripcolon(tokenizer_gettok_skipleft(tok, 3))
		};

		events_fireEvent(connection->events, "onquited", &evt);

		// Update the users storage.
		irclib_remove_user(connection->userStorage,
			irclib_find_user(connection->userStorage, evt.address->nick));

		free(evt.message);
	}

	_irclib_parse_end:
	tokenizer_free(tok);
} // irclib_parse

/**
 * Convert user prefix to symbol
 * @param connection IRCLib_Connection structure with filled userPrefixes and
 *   userPrefixesSymbols items
 * @param prefix Prefix that should be converted to symbol
 * @return Symbol corresponding to specified prefix. Return space (' ') if
 *   prefix was not found.
 */
char irclib_prefix2sym(IRCLib_Connection *connection, char prefix) {
	for (size_t i = 0; i < strlen(connection->userPrefixes); i++) {
		if (connection->userPrefixes[i] == prefix) {
			return connection->userPrefixesSymbols[i];
		}
	}
	return ' ';
} // irclib_prefix2sym

/**
 * Convert to symbol prefix
 * @param connection IRCLib_Connection structure with filled userPrefixes and
 *   userPrefixesSymbols items
 * @param sym Symbol that should be converted to prefix.
 * @return Symbol corresponding to specified prefix. Return space (' ') if
 *   symbol was not found.
 */
char irclib_sym2prefix(IRCLib_Connection *connection, char sym) {
	for (size_t i = 0; i < strlen(connection->userPrefixesSymbols); i++) {
		if (connection->userPrefixesSymbols[i] == sym) {
			return connection->userPrefixes[i];
		}
	}
	return ' ';
} // irclib_sym2prefix

/**
 * Determines whether nickname contains prefix
 * @param connection IRCLib_Connection structure with filled
 *   userPrefixesSymbols items
 * @param nick Nickname that can have prefix.
 * @return true if nickname contains prefix, false otherwise
 */
bool irclib_nickHasPrefix(IRCLib_Connection *connection, char *nick) {
	if (nick != NULL && strlen(nick) > 0) {
		char prefix = nick[0];
		return irclib_sym2prefix(connection, prefix) != ' ';
	} else {
		return false;
	}
} // irclib_nickHasPrefix

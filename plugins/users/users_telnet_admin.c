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

// Plugins API
#include <pluginapi.h>

// This plugin interface
#include "interface.h"

// My libraries
#include "../telnet/interface.h"
#include <toolbox/tb_string.h>
#include <tokenizer.h>
#include <config/config.h>

/**
 * Handles ontelnetcmd event
 * @param event Event data
 */
void users_telnetcommands(EVENT *event) {
	Telnet_Command *command = (Telnet_Command *)event->customData;
	UsersPluginData *plugData = (UsersPluginData *)event->handlerData;
	TelnetClient client = command->client;
	CONF_SECTION *usersdb = plugData->usersdb;

	if (eq(command->command, "users") || eq(command->command, "user")) {
		TOKENS tok = tokenizer_tokenize(command->params, ' ');
		char *subcommand = tokenizer_gettok(tok, 0);

		// users list
		// List known users
		if (eq(subcommand, "list")) {
			telnet_send(client, "Known users (listed in order that "
				"is defined in users database, to be able to review "
				"priority):");
			for (size_t i = 0; i < usersdb->subSectionsCount; i++) {
				telnet_send(client, "- %s",
					usersdb->subSections[i]->name);
			}

			goto _users_telnet_handled;
		}

		// users info <name>
		// Shows info about user
		if (eq(subcommand, "info")) {
			if (tok->count == 2) {
				char *name = tokenizer_gettok(tok, 1);
				if (config_validate_section(name)) {
					CONF_SECTION *user = config_lookup_section(usersdb, name,
						false);

					if (user != NULL) {
						telnet_send(client, "User info of user %s:", name);
						telnet_send(client, "Hosts:");
						size_t hostsCount = 0;
						for (size_t i = 0; i < user->valuesCount; i++) {
							if (eq(user->values[i]->name, "host")) {
								telnet_send(client, "- %s",
									htval_get_string(user->values[i]->value));
								hostsCount++;
							}
						}
						if (hostsCount == 0) {
							telnet_send(client, "User doesn't have any hosts "
								"defined.");
						}

						telnet_send(client, "Flags:");
						size_t flagsCount = 0;

						for (size_t i = 0; i < user->subSectionsCount; i++) {
							if (eq(user->subSections[i]->name, "privileges")) {
								CONF_SECTION *priv = user->subSections[i];
								if (priv->valuesCount > 0) {
									telnet_send(client, "- Global flags:");
									for (size_t gf = 0; gf < priv->valuesCount;
										gf++) {

										telnet_send(client, "  - %s = %d",
											priv->values[i]->name,
											htval_get_int(
												priv->values[i]->value));
											flagsCount++;
									}
								}

								for (size_t ps = 0;
									ps < priv->subSectionsCount; ps++) {

									CONF_SECTION *privsect =
										priv->subSections[i];

									if (privsect->valuesCount > 0) {
										telnet_send(client, "- %s:",
											privsect->name);

										for (size_t gf = 0;
											gf < privsect->valuesCount;	gf++) {

											telnet_send(client, "  - %s = %d",
												privsect->values[i]->name,
												htval_get_int(
													privsect->values[i]->
														value));
											flagsCount++;
										}
									}
								}
							}
						}

						if (flagsCount == 0) {
							telnet_send(client, "User doesn't have any flags "
								"defined.");
						}
					} else {
						telnet_send(client, "User '%s' doesn't exists.", name);
					}
				} else {
					telnet_send(client, "Invalid user name: '%s'", name);
				}
			} else {
				telnet_send(client, "Usage: %s info <name>", command->command);
			}

			goto _users_telnet_handled;
		}

		// users add <name>
		// Add new user
		if (eq(subcommand, "add")) {
			if (tok->count == 2) {
				char *newuser = tokenizer_gettok(tok, 1);
				// Test if user doesn't exists...
				for (size_t i = 0; i < usersdb->subSectionsCount; i++) {
					if (eq(usersdb->subSections[i]->name, newuser)) {
						telnet_send(client, "User with name '%s' already "
							"exists.", newuser);
						newuser = NULL;
						break;
					}
				}

				// User doesn't exists.
				if (newuser != NULL) {
					if (config_validate_section(newuser)) {
						config_create_section(usersdb, newuser);
						telnet_send(client, "User '%s' has been created.",
							newuser);
					} else {
						telnet_send(client, "Username '%s' is invalid.",
							newuser);
					}
				}
			} else {
				telnet_send(client, "Usage: %s add <name>", command->command);
			}

			goto _users_telnet_handled;
		}

		// users del <name>
		// Remove user
		if (eq(subcommand, "del")) {
			if (tok->count == 2) {
				char *deluser = tokenizer_gettok(tok, 1);
				for (size_t i = 0; i < usersdb->subSectionsCount; i++) {
					if (eq(usersdb->subSections[i]->name, deluser)) {
						config_remove_subsection(usersdb, i);
						telnet_send(client, "User '%s' has been removed.",
							deluser);
						deluser = NULL;
						break;
					}
				}

				if (deluser != NULL) {
					telnet_send(client, "User '%s' doesn't exists in "
						"database.", deluser);
				}
			} else {
				telnet_send(client, "Usage: %s add <name>", command->command);
			}

			goto _users_telnet_handled;
		}

		// users ren <name> <newname>
		// Rename user
		if (eq(subcommand, "ren")) {
			if (tok->count == 3) {
				char *name = tokenizer_gettok(tok, 1);
				if (config_validate_section(name)) {
					char *newname = tokenizer_gettok(tok, 2);

					if (config_validate_section(name)) {
						CONF_SECTION *user = config_lookup_section(usersdb,
							name, false);

						// User found, rename.
						if (user != NULL) {
							free(user->name);
							user->name = strdup(newname);
							telnet_send(client, "User '%s' has been renamed "
								"to '%s'.", name, newname);
						} else {
							telnet_send(client, "User '%s' doesn't exists, "
								"cannot rename.", name);
						}
					} else {
						telnet_send(client, "Invalid user name: '%s'",
							newname);
					}
				} else {
					telnet_send(client, "Invalid user name: '%s'", name);
				}
			} else {
				telnet_send(client, "Usage: %s ren <name> <newname>",
					command->command);
			}

			goto _users_telnet_handled;
		}

		// users up <name>
		// Move user up
		if (eq(subcommand, "up")) {
			if (tok->count == 2) {
				char *name = tokenizer_gettok(tok, 1);
				for (size_t i = 0; i < usersdb->subSectionsCount; i++) {
					if (eq(usersdb->subSections[i]->name, name)) {
						if (i > 0) {
							CONF_SECTION *temp = usersdb->subSections[i-1];
							usersdb->subSections[i-1] =
								usersdb->subSections[i];
							usersdb->subSections[i] = temp;

							temp->index++;
							usersdb->subSections[i-1]->index--;

							telnet_send(client, "User '%s' has been moved.",
								name);
						} else {
							telnet_send(client, "User '%s' cannot be moved: "
								"already at highest level.", name);
						}

						name = NULL;
						break;
					}
				}

				if (name != NULL) {
					telnet_send(client, "User '%s' doesn't exists.", name);
				}
			} else {
				telnet_send(client, "Usage: %s up <name>", command->command);
			}

			goto _users_telnet_handled;
		}

		// users down <name>
		// Move user down
		if (eq(subcommand, "down")) {
			if (tok->count == 2) {
				char *name = tokenizer_gettok(tok, 1);
				for (size_t i = 0; i < usersdb->subSectionsCount; i++) {
					if (eq(usersdb->subSections[i]->name, name)) {
						if (i < usersdb->subSectionsCount - 1) {
							CONF_SECTION *temp = usersdb->subSections[i+1];
							usersdb->subSections[i+1] =
								usersdb->subSections[i];
							usersdb->subSections[i] = temp;

							temp->index--;
							usersdb->subSections[i+1]->index++;


							telnet_send(client, "User '%s' has been moved.");
						} else {
							telnet_send(client, "User '%s' cannot be moved: "
								"already at lowest level.", name);
						}

						name = NULL;
						break;
					}
				}

				if (name != NULL) {
					telnet_send(client, "User '%s' doesn't exists.", name);
				}
			} else {
				telnet_send(client, "Usage: %s down <name>", command->command);
			}

			goto _users_telnet_handled;
		}

		// users addhost <user> <host>
		// Add new hostname to user
		if (eq(subcommand, "addhost")) {
			if (tok->count == 3) {
				char *name = tokenizer_gettok(tok, 1);
				char *host = tokenizer_gettok(tok, 2);

				if (config_validate_section(name)) {
					CONF_SECTION *user = config_lookup_section(usersdb, name,
						false);
					if (user != NULL) {
						for (size_t i = 0; i < user->valuesCount; i++) {
							if (eq(user->values[i]->name, "host") &&
								eq(htval_get_string(user->values[i]->value),
									host)) {

								telnet_send(client, "User '%s' already has "
									"hostname '%s'.", name, host);
								host = NULL;
								break;
							}
						}

						if (host != NULL) {
							config_set_string(user, "host[]", host);
							telnet_send(client, "User '%s' now has new "
								"host '%s'.", name, host);
						}
					} else {
						telnet_send(client, "User '%s' doesn't exists.", name);
					}
				} else {
					telnet_send(client, "Invalid user name: '%s'", name);
				}
			} else {
				telnet_send(client, "Usage: %s addhost <user> <host>",
					command->command);
			}

			goto _users_telnet_handled;
		}

		// users delhost <user> <host>
		// Delete hostname from user
		if (eq(subcommand, "delhost")) {
			if (tok->count == 3) {
				char *name = tokenizer_gettok(tok, 1);
				char *host = tokenizer_gettok(tok, 2);

				CONF_SECTION *user = config_lookup_section(usersdb, name,
					false);
				if (user != NULL) {
					// Find hostname
					for (size_t i = 0; i < user->valuesCount; i++) {
						if (eq(user->values[i]->name, "host") &&
							eq(htval_get_string(user->values[i]->value),
								host)) {

							config_remove_value(user, i);
							telnet_send(client, "Hostname '%s' was removed "
								"from user '%s'.", host, name);
							host = NULL;
							break;
						}
					}

					if (host != NULL) {
						telnet_send(client, "User '%s' doesn't have "
							"hostname '%s'.", name, host);
					}
				} else {
					telnet_send(client, "User '%s' doesn't exists.", name);
				}
			} else {
				telnet_send(client, "Usage: %s delhost <user> <host>",
					command->command);
			}

			goto _users_telnet_handled;
		}

		// users flag <user> [<section>] <flag> <value>
		// Set flag value
		if (eq(subcommand, "flag")) {
			char *user = NULL;
			char *section = NULL;
			char *flag = NULL;
			char *value = NULL;
			if (tok->count == 5) {
				// user section flag value
				user = tokenizer_gettok(tok, 1);
				section = tokenizer_gettok(tok, 2);
				flag = tokenizer_gettok(tok, 3);
				value = tokenizer_gettok(tok, 4);
			} else if (tok->count == 4) {
				// user flag value
				user = tokenizer_gettok(tok, 1);
				flag = tokenizer_gettok(tok, 2);
				value = tokenizer_gettok(tok, 3);
			} else {
				telnet_send(client, "Usage: %s flag <user> [<section>] "
					"<flag> <value>", command->command);
			}

			if (!config_validate_section(user)) {
				telnet_send(client, "Invalid user name: '%s'", user);
				user = NULL;
			}

			if (section != NULL && !config_validate_section(section)) {
				telnet_send(client, "Invalid section name: '%s'", section);
				user = NULL; // Don't proceed with global flag...
				section = NULL;
			}

			if (!config_validate_value(flag, false)) {
				telnet_send(client, "Invalid flag name: '%s'", flag);
				flag = NULL;
			}

			// Flag value must be integer
			int flagValue = 0;
			if (sscanf(value, "%d", &flagValue) != 1) {
				telnet_send(client, "Invalid flag value '%s', must be number.",
					value);
				value = NULL;
			}

			if (user != NULL && flag != NULL && value != NULL) {
				char *path;

				if (section != NULL) {
					asprintf(&path, "privileges:%s:%s", section, flag);
				} else {
					asprintf(&path, "privileges:%s", flag);
				}

				CONF_SECTION *suser = config_lookup_section(usersdb, user,
					false);

				if (suser != NULL) {
					config_set_int(suser, path, flagValue);
					telnet_send(client, "Flag was set.");
				} else {
					telnet_send(client, "User '%s' doesn't exists.", user);
				}

				if (path) {
					free(path);
				}
			}

			goto _users_telnet_handled;
		}

		// users delflag <user> [<section>] <flag>
		// Delete flag from user
		if (eq(subcommand, "delflag")) {
			char *user = NULL;
			char *section = NULL;
			char *flag = NULL;

			if (tok->count == 4) {
				user = tokenizer_gettok(tok, 1);
				section = tokenizer_gettok(tok, 2);
				flag = tokenizer_gettok(tok, 3);
			} else if (tok->count == 3) {
				user = tokenizer_gettok(tok, 1);
				flag = tokenizer_gettok(tok, 2);
			} else {
				telnet_send(client, "Usage: %s delflag <user> [<section>] "
					"<value>", command->command);
			}

			if (!config_validate_section(user)) {
				telnet_send(client, "Invalid user name: '%s'", user);
				user = NULL;
			}

			if (section != NULL && !config_validate_section(section)) {
				telnet_send(client, "Invalid section name: '%s'", section);
				user = NULL; // Don't proceed with global flag...
				section = NULL;
			}

			if (!config_validate_value(flag, false)) {
				telnet_send(client, "Invalid flag name: '%s'", flag);
				flag = NULL;
			}

			if (user != NULL && flag != NULL) {
				char *path;

				if (section != NULL) {
					asprintf(&path, "privileges:%s:%s", section, flag);
				} else {
					asprintf(&path, "privileges:%s", flag);
				}

				CONF_SECTION *suser = config_lookup_section(usersdb, user,
					false);

				if (suser != NULL) {
					if (config_remove(suser, path)) {
						if (section == NULL) {
							telnet_send(client, "User's global flag %s was "
								"removed.", flag, section);
						} else {
							telnet_send(client, "User's flag %s in section "
								"%s was removed.", flag, section);
						}
					} else {
						if (section == NULL) {
							telnet_send(client, "User '%s' doesn't have "
								"global flag %s set.", user, flag);
						} else {
							telnet_send(client, "User '%s' doesn't have flag "
								"%s in section %s set.", user, flag, section);
						}
					}
				} else {
					telnet_send(client, "User '%s' doesn't exists.", user);
				}

				if (path) {
					free(path);
				}
			}

			goto _users_telnet_handled;
		}

		// users
		// Help
		if (eq(subcommand, "")) {
			telnet_send(client, "%s list ....................... "
				"List known users", command->command);
			telnet_send(client, "%s info <name> ................ "
				"Shows info about user.", command->command);
			telnet_send(client, "%s add <name> ................. "
				"Add new user", command->command);
			telnet_send(client, "%s del <name> ................. "
				"Delete user", command->command);
			telnet_send(client, "%s ren <name> <newname> ....... "
				"Rename user", command->command);
			telnet_send(client, "%s up <name> .................. "
				"Move user up in database (give him lower priority)",
				command->command);
			telnet_send(client, "%s down <name> ................ "
				"Move user down in database (give him higher priority)",
				command->command);
			telnet_send(client, "%s addhost <user> <host> ...... "
				"Add new hostname to user", command->command);
			telnet_send(client, "%s delhost <user> <host> ...... "
				"Delete hostname from user", command->command);
			telnet_send(client, "%s flag <user> [<section>] <flag> "
				"<value> .. Add new flag to user, or change existing's flag "
				"value.", command->command);
			telnet_send(client, "%s delflag <user> [<section>] <flag> .. "
				"Delete flag from user, cause that it will be inherited.",
				command->command);

			goto _users_telnet_handled;
		}

		// Unhandled command
		command->handled = false;
		tokenizer_free(tok);
		return;

		_users_telnet_handled:

		// Fire users db changed event and save the database if the event was
		// not canceled.
		if (usersdb->changed) {
			Users_DbChangedEvent evt = {
				.usersdb = usersdb
			};

			if (events_fireEvent(plugData->info->events,
				"onusersdbchanged", &evt)) {

				FILE *f = fopen(config_getvalue_string(plugData->info->config,
					"users:dbfile", "./users.db"), "w");
				if (f) {
					config_save(usersdb, f);
					fclose(f);
				} else {
					telnet_send(client, "Unable to open user's database file, "
						"changes hasn't been saved.");
				}
			}
		}

		command->handled = true;
		tokenizer_free(tok);
		return;
	}
} // users_telnetcommands

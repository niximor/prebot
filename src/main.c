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

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Linux headers
#include <unistd.h>	// daemon, chdir
#include <signal.h> // signal
#include <time.h>	// time

// My interface
#include "main.h"
#include "version.h"

const char *APP_NAME = "IRCBot";
const char *APP_VERSION = "1.0.0" SVN_VERSION;

// My includes
#include "irclib/irclib.h"
#include "config/config.h"
#include "toolbox/dirs.h"
#include "io.h"
#include "events.h"
#include "socketpool.h"
#include "plugins.h"
#include "timers.h"

// Global variables section

bool breakLoop = false;		/**< If set to true during execution, forces
								 application to quit. */

time_t bootTime;

/**
 * Set application to quit in next main loop cycle.
 */
void system_quit() {
	breakLoop = true;
} // system_quit

/**
 * Get system boot time - read value of bootTime variable
 */
time_t system_boottime() {
	return bootTime;
} // system_boottime

/**
 * Signal handlers
 * @param signum Signal number
 */
void main_signalHandler(int signum) {
	// Reset the signal handler
	signal(signum, main_signalHandler);

	switch (signum) {
		case SIGINT:
			system_quit();
			break;

		case SIGHUP:
			break;
	}
} // signalHandler

struct sAllocInfo {
	void *ptr;
	size_t size;
	char *file;
	int line;
	struct sAllocInfo *next;
	struct sAllocInfo *prev;
};
struct sAllocInfo *alloc_info = NULL;

/**
 * Used to debug purposes.
 */
void *mymalloc(size_t size, char *file, int line) {
	#ifdef malloc
	# undef malloc
	// Find first free slot in alloc_info
	struct sAllocInfo *new_alloc = malloc(sizeof(struct sAllocInfo));
	new_alloc->prev = NULL;
	new_alloc->next = alloc_info;
	new_alloc->ptr = malloc(size);
	new_alloc->size = size;
	new_alloc->file = strdup(file);
	new_alloc->line = line;
	alloc_info = new_alloc;

	fprintf(stderr, "*** Allocated %p - %lu bytes in %s on line %d\n",
		new_alloc->ptr, size, file, line);
	return new_alloc->ptr;
	# define malloc(size) mymalloc(size, __FILE__, __LINE__)
	#else
	// Warning: Unused parameter ...
	if (file == file && line == line) {}
	return malloc(size);
	#endif
} // mymalloc

/**
 * Used to debug purposes
 */
void myfree(void *ptr, char *file, int line) {
	#ifdef free
	# undef free

	// Find in alloc_info correct row
	struct sAllocInfo *alloc = alloc_info;
	while (alloc != NULL) {
		if (alloc->ptr == ptr) break;

		alloc = alloc->next;
	}

	if (alloc != NULL) {
		fprintf(stderr, "*** Deallocating %p - %lu bytes allocated in %s on line "
			"%d (in %s on line %d)\n", ptr, alloc->size, alloc->file,
			alloc->line, file, line);

		free(alloc->file);
		if (alloc->prev != NULL) {
			alloc->prev->next = alloc->next;
		} else {
			alloc_info = alloc->next;
		}
		if (alloc->next != NULL) {
			alloc->next->prev = alloc->prev;
		}
		free(alloc->ptr);
		free(alloc);
	} else {
		fprintf(stderr, "*** Deallocating pointer %p (in %s on line %d)\n",
			ptr, file, line);
		free(ptr);
	}

	# define free(ptr) myfree(ptr, __FILE__, __LINE__)
	#else
	// Warning: Unused parameter ...
	if (file == file && line == line) {}
	free(ptr);
	#endif
} // myfree

/**
 * Debug raw received data
 * @param event IRCEvent_RawData event
 */
void main_rawreceive(EVENT *event) {
	IRCEvent_RawData *eventData = (IRCEvent_RawData *)event->customData;
	fprintf(stderr, ">>> %s\n", eventData->message);
} // main_rawreceive

/**
 * Debug raw sent data
 * @param event IRCEvent_Raw event
 */
void main_rawsend(EVENT *event) {
	IRCEvent_RawData *eventData = (IRCEvent_RawData *)event->customData;
	fprintf(stderr, "<<< %s\n", eventData->message);
} // main_rawreceive

/**
 * Main
 * @param argc Number of arguments on command line
 * @param argv Value of command line arguments
 */
int main(int argc, char *argv[]) {
	// Set boot time
	bootTime = time(NULL);

	// Get configuration file name
	char *dir;
	char *configfile;
	if (argc >= 2) {
		configfile = strdup(argv[1]);
	} else {
		if (argc >= 1) {
			dir = dirs_dirname(argv[0]);
		} else {
			dir = strdup("./");
		}
		configfile = dirs_makefullpath(dir, "ircbot.cfg");
	}
	printError("main", "Trying to load config from %s.", configfile);

	// Go to my root directory.
	chdir(dir);

	// Load configuration
	CONF_SECTION *config = config_parse(configfile);

	free(configfile);
	free(dir);

	// Install signal handlers
	signal(SIGINT, main_signalHandler);
	signal(SIGHUP, main_signalHandler);

	// Redirect logging if user wants
	char *logFileName = config_getvalue_string(config, "logging:file", NULL);
	if (logFileName != NULL && *logFileName != '\0') {
		if (config_getvalue_bool(config, "logging:append", false)) {
			redirLogAppend(logFileName);
		} else {
			redirLog(logFileName);
		}
	}

	printError("main", "Starting...");

	// Daemonize the process if user wants.
	if (config_getvalue_bool(config, "daemon", true)) {
		if (daemon(1, 1) < 0) {
			printError("main", "Failed to start as daemon. Resuming in "
				"foreground.");
		} else {
            printError("main", "Forked into background.");
        }
	}

	// Init timers
    printError("main", "Initializing timers...");
	timers_init();

	// Init events
    printError("main", "Initializing event subsystem...");
	EVENTS *events = events_init();

	// Init socket pool
    printError("main", "Initializing socketpool...");
	SocketPool socketpool = socketpool_init();

	// Init IRCLib
    printError("main", "Initializing IRC subsystem...");
	IRCLib_Connection irc = {
		.hostname = config_getvalue_string(config, "irc:server", "localhost"),
		.port = config_getvalue_int(config, "irc:port", 6667),
		.bind = config_getvalue_string(config, "irc:bind", NULL),
        .force_ipv4 = config_getvalue_bool(config, "irc:force_ipv4", false),
        .force_ipv6 = config_getvalue_bool(config, "irc:force_ipv6", false),
		.nickname = strdup(config_getvalue_string(config, "irc:nickname",
			"IRCBot")),
		.username = config_getvalue_string(config, "irc:username",
			"ircbot"),
		.realname = config_getvalue_string(config, "irc:realname",
			"IRCBot by Niximor"),
		.password = config_getvalue_string(config, "irc:password",
			NULL),
		.events = events,
		.socketpool = socketpool,
		.reconnect = config_getvalue_bool(config, "irc:reconnect", true),
		.aliveCheckTimeout = config_getvalue_int(config, "irc:alivecheck", 30)
	};

	irclib_init(&irc);

	// Load plugins
    printError("main", "Loading plugins...");
	plugins_init(&irc, config, events, socketpool);
	plugins_loaddir(NULL);

	// List loaded plugins for debug purposes:
	Plugin plug = loadedPlugins->first;
	while (plug != NULL) {
		printError("main", "Loaded plugin: %s", plug->name);
		plug = plug->next;
	}

	// End of initialization

	// Install debug events
	// TODO: Make this configurable
	//events_addEventListener(events, "onrawreceive", main_rawreceive, NULL);
	//events_addEventListener(events, "onrawsend", main_rawsend, NULL);

	// Connect to IRC
    printError("main", "Init done.");
	irclib_connect(&irc);

	// Main loop, doing things until breakLoop
	while (!breakLoop) {
		socketpool_pool(socketpool, 1000);
		timers_test();
	}

	printError("main", "Begin shutdown.");

	// Unload plugins
	plugins_close();

	printError("main", "Plugins unloaded.");

	// Close IRC
	irclib_close(&irc);

	// Free events
	events_free(events);

	// Shutdown socket pool
	socketpool_shutdown(socketpool);

	// Free timers
	timers_free();

	// Free configuration
	config_free(config);

	printError("main", "Done.");

	return EXIT_SUCCESS;
} // main

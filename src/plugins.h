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

#ifndef _PLUGINS_H
#define _PLUGINS_H 1

// My includes
#include "irclib/irclib.h"
#include "config/config.h"
#include "events.h"
#include "socketpool.h"
#include "pluginapi.h"

// Forward
typedef struct sPluginList *PluginList;
typedef struct sPlugin *Plugin;

// Global variable
PluginList loadedPlugins;	/**< List of plugins loaded into application */

/**
 * List of loaded plugins
 */
struct sPluginList {
	Plugin first;			/**< First plugin in chain */
	Plugin last;			/**< Last plugin in chain */

	// Instances of various libraries that plugins may use.
	IRCLib_Connection *irc;	/**< IRCLib instance */
	CONF_SECTION *config;	/**< Configuration file */
	EVENTS *events;			/**< Events */
	// ToDo: Timers
	SocketPool socketpool;	/**< Socketpool */
}; // sPluginList

/**
 * Plugin information structure
 */
struct sPlugin {
	Plugin prev;			/**< Previous plugin in chain */
	Plugin next;			/**< Next plugin in chain */

	void *handle;			/**< Handle of loaded library */
	char *path;				/**< File name including path */
	char *name;				/**< File name without path and suffix */
	PluginInfo *info;		/**< PluginInfo structure that plugin uses */
	char *deps;				/**< List of plugin's dependencies */

	PluginInitPrototype *init; /**< Plugin init function */
	PluginDonePrototype *done; /**< Plugin done function */
	PluginDonePrototype *beforeDepsUnload; /**< Called before dependencies
								 are unloaded. Not required function. */
}; // sPlugin

typedef enum {
	PLUG_OK = 0,			/**< Everything was OK, plugin is loaded. */
	PLUG_FILE_NOT_FOUND,	/**< Plugin was not found in path. */
	PLUG_PATH_NOT_FOUND,	/**< Plugin directory is invalid. */
	PLUG_DEPENDENCY,		/**< Plugin dependency error. */
	PLUG_DL_ERROR,			/**< Error while loading library. */
	PLUG_INVALID,			/**< File is not valid plugin. */
	PLUG_ALREADY_LOADED		/**< Plugin is already loaded. */
} PluginLoadStatus;

/**
 * Init plugins interface. All libraries must be initialized and ready to use.
 * @param irc IRCLib library
 * @param config Config file
 * @param events Events library
 * @param socketpool Socketpool
 */
extern void plugins_init(IRCLib_Connection *irc, CONF_SECTION *config,
	EVENTS *events, SocketPool socketpool);

/**
 * Load all plugin in directory. All executable libraries in that directory
 * will be loaded. If any of the libraries isn't valid plugin, error will
 * be reported.
 * Also plugin dependencies will be checked.
 * @param directory Directory where to scan for plugins. If directory is NULL,
 *   config file directory will be used.
 */
extern PluginLoadStatus plugins_loaddir(char *directory);

/**
 * Tries to find plugin with specified name (file name without suffix and path)
 * in standard plugin path.
 * If plugin is found, tries to load it, and if file is not valid plugin, error
 * will be reported.
 * Also check dependencies, and tries to load plugins that this depends on.
 * If anything fails, plugin won't be loaded, and error will be reported.
 * @param name Plugin name
 */
extern PluginLoadStatus plugins_load(char *name);

/**
 * Unloads plugin with specified name. Also unloads all plugins, that depends
 * on this plugin.
 * @param name Plugin name
 */
extern PluginLoadStatus plugins_unload(char *name);

/**
 * Unloads all plugins and free created structures
 */
extern PluginLoadStatus plugins_close();

/**
 * Get PluginInfo structure for plugin by name
 * @param name Plugin name.
 * @return PluginInfo for plugin with specified name, or NULL if that plugin
 *   isn't loaded.
 */
extern PluginInfo *plugins_getinfo(char *name);

#endif

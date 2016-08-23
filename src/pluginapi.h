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

#ifndef _PLUGINAPI_H
#define _PLUGINAPI_H 1

#include <config/config.h>
#include <irclib/irclib.h>
#include <events.h>
#include <socketpool.h>

/**
 * Structure PluginInfo is used for communication between application and
 * plugins. Plugin must fill in name, author and version, and application
 * fills in irc, config and events items.
 */
typedef struct {
	char *name;				/**< Name of plugin */
	char *author;			/**< Name of author */
	char *version;			/**< Version of plugin */

	void *customData;		/**< Custom data that plugin can set */

	IRCLib_Connection *irc;	/**< IRC connection */
	CONF_SECTION *config;	/**< Config file instance */
	EVENTS *events;			/**< Events instance */
	//TIMERS timers;		/**< Timers instance */
	SocketPool socketpool;	/**< Socket pool */
} PluginInfo;

/**
 * Prototype for PluginInit function
 */
typedef void (PluginInitPrototype)(PluginInfo *);

/**
 * Prototype for PluginDone function
 */
typedef void (PluginDonePrototype)(PluginInfo *);

/**
 * Prototype for PluginDeps function. Function is optional and used to
 * tell application about plugins, which this plugin needs to work.
 * Dependencies are returned in function argument, which the function must
 * allocate. Application will take care of free up the string. Plugins are
 * divided by coma without white space.
 */
typedef void (PluginDepsPrototype)(char **deps);

/**
 * Function is called when the module is being loaded.
 * @param info PluginInfo structure, where plugin must fill in name, author
 *   and version items, and application has filled in other items.
 */
void PluginInit(PluginInfo *info);

/**
 * Function is called when the plugin is being unloaded.
 * @param info PluginInfo structure that has been created during plugin
 *   initialization
 */
void PluginDone(PluginInfo *info);

/**
 * Another function isn't required, but when exists, it is called before
 * plugins, that depends on this plugin, are unloaded.
 * void PluginBeforeUnload(PluginInfo *info);
 */

#endif

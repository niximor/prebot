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

// Standard headers
#include <stdlib.h>
#include <string.h>

// Linux headers
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>

// This library interface
#include <plugins.h>
#include <pluginapi.h>

// My includes
#include "toolbox/dirs.h"
#include "config/config.h"
#include "io.h"
#include "tokenizer.h"

/**
 * Init plugins interface. All libraries must be initialized and ready to use.
 * @param irc IRCLib library
 * @param config Config file
 * @param events Events library
 * @param socketpool Socketpool
 */
void plugins_init(IRCLib_Connection *irc, CONF_SECTION *config,
	EVENTS *events, SocketPool socketpool) {

	loadedPlugins = malloc(sizeof(struct sPluginList));
	loadedPlugins->first = NULL;
	loadedPlugins->last = NULL;

	loadedPlugins->irc = irc;
	loadedPlugins->config = config;
	loadedPlugins->events = events;
	loadedPlugins->socketpool = socketpool;
} // plugins_init

/**
 * Scans plugins list to find whether plugin with certain name is loaded
 * or not.
 */
bool plugins_isloaded(char *name) {
	// Scan loaded plugins if this plugin isn't already loaded.
	Plugin loadedplugin = loadedPlugins->first;
	while (loadedplugin != NULL) {
		if (strcmp(loadedplugin->name, name) == 0) {
			return true;
		}
		loadedplugin = loadedplugin->next;
	}

	return false;
} // plugins_isloaded

/**
 * Unloads plugin
 * @param plugin Plugin that should be unloaded.
 */
void plugins_unloadplugin(Plugin plugin) {
	if (plugin->beforeDepsUnload != NULL) {
		plugin->beforeDepsUnload(plugin->info);
	}

	// Unload all plugin's dependencies.
	if (plugin->deps != NULL) {
		TOKENS tok = tokenizer_tokenize(plugin->deps, ',');

		for (size_t i = 0; i < tok->count; i++) {
			if (plugins_isloaded(tokenizer_gettok(tok, i))) {
				plugins_unload(tokenizer_gettok(tok, i));
			}
		}

		tokenizer_free(tok);
	}

	if (plugin->done != NULL) {
		plugin->done(plugin->info);
	}

	// Free plugin handle.
	if (dlclose(plugin->handle) != 0) {
		printError("plugins", "Unable to unload library: %s", dlerror());
	}

	free(plugin->info);
	free(plugin->name);
	free(plugin->path);

	// Remove plugin from chain
	if (plugin->prev != NULL) {
		plugin->prev->next = plugin->next;
	} else {
		loadedPlugins->first = plugin->next;
	}

	if (plugin->next != NULL) {
		plugin->next->prev = plugin->prev;
	} else {
		loadedPlugins->last = plugin->prev;
	}

	free(plugin);
} // plugins_unloadplugin

/**
 * Load plugin in exactly given filename. It checks plugin's dependencies,
 * and tries to load them too.
 * @param path Path to plugin
 * @return Plugin load status
 */
PluginLoadStatus plugins_loadplugin(char *path) {
	PluginLoadStatus result = PLUG_OK;

	char *pathdup = path;

	if (file_exists(pathdup)) {
		char *basename = dirs_basename(pathdup);

		if (plugins_isloaded(basename)) {
			free(basename);
			printError("plugins", "Plugin %s is already loaded.", pathdup);
			return PLUG_ALREADY_LOADED;
		}

		printError("plugins", "Loading plugin %s", pathdup);

		Plugin plugin = malloc(sizeof(struct sPlugin));

		// Add plugin to linked list, for circular dependency check
		plugin->prev = loadedPlugins->last;
		plugin->next = NULL;
		if (loadedPlugins->last != NULL) {
			loadedPlugins->last->next = plugin;
		} else {
			loadedPlugins->first = plugin;
		}
		loadedPlugins->last = plugin;

		plugin->handle = dlopen(pathdup, RTLD_LAZY | RTLD_GLOBAL);
		if (plugin->handle != NULL) {
			plugin->name = basename;
			plugin->path = strdup(path);

			plugin->info = malloc(sizeof(PluginInfo));
			plugin->info->name = NULL;
			plugin->info->author = NULL;
			plugin->info->version = NULL;
			plugin->info->customData = NULL;
			plugin->info->irc = loadedPlugins->irc;
			plugin->info->config = loadedPlugins->config;
			plugin->info->events = loadedPlugins->events;
			plugin->info->socketpool = loadedPlugins->socketpool;

			plugin->deps = NULL;

			plugin->init = (PluginInitPrototype *)dlsym(
				plugin->handle, "PluginInit");
			plugin->done = (PluginDonePrototype *)dlsym(
				plugin->handle, "PluginDone");
			plugin->beforeDepsUnload = (PluginDonePrototype *)dlsym(
				plugin->handle, "PluginBeforeUnload");

			if (plugin->init != NULL && plugin->done != NULL) {
				// Scan plugin dependencies.

				PluginLoadStatus depsLoad = PLUG_OK;

				// PluginDeps function is optional.
				PluginDepsPrototype *plugindeps =
					(PluginDepsPrototype *)dlsym(plugin->handle, "PluginDeps");
				if (plugindeps != NULL) {
					plugindeps(&plugin->deps);

					if (plugin->deps != NULL) {
						TOKENS tok = tokenizer_tokenize(plugin->deps, ',');
						for (size_t i = 0; i < tok->count; i++) {
							if (!plugins_isloaded(tokenizer_gettok(tok, i))) {
								depsLoad = plugins_load(
									tokenizer_gettok(tok, i));
								if (depsLoad != PLUG_OK) {
									break;
								}
							}
						}
						tokenizer_free(tok);
					}
				}

				if (depsLoad == PLUG_OK) {
					// And finally, when everything is OK, call init function.
					plugin->init(plugin->info);
					result = PLUG_OK;
				} else {
					printError("plugins", "Plugin %s will not be loaded, "
						"because of dependency error.", path);
					result = PLUG_DEPENDENCY;
				}
			} else {
				// If no init and done functions were found.
				plugins_unloadplugin(plugin);
				printError("plugins", "%s is not valid plugin.", pathdup);
				result = PLUG_INVALID;
			}

			// Unload plugin if it's loading failed.
			if (result != PLUG_OK) {
				plugin->done = NULL;
				plugins_unloadplugin(plugin);
			}
		// End of plug->handle != NULL
		} else {
			// Remove plugin from linked list
			loadedPlugins->last = loadedPlugins->last->prev;
			loadedPlugins->last->next = NULL;

			printError("plugins", "Plugin loading error: %s", dlerror());
			result = PLUG_DL_ERROR;
		}
	} else {
		printError("plugins", "Plugin file not found.");
		result = PLUG_FILE_NOT_FOUND;
	}

	return result;
} // plugins_loadplugin

/**
 * Load all plugin in directory. All executable libraries in that directory
 * will be loaded. If any of the libraries isn't valid plugin, error will
 * be reported.
 * Also plugin dependencies will be checked.
 * @param directory Directory where to scan for plugins. If directory is NULL,
 *   config file directory will be used.
 * @return Plugin load status
 */
PluginLoadStatus plugins_loaddir(char *directory) {
	if (directory == NULL) {
		// Load list of plugin directories from configuration file

		size_t count = config_getvalue_count(
			loadedPlugins->config, "pluginsdir");

		for (size_t i = 0; i < count; i++) {
			char *dir = config_getvalue_array_string(loadedPlugins->config,
				"pluginsdir", i, "./plugins/");

			if (dir != NULL) {
				plugins_loaddir(dir);
			}
		}
	} else {
		// Load plugins from specified directory
		DirList dl = dirs_load(directory);

		for (int i = 0; i < dl->filesCount; i++) {
			DirEntry file = dl->files[i];

			// Load plugin only if it is executable file
			if (file->mode & S_IXUSR && file->mode & S_IFREG) {
				// Load plugin
				plugins_loadplugin(file->path);
			}
		}

		dirs_freelist(dl);
	}

	return PLUG_OK;
} // plugins_loaddir

/**
 * Tries to find plugin with specified name (file name without suffix and path)
 * in standard plugin path.
 * If plugin is found, tries to load it, and if file is not valid plugin, error
 * will be reported.
 * Also check dependencies, and tries to load plugins that this depends on.
 * If anything fails, plugin won't be loaded, and error will be reported.
 * @param name Plugin name
 * @return Plugin load status
 */
PluginLoadStatus plugins_load(char *name) {
	PluginLoadStatus result = PLUG_FILE_NOT_FOUND;

	// Scan all configured plugin directories
	size_t count = config_getvalue_count(loadedPlugins->config,
		"pluginsdir");

	for (size_t i = 0; i < count; i++) {
		DirList dl = dirs_load(config_getvalue_array_string(
			loadedPlugins->config, "pluginsdir", i, "./plugins/"));

		for (int f = 0; f < dl->filesCount; f++) {
			char *basename = dirs_basename(dl->files[f]->name);

			if (dl->files[f]->mode & S_IXUSR && dl->files[f]->mode & S_IFREG &&
				strncmp(basename, name, strlen(name)) == 0) {

				result = plugins_loadplugin(dl->files[f]->path);
			}

			free(basename);

			if (result != PLUG_FILE_NOT_FOUND &&
				result != PLUG_PATH_NOT_FOUND) {

				break;
			}
		}

		dirs_freelist(dl);

		if (result != PLUG_FILE_NOT_FOUND && result != PLUG_PATH_NOT_FOUND) {
			break;
		}
	}

	return result;
} // plugins_load

/**
 * Unloads plugin with specified name. Also unloads all plugins, that depends
 * on this plugin.
 * @param name Plugin name
 * @return Plugin load status, PLUG_FILE_NOT_FOUND if plugin isn't loaded.
 */
PluginLoadStatus plugins_unload(char *name) {
	Plugin plugin = loadedPlugins->first;
	Plugin next;
	while (plugin != NULL) {
		next = plugin->next;

		if (strncmp(plugin->name, name, strlen(name)) == 0) {
			printError("plugins", "Unloading plugin %s", plugin->path);
			plugins_unloadplugin(plugin);
			return PLUG_OK;
		}

		plugin = next;
	}
	return PLUG_FILE_NOT_FOUND;
} // plugins_unload

/**
 * Unloads all plugins and free created structures
 * @return Plugin load status
 */
PluginLoadStatus plugins_close() {
	// Unload all plugins
	Plugin plug = loadedPlugins->first;
	Plugin next;
	while (plug != NULL) {
		next = plug->next;
		printError("plugins", "Unloading plugin %s", plug->path);
		plugins_unloadplugin(plug);
		plug = next;
	}

	free(loadedPlugins);
	return PLUG_OK;
} // plugins_close

/**
 * Get PluginInfo structure for plugin by name
 * @param name Plugin name.
 * @return PluginInfo for plugin with specified name, or NULL if that plugin
 *   isn't loaded.
 */
PluginInfo *plugins_getinfo(char *name) {
	Plugin plugin = loadedPlugins->first;

	while (plugin != NULL) {
		if (strncmp(plugin->name, name, strlen(name)) == 0) {
			return plugin->info;
		}
		plugin = plugin->next;
	}

	return NULL;
} // plugins_getinfo

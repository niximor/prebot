/**
 *  This file is part of the IRCbot project.
 *  Copyright (C) 2008  Michal Kuchta
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
 *
 *  Parts of this code are inspired by x-chat python plugin which is
 *  copyright (c) 2002-2003  Gustavo Niemeyer <niemeyer@conectiva.com>
 */

// Python
#include <Python.h>
#include "structmember.h"

// This plugin interface
#include "interface.h"

// Standard libraries
#include <string.h>
#include <errno.h>

// My libraries
#include <toolbox/linkedlist.h>
#include <toolbox/tb_string.h>
#include <toolbox/dirs.h>

/**
 * Plugin Python class definition
 */
PyTypeObject plugin_PyTypeObject = {
	PyObject_HEAD_INIT(NULL)
	0,
	"ircbot.plugin",
	sizeof(plugin_PyObject),
	0,
	(destructor)pyplugin_plugin_dealloc,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	"Holds current plugin information",

	NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL,
	#if COUNT_ALLOCS
	0, 0, 0, NULL, NULL
	#endif
}; // plugin_PyTypeObject

/**
 * Create new instance of Python plugin object
 * @param path Path to Python file
 */
PyObject *pyplugin_load(const char *path) {
	char *argv[] = {"<xchat>", 0};

	plugin_PyObject *plugin = PyObject_New(
		plugin_PyObject,
		&(plugin_PyTypeObject)
	);

	if (!plugin) {
		printError(PLUGIN_NAME, "Unable to create Python plugin.");
		goto error;
	}

	PyEval_AcquireLock();
	plugin->tstate = Py_NewInterpreter();

	PySys_SetArgv(1, argv);

	if (plugin->tstate == NULL) {
		printError(PLUGIN_NAME, "Unable to create Python thread state.");
		goto error;
	}

	PySys_SetObject("__plugin__", (PyObject *)plugin);

	pyplugin_install_api(plugin);

	if (path) {
		plugin->fileName = strdup(path);
		FILE *f = fopen(plugin->fileName, "r");
		if (!f) {
			printError(
				PLUGIN_NAME,
				"Unable to open script file %s: %s",
				plugin->fileName,
				strerror(errno)
			);
			goto error;
		}

		if (PyRun_SimpleFile(f, plugin->fileName) != 0) {
			printError(
				PLUGIN_NAME,
				"Script %s failed to load.",
				plugin->fileName
			);
			fclose(f);
			goto error;
		}
		fclose(f);

		PyObject *m = PyDict_GetItemString(PyImport_GetModuleDict(), "__main__");
		if (m == NULL) {
			printError(
				PLUGIN_NAME,
				"%s: Can't get __main__ module",
				plugin->fileName
			);
			goto error;
		}
	}

	PyEval_ReleaseThread(plugin->tstate);

	return (PyObject *)plugin;

	error:
		if (plugin) {
			if (plugin->tstate) {
				Py_EndInterpreter(plugin->tstate);
			}

			Py_DECREF(plugin);
		}
		PyEval_ReleaseLock();

		return NULL;
} // pyplugin_load

/**
 * Get currently active Python plugin
 */
plugin_PyObject *pyplugin_getCurrent() {
	PyObject *plugin = PySys_GetObject("__plugin__");
	if (plugin == NULL) {
		// This cannot happen, because when we don't have __plugin__,
		// we don't have what we are.
		PyErr_SetString(PyExc_RuntimeError, "lost sys.__plugin__");
	}

	return (plugin_PyObject *)plugin;
} // pyplugin_getCurrent

/**
 * Find script by it's file name.
 * @param path File name of script to be found
 */
plugin_PyObject *pyplugin_find_plugin(const char *path) {
	ll_loop(python_plugin_data, plugin) {
		if (eq(plugin->fileName, path)) {
			return plugin;
		}
	}
	return NULL;
} // pyplugin_find_plugin

/**
 * Free plugin Python object
 * @param self plugin PyObject
 */
void pyplugin_plugin_dealloc(plugin_PyObject *self) {
	if (self->fileName) {
		free(self->fileName);
	}

	// Causes double free? So why in x-chat this works?
	//self->ob_type->tp_free((PyObject *)self);
} // pyplugin_plugin_dealloc

/**
 * Remove hooks owner by plugin.
 * @param event Event to scan for hooks
 * @param plugin Plugin which owns events, that should be removed.
 */
void pyplugin_remove_hooks(pyplugin_event *event, plugin_PyObject *plugin) {
	ll_loop(event, hook) {
		if (hook->plugin == plugin) {
			ll_remove(event, hook);
			Py_XDECREF(hook->callback);
			free(hook);
		}
	}
} // pyplugin_remove_hooks

/**
 * Unload loaded python script
 * @param plugin Plugin to unload
 */
void pyplugin_unload(plugin_PyObject *plugin) {
	PyThreadState *tstate = plugin->tstate;
	PyEval_AcquireThread(tstate);

	// Remove plugin from list of loaded plugins
	ll_remove(python_plugin_data, plugin);

	// Remove all events hooked by this plugin
	pyplugin_remove_hooks(&python_plugin_data->channel_message, plugin);
	pyplugin_remove_hooks(&python_plugin_data->private_message, plugin);
	pyplugin_remove_hooks(&python_plugin_data->channel_action, plugin);
	pyplugin_remove_hooks(&python_plugin_data->private_action, plugin);
	pyplugin_remove_hooks(&python_plugin_data->channel_notice, plugin);
	pyplugin_remove_hooks(&python_plugin_data->private_notice, plugin);
	pyplugin_remove_hooks(&python_plugin_data->join, plugin);
	pyplugin_remove_hooks(&python_plugin_data->part, plugin);
	pyplugin_remove_hooks(&python_plugin_data->quit, plugin);
	pyplugin_remove_hooks(&python_plugin_data->kick, plugin);
	pyplugin_remove_hooks(&python_plugin_data->mode, plugin);
	pyplugin_remove_hooks(&python_plugin_data->raw, plugin);
	pyplugin_remove_hooks(&python_plugin_data->nick, plugin);
	pyplugin_remove_hooks(&python_plugin_data->nick_changed, plugin);
	pyplugin_remove_hooks(&python_plugin_data->connected, plugin);
	pyplugin_remove_hooks(&python_plugin_data->disconnected, plugin);
	pyplugin_remove_hooks(&python_plugin_data->joined, plugin);
	pyplugin_remove_hooks(&python_plugin_data->kicked, plugin);
	pyplugin_remove_hooks(&python_plugin_data->parted, plugin);
	pyplugin_remove_hooks(&python_plugin_data->exit, plugin);

	Py_DECREF(plugin);
	Py_EndInterpreter(tstate);
	PyThreadState_Swap(NULL);
	PyEval_ReleaseLock();
} // pyplugin_unload

/**
 * Unload all loaded python scripts
 */
void pyplugin_unload_all() {
	ll_loop(python_plugin_data, plugin) {
		pyplugin_unload(plugin);
	}
} // python_unload_all

/**
 * Load all python scripts from specified directory
 */
void pyplugin_load_dir(char *dirname) {
	if (dirname == NULL) return;

	printError(PLUGIN_NAME,
		"Autoloading Python scripts from directory %s", dirname);

	DirList dl = dirs_load(dirname);
	if (!dl) return;

	for (int i = 0; i < dl->filesCount; i++) {
		plugin_PyObject *plug =
			(plugin_PyObject *)pyplugin_load(dl->files[i]->path);

		if (plug) {
			ll_append(python_plugin_data, plug);
		}
	}
	dirs_freelist(dl);
} // pyplugin_load_dir
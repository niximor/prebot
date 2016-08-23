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
 */

/**
 * ToDo:
 * - IRC events handling
 * - Users objects must be correctly deleted when user leaves
 *   channel or IRC, when changes his nick.
 * - Channels objects must be correctly deleted when application leaves channel
 *   or gets kicked or gets disconnected.
 * - Config access from python scripts
 * - Channel has additional properties (or methods?) with it's modes, topic and
 *   other channel properties
 */

// Python
#include <Python.h>
#include "structmember.h"

// Standard libraries
#include <stdio.h>
#include <string.h>
#include <errno.h>

// Plugins API
#include <pluginapi.h>

// This plugin interface
#include "interface.h"

// Linux headers
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// My libraries
#include <toolbox/tb_string.h>
#include <toolbox/linkedlist.h>
#include <config/config.h>

PythonPluginData *python_plugin_data; /**< Python plugin data */

/**
 * Initialize plugin.
 * @param info Plugin info, where this function must fill in some informations
 */
void PluginInit(PluginInfo *info) {
	info->name = "Python";
	info->author = "Niximor";
	info->version = "1.0.0";

	PythonPluginData *plugData = malloc(sizeof(PythonPluginData));
	plugData->info = info;

	info->customData = plugData;
	python_plugin_data = plugData;

	// Initialize Python
	Py_SetProgramName("ircbot");
	Py_Initialize();

	plugin_PyTypeObject.ob_type = &PyType_Type;
	channel_PyTypeObject.ob_type = &PyType_Type;
	user_PyTypeObject.ob_type = &PyType_Type;
	command_PyTypeObject.ob_type = &PyType_Type;
	timer_PyTypeObject.ob_type = &PyType_Type;

	pyplugin_install_user_class();
	pyplugin_install_channel_class();

	timer_PyTypeObject.tp_new = PyType_GenericNew;
	PyType_Ready(&timer_PyTypeObject);
	Py_INCREF(&timer_PyTypeObject);

	command_PyTypeObject.tp_new = PyType_GenericNew;
	PyType_Ready(&command_PyTypeObject);
	Py_INCREF(&command_PyTypeObject);

	PyEval_InitThreads();
	python_plugin_data->lock = PyThread_allocate_lock();

	python_plugin_data->tstate = PyEval_SaveThread();

	python_plugin_data->channel_dict = PyDict_New();
	python_plugin_data->user_dict = PyDict_New();

	//pyplugin_install_api();

	python_plugin_data->interp_plugin = pyplugin_load(NULL);
	if (python_plugin_data->interp_plugin == NULL) {
		printError(PLUGIN_NAME, "pyplugin_load failed.");
		return;
	}

	// Init linked lists for events
	ll_inits(plugData->channel_message);
	ll_inits(plugData->private_message);
	ll_inits(plugData->channel_action);
	ll_inits(plugData->private_action);
	ll_inits(plugData->channel_notice);
	ll_inits(plugData->private_notice);
	ll_inits(plugData->join);
	ll_inits(plugData->part);
	ll_inits(plugData->quit);
	ll_inits(plugData->kick);
	ll_inits(plugData->mode);
	ll_inits(plugData->raw);
	ll_inits(plugData->nick);
	ll_inits(plugData->nick_changed);
	ll_inits(plugData->connected);
	ll_inits(plugData->disconnected);
	ll_inits(plugData->joined);
	ll_inits(plugData->kicked);
	ll_inits(plugData->parted);
	ll_inits(plugData->exit);

	// Here is python environment ready... So load scripts
	ll_init(plugData);
	pyplugin_load_dir(
		config_getvalue_string(
			plugData->info->config,
			"python:scriptsdir",
			NULL
		)
	);

	// Install event handlers in core
	plugData->onconnected = events_addEventListener(
		plugData->info->events,
		"onconnected",
		pyplugin_event_onconnected,
		NULL
	);

	plugData->ondisconnected = events_addEventListener(
		plugData->info->events,
		"ondisconnected",
		pyplugin_event_ondisconnected,
		NULL
	);

	plugData->onrawreceive = events_addEventListener(
		plugData->info->events,
		"onrawreceive",
		pyplugin_event_onrawreceive,
		NULL
	);

	plugData->onjoin = events_addEventListener(
		plugData->info->events,
		"onjoin",
		pyplugin_event_onjoin,
		NULL
	);

	plugData->onjoined = events_addEventListener(
		plugData->info->events,
		"onjoined",
		pyplugin_event_onjoined,
		NULL
	);

	plugData->onpart = events_addEventListener(
		plugData->info->events,
		"onpart",
		pyplugin_event_onpart,
		NULL
	);

	plugData->onparted = events_addEventListener(
		plugData->info->events,
		"onparted",
		pyplugin_event_onparted,
		NULL
	);

	plugData->onchannelmessage = events_addEventListener(
		plugData->info->events,
		"onchannelmessage",
		pyplugin_event_onchannelmessage,
		NULL
	);

	plugData->onprivatemessage = events_addEventListener(
		plugData->info->events,
		"onprivatemessage",
		pyplugin_event_onprivatemessage,
		NULL
	);

	plugData->onchannelnotice = events_addEventListener(
		plugData->info->events,
		"onchannelnotice",
		pyplugin_event_onchannelnotice,
		NULL
	);

	plugData->onprivatenotice = events_addEventListener(
		plugData->info->events,
		"onprivatenotice",
		pyplugin_event_onprivatenotice,
		NULL
	);

	plugData->onchangeprefix = events_addEventListener(
		plugData->info->events,
		"onchangeprefix",
		pyplugin_event_onchangeprefix,
		NULL
	);

	plugData->onop = events_addEventListener(
		plugData->info->events,
		"onop",
		pyplugin_event_onop,
		NULL
	);

	plugData->ondeop = events_addEventListener(
		plugData->info->events,
		"ondeop",
		pyplugin_event_ondeop,
		NULL
	);

	plugData->onvoice = events_addEventListener(
		plugData->info->events,
		"onvoice",
		pyplugin_event_onvoice,
		NULL
	);

	plugData->ondevoice = events_addEventListener(
		plugData->info->events,
		"ondevoice",
		pyplugin_event_ondevoice,
		NULL
	);

	plugData->onhalfop = events_addEventListener(
		plugData->info->events,
		"onhalfop",
		pyplugin_event_onhalfop,
		NULL
	);

	plugData->ondehalfop = events_addEventListener(
		plugData->info->events,
		"ondehalfop",
		pyplugin_event_ondehalfop,
		NULL
	);

	plugData->onchangelist = events_addEventListener(
		plugData->info->events,
		"onchangelist",
		pyplugin_event_onchangelist,
		NULL
	);

	plugData->onban = events_addEventListener(
		plugData->info->events,
		"onban",
		pyplugin_event_onban,
		NULL
	);

	plugData->onunban = events_addEventListener(
		plugData->info->events,
		"onunban",
		pyplugin_event_onunban,
		NULL
	);

	plugData->onmode = events_addEventListener(
		plugData->info->events,
		"onmode",
		pyplugin_event_onmode,
		NULL
	);

	plugData->onkick = events_addEventListener(
		plugData->info->events,
		"onkick",
		pyplugin_event_onkick,
		NULL
	);

	plugData->onkicked = events_addEventListener(
		plugData->info->events,
		"onkicked",
		pyplugin_event_onkicked,
		NULL
	);

	plugData->onnickchanged = events_addEventListener(
		plugData->info->events,
		"onnickchanged",
		pyplugin_event_onnickchanged,
		NULL
	);

	plugData->onnick = events_addEventListener(
		plugData->info->events,
		"onnick",
		pyplugin_event_onnick,
		NULL
	);
} // PluginInit

/**
 * Close plugin
 * @param info Plugin info, which this function may use to get some
 *   informations it may need.
 */
void PluginDone(PluginInfo *info) {
	PythonPluginData *plugData = (PythonPluginData *)info->customData;

	// Remove IRC events
	events_removeEventListener(plugData->onconnected);
	events_removeEventListener(plugData->ondisconnected);
	events_removeEventListener(plugData->onrawreceive);
	events_removeEventListener(plugData->onjoin);
	events_removeEventListener(plugData->onjoined);
	events_removeEventListener(plugData->onpart);
	events_removeEventListener(plugData->onparted);
	events_removeEventListener(plugData->onchannelmessage);
	events_removeEventListener(plugData->onprivatemessage);
	events_removeEventListener(plugData->onchannelnotice);
	events_removeEventListener(plugData->onprivatenotice);
	events_removeEventListener(plugData->onchangeprefix);
	events_removeEventListener(plugData->onop);
	events_removeEventListener(plugData->ondeop);
	events_removeEventListener(plugData->onvoice);
	events_removeEventListener(plugData->ondevoice);
	events_removeEventListener(plugData->onhalfop);
	events_removeEventListener(plugData->ondehalfop);
	events_removeEventListener(plugData->onchangelist);
	events_removeEventListener(plugData->onban);
	events_removeEventListener(plugData->onunban);
	events_removeEventListener(plugData->onmode);
	events_removeEventListener(plugData->onkick);
	events_removeEventListener(plugData->onkicked);
	events_removeEventListener(plugData->onnickchanged);
	events_removeEventListener(plugData->onnick);

	// Unload scripts
	pyplugin_unload_all();

	Py_DECREF(python_plugin_data->channel_dict);
	Py_DECREF(python_plugin_data->user_dict);

	if (python_plugin_data->interp_plugin) {
		Py_DECREF(python_plugin_data->interp_plugin);
	}

	PyThreadState_Swap(plugData->tstate);

	// Finalize Python
	Py_Finalize();
	Py_SetProgramName(NULL);

	PyThread_free_lock(python_plugin_data->lock);

	free(plugData);
} // PluginDone

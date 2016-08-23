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

// Python
#include <Python.h>
#include "structmember.h"

// Plugins API
#include <pluginapi.h>

// This plugin interface
#include "interface.h"

// My libraries
#include <toolbox/linkedlist.h>
#include <toolbox/tb_string.h>

/**
 * Methods available in ircbot module
 */
PyMethodDef python_IrcBotMethods[] = {
	{
		"raw_send",
		pyplugin_raw_send,
		METH_VARARGS,
		"Send RAW irc message to IRC server."
	},
	{
		"find_channel",
		pyplugin_find_channel,
		METH_VARARGS,
		"Find channel by name."
	},
	{
		"find_user",
		pyplugin_find_user,
		METH_VARARGS,
		"Find user by nickname."
	},
	{
		"hook_event",
		pyplugin_hook_event,
		METH_VARARGS,
		"Hook event handler to specified event."
	},
	{NULL, NULL, 0, NULL}
}; // python_IrcBotMethods

/**
 * Hook callback to event
 * API function ircbot.hook_event(event_name, callback)
 */
PyObject *pyplugin_hook_event(PyObject *self, PyObject *args) {
	char *event_name;
	PyObject *callback;
	PyObject *result = NULL;

	// Try to find corresponding script
	plugin_PyObject *plugin = pyplugin_getCurrent();

	if (PyArg_ParseTuple(args, "sO", &event_name, &callback)) {
		if (callback == NULL) return NULL;

		pyplugin_event *event = NULL;

		if (eq(event_name, "channel_message"))
			event = &(python_plugin_data->channel_message);

		else if (eq(event_name, "private_message"))
			event = &(python_plugin_data->private_message);

		else if (eq(event_name, "channel_action"))
			event = &(python_plugin_data->channel_action);

		else if (eq(event_name, "private_action"))
			event = &(python_plugin_data->private_action);

		else if (eq(event_name, "channel_notice"))
			event = &(python_plugin_data->channel_notice);

		else if (eq(event_name, "private_notice"))
			event = &(python_plugin_data->private_notice);

		else if (eq(event_name, "join"))
			event = &(python_plugin_data->join);

		else if (eq(event_name, "part"))
			event = &(python_plugin_data->part);

		else if (eq(event_name, "quit"))
			event = &(python_plugin_data->quit);

		else if (eq(event_name, "kick"))
			event = &(python_plugin_data->kick);

		else if (eq(event_name, "mode"))
			event = &(python_plugin_data->mode);

		else if (eq(event_name, "raw"))
			event = &(python_plugin_data->raw);

		else if (eq(event_name, "nick"))
			event = &(python_plugin_data->nick);

		else if (eq(event_name, "nick_changed"))
			event = &(python_plugin_data->nick_changed);

		else if (eq(event_name, "connected"))
			event = &(python_plugin_data->connected);

		else if (eq(event_name, "disconnected"))
			event = &(python_plugin_data->disconnected);

		else if (eq(event_name, "joined"))
			event = &(python_plugin_data->joined);

		else if (eq(event_name, "kicked"))
			event = &(python_plugin_data->kicked);

		else if (eq(event_name, "parted"))
			event = &(python_plugin_data->parted);

		else if (eq(event_name, "exit"))
			event = &(python_plugin_data->exit);

		if (event) {
			pyplugin_event_cb handler =
				malloc(sizeof(struct s_pyplugin_event_cb));

			Py_INCREF(callback);
			handler->callback = callback;
			handler->plugin = plugin;
			ll_append(event, handler);

			Py_INCREF(Py_None);
			result = Py_None;
		}
	}

	return result;
	(void)self;
} // pyplugin_hook_event

/**
 * Send raw message to IRC server
 * API function ircbot.raw_send(text)
 */
PyObject *pyplugin_raw_send(PyObject *self, PyObject *args) {
	char *msg;
	PyArg_ParseTuple(args, "s", &msg);
	irclib_sendraw(python_plugin_data->info->irc, "%s", msg);

	Py_RETURN_NONE;
	(void)self;
} // pyplugin_raw_send

/**
 * Construct python channel object from IRCLib channel
 * @param channel IRCLib channel
 * @return Python channel class instance
 */
PyObject *pyplugin_channel_object(IRCLib_Channel channel) {
	if (channel == NULL) return NULL;

	PyObject *channel_name = PyString_FromString(channel->name);

	// Test if channel is already in dictionary, if so, return it instead
	// of creating new object.
	if (
		PyDict_Contains(
			python_plugin_data->channel_dict,
			channel_name
		)
	) {
		Py_DECREF(channel_name);
		return PyDict_GetItem(python_plugin_data->channel_dict, channel_name);
	}

	// Channel is not in dictionary, create new one.

	// SEGFAULT HERE
	PyObject *result = pyplugin_channel_new(
		&(channel_PyTypeObject),
		NULL,
		NULL
	);

	PyObject *temp = ((channel_PyObject *)result)->name;
	((channel_PyObject *)result)->name = channel_name;
	Py_XDECREF(temp);

	// ToDo: Fill in users tuple

	// Add channel to dictionary
	PyDict_SetItem(python_plugin_data->channel_dict, channel_name, result);

	return result;
} // pyplugin_channel_object

/**
 * Find channel acording to channel name
 * @return channel class instance
 */
PyObject *pyplugin_find_channel(PyObject *self, PyObject *args) {
	char *channel;
	if (!PyArg_ParseTuple(args, "s", &channel)) return NULL;

	IRCLib_Channel s_channel = irclib_find_channel(
		python_plugin_data->info->irc->channelStorage,
		channel
	);

	PyObject *result = Py_None;
	Py_INCREF(Py_None);

	if (channel != NULL) {
		PyObject *temp = result;
		result = pyplugin_channel_object(s_channel);
		Py_XDECREF(temp);
	}

	return result;
	(void)self;
} // pyplugin_find_channel

/**
 * Construct python user object from IRCLib user.
 * @param user IRCLib user structure
 * @return Python user class instance
 */
PyObject *pyplugin_user_object(IRCLib_User user) {
	if (user == NULL) return NULL;

	PyObject *user_name = PyString_FromString(user->host->nick);

	// Test if user is already in dictionary, if so, return it instead
	// of creating new object.
	if (
		PyDict_Contains(
			python_plugin_data->user_dict,
			user_name
		)
	) {
		Py_DECREF(user_name);
		return PyDict_GetItem(python_plugin_data->user_dict, user_name);
	}

	// Channel is not in dictionary, create new one.

	PyObject *result = pyplugin_user_new(
		&(user_PyTypeObject),
		NULL,
		NULL
	);

	PyObject *temp = ((user_PyObject *)result)->nick;
	((user_PyObject *)result)->nick = PyString_FromString(user->host->nick);
	Py_XDECREF(temp);

	temp = ((user_PyObject *)result)->user;
	((user_PyObject *)result)->user = PyString_FromString(user->host->user);
	Py_XDECREF(temp);

	temp = ((user_PyObject *)result)->hostname;
	((user_PyObject *)result)->hostname = PyString_FromString(user->host->host);
	Py_XDECREF(temp);

	// ToDo: Fill in channels tuple

	// Add channel to dictionary
	PyDict_SetItem(python_plugin_data->user_dict, user_name, result);

	return result;
} // pyplugin_user_object

/**
 * Find user acording to his nick.
 * API function ircbot.find_user(nick)
 * @return user class instance with found user or Py_None if user cannot
 *   be found.
 */
PyObject *pyplugin_find_user(PyObject *self, PyObject *args) {
	char *nick;
	if (!PyArg_ParseTuple(args, "s", &nick)) return NULL;

	IRCLib_User user = irclib_find_user(
		python_plugin_data->info->irc->userStorage,
		nick
	);

	PyObject *result = Py_None;
	Py_INCREF(Py_None);

	if (user != NULL) {
		PyObject *temp = result;
		result = pyplugin_user_object(user);
		Py_XDECREF(temp);
	}

	return result;
	(void)self;
} // pyplugin_find_user
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

// This plugin interface
#include "interface.h"

#include <toolbox/tb_string.h>

/**
 * Channel Python class definition
 */
PyTypeObject channel_PyTypeObject = {
	PyObject_HEAD_INIT(NULL)
	0,							/*ob_size*/
	"ircbot.channel",			/*tp_name*/
	sizeof(channel_PyObject),	/*tp_basicsize*/
	0,							/*tp_itemsize*/
	0,							/*tp_dealloc*/
	0,							/*tp_print*/
	0,							/*tp_getattr*/
	0,							/*tp_setattr*/
	0,							/*tp_compare*/
	0,							/*tp_repr*/
	0,							/*tp_as_number*/
	0,							/*tp_as_sequence*/
	0,							/*tp_as_mapping*/
	0,							/*tp_hash */
	0,							/*tp_call*/
	0,							/*tp_str*/
	pyplugin_channel_getattro,	/*tp_getattro*/
	pyplugin_channel_setattro,	/*tp_setattro*/
	0,							/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,			/*tp_flags*/
	"IRC channel class",		/*tp_doc*/

	NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL,
	#if COUNT_ALLOCS
	0, 0, 0, NULL, NULL
	#endif
}; // channel_PyTypeObject

/**
 * Members of channel Python class
 */
PyMemberDef channel_members[] = {
	{
		"name",
		T_OBJECT_EX,
		offsetof(channel_PyObject, name),
		READONLY,
		"Name of the channel"
	},
	{NULL, 0, 0, 0, NULL}
}; // channel_members

/**
 * Methods of channel Python class
 */
PyMethodDef channel_methods[] = {
	{
		"message",
		pyplugin_channel_message,
		METH_VARARGS,
		"Send message to channel."
	},
	{
		"action",
		pyplugin_channel_action,
		METH_VARARGS,
		"Sends action (/me) to channel."
	},
	{
		"notice",
		pyplugin_channel_notice,
		METH_VARARGS,
		"Sends notice to channel."
	},
	{
		"kick",
		pyplugin_channel_kick,
		METH_VARARGS,
		"Kicks user from channel."
	},
	{
		"ban",
		pyplugin_channel_ban,
		METH_VARARGS,
		"Bans address on channel."
	},
	{
		"unban",
		pyplugin_channel_unban,
		METH_VARARGS,
		"Unbans address on channel."
	},
	{
		"mode",
		pyplugin_channel_mode,
		METH_VARARGS,
		"Sends mode string to channel."
	},
	{
		"part",
		pyplugin_channel_mode,
		METH_VARARGS,
		"Parts channel."
	},
	{NULL, NULL, 0, NULL},
}; // channel_methods

/**
 * Get attribute of channel object
 * @param obj Channel object
 * @param name Attribute name
 */
PyObject *pyplugin_channel_getattro(PyObject *obj, PyObject *name) {
	PyObject *result = PyObject_GenericGetAttr(obj, name);

	if (!result) {
		// Not-existing property is OK here...
		PyErr_Clear();

		// Get attribute from dictionary
		channel_PyObject *chan = (channel_PyObject *)obj;
		result = PyDict_GetItem(chan->dict, name);

		// Property does not exists in dictionary. It will be created
		// as soon as set is called, until then, it is None.
		if (!result) {
			result = Py_None;
			Py_INCREF(Py_None);
		}
		Py_INCREF(result);
	}

	return result;
} // pyplugin_channel_getattr

/**
 * Set attribute of channel object.
 * @param obj Channel object
 * @param name Attribute name
 * @param value Attribute value
 */
int pyplugin_channel_setattro(PyObject *obj, PyObject *name, PyObject *value) {
	if (PyObject_GenericSetAttr(obj, name, value) != 0) {
		// Non existing property is OK here...
		PyErr_Clear();

		// Set attribute in dictionary
		channel_PyObject *chan = (channel_PyObject *)obj;

		return PyDict_SetItem(chan->dict, name, value);
	}

	// 0 means success... (weird...)
	return 0;
} // pyplugin_channel_setattr

/**
 * Install class channel to Python environment
 */
void pyplugin_install_channel_class() {
	// Add class channel to ircbot module.
	channel_PyTypeObject.tp_new = pyplugin_channel_new;
	channel_PyTypeObject.tp_dealloc = (destructor)pyplugin_channel_dealloc;
	channel_PyTypeObject.tp_members = channel_members;
	channel_PyTypeObject.tp_methods = channel_methods;

	/*PyType_Ready(&channel_PyTypeObject);*/
	Py_INCREF(&channel_PyTypeObject);
} // pyplugin_install_channel_class

/**
 * Channel class constructor
 * @param type Object type
 * @param args Arguments passed to constructor
 * @param kwds Keyword arguments
 */
PyObject *pyplugin_channel_new(PyTypeObject *type, PyObject *args,
	PyObject *kwds) {

	channel_PyObject *self;

	// Allocate the object
	self = //(channel_PyObject *)type->tp_alloc(type, 0);
		PyObject_New(
			channel_PyObject,
			type
		);

	if (self != NULL) {
		// Set default content of properties
		self->name = PyString_FromString("");
		self->users = PyTuple_New(0);
		self->dict = PyDict_New();
		Py_INCREF(Py_None);
		PyDict_SetItemString(self->dict, "test", Py_None);
	}

	return (PyObject *)self;

	// Unused arguments
	(void)args;
	(void)kwds;
} // pyplugin_user_new

/**
 * Destructor that frees up memory for channel class
 * @param self user_PyObject to be freed
 */
void pyplugin_channel_dealloc(channel_PyObject *self) {
	Py_XDECREF(self->name);
	Py_XDECREF(self->users);
	Py_XDECREF(self->dict);
	self->ob_type->tp_free((PyObject *)self);
} // pyplugin_user_dealloc

/**
 * Send message to channel
 * API function ircbot.channel.message(self, text)
 */
PyObject *pyplugin_channel_message(PyObject *self, PyObject *args) {
	PyObject *result = NULL;

	char *message;
	if (PyArg_ParseTuple(args, "s", &message)) {
		char *channel = PyString_AsString(((channel_PyObject *)self)->name);
		if (!eq(channel, "")) {
			irclib_message(python_plugin_data->info->irc, channel, "%s", message);
			Py_INCREF(Py_None);
			result = Py_None;
		}
	}
	return result;
} // pyplugin_channel_message

/**
 * Send ACTION to channel
 * API function ircbot.channel.action(self, text)
 */
PyObject *pyplugin_channel_action(PyObject *self, PyObject *args) {
	PyObject *result = NULL;

	char *message;
	if (PyArg_ParseTuple(args, "s", &message)) {
		char *channel = PyString_AsString(((channel_PyObject *)self)->name);
		if (!eq(channel, "")) {
			irclib_action(python_plugin_data->info->irc, channel, "%s", message);
			Py_INCREF(Py_None);
			result = Py_None;
		}
	}
	return result;
} // pyplugin_channel_action

/**
 * Send notice to channel
 * API function ircbot.channel.notice(self, text)
 */
PyObject *pyplugin_channel_notice(PyObject *self, PyObject *args) {
	PyObject *result = NULL;

	char *message;
	if (PyArg_ParseTuple(args, "s", &message)) {
		char *channel = PyString_AsString(((channel_PyObject *)self)->name);
		if (!eq(channel, "")) {
			irclib_notice(python_plugin_data->info->irc, channel, "%s", message);
			Py_INCREF(Py_None);
			result = Py_None;
		}
	}
	return result;
} // pyplugin_channel_notice

/**
 * Kicks nick from channel
 * API function ircbot.channel.kick(self, nick, reason)
 */
PyObject *pyplugin_channel_kick(PyObject *self, PyObject *args) {
	PyObject *result = NULL;

	char *nick;
	char *reason;

	if (PyArg_ParseTuple(args, "s|s", &nick, &reason)) {
		char *channel = PyString_AsString(((channel_PyObject *)self)->name);
		if (!eq(channel, "")) {

			if (reason != NULL) {
				irclib_kick(python_plugin_data->info->irc, channel, nick, "%s",
					reason);
			} else {
				irclib_kick(python_plugin_data->info->irc, channel, nick, "");
			}

			Py_INCREF(Py_None);
			result = Py_None;
		}
	}
	return result;
} // pyplugin_channel_kick

/**
 * Ban address from channel
 * API function ircbot.channel.ban(self, address)
 */
PyObject *pyplugin_channel_ban(PyObject *self, PyObject *args) {
	PyObject *result = NULL;

	char *address;
	if (PyArg_ParseTuple(args, "s", &address)) {
		char *channel = PyString_AsString(((channel_PyObject *)self)->name);
		if (!eq(channel, "")) {
			irclib_mode(python_plugin_data->info->irc, "%s +b %s",
				channel, address);

			Py_INCREF(Py_None);
			result = Py_None;
		}
	}
	return result;
} // pyplugin_channel_ban

/**
 * Unban address from channel
 * API function ircbot.channel.ban(self, address)
 */
PyObject *pyplugin_channel_unban(PyObject *self, PyObject *args) {
	PyObject *result = NULL;

	char *address;
	if (PyArg_ParseTuple(args, "s", &address)) {
		char *channel = PyString_AsString(((channel_PyObject *)self)->name);
		if (!eq(channel, "")) {
			irclib_mode(python_plugin_data->info->irc, "%s -b %s",
				channel, address);

			Py_INCREF(Py_None);
			result = Py_None;
		}
	}
	return result;
} // pyplugin_channel_unban

/**
 * Sends mode string to channel
 * API function ircbot.channel.ban(self, address)
 */
PyObject *pyplugin_channel_mode(PyObject *self, PyObject *args) {
	PyObject *result = NULL;

	char *mode;
	if (PyArg_ParseTuple(args, "s", &mode)) {
		char *channel = PyString_AsString(((channel_PyObject *)self)->name);
		if (!eq(channel, "")) {
			irclib_mode(python_plugin_data->info->irc, "%s %s",
				channel, mode);

			Py_INCREF(Py_None);
			result = Py_None;
		}
	}
	return result;
} // pyplugin_channel_mode

/**
 * Part the channel
 * API function ircbot.channel.part(self, nick, reason)
 */
PyObject *pyplugin_channel_part(PyObject *self, PyObject *args) {
	PyObject *result = NULL;

	char *reason;

	if (PyArg_ParseTuple(args, "|s", &reason)) {
		char *channel = PyString_AsString(((channel_PyObject *)self)->name);
		if (!eq(channel, "")) {

			if (reason != NULL) {
				irclib_part(python_plugin_data->info->irc, channel, "%s",
					reason);
			} else {
				irclib_part(python_plugin_data->info->irc, channel, "");
			}

			Py_INCREF(Py_None);
			result = Py_None;
		}
	}
	return result;
} // pyplugin_channel_part
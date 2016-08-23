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

// My libraries
#include <toolbox/tb_string.h>

/**
 * User Python class
 */
PyTypeObject user_PyTypeObject = {
	PyObject_HEAD_INIT(NULL)
	0,							/*ob_size*/
	"ircbot.user",				/*tp_name*/
	sizeof(user_PyObject),		/*tp_basicsize*/
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
	pyplugin_user_getattro,		/*tp_getattro*/
	pyplugin_user_setattro,		/*tp_setattro*/
	0,							/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,			/*tp_flags*/
	"IRC user class",			/*tp_doc*/

	NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL,
	#if COUNT_ALLOCS
	0, 0, 0, NULL, NULL
	#endif
}; // user_PyTypeObject

/**
 * Members of user Python class
 */
PyMemberDef user_members[] = {
	{
		"nick",
		T_OBJECT_EX,
		offsetof(user_PyObject, nick),
		READONLY,
		"Nick of user"
	},
	{
		"user",
		T_OBJECT_EX,
		offsetof(user_PyObject, user),
		READONLY,
		"Username of user"
	},
	{
		"hostname",
		T_OBJECT_EX,
		offsetof(user_PyObject, hostname),
		READONLY,
		"Hostname of user"
	},
	{NULL, 0, 0, 0, NULL}
}; // user_members

/**
 * Methods of user Python class
 */
PyMethodDef user_methods[] = {
	{
		"message",
		pyplugin_user_message,
		METH_VARARGS,
		"Sends message to user."
	},
	{
		"action",
		pyplugin_user_action,
		METH_VARARGS,
		"Sends action (/me) to user."
	},
	{
		"notice",
		pyplugin_user_notice,
		METH_VARARGS,
		"Sends notice to user."
	},
	{NULL, NULL, 0, NULL},
}; // user_methods

/**
 * Get attribute of user object
 * @param obj User object
 * @param name Attribute name
 */
PyObject *pyplugin_user_getattro(PyObject *obj, PyObject *name) {
	PyObject *result = PyObject_GenericGetAttr(obj, name);

	if (!result) {
		// Not-existing property is OK here...
		PyErr_Clear();

		// Get attribute from dictionary
		user_PyObject *user = (user_PyObject *)obj;
		result = PyDict_GetItem(user->dict, name);

		// Property does not exists in dictionary. It will be created
		// as soon as set is called, until then, it is None.
		if (!result) {
			result = Py_None;
			Py_INCREF(Py_None);
		}
	}

	return result;
} // pyplugin_user_getattr

/**
 * Set attribute of user object.
 * @param obj User object
 * @param name Attribute name
 * @param value Attribute value
 */
int pyplugin_user_setattro(PyObject *obj, PyObject *name, PyObject *value) {
	if (PyObject_GenericSetAttr(obj, name, value) != 0) {
		// Non existing property is OK here...
		PyErr_Clear();

		// Set attribute in dictionary
		user_PyObject *user = (user_PyObject *)obj;

		return PyDict_SetItem(user->dict, name, value);
	}

	// 0 means success... (weird...)
	return 0;
} // pyplugin_channel_setattr

/**
 * Install class user to Python environment
 */
void pyplugin_install_user_class() {
	// Add class user to ircbot module.
	user_PyTypeObject.tp_new = pyplugin_user_new;
	user_PyTypeObject.tp_dealloc = (destructor)pyplugin_user_dealloc;
	user_PyTypeObject.tp_members = user_members;
	user_PyTypeObject.tp_methods = user_methods;

	/*PyType_Ready(&(user_PyTypeObject));*/
	Py_INCREF(&user_PyTypeObject);
} // pyplugin_install_user_class

/**
 * User class constructor
 * @param type Object type
 * @param args Arguments passed to constructor
 * @param kwds Keyword arguments
 */
PyObject *pyplugin_user_new(PyTypeObject *type, PyObject *args,
	PyObject *kwds) {

	user_PyObject *self;

	// Allocate the object
	self = //(user_PyObject *)type->tp_alloc(type, 0);
		PyObject_New(
			user_PyObject,
			type
		);
	if (self != NULL) {
		// Set default content of properties
		self->nick = PyString_FromString("");
		self->user = PyString_FromString("");
		self->hostname = PyString_FromString("");
		self->channels = PyTuple_New(0);
		self->dict = PyDict_New();
	}

	return (PyObject *)self;

	// Unused arguments
	(void)args;
	(void)kwds;
} // pyplugin_user_new

/**
 * Destructor that frees up memory for user class
 * @param self user_PyObject to be freed
 */
void pyplugin_user_dealloc(user_PyObject *self) {
	Py_XDECREF(self->nick);
	Py_XDECREF(self->user);
	Py_XDECREF(self->hostname);
	Py_XDECREF(self->dict);
	self->ob_type->tp_free((PyObject *)self);
} // pyplugin_user_dealloc

/**
 * Sends message to user - method ircbot.user.message(self, text)
 * @param self user class instance
 * @param args Arguments passed to method
 */
PyObject *pyplugin_user_message(PyObject *self, PyObject *args) {
	PyObject *result = NULL;

	char *message;
	if (PyArg_ParseTuple(args, "s", &message)) {
		char *nick = PyString_AsString(((user_PyObject *)self)->nick);
		if (!eq(nick, "")) {
			irclib_message(python_plugin_data->info->irc, nick, "%s", message);
			Py_INCREF(Py_None);
			result = Py_None;
		}
	}
	return result;
} // pyplugin_user_message

/**
 * Sends ACTION to user - method ircbot.user.action(self, text)
 * @param self user class instance
 * @param args Arguments passed to method
 */
PyObject *pyplugin_user_action(PyObject *self, PyObject *args) {
	PyObject *result = NULL;

	char *message;
	if (PyArg_ParseTuple(args, "s", &message)) {
		char *nick = PyString_AsString(((user_PyObject *)self)->nick);
		if (!eq(nick, "")) {
			irclib_action(python_plugin_data->info->irc, nick, "%s", message);
			Py_INCREF(Py_None);
			result = Py_None;
		}
	}
	return result;
} // pyplugin_user_action

/**
 * Sends NOTICE to user - method ircbot.user.notice(self, text)
 * @param self user class instance
 * @param args Arguments passed to method
 */
PyObject *pyplugin_user_notice(PyObject *self, PyObject *args) {
	PyObject *result = NULL;

	char *message;
	if (PyArg_ParseTuple(args, "s", &message)) {
		char *nick = PyString_AsString(((user_PyObject *)self)->nick);
		if (!eq(nick, "")) {
			irclib_notice(python_plugin_data->info->irc, nick, "%s", message);
			Py_INCREF(Py_None);
			result = Py_None;
		}
	}
	return result;
} // pyplugin_user_action
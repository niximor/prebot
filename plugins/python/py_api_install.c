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

/**
 * Timer Python class definition
 */
PyTypeObject timer_PyTypeObject = {
	PyObject_HEAD_INIT(NULL)
	0,							/*ob_size*/
	"ircbot.timer",				/*tp_name*/
	sizeof(timer_PyObject),		/*tp_basicsize*/
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
	0,							/*tp_getattro*/
	0,							/*tp_setattro*/
	0,							/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,			/*tp_flags*/
	"Timer class",				/*tp_doc*/

	NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL,
	#if COUNT_ALLOCS
	0, 0, 0, NULL, NULL
	#endif
}; // timer_PyTypeObject

/**
 * Command Python class definition
 */
PyTypeObject command_PyTypeObject = {
	PyObject_HEAD_INIT(NULL)
	0,							/*ob_size*/
	"ircbot.command",			/*tp_name*/
	sizeof(command_PyObject),	/*tp_basicsize*/
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
	0,							/*tp_getattro*/
	0,							/*tp_setattro*/
	0,							/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,			/*tp_flags*/
	"Command class",			/*tp_doc*/

	NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL,
	#if COUNT_ALLOCS
	0, 0, 0, NULL, NULL
	#endif
}; // command_PyTypeObject

/**
 * Install plugin API to Python environment
 */
void pyplugin_install_api(plugin_PyObject *object) {
	// Init ircbot module
	object->ircbot_module = Py_InitModule3("ircbot", python_IrcBotMethods,
		"ircbot API");

	Py_INCREF((PyObject *)&user_PyTypeObject);
	PyModule_AddObject(
		object->ircbot_module,
		"user",
		(PyObject *)&user_PyTypeObject
	);

	Py_INCREF((PyObject *)&channel_PyTypeObject);
	PyModule_AddObject(
		object->ircbot_module,
		"channel",
		(PyObject *)&channel_PyTypeObject
	);

	Py_INCREF((PyObject *)&plugin_PyTypeObject);
	PyModule_AddObject(
		object->ircbot_module,
		"plugin",
		(PyObject *)&plugin_PyTypeObject
	);

	Py_INCREF((PyObject *)&timer_PyTypeObject);
	PyModule_AddObject(
		object->ircbot_module,
		"timer",
		(PyObject *)&timer_PyTypeObject
	);

	Py_INCREF((PyObject *)&command_PyTypeObject);
	PyModule_AddObject(
		object->ircbot_module,
		"command",
		(PyObject *)&command_PyTypeObject
	);
} // pyplugin_install_api

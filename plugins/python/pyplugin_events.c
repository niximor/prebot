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
#include <toolbox/linkedlist.h>

/**
 * Call callback functions for specified list of callbacks.
 * @param callbacks List of callbacks
 * @param arglist Arguments that will be passed to the Python function
 */
void pyplugin_callback(pyplugin_event *callbacks, PyObject *arglist) {
	ll_loop(callbacks, cb) {
		PyEval_AcquireThread(cb->plugin->tstate);

		Py_XINCREF(arglist);
		PyObject *result = PyEval_CallObject(cb->callback, arglist);
		if (!result) {
			PyErr_Print();
		}
		Py_XDECREF(arglist);
		Py_XDECREF(result);

		PyEval_ReleaseThread(cb->plugin->tstate);
	}
} // pyplugin_callback

/**
 * onconnected IRC event handler
 */
void pyplugin_event_onconnected(EVENT *event) {
	PyObject *arglist = Py_BuildValue("()");
	pyplugin_callback(&(python_plugin_data->connected), arglist);
	Py_DECREF(arglist);
	(void)event;
} // pyplugin_event_onconnected

/**
 * ondisconnected IRC event handler
 */
void pyplugin_event_ondisconnected(EVENT *event) {
	PyObject *arglist = Py_BuildValue("()");
	pyplugin_callback(&(python_plugin_data->disconnected), arglist);
	Py_DECREF(arglist);
	(void)event;
} // pyplugin_event_ondisconnected

/**
 * onrawreceive IRC event handler
 */
void pyplugin_event_onrawreceive(EVENT *event) {
	PyObject *arglist = Py_BuildValue(
		"(s)",
		((IRCEvent_RawData *)event->customData)->message
	);
	pyplugin_callback(&(python_plugin_data->raw), arglist);
	Py_DECREF(arglist);
	(void)event;
} // pyplugin_event_onrawreceive

/**
 * onjoin IRC event handler
 */
void pyplugin_event_onjoin(EVENT *event) {
	IRCEvent_JoinPart *evt = (IRCEvent_JoinPart *)(event->customData);

	IRCLib_Channel channel = irclib_find_channel(
		python_plugin_data->info->irc->channelStorage,
		evt->channel
	);

	IRCLib_User user = irclib_find_user(
		python_plugin_data->info->irc->userStorage,
		evt->address->nick
	);

	PyObject *po_channel = pyplugin_channel_object(channel);
	PyObject *po_user = pyplugin_user_object(user);

	Py_INCREF(po_channel);
	Py_INCREF(po_user);

	PyObject *arglist = Py_BuildValue("(OO)", po_channel, po_user);
	pyplugin_callback(&(python_plugin_data->join), arglist);
	Py_DECREF(arglist);

	Py_DECREF(po_channel);
	Py_DECREF(po_user);
} // pyplugin_event_onjoin

/**
 * onjoined IRC event handler
 */
void pyplugin_event_onjoined(EVENT *event) {
	IRCEvent_JoinPart *evt = (IRCEvent_JoinPart *)(event->customData);

	IRCLib_Channel channel = irclib_find_channel(
		python_plugin_data->info->irc->channelStorage,
		evt->channel
	);

	PyObject *po_channel = pyplugin_channel_object(channel);

	Py_INCREF(po_channel);

	PyObject *arglist = Py_BuildValue("(O)", po_channel);
	pyplugin_callback(&(python_plugin_data->joined), arglist);
	Py_DECREF(arglist);

	Py_DECREF(po_channel);
} // pyplugin_event_onjoined

/**
 * onpart IRC event handler
 */
void pyplugin_event_onpart(EVENT *event) {
	IRCEvent_JoinPart *evt = (IRCEvent_JoinPart *)(event->customData);

	IRCLib_Channel channel = irclib_find_channel(
		python_plugin_data->info->irc->channelStorage,
		evt->channel
	);

	IRCLib_User user = irclib_find_user(
		python_plugin_data->info->irc->userStorage,
		evt->address->nick
	);

	PyObject *po_channel = pyplugin_channel_object(channel);
	PyObject *po_user = pyplugin_user_object(user);

	Py_INCREF(po_channel);
	Py_INCREF(po_user);

	PyObject *arglist = Py_BuildValue(
		"(OOs)",
		po_channel,
		po_user,
		evt->reason
	);
	pyplugin_callback(&(python_plugin_data->part), arglist);
	Py_DECREF(arglist);

	Py_DECREF(po_channel);
	Py_DECREF(po_user);
} // pyplugin_event_onpart

/**
 * onparted IRC event handler
 */
void pyplugin_event_onparted(EVENT *event) {
	IRCEvent_JoinPart *evt = (IRCEvent_JoinPart *)(event->customData);

	IRCLib_Channel channel = irclib_find_channel(
		python_plugin_data->info->irc->channelStorage,
		evt->channel
	);

	PyObject *po_channel = pyplugin_channel_object(channel);

	Py_INCREF(po_channel);

	PyObject *arglist = Py_BuildValue("(Os)", po_channel, evt->reason);
	pyplugin_callback(&(python_plugin_data->parted), arglist);
	Py_DECREF(arglist);

	Py_DECREF(po_channel);
} // pyplugin_event_onparted

/**
 * onchannelmessage IRC event handler
 */
void pyplugin_event_onchannelmessage(EVENT *event) {
	IRCEvent_Message *message = (IRCEvent_Message *)event->customData;

	IRCLib_Channel channel = irclib_find_channel(
		python_plugin_data->info->irc->channelStorage,
		message->channel
	);

	IRCLib_User user = irclib_find_user(
		python_plugin_data->info->irc->userStorage,
		message->address->nick
	);

	PyObject *po_channel = pyplugin_channel_object(channel);
	PyObject *po_user = pyplugin_user_object(user);

	Py_INCREF(po_channel);
	Py_INCREF(po_user);

	PyObject *arglist = Py_BuildValue(
		"(OOs)",
		po_channel,
		po_user,
		message->message
	);

	pyplugin_callback(
		&(python_plugin_data->channel_message),
		arglist
	);

	Py_DECREF(arglist);
	Py_DECREF(po_channel);
	Py_DECREF(po_user);
} // pyplugin_event_onchannelmessage

/**
 * onprivatemessage IRC event handler
 */
void pyplugin_event_onprivatemessage(EVENT *event) {
	IRCEvent_Message *message = (IRCEvent_Message *)event->customData;

	IRCLib_User user = irclib_find_user(
		python_plugin_data->info->irc->userStorage,
		message->address->nick
	);

	PyObject *po_user = pyplugin_user_object(user);

	Py_INCREF(po_user);

	PyObject *arglist = Py_BuildValue(
		"(Os)",
		po_user,
		message->message
	);

	pyplugin_callback(
		&(python_plugin_data->private_message),
		arglist
	);

	Py_DECREF(arglist);
	Py_DECREF(po_user);
} // pyplugin_event_onprivatemessage

/**
 * onchannelnotice IRC event handler
 */
void pyplugin_event_onchannelnotice(EVENT *event) {
	IRCEvent_Message *message = (IRCEvent_Message *)event->customData;

	IRCLib_Channel channel = irclib_find_channel(
		python_plugin_data->info->irc->channelStorage,
		message->channel
	);

	IRCLib_User user = irclib_find_user(
		python_plugin_data->info->irc->userStorage,
		message->address->nick
	);

	PyObject *po_channel = pyplugin_channel_object(channel);
	PyObject *po_user = pyplugin_user_object(user);

	Py_INCREF(po_channel);
	Py_INCREF(po_user);

	PyObject *arglist = Py_BuildValue(
		"(OOs)",
		po_channel,
		po_user,
		message->message
	);

	pyplugin_callback(
		&(python_plugin_data->channel_notice),
		arglist
	);

	Py_DECREF(arglist);
	Py_DECREF(po_channel);
	Py_DECREF(po_user);
} // pyplugin_event_onchannelnotice

/**
 * onprivatenotice IRC event handler
 */
void pyplugin_event_onprivatenotice(EVENT *event) {
	IRCEvent_Message *message = (IRCEvent_Message *)event->customData;

	IRCLib_User user = irclib_find_user(
		python_plugin_data->info->irc->userStorage,
		message->address->nick
	);

	PyObject *po_user = pyplugin_user_object(user);

	Py_INCREF(po_user);

	PyObject *arglist = Py_BuildValue(
		"(Os)",
		po_user,
		message->message
	);

	pyplugin_callback(
		&(python_plugin_data->private_notice),
		arglist
	);

	Py_DECREF(arglist);
	Py_DECREF(po_user);
} // pyplugin_event_onprivatenotice

/**
 * onchangeprefix IRC event handler
 */
void pyplugin_event_onchangeprefix(EVENT *event) {
	(void)event;
} // pyplugin_event_onchangeprefix

/**
 * onop IRC event handler
 */
void pyplugin_event_onop(EVENT *event) {
	(void)event;
} // pyplugin_event_onop

/**
 * ondeop IRC event handler
 */
void pyplugin_event_ondeop(EVENT *event) {
	(void)event;
} // pyplugin_event_ondeop

/**
 * onvoice IRC event handler
 */
void pyplugin_event_onvoice(EVENT *event) {
	(void)event;
} // pyplugin_event_onvoice

/**
 * ondevoice IRC event handler
 */
void pyplugin_event_ondevoice(EVENT *event) {
	(void)event;
} // pyplugin_event_ondevoice

/**
 * onhalfop IRC event handler
 */
void pyplugin_event_onhalfop(EVENT *event) {
	(void)event;
} // pyplugin_event_onhalfop

/**
 * ondehalfop IRC event handler
 */
void pyplugin_event_ondehalfop(EVENT *event) {
	(void)event;
} // pyplugin_event_ondehalfop

/**
 * onchangelist IRC event handler
 */
void pyplugin_event_onchangelist(EVENT *event) {
	(void)event;
} // pyplugin_event_onchangelist

/**
 * onban IRC event handler
 */
void pyplugin_event_onban(EVENT *event) {
	(void)event;
} // pyplugin_event_onban

/**
 * onunban IRC event handler
 */
void pyplugin_event_onunban(EVENT *event) {
	(void)event;
} // pyplugin_event_onunban

/**
 * onmode IRC event handler
 */
void pyplugin_event_onmode(EVENT *event) {
	(void)event;
} // pyplugin_event_onmode

/**
 * onkick IRC event handler
 */
void pyplugin_event_onkick(EVENT *event) {
	IRCEvent_Kick *message = (IRCEvent_Kick *)(event->customData);

	IRCLib_Channel channel = irclib_find_channel(
		python_plugin_data->info->irc->channelStorage,
		message->channel
	);

	IRCLib_User user = irclib_find_user(
		python_plugin_data->info->irc->userStorage,
		message->address->nick
	);

	IRCLib_User kicked = irclib_find_user(
		python_plugin_data->info->irc->userStorage,
		message->nick
	);

	PyObject *po_channel = pyplugin_channel_object(channel);
	PyObject *po_user = pyplugin_user_object(user);
	PyObject *po_kicked = pyplugin_user_object(kicked);

	Py_INCREF(po_channel);
	Py_INCREF(po_user);
	Py_INCREF(po_kicked);

	PyObject *arglist = Py_BuildValue(
		"(OOOs)",
		po_channel,
		po_user,
		po_kicked,
		message->reason
	);

	pyplugin_callback(
		&(python_plugin_data->kicked),
		arglist
	);

	// ToDo: Change user's belonging to channel in user's and channel object.

	Py_DECREF(po_channel);
	Py_DECREF(po_user);
	Py_DECREF(po_kicked);
	Py_DECREF(arglist);
} // pyplugin_event_onkick

/**
 * onkicked IRC event handler
 */
void pyplugin_event_onkicked(EVENT *event) {
	IRCEvent_Kick *message = (IRCEvent_Kick *)(event->customData);

	IRCLib_Channel channel = irclib_find_channel(
		python_plugin_data->info->irc->channelStorage,
		message->channel
	);

	IRCLib_User user = irclib_find_user(
		python_plugin_data->info->irc->userStorage,
		message->address->nick
	);

	PyObject *po_channel = pyplugin_channel_object(channel);
	PyObject *po_user = pyplugin_user_object(user);

	Py_INCREF(po_channel);
	Py_INCREF(po_user);

	PyObject *arglist = Py_BuildValue(
		"(OOs)",
		po_channel,
		po_user,
		message->reason
	);

	pyplugin_callback(
		&(python_plugin_data->kicked),
		arglist
	);

	// ToDo: Change my belonging to channel in user's and channel object.

	Py_DECREF(po_channel);
	Py_DECREF(po_user);
	Py_DECREF(arglist);
} // pyplugin_event_onkicked

/**
 * onnickchanged IRC event handler
 */
void pyplugin_event_onnickchanged(EVENT *event) {
	IRCEvent_NickChange *evt = (IRCEvent_NickChange *)(event->customData);

	PyObject *arglist = Py_BuildValue("(ss)", evt->address->nick, evt->newnick);
	Py_INCREF(arglist);
	pyplugin_callback(
		&(python_plugin_data->nick_changed),
		arglist
	);
	Py_DECREF(arglist);

	// ToDo: Change nick of me in user storage (if my object exists)

} // pyplugin_event_onnickchanged

/**
 * onnick IRC event handler
 */
void pyplugin_event_onnick(EVENT *event) {
	IRCEvent_NickChange *message = (IRCEvent_NickChange *)(event->customData);

	IRCLib_User user = irclib_find_user(
		python_plugin_data->info->irc->userStorage,
		message->address->nick
	);

	PyObject *po_user = pyplugin_user_object(user);
	Py_INCREF(po_user);

	PyObject *arglist = Py_BuildValue("(Os)", po_user, message->newnick);
	pyplugin_callback(
		&(python_plugin_data->nick_changed),
		arglist
	);
	Py_DECREF(arglist);

	// ToDo: Change nick of user in po_user object

	Py_DECREF(po_user);
} // pyplugin_event_onnick
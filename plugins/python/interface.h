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

#ifndef _PYTHON_INTERFACE
#define _PYTHON_INTERFACE 1

#include <Python.h>
#include "pythread.h"
#include <events.h>

#include <pluginapi.h>
#include <events.h>

#ifndef PLUGIN_NAME
# define PLUGIN_NAME "python"
#endif

typedef struct s_plugin_PyObject {
	PyObject_HEAD
	PyThreadState *tstate;				/**< Thread state
											 (working name 'python instance
											 identifier') */
	char *fileName;						/**< Filename of script */
	PyObject *ircbot_module;			/**< ircbot python module */
	struct s_plugin_PyObject *prev;		/**< Previous loaded script */
	struct s_plugin_PyObject *next;		/**< Next loaded script */
} plugin_PyObject;

/**
 * IRC user Python object
 */
typedef struct {
	PyObject_HEAD
	PyObject *nick;			/**< Nick of user */
	PyObject *user;			/**< User name */
	PyObject *hostname;		/**< Hostname */
	PyObject *channels;		/**< List of channels the user is on. */
	PyObject *dict;			/**< Dictionary with other properties set by
		scripts. */
} user_PyObject;

/**
 * IRC channel Python object
 */
typedef struct {
	PyObject_HEAD
	PyObject *name;			/**< Channel name */
	PyObject *users;		/**< List of users that are on the channel. */
	PyObject *dict;			/**< Dictionary with other properties set by
								 scripts. */
} channel_PyObject;

/**
 * Timer Python object
 */
typedef struct {
	PyObject_HEAD
	PyObject *timeout;		/**< Timeout value */
} timer_PyObject;

/**
 * Command Python object
 */
typedef struct {
	PyObject_HEAD
	PyObject *command;		/**< Command name */
} command_PyObject;

// Forward...
typedef struct s_pyplugin_event_cb *pyplugin_event_cb;

/**
 * Event callback
 */
struct s_pyplugin_event_cb {
	PyObject *callback;			/**< Callback function */
	plugin_PyObject *plugin;	/**< Plugin that owns callback */

	pyplugin_event_cb prev;		/**< Previous callback in chain */
	pyplugin_event_cb next;		/**< Next callback in chain */
}; // struct s_pyplugin_event_cb

/**
 * One event
 */
typedef struct {
	pyplugin_event_cb first;	/**< First callback in chain */
	pyplugin_event_cb last;		/**< Last callback in chain */
} pyplugin_event;

extern PyTypeObject plugin_PyTypeObject;	/**< Plugin python class type */

extern PyTypeObject user_PyTypeObject;		/**< User python class type */
extern PyMemberDef user_members[];			/**< Members of user class */
extern PyMethodDef user_methods[];			/**< Methods of user class */

extern PyTypeObject channel_PyTypeObject;	/**< Channel python class type */
extern PyMemberDef channel_members[];		/**< Members of channel  class */
extern PyMethodDef channel_methods[];		/**< Methods of channel class */

extern PyTypeObject timer_PyTypeObject;		/**< Timer python class type */

extern PyTypeObject command_PyTypeObject;	/**< Command python clas type */

extern PyMethodDef python_IrcBotMethods[];	/**< Methods of ircbot module */

/**
 * Python plugin data
 */
typedef struct {
	PluginInfo *info;			/**< Plugin info from plugin core */

	plugin_PyObject *first;		/**< First plugin */
	plugin_PyObject *last;		/**< Last plugin */

	// events
	pyplugin_event channel_message;
	pyplugin_event private_message;
	pyplugin_event channel_action;
	pyplugin_event private_action;
	pyplugin_event channel_notice;
	pyplugin_event private_notice;
	pyplugin_event join;
	pyplugin_event part;
	pyplugin_event quit;
	pyplugin_event kick;
	pyplugin_event mode;
	pyplugin_event raw;
	pyplugin_event nick;
	pyplugin_event nick_changed;
	pyplugin_event connected;
	pyplugin_event disconnected;
	pyplugin_event joined;
	pyplugin_event kicked;
	pyplugin_event parted;
	pyplugin_event exit;

	EVENT_HANDLER *onconnected;
	EVENT_HANDLER *ondisconnected;
	EVENT_HANDLER *onrawreceive;
	EVENT_HANDLER *onjoin;
	EVENT_HANDLER *onjoined;
	EVENT_HANDLER *onpart;
	EVENT_HANDLER *onparted;
	EVENT_HANDLER *onchannelmessage;
	EVENT_HANDLER *onprivatemessage;
	EVENT_HANDLER *onchannelnotice;
	EVENT_HANDLER *onprivatenotice;
	EVENT_HANDLER *onchangeprefix;
	EVENT_HANDLER *onop;
	EVENT_HANDLER *ondeop;
	EVENT_HANDLER *onvoice;
	EVENT_HANDLER *ondevoice;
	EVENT_HANDLER *onhalfop;
	EVENT_HANDLER *ondehalfop;
	EVENT_HANDLER *onchangelist;
	EVENT_HANDLER *onban;
	EVENT_HANDLER *onunban;
	EVENT_HANDLER *onmode;
	EVENT_HANDLER *onkick;
	EVENT_HANDLER *onkicked;
	EVENT_HANDLER *onnickchanged;
	EVENT_HANDLER *onnick;

	PyObject *channel_dict;			/**< Channel objects dictionary for
		persistent channel object storage */
	PyObject *user_dict;			/**< User objects dictionary for
		persistent user object storage */

	PyThread_type_lock lock;		/**< Thread lock for Python instance */
	PyThreadState *tstate;			/**< Main interpreter thread state */
	PyObject *interp_plugin;		/**< Dummy plugin (why is this needed?) */
} PythonPluginData;

extern PythonPluginData *python_plugin_data; /**< Python plugin data */

/**
 * Create new instance of Python plugin object
 * @param path Path to Python file
 */
extern PyObject *pyplugin_load(const char *path);

/**
 * Load all python scripts from specified directory
 */
extern void pyplugin_load_dir(char *dirname);

/**
 * Unload loaded python script
 * @param plugin Plugin to unload
 */
extern void pyplugin_unload(plugin_PyObject *plugin);

/**
 * Unload all loaded python scripts
 */
extern void pyplugin_unload_all();

/**
 * Get currently active Python plugin
 */
extern plugin_PyObject *pyplugin_getCurrent();

/**
 * Install plugin API to Python environment
 */
extern void pyplugin_install_api(plugin_PyObject *object);

/**
 * Get attribute of user object
 * @param obj User object
 * @param name Attribute name
 */
extern PyObject *pyplugin_user_getattro(PyObject *obj, PyObject *name);

/**
 * Set attribute of user object.
 * @param obj User object
 * @param name Attribute name
 * @param value Attribute value
 */
extern int pyplugin_user_setattro(PyObject *obj, PyObject *name, PyObject *value);

/**
 * Install class user to Python environment
 */
extern void pyplugin_install_user_class();

/**
 * Get attribute of channel object
 * @param obj Channel object
 * @param name Attribute name
 */
extern PyObject *pyplugin_channel_getattro(PyObject *obj, PyObject *name);

/**
 * Set attribute of channel object.
 * @param obj Channel object
 * @param name Attribute name
 * @param value Attribute value
 */
extern int pyplugin_channel_setattro(PyObject *obj, PyObject *name, PyObject *value);

/**
 * Install class channel to Python environment
 */
extern void pyplugin_install_channel_class();

/**
 * Free plugin Python object
 * @param self plugin PyObject
 */
extern void pyplugin_plugin_dealloc(plugin_PyObject *self);



/**
 * Hook callback to event
 * API function ircbot.hook_event(event_name, callback)
 */
extern PyObject *pyplugin_hook_event(PyObject *self, PyObject *args);

/**
 * Send raw message to IRC server
 * API function ircbot.raw_send(text)
 */
extern PyObject *pyplugin_raw_send(PyObject *self, PyObject *args);

/**
 * Construct python channel object from IRCLib channel
 * @param channel IRCLib channel
 * @return Python channel class instance
 */
extern PyObject *pyplugin_channel_object(IRCLib_Channel channel);

/**
 * Find channel acording to channel name
 * @return channel class instance
 */
extern PyObject *pyplugin_find_channel(PyObject *self, PyObject *args);

/**
 * Construct python user object from IRCLib user.
 * @param user IRCLib user structure
 * @return Python user class instance
 */
extern PyObject *pyplugin_user_object(IRCLib_User user);

/**
 * Find user acording to his nick.
 * API function ircbot.find_user(nick)
 * @return user class instance with found user or Py_None if user cannot
 *   be found.
 */
extern PyObject *pyplugin_find_user(PyObject *self, PyObject *args);

/**
 * User class constructor
 * @param type Object type
 * @param args Arguments passed to constructor
 * @param kwds Keyword arguments
 */
extern PyObject *pyplugin_user_new(PyTypeObject *type, PyObject *args,
	PyObject *kwds);

/**
 * Destructor that frees up memory for user class
 * @param self user_PyObject to be freed
 */
extern void pyplugin_user_dealloc(user_PyObject *self);

/**
 * Sends message to user - method ircbot.user.message(text)
 * @param self user class instance
 * @param args Arguments passed to method
 */
extern PyObject *pyplugin_user_message(PyObject *self, PyObject *args);

/**
 * Sends ACTION to user - method ircbot.user.action(self, text)
 * @param self user class instance
 * @param args Arguments passed to method
 */
extern PyObject *pyplugin_user_action(PyObject *self, PyObject *args);

/**
 * Sends NOTICE to user - method ircbot.user.notice(self, text)
 * @param self user class instance
 * @param args Arguments passed to method
 */
extern PyObject *pyplugin_user_notice(PyObject *self, PyObject *args);

/**
 * Channel class constructor
 * @param type Object type
 * @param args Arguments passed to constructor
 * @param kwds Keyword arguments
 */
extern PyObject *pyplugin_channel_new(PyTypeObject *type, PyObject *args,
	PyObject *kwds);

/**
 * Destructor that frees up memory for channel class
 * @param self user_PyObject to be freed
 */
extern void pyplugin_channel_dealloc(channel_PyObject *self);

/**
 * Send message to channel
 * API function ircbot.channel.message(text)
 */
extern PyObject *pyplugin_channel_message(PyObject *self, PyObject *args);

/**
 * Send ACTION to channel
 * API function ircbot.channel.action(text)
 */
extern PyObject *pyplugin_channel_action(PyObject *self, PyObject *args);

/**
 * Send notice to channel
 * API function ircbot.channel.notice(text)
 */
extern PyObject *pyplugin_channel_notice(PyObject *self, PyObject *args);

/**
 * Kicks nick from channel
 * API function ircbot.channel.kick(nick, reason)
 */
extern PyObject *pyplugin_channel_kick(PyObject *self, PyObject *args);

/**
 * Ban address from channel
 * API function ircbot.channel.ban(self, address)
 */
extern PyObject *pyplugin_channel_ban(PyObject *self, PyObject *args);

/**
 * Unban address from channel
 * API function ircbot.channel.ban(self, address)
 */
extern PyObject *pyplugin_channel_unban(PyObject *self, PyObject *args);

/**
 * Sends mode string to channel
 * API function ircbot.channel.ban(self, address)
 */
extern PyObject *pyplugin_channel_mode(PyObject *self, PyObject *args);

/**
 * Part the channel
 * API function ircbot.channel.part(self, nick, reason)
 */
extern PyObject *pyplugin_channel_part(PyObject *self, PyObject *args);

/**
 * Call callback functions for specified list of callbacks.
 * @param callbacks List of callbacks
 * @param arglist Arguments that will be passed to the Python function
 */
extern void pyplugin_callback(pyplugin_event *callbacks, PyObject *arglist);

/**
 * onconnected IRC event handler
 */
extern void pyplugin_event_onconnected(EVENT *event);

/**
 * ondisconnected IRC event handler
 */
extern void pyplugin_event_ondisconnected(EVENT *event);

/**
 * onrawreceive IRC event handler
 */
extern void pyplugin_event_onrawreceive(EVENT *event);


/**
 * onjoin IRC event handler
 */
extern void pyplugin_event_onjoin(EVENT *event);

/**
 * onjoined IRC event handler
 */
extern void pyplugin_event_onjoined(EVENT *event);

/**
 * onpart IRC event handler
 */
extern void pyplugin_event_onpart(EVENT *event);

/**
 * onparted IRC event handler
 */
extern void pyplugin_event_onparted(EVENT *event);

/**
 * onchannelmessage IRC event handler
 */
extern void pyplugin_event_onchannelmessage(EVENT *event);

/**
 * onprivatemessage IRC event handler
 */
extern void pyplugin_event_onprivatemessage(EVENT *event);

/**
 * onchannelnotice IRC event handler
 */
extern void pyplugin_event_onchannelnotice(EVENT *event);

/**
 * onprivatenotice IRC event handler
 */
extern void pyplugin_event_onprivatenotice(EVENT *event);

/**
 * onchangeprefix IRC event handler
 */
extern void pyplugin_event_onchangeprefix(EVENT *event);

/**
 * onop IRC event handler
 */
extern void pyplugin_event_onop(EVENT *event);

/**
 * ondeop IRC event handler
 */
extern void pyplugin_event_ondeop(EVENT *event);

/**
 * onvoice IRC event handler
 */
extern void pyplugin_event_onvoice(EVENT *event);

/**
 * ondevoice IRC event handler
 */
extern void pyplugin_event_ondevoice(EVENT *event);

/**
 * onhalfop IRC event handler
 */
extern void pyplugin_event_onhalfop(EVENT *event);

/**
 * ondehalfop IRC event handler
 */
extern void pyplugin_event_ondehalfop(EVENT *event);

/**
 * onchangelist IRC event handler
 */
extern void pyplugin_event_onchangelist(EVENT *event);

/**
 * onban IRC event handler
 */
extern void pyplugin_event_onban(EVENT *event);

/**
 * onunban IRC event handler
 */
extern void pyplugin_event_onunban(EVENT *event);

/**
 * onmode IRC event handler
 */
extern void pyplugin_event_onmode(EVENT *event);

/**
 * onkick IRC event handler
 */
extern void pyplugin_event_onkick(EVENT *event);

/**
 * onkicked IRC event handler
 */
extern void pyplugin_event_onkicked(EVENT *event);

/**
 * onnickchanged IRC event handler
 */
extern void pyplugin_event_onnickchanged(EVENT *event);

/**
 * onnick IRC event handler
 */
extern void pyplugin_event_onnick(EVENT *event);

#endif

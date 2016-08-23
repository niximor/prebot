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

#ifndef _EVENTS_H
#define _EVENTS_H 1

#include <stdbool.h>

// Forward declaration
typedef struct sEVENT EVENT;
typedef struct sEVENT_HANDLER EVENT_HANDLER;
typedef struct sEVENT_CHAIN EVENT_CHAIN;
typedef struct sEVENTS EVENTS;

/**
 * Event handler function prototype
 */
typedef void eventHandler(EVENT *event);

/**
 * One event handler in list at each event
 */
struct sEVENT_HANDLER {
	eventHandler *handler;		/**< Event handler */
	EVENT_CHAIN *event;			/**< Event that the handler belongs to. */
	void *customData;			/**< User data */
	EVENT_HANDLER *next;		/**< Next event handler in chain */
	EVENT_HANDLER *prev;		/**< Previous event handler in chain */
}; // sEVENT_HANDLER

/**
 * One element in list of events and it's handlers
 */
struct sEVENT_CHAIN {
	char *eventName;			/**< Name of event */
	EVENT_HANDLER *handler;		/**< First event handler */
	EVENT_CHAIN *next;			/**< Pointer to next event in chain. */
}; // sEVENT_CHAIN

/**
 * Events structure
 */
struct sEVENTS{
	EVENT_CHAIN *first;			/**< Pointer to first event
									 in chain. */
}; // sEVENTS

/**
 * Used to inform event handlers about event
 */
struct sEVENT {
	bool cancelBubble;	/**< If set true by event handler, no
							 other event handlers in chain will be
							 executed. */
	EVENT_CHAIN *event;	/**< Pointer to event that is handled. */
	void *customData;	/**< Pointer to user data where application
							 can pass data to handler */
	void *handlerData;	/**< Pointer to handler data that were passed
							 on handler registration */
}; // sEVENT

/**
 * Init events
 * @return Pointer to events structure that you should use for another
 *   functions.
 */
extern EVENTS *events_init();

/**
 * Free events structure
 * @param events Structure initialized by events_init.
 */
extern void events_free(EVENTS *events);

/**
 * Finds event in chain and return pointer to it if it exists, or null
 * if event of that name doesn't exists.
 * @param events Events structure
 * @param name Name of event
 * @return Pointer to event chain or null.
 */
extern EVENT_CHAIN *events_seekEvent(EVENTS *events, char *name);

/**
 * Add new event. If event of that name already exists, it will return
 * pointer to that existing event, otherwise it will return pointer to newly
 * created event.
 * @param events Events structure
 * @param name Name of event
 * @return Pointer to created event or NULL if memory allocation error has
 *   occured.
 */
extern EVENT_CHAIN *events_addEvent(EVENTS *events, char *name);

/**
 * Add new handler to event.
 * @param events Events structure.
 * @param name The name of event which the handler handles.
 * @param handler Pointer to handler function
 * @param customData Custom data that the handler can use.
 */
extern EVENT_HANDLER *events_addEventListener(EVENTS *events, char *name,
	eventHandler *handler, void *customData);

/**
 * Remove event handler from chain
 * @param handler Pointer to event handler that should be removed.
 */
extern void events_removeEventListener(EVENT_HANDLER *handler);

/**
 * Fire event.
 * @param events Events structure
 * @param name Name of event
 * @param data User data to pass for event
 * @return Returns false if event chain was cancelled and true if all handlers
 *   in chain were called.
 */
extern bool events_fireEvent(EVENTS *events, char *name, void *data);

#endif

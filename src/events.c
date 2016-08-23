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

// Standard includes
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// This library interface
#include "events.h"

// Other my includes
#include "io.h"

/**
 * Init events
 * @return Pointer to events structure that you should use for another
 *   functions.
 */
EVENTS *events_init() {
	EVENTS *result = malloc(sizeof(EVENTS));

	result->first = NULL;

	return result;
} // events_init

/**
 * Free events structure
 * @param events Structure initialized by events_init.
 */
void events_free(EVENTS *events) {
	// Free events
	EVENT_CHAIN *node = events->first, *next;
	while (node != NULL) {
		next = node->next;

		// Free node name
		free(node->eventName);

		// Free handlers
		EVENT_HANDLER *handler = node->handler;
		EVENT_HANDLER *nextHandler;
		while (handler != NULL) {
			nextHandler = handler->next;
			free(handler);
			handler = nextHandler;
		}

		// Free node itself
		free(node);

		node = next;
	}

	// Free the events structure itself
	free(events);
} // events_free

/**
 * Finds event in chain and return pointer to it if it exists, or null
 * if event of that name doesn't exists.
 * @param events Events structure
 * @param name Name of event
 * @return Pointer to event chain or null.
 */
EVENT_CHAIN *events_seekEvent(EVENTS *events, char *name) {
	EVENT_CHAIN *node = events->first;
	while (node != NULL) {
		// Event was found
		if (strcmp(node->eventName, name) == 0) {
			return node;
		}

		node = node->next;
	}

	// If we didn't find the event.
	return NULL;
} // events_seekEvent

/**
 * Add new event. If event of that name already exists, it will return
 * pointer to that existing event, otherwise it will return pointer to newly
 * created event.
 * @param events Events structure
 * @param name Name of event
 * @return Pointer to created event or NULL if memory allocation error has
 *   occured.
 */
EVENT_CHAIN *events_addEvent(EVENTS *events, char *name) {
	// Test if event already exists.
	EVENT_CHAIN *node = events_seekEvent(events, name);
	if (node != NULL) return node;

	// Event doesn't exists, create new one at the begining of the list.
	node = malloc(sizeof(EVENT_CHAIN));
	if (node != NULL) {
		node->eventName = strdup(name);

		// No handlers are assigned with event by default.
		node->handler = NULL;

		// Insert at the begining of the list.
		node->next = events->first;
		events->first = node;
	}

	return node;
} // events_addEvent

/**
 * Add new handler to event.
 * @param events Events structure.
 * @param name The name of event which the handler handles.
 * @param handler Pointer to handler function
 * @param customData Pointer to custom data that handler can use.
 */
EVENT_HANDLER *events_addEventListener(EVENTS *events, char *name,
	eventHandler *handler, void *customData) {

	EVENT_CHAIN *node = events_seekEvent(events, name);
	if (node != NULL) {
		// Install new event handler.
		EVENT_HANDLER *newHandler = malloc(sizeof(EVENT_HANDLER));

		if (newHandler != NULL) {
			newHandler->next = node->handler;
			newHandler->prev = NULL;
			if (newHandler->next != NULL) {
				newHandler->next->prev = newHandler;
			}

			newHandler->handler = handler;
			newHandler->customData = customData;
			newHandler->event = node;
			node->handler = newHandler;
		}
		return newHandler;
	} else {
		// If event doesn't exists, create it. This makes possible
		// to create events inside plugins and then use them within
		// these plugins.
		events_addEvent(events, name);
		return events_addEventListener(events, name, handler, customData);
	}
} // events_addEventListener

/**
 * Remove event handler from chain
 * @param handler Pointer to event handler that should be removed.
 */
void events_removeEventListener(EVENT_HANDLER *handler) {
	if (handler->prev != NULL) {
		// Not at the begining of chain
		handler->prev->next = handler->next;
	} else {
		// At the begining of chain
		handler->event->handler = handler->next;
	}

	// Not at the end of chain
	if (handler->next != NULL) {
		handler->next->prev = handler->prev;
	}

	free(handler);
} // events_removeEventListener

/**
 * Fire event.
 * @param events Events structure
 * @param name Name of event
 * @param data User data to pass for event
 * @return Returns false if event chain was cancelled and true if all handlers
 *   in chain were called.
 */
bool events_fireEvent(EVENTS *events, char *name, void *data) {
	EVENT_CHAIN *node = events_seekEvent(events, name);
	if (node != NULL) {
		EVENT *event = malloc(sizeof(EVENT));
		if (event != NULL) {
			event->cancelBubble = false;
			event->event = node;
			event->customData = data;

			EVENT_HANDLER *handler = node->handler;
			while (handler != NULL && !event->cancelBubble) {
				event->handlerData = handler->customData;
				handler->handler(event);
				handler = handler->next;
			}

			// If event was cancelled, return false, else return true.
			int ret = !event->cancelBubble;

			// Free event structure
			free(event);

			return ret;
		} else {
			// Memory allocation error, event wasn't cancelled.
			return true;
		}
	} else {
		printError("events", "Trying to fire non existing event %s",
			name);
		// Event wasn't cancelled.
		return true;
	}
} // events_fireEvent

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

#ifndef _LINKEDLIST_H
# define _LINKEDLIT_H 1

#include <stdlib.h>

/**
 * Init linked list
 * @param s Pointer to linked list
 */
#define ll_init(s)                                                            \
	s->first = NULL;                                                          \
	s->last = NULL                                                            \

/**
 * Init linked list (version for lists passed directly without pointer)
 * @param s Linked list
 */
#define ll_inits(s)                                                           \
	s.first = NULL;                                                           \
	s.last = NULL                                                             \

/**
 * Append item to the end of linked list
 * @param s Pointer to linked list
 * @param i Item to append
 */
#define ll_append(s, i)                                                       \
	i->prev = s->last;                                                        \
	i->next = NULL;                                                           \
	s->last = i;                                                              \
	if (i->prev != NULL) {                                                    \
		i->prev->next = i;                                                    \
	} else {                                                                  \
		s->first = i;                                                         \
	}

/**
 * Append item to the end of linked list (version for lists passed directly
 * without pointer)
 * @param s Linked list
 * @param i Item to append
 */
#define ll_appends(s, i)                                                      \
	i->prev = s.last;                                                         \
	i->next = NULL;                                                           \
	s->last = i;                                                              \
	if (i->prev != NULL) {                                                    \
		i->prev->next = i;                                                    \
	} else {                                                                  \
		s.first = i;                                                          \
	}

/**
 * Remove item from linked list
 * @param s Pointer to linked list
 * @param i Item to be removed from linked list
 */
#define ll_remove(s, i)                                                       \
	if (i->prev != NULL) {                                                    \
		i->prev->next = i->next;                                              \
	} else {                                                                  \
		s->first = i->next;                                                   \
	}                                                                         \
	if (i->next != NULL) {                                                    \
		i->next->prev = i->prev;                                              \
	} else {                                                                  \
		s->last = i->prev;                                                    \
	}

/**
 * Remove item from linked list (version for lists passed directly without
 * pointer).
 * @param s Linked list
 * @param i Item to be removed from s.
 */
#define ll_removes(s, i)                                                      \
	if (i->prev != NULL) {                                                    \
		i->prev->next = i->next;                                              \
	} else {                                                                  \
		s.first = i->next;                                                    \
	}                                                                         \
	if (i->next != NULL) {                                                    \
		i->next->prev = i->prev;                                              \
	} else {                                                                  \
		s.last = i->prev;                                                     \
	}

/**
 * Loop through linked list
 * Usage:
 *   my_linked_list *list;
 *   ll_loop(list, curitem) {
 *      printf("%s\n", curitem->text);
 *   }
 * @param s Pointer to linked list
 * @param item Variable name where current item of the list will be stored.
 */
#define ll_loop(s, item)                                                      \
	typeof(s->first) item = NULL;                                             \
	typeof(s->first) __ll_next_##item = s->first;                             \
	while ((item = __ll_next_##item) != NULL &&                               \
		((__ll_next_##item = item->next) != NULL ||                           \
		(__ll_next_##item = item->next) == NULL))

/**
 * Loop through linked list (version for lists passed directly without pointer)
 * Usage:
 *   my_linked_list list;
 *   ll_loop(list, curitem) {
 *      printf("%s\n", curitem->text);
 *   }
 * @param s Linked list
 * @param item Variable name where current item of the list will be stored.
 */
#define ll_loops(s, item)                                                     \
	typeof(s.first) item = NULL;                                              \
	typeof(s.first) __ll_next_##item = s.first;                               \
	while ((item = __ll_next_##item) != NULL &&                               \
		((__ll_next_##item = item->next) != NULL ||                           \
		(__ll_next_##item = item->next) == NULL))

#endif

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

#ifndef _KEYVALPAIR_H
# define _KEYVALPAIR_H 1

// Standard libraries
#include <stdbool.h>

// My libraries
#include "htval.h"

// Forward
typedef struct sKVPArray *KVPArray;
typedef struct sKeyValPair *KeyValPair;

/**
 * Key-Value pair array
 */
struct sKVPArray {
	KeyValPair first;
	KeyValPair last;
}; // sKVPArray

/**
 * Key-Value pair
 */
struct sKeyValPair {
	char *key;				/**< Key */
	HTVAL value;			/**< Value */

	KeyValPair prev;		/**< Previous pair in Key-Value pair array */
	KeyValPair next;		/**< Next pair in Key-Value pair array */
}; // sKeyValPair

/**
 * Init Key-Value pair array
 * @return Key-Value pair array
 */
extern KVPArray kvp_init();

/**
 * Free Key-Value pair array
 * @param kvp Initialized Key-Value pair array
 */
extern void kvp_free(KVPArray kvp);

/**
 * Set key in Key-Value pair array to value
 * @param kvp Key-Value pair array
 * @param key Key to be set
 * @param value Value that the key should have
 */
extern void kvp_set(KVPArray kvp, char *key, HTVAL value);

/**
 * Unset Key-Value pari array key
 * @param kvp Key-Value pair array
 * @param key Key to be unset
 */
extern void kvp_unset(KVPArray kvp, char *key);

/**
 * Get whether key exists in Key-Value pair array
 * @param kvp Key-Value pair array
 * @param key Key
 * @return True if key exists in Key-Value pair array, false otherwise.
 */
extern bool kvp_isset(KVPArray kvp, char *key);

/**
 * Return value of key in Key-Value pair array
 * @param kvp Key-Value pair array
 * @param key Key to get value of
 * @return Value of key or NULL if key doesn't exists.
 */
extern HTVAL kvp_get(KVPArray kvp, char *key);

#endif

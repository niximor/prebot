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

// Standard libraries
#include <stdlib.h>
#include <string.h>

// This library interface
#include "keyvalpair.h"

// My libraries
#include <toolbox/linkedlist.h>
#include <toolbox/tb_string.h>

/**
 * Init Key-Value pair array
 * @return Key-Value pair array
 */
KVPArray kvp_init() {
	KVPArray result = malloc(sizeof(struct sKVPArray));

	ll_init(result);

	return result;
} // kvp_init

/**
 * Free Key-Value pair array
 * @param kvp Initialized Key-Value pair array
 */
void kvp_free(KVPArray kvp) {
	ll_loop(kvp, kv) {
		htval_free(kv->value);
		free(kv->key);
		free(kv);
	}

	free(kvp);
} // kvp_free

/**
 * Locate Key-Value pair in Key-Val pair array
 * @param kvp Key-Value pair array
 * @param key Key to be located
 * @return Key-Value pair of NULL if the key doesn't exists in array.
 */
KeyValPair kvp_locate(KVPArray kvp, char *key) {
	ll_loop(kvp, kv) {
		if (eq(kv->key, key)) {
			return kv;
		}
	}
	return NULL;
} // kvp_locate

/**
 * Set key in Key-Value pair array to value
 * @param kvp Key-Value pair array
 * @param key Key to be set
 * @param value Value that the key should have
 */
void kvp_set(KVPArray kvp, char *key, HTVAL value) {
	KeyValPair kv = kvp_locate(kvp, key);
	if (kv == NULL) {
		kv = malloc(sizeof(struct sKeyValPair));
		kv->key = strdup(key);
		ll_append(kvp, kv);
	} else {
		htval_free(kv->value);
	}

	kv->value = value;
} // kvp_set

/**
 * Unset Key-Value pari array key
 * @param kvp Key-Value pair array
 * @param key Key to be unset
 */
void kvp_unset(KVPArray kvp, char *key) {
	KeyValPair kv = kvp_locate(kvp, key);
	if (kv != NULL) {
		ll_remove(kvp, kv);
		free(kv->key);
		htval_free(kv->value);
		free(kv);
	}
} // kvp_unset

/**
 * Get whether key exists in Key-Value pair array
 * @param kvp Key-Value pair array
 * @param key Key
 * @return True if key exists in Key-Value pair array, false otherwise.
 */
bool kvp_isset(KVPArray kvp, char *key) {
	return kvp_locate(kvp, key) != NULL;
} // kvp_isset

/**
 * Return value of key in Key-Value pair array
 * @param kvp Key-Value pair array
 * @param key Key to get value of
 * @return Value of key or NULL if key doesn't exists.
 */
HTVAL kvp_get(KVPArray kvp, char *key) {
	KeyValPair kv = kvp_locate(kvp, key);
	if (kv != NULL) {
		return kv->value;
	} else {
		return NULL;
	}
} // kvp_get

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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

// Standard libraries
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// This library interface
#include "htval.h"

/**
 * Create new HTVAL or set type of existing HTVAL to NULL
 * @param existing Existing HTVAL to modify. If NULL, new htval will be
 *   created.
 */
HTVAL htval_nulle(HTVAL existing) {
	if (existing == NULL) {
		existing = malloc(sizeof(struct sHTVAL));
	} else {
		htval_free_value(existing);
	}
	existing->type = HT_NULL;

	return existing;
} // htval_nulle

/**
 * Create new HTVAL or set type of existing HTVAL to integer
 * @param value New value of HTVAL
 * @param existing Existing HTVAL to modify. If NULL, new htval will be
 *   created.
 */
HTVAL htval_inte(long int value, HTVAL existing) {
	existing = htval_nulle(existing);
	existing->type = HT_INT;
	existing->value.ival = value;
	return existing;
} // htval_inte

/**
 * Create new HTVAL or set type of existing HTVAL to float
 * @param value New value of HTVAL
 * @param existing Existing HTVAL to modify. If NULL, new htval will be
 *   created.
 */
HTVAL htval_floate(double value, HTVAL existing) {
	existing = htval_nulle(existing);
	existing->type = HT_FLOAT;
	existing->value.fval = value;
	return existing;
} // htval_floate

/**
 * Create new HTVAL or set type of existing HTVAL to string
 * @param value New value of HTVAL
 * @param existing Existing HTVAL to modify. If NULL, new htval will be
 *   created.
 */
HTVAL htval_stringe(char *value, HTVAL existing) {
	existing = htval_nulle(existing);
	existing->type = HT_STRING;
	existing->value.sval = strdup(value);
	return existing;
} // htval_stringe

/**
 * Create new HTVAL or set type of existing HTVAL to array
 * @param value New value of HTVAL - array must be created before setting it
 *   into htval.
 * @param existing Existing HTVAL to modify. If NULL, new htval will be
 *   created.
 */
HTVAL htval_arraye(HTVAL_Array value, HTVAL existing) {
	existing = htval_nulle(existing);
	existing->type = HT_ARRAY;
	existing->value.aval = value;
	return existing;
} // htval_arraye

/**
 * Free value of HTVAL. Sets HTVAL type to NULL.
 * @param value HTVAL which value should be freed.
 */
void htval_free_value(HTVAL value) {
	switch (value->type) {
		case HT_INT:
		case HT_FLOAT:
		case HT_NULL:
			// No action, don't need to be freed.
			break;

		case HT_STRING:
			free(value->value.sval);
			break;

		case HT_ARRAY:
			for (size_t i = 0; i < value->value.aval.length; i++) {
				htval_free(value->value.aval.values[i]);
			}
			free(value->value.aval.values);
			break;
	}

	value->type = HT_NULL;
} // htval_freevalue

/**
 * Free existing htval
 * @param value HTVAL to free
 */
void htval_free(HTVAL value) {
	htval_free_value(value);
	free(value);
} // htval_free

/**
 * Set HTVAL type to integer. Do conversion of value, if possible, if existing
 * value cannot be converted, sets value to 0.
 * @return Integer value of HTVAL
 */
long int htval_get_int(HTVAL value) {
	if (value == NULL) return 0;

	switch (value->type) {
		case HT_INT: {
			// No action.
			break;
		}

		case HT_FLOAT: {
			long int newval = (long int)value->value.fval;
			htval_free_value(value);
			value->type = HT_INT;
			value->value.ival = newval;
			break;
		}

		case HT_STRING: {
			long int newval = 0;
			if (sscanf(value->value.sval, "%ld", &newval) <= 0) {
				newval = atol(value->value.sval);
			}
			htval_free_value(value);
			value->type = HT_INT;
			value->value.ival = newval;
			break;
		}

		case HT_ARRAY: {
			htval_free_value(value);
			value->type = HT_INT;
			value->value.ival = 0;
			break;
		}

		case HT_NULL: {
			value->type = HT_INT;
			value->value.ival = 0;
			break;
		}
	}

	return value->value.ival;
} // htval_get_int

/**
 * Set HTVAL type to float. Do conversion of value, if possible, if existing
 * value cannot be converted, sets value to NAN
 * @return Float value of HTVAL
 */
double htval_get_float(HTVAL value) {
	if (value == NULL) return 0;

	switch (value->type) {
		case HT_INT: {
			double newval = (double)value->value.ival;
			htval_free_value(value);
			value->type = HT_FLOAT;
			value->value.fval = newval;
			break;
		}

		case HT_FLOAT: {
			// No action.
			break;
		}

		case HT_STRING: {
			double newval = NAN;
			if (sscanf(value->value.sval, "%lf", &newval) <= 0) {
				newval = atof(value->value.sval);
			}
			htval_free_value(value);
			value->type = HT_FLOAT;
			value->value.fval = newval;
			break;
		}

		case HT_ARRAY: {
			htval_free_value(value);
			value->type = HT_FLOAT;
			value->value.fval = NAN;
			break;
		}

		case HT_NULL: {
			value->type = HT_FLOAT;
			value->value.fval = NAN;
		}
	}

	return value->value.fval;
} // htval_get_double

/**
 * Set HTVAL type to string. Do conversion of existing value.
 * @return String value of HTVAL
 */
char *htval_get_string(HTVAL value) {
	if (value == NULL) return NULL;

	switch (value->type) {
		case HT_INT: {
			char *str;
			asprintf(&str, "%ld", value->value.ival);
			htval_free_value(value);
			value->type = HT_STRING;
			value->value.sval = str;
			break;
		}

		case HT_FLOAT: {
			char *str;
			asprintf(&str, "%lf", value->value.fval);
			htval_free_value(value);
			value->type = HT_STRING;
			value->value.sval = str;
			break;
		}

		case HT_STRING: {
			// No action.
			break;
		}

		case HT_ARRAY: {
			htval_free_value(value);
			value->type = HT_STRING;
			value->value.sval = strdup("Array");
			break;
		}

		case HT_NULL: {
			value->type = HT_STRING;
			value->value.sval = strdup("");
			break;
		}
	}

	return value->value.sval;
} // htval_get_string

/**
 * Set HTVAL type to array. Existing value is discarded, if HTVAL is not
 * already an array.
 * @return Array value of HTVAL.
 */
HTVAL_Array htval_get_array(HTVAL value) {
	switch (value->type) {
		case HT_INT:
		case HT_FLOAT:
		case HT_STRING:
		case HT_NULL: {
			// Create empty array
			htval_free_value(value);
			value->type = HT_ARRAY;
			value->value.aval.length = 0;
			value->value.aval.values = NULL;
			break;
		}

		case HT_ARRAY: {
			// No action.
			break;
		}
	}

	return value->value.aval;
} // htval_get_array

/**
 * Get item from HTVAL of array type
 * @param value HTVAL
 * @param index Index in array
 * @return HTVAL that is on specified index in array, or NULL if index
 *   is out of array boundaries.
 */
HTVAL htval_get_index(HTVAL value, size_t index) {
	HTVAL_Array array = htval_get_array(value);
	if (array.length <= index) {
		return array.values[index];
	} else {
		return NULL;
	}
} // htval_get_index

/**
 * Set value in HTVAL of array type
 * @param value HTVAL of array type
 * @param index Index where new htval will be set
 * @param newval New HTVAL that will be set to specified index in array
 */
void htval_set_index(HTVAL value, size_t index, HTVAL newval) {
	HTVAL_Array array = htval_get_array(value);
	if (array.length <= index) {
		if (array.length == 0) {
			array.length = index + 1;
			array.values = malloc(array.length * sizeof(HTVAL));
		} else {
			size_t oldlength = array.length;
			array.length = index + 1;
			array.values = realloc(array.values, array.length * sizeof(HTVAL));
			for (size_t i = oldlength; i < array.length; i++) {
				array.values[i] = NULL;
			}
		}
	}
	array.values[index] = newval;
} // htval_set_index

/**
 * Add new value into HTVAL of array type
 * @param value HTVAL of array type
 * @param newval New value that will be inserted at the end of array
 */
void htval_append(HTVAL value, HTVAL newval) {
	HTVAL_Array array = htval_get_array(value);
	htval_set_index(value, array.length, newval);
} // htval_append

/**
 * Insert value into array. All values with indexes bigger than inserted one
 * will be moved, so no item is lost.
 * @param value HTVAL of array type
 * @param index Index where item should be inserted
 * @param newval New HTVAL that will be inserted at specified index
 */
void htval_insert(HTVAL value, size_t index, HTVAL newval) {
	HTVAL_Array array = htval_get_array(value);
	for (size_t i = array.length; i > index; i--) {
		htval_set_index(value, i, htval_get_index(value, i-1));
	}
	htval_set_index(value, index, newval);
} // htval_insert

/**
 * Remove item from array, move all indexes to ensure array is compact.
 * @param value HTVAL of array type
 * @param index Index to remove
 */
void htval_remove(HTVAL value, size_t index) {
	HTVAL_Array array = htval_get_array(value);
	for (size_t i = index; i < array.length - 1; i++) {
		htval_set_index(value, i, htval_get_index(value, i+1));
	}
	htval_free(array.values[array.length - 1]);
	array.length--;
} // htval_remove

/**
 * Get HTVAL value type
 * @param value HTVAL.
 */
HTVAL_Type htval_type(HTVAL value) {
	return value->type;
} // htval_type


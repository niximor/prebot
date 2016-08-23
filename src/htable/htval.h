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

#ifndef _HTVAL_H
# define _HTVAL_H 1

// Forward
typedef struct sHTVAL *HTVAL;

typedef enum {
	HT_NULL,					/**< NULL value */
	HT_INT,						/**< Integer (long int) */
	HT_FLOAT,					/**< Float (double) */
	HT_STRING,					/**< String (char *) */
	HT_ARRAY					/**< Array (HTVAL_Array) */
} HTVAL_Type;

/**
 * HTVAL Array type
 */
typedef struct {
	size_t length;				/**< Number of items in array */
	HTVAL *values;				/**< Values of array */
} HTVAL_Array;

/**
 * HTVAL
 */
struct sHTVAL {
	HTVAL_Type type;			/**< Value data type */
	union {
		long int ival;			/**< Integer value */
		double fval;			/**< Float value */
		char *sval;				/**< String value */
		HTVAL_Array aval;		/**< Array value */
	} value;					/**< Value union */
}; // sHTVAL

/**
 * Create new HTVAL of NULL type
 */
#define htval_null() htval_nulle(NULL)

/**
 * Create new HTVAL of int type
 * @param value Value of htval
 */
#define htval_int(value) htval_inte(value, NULL)

/**
 * Create new HTVAL of float type
 * @param value Value of htval
 */
#define htval_float(value) htval_floate(value, NULL)

/**
 * Create new HTVAL of string type
 * @param value Value of htval
 */
#define htval_string(value) htval_stringe(value, NULL)

/**
 * Create new HTVAL with array type
 * @param value Existing array of htvals
 */
#define htval_array(value) htval_arraye(value, NULL)

/**
 * Create new HTVAL or set type of existing HTVAL to NULL
 * @param existing Existing HTVAL to modify. If NULL, new htval will be
 *   created.
 */
extern HTVAL htval_nulle(HTVAL existing);

/**
 * Create new HTVAL or set type of existing HTVAL to integer
 * @param value New value of HTVAL
 * @param existing Existing HTVAL to modify. If NULL, new htval will be
 *   created.
 */
extern HTVAL htval_inte(long int value, HTVAL existing);

/**
 * Create new HTVAL or set type of existing HTVAL to float
 * @param value New value of HTVAL
 * @param existing Existing HTVAL to modify. If NULL, new htval will be
 *   created.
 */
extern HTVAL htval_floate(double value, HTVAL existing);

/**
 * Create new HTVAL or set type of existing HTVAL to string
 * @param value New value of HTVAL
 * @param existing Existing HTVAL to modify. If NULL, new htval will be
 *   created.
 */
extern HTVAL htval_stringe(char *value, HTVAL existing);

/**
 * Create new HTVAL or set type of existing HTVAL to array
 * @param value New value of HTVAL - array must be created before setting it
 *   into htval.
 * @param existing Existing HTVAL to modify. If NULL, new htval will be
 *   created.
 */
extern HTVAL htval_arraye(HTVAL_Array value, HTVAL existing);

/**
 * Free value of HTVAL. Sets HTVAL type to NULL.
 * @param value HTVAL which value should be freed.
 */
extern void htval_free_value(HTVAL value);

/**
 * Free existing htval
 * @param value HTVAL to free
 */
extern void htval_free(HTVAL value);

/**
 * Set HTVAL type to integer. Do conversion of value, if possible, if existing
 * value cannot be converted, sets value to 0.
 * @return Integer value of HTVAL
 */
extern long int htval_get_int(HTVAL value);

/**
 * Set HTVAL type to float. Do conversion of value, if possible, if existing
 * value cannot be converted, sets value to NaN
 * @return Float value of HTVAL
 */
extern double htval_get_float(HTVAL value);

/**
 * Set HTVAL type to string. Do conversion of existing value.
 * @return String value of HTVAL
 */
extern char *htval_get_string(HTVAL value);

/**
 * Set HTVAL type to array. Existing value is discarded, if HTVAL is not
 * already an array.
 * @return Array value of HTVAL.
 */
extern HTVAL_Array htval_get_array(HTVAL value);

/**
 * Get item from HTVAL of array type
 * @param value HTVAL
 * @param index Index in array
 * @return HTVAL that is on specified index in array, or NULL if index
 *   is out of array boundaries.
 */
extern HTVAL htval_get_index(HTVAL value, size_t index);

/**
 * Set value in HTVAL of array type
 * @param value HTVAL of array type
 * @param index Index where new htval will be set
 * @param newval New HTVAL that will be set to specified index in array
 */
extern void htval_set_index(HTVAL value, size_t index, HTVAL newval);

/**
 * Add new value into HTVAL of array type
 * @param value HTVAL of array type
 * @param newval New value that will be inserted at the end of array
 */
extern void htval_append(HTVAL value, HTVAL newval);

/**
 * Insert value into array. All values with indexes bigger than inserted one
 * will be moved, so no item is lost.
 * @param value HTVAL of array type
 * @param index Index where item should be inserted
 * @param newval New HTVAL that will be inserted at specified index
 */
extern void htval_insert(HTVAL value, size_t index, HTVAL newval);

/**
 * Remove item from array, move all indexes to ensure array is compact.
 * @param value HTVAL of array type
 * @param index Index to remove
 */
extern void htval_remove(HTVAL value, size_t index);

/**
 * Get HTVAL value type
 * @param value HTVAL.
 */
extern HTVAL_Type htval_type(HTVAL value);

#endif

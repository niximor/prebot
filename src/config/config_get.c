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

// This library interface
#include "config.h"

/**
 * Parse value of item and return it as integer
 * @param value CONF_VALUE pointer
 * @return Value of item as integer
 */
long int config_parsevalue_int(CONF_VALUE *value) {
	if (value == NULL) {
		return 0;
	}

	return htval_get_int(value->value);
} // config_parsevalue_int

/**
 * Parse value of item and return it as float
 * @param value CONF_VALUE pointer
 * @return Value of item as float
 */
double config_parsevalue_float(CONF_VALUE *value) {
	if (value == NULL) {
		return 0;
	}

	return htval_get_float(value->value);
} // config_parsevalue_float

/**
 * Parse value of item and return it as string
 * @param value CONF_VALUE pointer
 * @return Value of item as string
 */
char *config_parsevalue_string(CONF_VALUE *value) {
	if (value == NULL) {
		return NULL;
	}

	return htval_get_string(value->value);
} // config_parsevalue_string

/**
 * Parse value of item and return it as bool
 * @param value CONF_VALUE pointer
 * @return Value of item as bool
 */
bool config_parsevalue_bool(CONF_VALUE *value) {
	if (value == NULL) {
		return false;
	}

	return htval_get_int(value->value) != 0;
} // config_parsevalue_bool

/**
 * Get value of configuration item as integer
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as integer. If value not exists, value of parameter
 *   notexists is returned.
 */
long int config_getvalue_int(CONF_SECTION *config, char *path,
	long int notexists) {

	CONF_VALUE *value = config_lookup(config, path, false);
	if (value == NULL) {
		return notexists;
	}

	return config_parsevalue_int(value);
} // config_getvalue_int

/**
 * Get value of configuration item as float
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as float. If value not exists, value of parameter
 *   notexists is returned.
 */
double config_getvalue_float(CONF_SECTION *config, char *path,
	double notexists) {

	CONF_VALUE *value = config_lookup(config, path, false);
	if (value == NULL) {
		return notexists;
	}

	return config_parsevalue_float(value);
} // config_getvalue_float

/**
 * Get value of configuration item as string
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as string. If value not exists, value of parameter
 *   notexists is returned.
 */
char *config_getvalue_string(CONF_SECTION *config, char *path,
	char *notexists) {

	CONF_VALUE *value = config_lookup(config, path, false);
	if (value == NULL) {
		return notexists;
	}

	return config_parsevalue_string(value);
} // config_getvalue_string

/**
 * Get value of configuration item as bool
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as bool (1 for true, 0 for false). If value not
 *   exists, value of parameter notexists is returned.
 */
bool config_getvalue_bool(CONF_SECTION *config, char *path, bool notexists) {
	CONF_VALUE *value = config_lookup(config, path, false);
	if (value == NULL) {
		return notexists;
	}

	return config_parsevalue_bool(value);
} // config_getvalue_bool

/**
 * Get value of configuration item as integer
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param num Number of item if multiple items of same name exists.
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as integer. If value not exists, value of parameter
 *   notexists is returned.
 */
long int config_getvalue_array_int(CONF_SECTION *config, char *path, int num,
	long int notexists) {

	CONF_VALUE *value = config_lookup_array(config, path, num, false);
	if (value == NULL) {
		return notexists;
	}

	return config_parsevalue_int(value);
} // config_getvalue_int

/**
 * Get value of configuration item as float
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param num Number of item if multiple items of same name exists.
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as float. If value not exists, value of parameter
 *   notexists is returned.
 */
double config_getvalue_array_float(CONF_SECTION *config, char *path, int num,
	double notexists) {

	CONF_VALUE *value = config_lookup_array(config, path, num, false);
	if (value == NULL) {
		return notexists;
	}

	return config_parsevalue_float(value);
} // config_getvalue_float

/**
 * Get value of configuration item as string
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param num Number of item if multiple items of same name exists.
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as string. If value cannot be found, value of
 *   notexists parameter returned.
 */
char *config_getvalue_array_string(CONF_SECTION *config, char *path, int num,
	char *notexists) {

	CONF_VALUE *value = config_lookup_array(config, path, num, false);
	if (value == NULL) {
		return notexists;
	}

	return config_parsevalue_string(value);
} // config_getvalue_string

/**
 * Get value of configuration item as bool
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param num Number of item if multiple items of same name exists.
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as bool (1 for true, 0 for false). If value doesn't
 *   exists, notexists is returned.
 */
bool config_getvalue_array_bool(CONF_SECTION *config, char *path, int num,
	bool notexists) {

	CONF_VALUE *value = config_lookup_array(config, path, num, false);
	if (value == NULL) {
		return notexists;
	}

	return config_parsevalue_bool(value);
} // config_getvalue_bool

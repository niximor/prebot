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

// This library interface
#include "config.h"

/**
 * Set new configuration value as int
 * @param config Configuration section where to begin with creating
 * @param path Path to item (section:section:value[123])
 * @param value Value to set
 */
CONF_VALUE *config_set_int(CONF_SECTION *config, char *path, int value) {
	CONF_VALUE *cvalue = config_lookup(config, path, true);
	cvalue->value = htval_inte(value, cvalue->value);
	config_set_changed(cvalue->section);
	return cvalue;
} // config_set_int

/**
 * Set new configuration value as float
 * @param config Configuration section where to begin with creating
 * @param path Path to item (section:section:value[123])
 * @param value Value to set
 */
CONF_VALUE *config_set_float(CONF_SECTION *config, char *path, double value) {
	CONF_VALUE *cvalue = config_lookup(config, path, true);
	cvalue->value = htval_floate(value, cvalue->value);
	config_set_changed(cvalue->section);
	return cvalue;
} // config_set_int

/**
 * Set new configuration value as string
 * @param config Configuration section where to begin with creating
 * @param path Path to item (section:section:value[123])
 * @param value Value to set
 */
CONF_VALUE *config_set_string(CONF_SECTION *config, char *path, char *value) {
	CONF_VALUE *cvalue = config_lookup(config, path, true);
	cvalue->value = htval_stringe(value, cvalue->value);
	config_set_changed(cvalue->section);
	return cvalue;
} // config_set_int

/**
 * Set new configuration value as bool
 * @param config Configuration section where to begin with creating
 * @param path Path to item (section:section:value[123])
 * @param value Value to set
 */
CONF_VALUE *config_set_bool(CONF_SECTION *config, char *path, bool value) {
	CONF_VALUE *cvalue = config_lookup(config, path, true);
	cvalue->value = htval_inte(value, cvalue->value);
	config_set_changed(cvalue->section);
	return cvalue;
} // config_set_int

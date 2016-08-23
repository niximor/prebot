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

#ifndef _CONFIG_H
#define _CONFIG_H 1

#include <stdio.h>
#include <stdbool.h>
#include <htable/htval.h>

/**
 * Allowed characters in identifier name
 */
extern const char *config_identifierAllowed;

// Forward
typedef struct sCONF_SECTION CONF_SECTION;
typedef struct sCONF_VALUE CONF_VALUE;

/**
 * Config value
 */
struct sCONF_VALUE {
	char *name;						/**< Name of value */
	HTVAL value;					/**< Value of value */

	CONF_SECTION *section;			/**< Section that this value belongs to */
	size_t index;					/**< Index in section values array */
}; // sCONF_VALUE

/**
 * Configuration section
 */
struct sCONF_SECTION {
	char *name;						/**< Section name */
	CONF_SECTION *parent;			/**< Parent section */
	size_t index;					/**< Index in parent section array */
	bool changed;					/**< Identifies whether section's content
										 was changed. */

	size_t subSectionsCount;		/**< Number of subsections */
	size_t subSectionsAllocated;	/**< Number of allocated space
										 for subsections */
	CONF_SECTION **subSections;		/**< Array containing
										 subsections */

	size_t valuesCount;				/**< Number of values */
	size_t valuesAllocated;			/**< Number of allocated space
										 for values */
	CONF_VALUE **values;			/**< Array containing values */
}; // sCONF_SECTION

/**
 * Loads configuration from file. If error occures during parsing,
 * NULL is returned and error message is printed using printError from io
 * module.
 * @param fileName Path and name of file with configuration
 * @return Configuration or NULL if error has occured during parsing.
 */
extern CONF_SECTION *config_parse(char *fileName);

/**
 * Free configuration structures
 * @param config CONF_SECTION pointer
 */
extern void config_free(CONF_SECTION *config);

/**
 * Get value of configuration item as integer
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as integer. If value not exists, value of parameter
 *   notexists is returned.
 */
extern long int config_getvalue_int(CONF_SECTION *config, char *path,
	long int notexists);

/**
 * Get value of configuration item as float
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as float. If value not exists, value of parameter
 *   notexists is returned.
 */
extern double config_getvalue_float(CONF_SECTION *config, char *path,
	double notexists);

/**
 * Get value of configuration item as string
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as string. If value not exists, value of parameter
 *   notexists is returned.
 */
extern char *config_getvalue_string(CONF_SECTION *config, char *path,
	char *notexists);

/**
 * Get value of configuration item as bool
 * @param config CONF_SECTION pointer
 * @param path Path to item (sections are divided by ':')
 * @param notexists Value that will be returned when config value doesn't
 *   exists in loaded structures.
 * @return Value of item as bool (1 for true, 0 for false). If value not
 *   exists, value of parameter notexists is returned.
 */
extern bool config_getvalue_bool(CONF_SECTION *config, char *path,
	bool notexists);

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
extern long int config_getvalue_array_int(CONF_SECTION *config, char *path,
	int num, long int notexists);

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
extern double config_getvalue_array_float(CONF_SECTION *config, char *path,
	int num, double notexists);

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
extern char *config_getvalue_array_string(CONF_SECTION *config, char *path,
	int num, char *notexists);

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
extern bool config_getvalue_array_bool(CONF_SECTION *config, char *path,
	int num, bool notexists);

/**
 * Lookup section and create path elements if create is true.
 * @param config Root section
 * @param path Path to section
 * @param create If set to true, create path elements if they doesn't exists.
 * @return Section or NULL if it doesn't exists and create is false.
 */
extern CONF_SECTION *config_lookup_section(CONF_SECTION *config, char *path,
	bool create);

/**
 * Lookup value in section. Return that value, or false if create is false
 * and value doesn't exists.
 * @param section Section to look in
 * @param valueName Value name
 * @param itemIndex Item index
 * @param create If set to true, non existing value will be created,
 *   if set to false, non existing value will result in NULL return value.
 * @return Value or NULL if not found.
 */
extern CONF_VALUE *config_lookup_value_array(CONF_SECTION *section,
	char *valueName, int itemIndex, bool create);

/**
 * Get number of values with same name in section.
 * @param config Config section where to start looking
 * @param path Path including section and value name (section:section:value)
 * @return Number of items in that section
 */
extern size_t config_getvalue_count(CONF_SECTION *config, char *path);

/**
 * Lookup value in section. Return that value, or false if create is false and
 * value doesn't exists.
 * @param section Section to look in
 * @param value Value name
 * @param create If set to true, non existing value will be created,
 *   if set to false, non existing value will result in NULL return value.
 * @return Value or NULL if not found.
 */
extern CONF_VALUE *config_lookup_value(CONF_SECTION *section, char *value,
	bool create);

/**
 * Lookup value in config structure, create non existing path elements if they
 * don't exists and create is set to true.
 * @param config Config section where to start with searching
 * @param path Path to value (section:section:section:value[123])
 * @param create If set to true, non existing path elements will be created,
 *   if set to false, if something in path doesn't exists, NULL is returned.
 */
extern CONF_VALUE *config_lookup(CONF_SECTION *config, char *path,
	bool create);

/**
 * Lookup value in config structure, create non existing path elements if they
 * don't exists and create is set to true.
 * @param config Config section where to start with searching
 * @param path Path to value (section:section:section:value[123])
 * @param create If set to true, non existing path elements will be created,
 *   if set to false, if something in path doesn't exists, NULL is returned.
 */
extern CONF_VALUE *config_lookup_array(CONF_SECTION *config, char *path,
	int itemIndex, bool create);

/**
 * Create new configuration section
 * @param name Section name
 * @return Created section
 */
extern CONF_SECTION *config_new_section(char *name);

/**
 * Create new value
 * @param name Value name
 * @return Created section
 */
extern CONF_VALUE *config_new_value(char *name);

/**
 * Insert new section into dynamic array of sections
 * @param section Section to insert to.
 * @param newSection New section which should be inserted.
 */
extern void config_insert_section(CONF_SECTION *section,
	CONF_SECTION *newSection);

/**
 * Insert new value into dynamic array of sections
 * @param section Section to insert value into
 * @param value Value to insert
 */
extern void config_insert_value(CONF_SECTION *section, CONF_VALUE *value);

/**
 * Create config section as child of another section
 * @param parent Parent section
 * @param name New section name
 * @return New section
 */
extern CONF_SECTION *config_create_section(CONF_SECTION *parent, char *name);

/**
 * Create new value in section
 * @param section Section where create the value
 * @param name Name of new value
 */
extern CONF_VALUE *config_create_value(CONF_SECTION *section, char *name);

/**
 * Set new configuration value as int
 * @param config Configuration section where to begin with creating
 * @param path Path to item (section:section:value[123])
 * @param value Value to set
 */
extern CONF_VALUE *config_set_int(CONF_SECTION *config, char *path,
	int value);

/**
 * Set new configuration value as float
 * @param config Configuration section where to begin with creating
 * @param path Path to item (section:section:value[123])
 * @param value Value to set
 */
extern CONF_VALUE *config_set_float(CONF_SECTION *config, char *path,
	double value);

/**
 * Set new configuration value as string
 * @param config Configuration section where to begin with creating
 * @param path Path to item (section:section:value[123])
 * @param value Value to set
 */
extern CONF_VALUE *config_set_string(CONF_SECTION *config, char *path,
	char *value);

/**
 * Set new configuration value as bool
 * @param config Configuration section where to begin with creating
 * @param path Path to item (section:section:value[123])
 * @param value Value to set
 */
extern CONF_VALUE *config_set_bool(CONF_SECTION *config, char *path,
	bool value);

/**
 * Save one section and all it's subsections into stream
 * @param section Configuration section where to start with saving
 * @param indent Number of tabs to indent values
 * @param stream Output stream
 */
extern void config_save_section(CONF_SECTION *section, int indent,
	FILE *stream);

/**
 * Save configuration into stream
 * @param config Configuration section where to start with saving
 * @param stream Output stream
 */
#define config_save(config, stream) config_save_section(config, 0, stream)

/**
 * Validate section name.
 * @param name Name of section
 * @return True if section name is valid, false if invalid.
 */
extern bool config_validate_section(char *name);

/**
 * Validate value name
 * @param name Name of value
 * @param array Allow array identifier? If true, array identifiers ([]) will be
 *   allowed, if false, only plain value name is allowed.
 * @return True if section name is valid, false if invalid.
 */
extern bool config_validate_value(char *name, bool array);

/**
 * Remove subsection from section
 * @param section Section where subsection that should be removed is located.
 * @param index Subsection index
 */
extern void config_remove_subsection(CONF_SECTION *section, size_t index);

/**
 * Remove value from section
 * @param section Section where value that should be removed is located
 * @param index Value index
 */
extern void config_remove_value(CONF_SECTION *section, size_t index);

/**
 * Remove value from configuration file, specified by path to that value.
 * @param config Configuration instance
 * @param path Path to value
 * @return True if value was removed, false if no such value exists.
 */
extern bool config_remove(CONF_SECTION *config, char *path);

/**
 * Set changed flag. Also set flag to all parent sections.
 * @param section Section which change.
 */
extern void config_set_changed(CONF_SECTION *section);

#endif

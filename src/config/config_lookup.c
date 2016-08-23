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
#include "config.h"

// My libraries
#include <io.h>
#include <tokenizer.h>

/**
 * Parse value name. Parameter valueName is altered so it contains only
 * value name without array identifier, and array identifier is returned
 * as result of this function.
 * @param valueName Value name that should be parsed. Should contain value name
 *   of format foo[123] or foo or foo[].
 * @return Array identifier, 0 if no identifier was given, -1 if empty
 *   character identifier was given ([]), and -2 if error occured (bad
 *   character in section name or so.
 */
int config_parse_value_name(char *valueName) {
	size_t length = strlen(valueName);
	enum {
		CVP_NAME,
		CVP_NUM,
		CVP_END
	} state = CVP_NAME;

	int itemNumber = -1;

	for (size_t i = 0; i < length; i++) {
		switch (state) {
			case CVP_NAME:
				if (valueName[i] == '[') {
					state = CVP_NUM;
					valueName[i] = '\0';
				} else if (
					strchr(config_identifierAllowed, valueName[i]) == NULL) {

					printError("config",
						"Invalid character in value name (%c) on position "
						"%d.", valueName[i], i);
					return -2;
				}
				break;

			case CVP_NUM:
				if (valueName[i] >= '0' && valueName[i] <= '9') {
					if (itemNumber == -1) itemNumber = 0;

					itemNumber *= 10;
					itemNumber += valueName[i] - '0';
				} else if (valueName[i] == ']') {
					state = CVP_END;
				} else {
					printError("config", "Invalid character in value array"
						"identifier (%c), position %d", valueName[i], i);
					return -2;
				}
				break;

			case CVP_END:
				printError("config", "Invalid character in value name (%c), "
					"expected end of string (position %d)", valueName[i], i);
				return -2;
				break;
		}
	}

	if (state == CVP_NUM) {
		// Error, not closed character
		printError("config", "Unexpected end of string in value name.");
		return -2;
	} else if (state == CVP_NAME) {
		// If no array identifier was given.
		itemNumber = 0;
	}

	return itemNumber;
} // config_parse_value_name

/**
 * Lookup section and create path elements if create is true.
 * @param config Root section
 * @param path Path to section
 * @param create If set to true, create path elements if they doesn't exists.
 * @return Section or NULL if it doesn't exists and create is false.
 */
CONF_SECTION *config_lookup_section(CONF_SECTION *config, char *path,
	bool create) {

	TOKENS tok = tokenizer_tokenize(path, ':');
	for (size_t i = 0; i < tok->count; i++) {
		if (config == NULL) break;

		char *sectionName = tokenizer_gettok(tok, i);
		// Scan for this section in actual section
		CONF_SECTION *oldSection = config;
		for (size_t s = 0; s < config->subSectionsCount; s++) {
			if (strcmp(config->subSections[s]->name, sectionName) == 0) {
				config = config->subSections[s];
				break;
			}
		}
		// New section was not found.
		if (oldSection == config) {
			if (create) {
				// Create section if user wants to (create is true)
				config = config_create_section(config, sectionName);
			} else {
				// Don't create section, return NULL as not found.
				config = NULL;
				break;
			}
		}
	}
	tokenizer_free(tok);
	return config;
} // config_lookup_section

/**
 * Get number of values with same name in section.
 * @param config Config section where to start looking
 * @param path Path including section and value name (section:section:value)
 * @return Number of items in that section
 */
size_t config_getvalue_count(CONF_SECTION *config, char *path) {
	char *mypath = strdup(path);

	char *sectionName = mypath;
	char *valueName = strrchr(mypath, ':');

	if (valueName == NULL) {
		valueName = sectionName;
		sectionName = NULL;
	} else {
		*valueName = '\0';
		valueName++;
	}

	if (sectionName != NULL) {
		config = config_lookup_section(config, sectionName, false);
	}

	if (config != NULL) {
		size_t result = 0;
		for (size_t i = 0; i < config->valuesCount; i++) {
			if (strcmp(config->values[i]->name, valueName) == 0) {
				result++;
			}
		}
		free(mypath);
		return result;
	} else {
		free(mypath);
		return 0;
	}
} // config_getvalue_count

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
CONF_VALUE *config_lookup_value_array(CONF_SECTION *section, char *valueName,
	int itemIndex, bool create) {

	// Error in value name
	if (itemIndex == -2) return NULL;

	// If itemIndex == -1 (which means value name foo[]), create new value
	// always
	if (itemIndex >= 0) {
		for (size_t i = 0; i < section->valuesCount; i++) {
			if (strcmp(valueName, section->values[i]->name) == 0) {
				if (itemIndex-- == 0) {
					return section->values[i];
				}
			}
		}
	}

	// Value was not found
	if (create) {
		CONF_VALUE *value = config_create_value(section, valueName);
		return value;
	} else {
		return NULL;
	}
} // config_lookup_value_array

/**
 * Lookup value in section. Return that value, or false if create is false and
 * value doesn't exists.
 * @param section Section to look in
 * @param value Value name
 * @param create If set to true, non existing value will be created,
 *   if set to false, non existing value will result in NULL return value.
 * @return Value or NULL if not found.
 */
CONF_VALUE *config_lookup_value(CONF_SECTION *section, char *value,
	bool create) {

	char *valueName = strdup(value);
	int itemIndex = config_parse_value_name(valueName);

	CONF_VALUE *result = config_lookup_value_array(section, valueName,
		itemIndex, create);

	free(valueName);
	return result;
} // config_lookup_value

/**
 * Lookup value in config structure, create non existing path elements if they
 * don't exists and create is set to true.
 * @param config Config section where to start with searching
 * @param path Path to value (section:section:section:value[123])
 * @param create If set to true, non existing path elements will be created,
 *   if set to false, if something in path doesn't exists, NULL is returned.
 */
CONF_VALUE *config_lookup(CONF_SECTION *config, char *path, bool create) {
	char *mypath = strdup(path);

	char *sectionName = mypath;
	char *valueName = strrchr(mypath, ':');

	if (valueName == NULL) {
		valueName = sectionName;
		sectionName = NULL;
	} else {
		*valueName = '\0';
		valueName++;
	}

	if (sectionName != NULL) {
		config = config_lookup_section(config, sectionName, create);
	}

	if (config != NULL) {
		CONF_VALUE *value = config_lookup_value(config, valueName, create);
		free(mypath);
		return value;
	} else {
		free(mypath);
		return NULL;
	}
} // config_lookup

/**
 * Lookup value in config structure, create non existing path elements if they
 * don't exists and create is set to true.
 * @param config Config section where to start with searching
 * @param path Path to value (section:section:section:value[123])
 * @param create If set to true, non existing path elements will be created,
 *   if set to false, if something in path doesn't exists, NULL is returned.
 */
CONF_VALUE *config_lookup_array(CONF_SECTION *config, char *path,
	int itemIndex, bool create) {

	char *mypath = strdup(path);
	char *sectionName = mypath;
	char *valueName = strrchr(mypath, ':');

	if (valueName == NULL) {
		valueName = sectionName;
		sectionName = NULL;
	} else {
		*valueName = '\0';
		valueName++;
	}

	if (sectionName != NULL) {
		config = config_lookup_section(config, sectionName, create);
	}

	if (config != NULL) {
		CONF_VALUE *result =
			config_lookup_value_array(config, valueName, itemIndex, create);
		free(mypath);
		return result;
	} else {
		free(mypath);
		return NULL;
	}
} // config_lookup_array

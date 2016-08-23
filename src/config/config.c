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
#include <stdbool.h>

// This library interface
#include "config.h"

/**
 * Allowed characters in identifier name
 */
const char *config_identifierAllowed =
	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"_@$#%^&*-+.0123456789";

/**
 * Create new configuration section
 * @param name Section name
 * @return Created section
 */
CONF_SECTION *config_new_section(char *name) {
	CONF_SECTION *section = malloc(sizeof(CONF_SECTION));
	section->name = strdup(name);
	section->parent = NULL;
	section->index = 0;
	section->changed = false;
	section->subSectionsCount = 0;
	section->subSectionsAllocated = 0;
	section->subSections = NULL;
	section->valuesCount = 0;
	section->valuesAllocated = 0;
	section->values = NULL;

	return section;
} // config_new_section

/**
 * Create new value
 * @param name Value name
 * @return Created section
 */
CONF_VALUE *config_new_value(char *name) {
	CONF_VALUE *value = malloc(sizeof(CONF_VALUE));
	value->name = strdup(name);
	value->value = htval_null();
	value->section = NULL;
	value->index = 0;

	return value;
} // config_new_value

/**
 * Insert new section into dynamic array of sections
 * @param section Section to insert to.
 * @param newSection New section which should be inserted.
 */
void config_insert_section(CONF_SECTION *section, CONF_SECTION *newSection) {
	static const int BLOCK_SIZE = 16;

	if (section->subSectionsAllocated == 0) {
		section->subSectionsAllocated += BLOCK_SIZE;
		section->subSections = malloc(section->subSectionsAllocated *
			sizeof(CONF_SECTION));
	}

	if (section->subSectionsCount == section->subSectionsAllocated) {
		section->subSectionsAllocated += BLOCK_SIZE;
		section->subSections = realloc(section->subSections,
			section->subSectionsAllocated * sizeof(CONF_SECTION));
	}

	newSection->parent = section;
	newSection->index = section->subSectionsCount;

	section->subSections[section->subSectionsCount] = newSection;
	section->subSectionsCount++;
} // config_insert_section

/**
 * Insert new value into dynamic array of sections
 * @param section Section to insert value into
 * @param value Value to insert
 */
void config_insert_value(CONF_SECTION *section, CONF_VALUE *value) {
	static const int BLOCK_SIZE = 16;

	if (section->valuesAllocated == 0) {
		section->valuesAllocated = BLOCK_SIZE;
		section->values = malloc(section->valuesAllocated *
			sizeof(CONF_VALUE));
	}

	if (section->valuesCount == section->valuesAllocated) {
		section->valuesAllocated += BLOCK_SIZE;
		section->values = realloc(section->values,
			section->valuesAllocated * sizeof(CONF_VALUE));
	}

	value->section = section;
	value->index = section->valuesCount;

	section->values[section->valuesCount] = value;
	section->valuesCount++;
} // config_insert_value

/**
 * Create config section as child of another section
 * @param parent Parent section
 * @param name New section name
 * @return New section
 */
CONF_SECTION *config_create_section(CONF_SECTION *parent, char *name) {
	CONF_SECTION *newSection = config_new_section(name);
	config_insert_section(parent, newSection);
	config_set_changed(newSection);
	return newSection;
} // config_create_section

/**
 * Create new value in section
 * @param section Section where create the value
 * @param name Name of new value
 */
CONF_VALUE *config_create_value(CONF_SECTION *section, char *name) {
	CONF_VALUE *value = config_new_value(name);
	config_insert_value(section, value);
	config_set_changed(section);
	return value;
} // config_create_section

/**
 * Free configuration structures
 * @param config CONF_SECTION pointer
 */
void config_free(CONF_SECTION *config) {
	if (config == NULL) return;

	free(config->name);

	// Free subsections
	for (size_t i = 0; i < config->subSectionsCount; i++) {
		config_free(config->subSections[i]);
	}
	if (config->subSections != NULL) {
		free(config->subSections);
	}

	// Free values
	for (size_t i = 0; i < config->valuesCount; i++) {
		free(config->values[i]->name);
		htval_free(config->values[i]->value);
		free(config->values[i]);
	}
	if (config->values != NULL) {
		free(config->values);
	}

	free(config);
} // config_free

/**
 * Save one section and all it's subsections into stream
 * @param section Configuration section where to start with saving
 * @param indent Number of tabs to indent values
 * @param stream Output stream
 */
void config_save_section(CONF_SECTION *section, int indent, FILE *stream) {
	// Save values
	for (size_t i = 0; i < section->valuesCount; i++) {
		// Indent
		for (int dummy = 0; dummy < indent; dummy++) {
			fprintf(stream, "\t");
		}

		switch (htval_type(section->values[i]->value)) {
			case HT_INT:
				fprintf(stream, "%s = %ld;\n", section->values[i]->name,
					htval_get_int(section->values[i]->value));
				break;

			case HT_FLOAT:
				fprintf(stream, "%s = %lf;\n", section->values[i]->name,
					htval_get_float(section->values[i]->value));
				break;

			default:
				fprintf(stream, "%s = \"%s\";\n", section->values[i]->name,
					htval_get_string(section->values[i]->value));
				break;
		}
	}

	if (section->valuesCount > 0 && section->subSectionsCount > 0) {
		fprintf(stream, "\n");
	}

	// Save subsections
	for (size_t i = 0; i < section->subSectionsCount; i++) {
		// Indent
		for (int dummy = 0; dummy < indent; dummy++) {
			fprintf(stream, "\t");
		}

		fprintf(stream, "%s {\n", section->subSections[i]->name);

		config_save_section(section->subSections[i], indent + 1, stream);

		// Indent
		for (int dummy = 0; dummy < indent; dummy++) {
			fprintf(stream, "\t");
		}
		fprintf(stream, "}\n\n");
	}

	section->changed = false;
} // config_save_section

/**
 * Remove subsection from section
 * @param section Section where subsection that should be removed is located.
 * @param index Subsection index
 */
void config_remove_subsection(CONF_SECTION *section, size_t index) {
	if (index < section->subSectionsCount) {
		// Free deleted section
		config_free(section->subSections[index]);

		// Make the array compact again
		for (size_t i = index; i < section->subSectionsCount-1; i++) {
			section->subSections[i] = section->subSections[i+1];
		}
		section->subSectionsCount--;
		config_set_changed(section);
	}
} // config_remove_subsection

/**
 * Remove value from section
 * @param section Section where value that should be removed is located
 * @param index Value index
 */
void config_remove_value(CONF_SECTION *section, size_t index) {
	if (index < section->valuesCount) {
		// Free deleted value
		free(section->values[index]->name);
		htval_free(section->values[index]->value);
		free(section->values[index]);

		// Make the array compact again
		for (size_t i = index; i < section->valuesCount-1; i++) {
			section->values[i] = section->values[i+1];
		}
		section->valuesCount--;
		config_set_changed(section);
	}
} // config_remove_value

/**
 * Remove value from configuration file, specified by path to that value.
 * @param config Configuration instance
 * @param path Path to value
 */
bool config_remove(CONF_SECTION *config, char *path) {
	CONF_VALUE *value = config_lookup(config, path, false);
	if (value != NULL) {
		config_remove_value(value->section, value->index);
		return true;
	} else {
		return false;
	}
} // config_remove

/**
 * Set changed flag. Also set flag to all parent sections.
 * @param section Section which change.
 */
void config_set_changed(CONF_SECTION *section) {
	while (section != NULL && !section->changed) {
		section->changed = true;
		section = section->parent;
	}
} // config_set_changed

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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// This library interface
#include "config.h"

// My libraries
#include <htable/htval.h>
#include <dynastring.h>
#include <io.h>

/**
 * Config finite state machine states and return values
 */
enum CFSM_STATE {
	CFSM_WHITESPACE = 0,			/**< White space before identifier */
	CFSM_COMMENT_1,					/**< Maybe comment? */
	CFSM_COMMENT,					/**< Comment to the end of line */
	CFSM_IDENTIFIER,				/**< Inside identifier */
	CFSM_AFTER_IDENTIFIER,			/**< White space after identifier,
									     expecting { or = chars */
	CFSM_VALUE_WHITESPACE,			/**< White space after =, expecting
									     value */
	CFSM_VALUE_DIGIT,				/**< Expecting digit or . */
	CFSM_VALUE_DIGIT_END,			/**< Expecting ; */
	CFSM_VALUE_STRING,				/**< Expecting any character, " ends
									     this mode. */
	CFSM_VALUE_STRING_WHITESPACE,	/**< Expecting " or ; */
	CFSM_VALUE_BOOL_TRUE_1,			/**< Received T, expecting R */
	CFSM_VALUE_BOOL_TRUE_2,			/**< Received TR, expecting U */
	CFSM_VALUE_BOOL_TRUE_3,			/**< Received TRU, expecting E */
	CFSM_VALUE_BOOL_FALSE_1,		/**< Received F, expecting A */
	CFSM_VALUE_BOOL_FALSE_2,		/**< Received FA, expecting L */
	CFSM_VALUE_BOOL_FALSE_3,		/**< Received FAL, expecting S */
	CFSM_VALUE_BOOL_FALSE_4,		/**< Received FALS, expecting E */
	CFSM_VALUE_BOOL_END,			/**< Expecting ; */

	// Return values
	CFSM_NO_ACTION = 1000,			/**< No action, we are inside
									     something */
	CFSM_ERROR,						/**< Error, stop parsing */
	CFSM_BEGIN_IDENTIFIER,			/**< We have identifier name in
									     retval */
	CFSM_BEGIN_SECTION,				/**< We have section name in retval */
	CFSM_END_SECTION,
	CFSM_PROCESS_VALUE_DIGIT,		/**< We have digit value to process */
	CFSM_PROCESS_VALUE_STRING,		/**< We have string value to process */
	CFSM_PROCESS_VALUE_BOOL			/**< We have bool value to process */
}; // CFSM_STATE

/**
 * Finite state machine for parsing config file
 * @param c Char from file
 * @param fn File name for error messages
 * @param retval Return value if we have something to return
 * @return If we have something to return, function returns code of what is
 *   in return parameter. Otherwise it will return 0.
 */
static enum CFSM_STATE config_fsm(int c, char *fn, string retval) {
	static int line = 1;
	static int column = 1;

	static enum CFSM_STATE state = CFSM_WHITESPACE;

	if (c == '\n') {
		line++;
		column = 1;

		// End of line kills comments.
		if (state == CFSM_COMMENT) {
			state = CFSM_WHITESPACE;
		}
	}

	switch (state) {
		// Whitespace inside section or on root level of config
		// file. Characters that may follow are: a-z or _ as begining
		// of identifier. Also } may follow as end of section. Any
		// other characters are illegal.
		case CFSM_WHITESPACE:
			if (strchr(config_identifierAllowed, c) != NULL) {
				state = CFSM_IDENTIFIER;
				dynastring_appendchar(retval, c);
			} else if (c == '}') {
				state = CFSM_WHITESPACE;
				return CFSM_END_SECTION;
			} else if (isspace(c)) {
				break;
			} else if (c == '/') {
				state = CFSM_COMMENT_1;
			} else {
				printError("config", "Parse error - illegal character %c "
					"in %s on line %d, column %d.", c, fn, line, column);

				return CFSM_ERROR;
			}
			break;

		// Comment begins with //. If anything else is passed after first /,
		// it is considered illegal.
		case CFSM_COMMENT_1:
			if (c == '/') {
				state = CFSM_COMMENT;
			} else {
				printError("config", "Parse error - illegal character %c "
					"in %s on line %d, column %d.", c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// Inside identifier - after first char (a-z or _). Characters
		// that may follow are: a-z, 0-9 or _. Also whitespace
		// can occur, or = or { as begining of identifier value or
		// section.
		case CFSM_IDENTIFIER:
			if (strchr(config_identifierAllowed, c) != NULL) {
				dynastring_appendchar(retval, c);
			} else if (isspace(c)) {
				state = CFSM_AFTER_IDENTIFIER;
			} else if (c == '=') {
				state = CFSM_VALUE_WHITESPACE;
				return CFSM_BEGIN_IDENTIFIER;
			} else if (c == '{') {
				state = CFSM_WHITESPACE;
				return CFSM_BEGIN_SECTION;
			} else {
				printError("config", "Parse error - illegal character %c "
					"in %s on line %d, column %d.", c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// Identifier has ended with white space, expecting = or {
		// characters as begining of identifier value or section.
		case CFSM_AFTER_IDENTIFIER:
			if (c == '=') {
				state = CFSM_VALUE_WHITESPACE;
				return CFSM_BEGIN_IDENTIFIER;
			} else if (c == '{') {
				state = CFSM_WHITESPACE;
				return CFSM_BEGIN_SECTION;
			} else if (isspace(c)) {
				break;
			} else {
				printError("config", "Parse error - illegal character %c "
					"(expected = or {) in %s on line %d, column %d",
					c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// White space before identifer value. Expecting 0-9, +-,
		// ", t, f, T or F.
		case CFSM_VALUE_WHITESPACE:
			if (isspace(c)) {
				break;
			} else if (isdigit(c) || c == '+' || c == '-' ||
				c == '.') {

				state = CFSM_VALUE_DIGIT;
				dynastring_appendchar(retval, c);
			} else if (c == '"') {
				state = CFSM_VALUE_STRING;
			} else if (c == 't' || c == 'T') {
				state = CFSM_VALUE_BOOL_TRUE_1;
				dynastring_appendchar(retval, 't');
			} else if (c == 'f' || c == 'F') {
				state = CFSM_VALUE_BOOL_FALSE_1;
				dynastring_appendchar(retval, 'f');
			} else {
				printError("config", "Parse error - illegal character %c "
					"(expected digit, string or true / false) in %s on "
					"line %d, column %d", c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// It seems user wants to enter digit. Expecting 0-9, .
		// or whitespace or ;
		case CFSM_VALUE_DIGIT:
			if (isdigit(c) || c == '.' || c == 'e' || c == 'E') {
				// Correct digit
				state = CFSM_VALUE_DIGIT;
				dynastring_appendchar(retval, c);
			} else if (isspace(c)) {
				state = CFSM_VALUE_DIGIT_END;
			} else if (c == ';') {
				state = CFSM_WHITESPACE;
				return CFSM_PROCESS_VALUE_DIGIT;
			} else {
				printError("config", "Parse error - illegal character %c "
					"(expected digit or ;) in %s on line %d, column %d",
					c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// After correct value, nothing can follow, just ;
		case CFSM_VALUE_DIGIT_END:
			if (isspace(c)) {
				break;
			} else if (c == ';') {
				state = CFSM_WHITESPACE;
				return CFSM_PROCESS_VALUE_DIGIT;
			} else {
				printError("config", "Parse error - illegal characted %c "
					"(expected ;) in %s on line %d, column %d",
					c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// We are inside "" block. There is no parser error, just
		// append to string or switch to whitespace.
		case CFSM_VALUE_STRING:
			if (c == '"') {
				state = CFSM_VALUE_STRING_WHITESPACE;
			} else {
				dynastring_appendchar(retval, c);
			}
			break;

		// We are inside string (after "" block) but before ;.
		// " may follow to concatenate long strings on multiple lines
		// or ; may follow to end value.
		case CFSM_VALUE_STRING_WHITESPACE:
			if (c == '"') {
				state = CFSM_VALUE_STRING;
			} else if (isspace(c)) {
				break;
			} else if (c == ';') {
				state = CFSM_WHITESPACE;
				dynastring_appendchar(retval, '\0');
				return CFSM_PROCESS_VALUE_STRING;
			} else {
				printError("config", "Parse error - illegal character %c "
					"(expected \" or ;) in %s on line %d, column %d",
					c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// We received T, and needs R
		case CFSM_VALUE_BOOL_TRUE_1:
			if (c == 'r' || c == 'R') {
				dynastring_appendchar(retval, 'r');
				state = CFSM_VALUE_BOOL_TRUE_2;
			} else {
				printError("config", "Parse error - illegal character %c "
					"(expected r) in %s on line %d, column %d",
					c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// We received TR, and needs U
		case CFSM_VALUE_BOOL_TRUE_2:
			if (c == 'u' || c == 'U') {
				dynastring_appendchar(retval, 'u');
				state = CFSM_VALUE_BOOL_TRUE_3;
			} else {
				printError("config", "Parse error - illegal character %c "
					"(expected u) in %s on line %d, column %d",
					c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// We received TRU, and needs E
		case CFSM_VALUE_BOOL_TRUE_3:
			if (c == 'e' || c == 'E') {
				dynastring_appendchar(retval, 'e');
				state = CFSM_VALUE_BOOL_END;
			} else {
				printError("config", "Parse error - illegal character %c "
					"(expected r) in %s on line %d, column %d",
					c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// We received F, and needs A
		case CFSM_VALUE_BOOL_FALSE_1:
			if (c == 'a' || c == 'A') {
				dynastring_appendchar(retval, 'a');
				state = CFSM_VALUE_BOOL_FALSE_2;
			} else {
				printError("config", "Parse error - illegal character %c "
					"(expected a) in %s on line %d, column %d",
					c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// We received FA, and needs L
		case CFSM_VALUE_BOOL_FALSE_2:
			if (c == 'l' || c == 'L') {
				dynastring_appendchar(retval, 'l');
				state = CFSM_VALUE_BOOL_FALSE_3;
			} else {
				printError("config", "Parse error - illegal character %c "
					"(expected l) in %s on line %d, column %d",
					c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// We received FAL, and needs S
		case CFSM_VALUE_BOOL_FALSE_3:
			if (c == 's' || c == 'S') {
				dynastring_appendchar(retval, 's');
				state = CFSM_VALUE_BOOL_FALSE_4;
			} else {
				printError("config", "Parse error - illegal character %c "
					"(expected s) in %s on line %d, column %d",
					c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// We received FALS, and needs E
		case CFSM_VALUE_BOOL_FALSE_4:
			if (c == 'e' || c == 'E') {
				dynastring_appendchar(retval, 'e');
				state = CFSM_VALUE_BOOL_END;
			} else {
				printError("config", "Parse error - illegal character %c "
					"(expected e) in %s on line %d, column %d",
					c, fn, line, column);
				return CFSM_ERROR;
			}
			break;

		// After correct value, nothing can follow, just ;
		case CFSM_VALUE_BOOL_END:
			if (isspace(c)) {
				break;
			} else if (c == ';') {
				state = CFSM_WHITESPACE;
				return CFSM_PROCESS_VALUE_BOOL;
			} else {
				printError("config", "Parse error - illegal characted %c "
					"(expected ;) in %s on line %d, column %d", c, fn, line,
					column);
			}
			break;

		// Ignore all comment chars.
		case CFSM_COMMENT:
			break;

		// Enumeration value XYZ not handled in switch.
		default:
			printError("config", "Unknown error, this should never happen, "
				"in %s on line %d, column %d", fn, line, column);
			break;
	}

	column++;

	return CFSM_NO_ACTION;
} // config_fsm

/**
 * Loads configuration from file. If error occures during parsing,
 * NULL is returned and error message is printed using printError from io
 * module.
 * @param fileName Path and name of file with configuration
 * @return Configuration or NULL if error has occured during parsing.
 */
CONF_SECTION *config_parse(char *fileName) {
	FILE *f = fopen(fileName, "r");

	if (f != NULL) {
		CONF_SECTION *rootSection = config_new_section("");
		CONF_SECTION *currentSection = rootSection;

		int cancelParsing = 0;

		string retval = dynastring_init();

		CONF_VALUE *value = malloc(sizeof(CONF_VALUE));
		value->name = NULL;
		value->value = htval_null();

		int c = 0;
		int line = 1, column = 1;
		while ((c = fgetc(f)) != EOF) {
			if (c == '\n') {
				line++;
				column = 1;
			}

			switch (config_fsm(c, fileName, retval)) {
				// Parse error, stop parsing config file and
				// go back to program.
				case CFSM_ERROR: {
					cancelParsing = 1;
					dynastring_clear(retval);
					break;
				}

				// We have identifier name, but not identifier
				// value.
				case CFSM_BEGIN_IDENTIFIER: {
					value->name = strdup(dynastring_getstring(retval));
					dynastring_clear(retval);
					break;
				}

				// We are inside new section, we have it's
				// name.
				case CFSM_BEGIN_SECTION: {
					CONF_SECTION *newSection =
						config_new_section(dynastring_getstring(retval));

					config_insert_section(currentSection, newSection);

					newSection->parent = currentSection;
					currentSection = newSection;
					dynastring_clear(retval);
					break;
				}

				// We are at the end of section.
				case CFSM_END_SECTION: {
					if (currentSection->parent != NULL) {
						currentSection = currentSection->parent;
					} else {
						printError("config", "Parse error - unexpected } "
							"in %s on line %d, column %d", fileName, line,
							column);
					}
					dynastring_clear(retval);
					break;
				}

				// We are at end of digit value of identifier
				case CFSM_PROCESS_VALUE_DIGIT: {
					// Try to convert string into float.
					// If this succedes, value is float,
					// else it is int, maybe...
					char *restOfString;
					long int iv = strtol(dynastring_getstring(retval),
						&restOfString, 10);

					if (strlen(restOfString) == 0) {
						value->value = htval_inte(iv, value->value);
					} else {
						double fv = strtod(dynastring_getstring(retval),
							&restOfString);

						if (strlen(restOfString) == 0) {

							value->value = htval_floate(fv, value->value);
						} else {
							printError("config", "Parse error - %s is not "
								"a digit - in %s on line %d, column %d",
								dynastring_getstring(retval), fileName, line,
								column);
							break;
						}
					}

					// Insert value into section
					config_insert_value(currentSection, value);

					// Allocate memory for new value
					value = malloc(sizeof(CONF_VALUE));
					value->name = NULL;
					value->value = htval_null();
					dynastring_clear(retval);
					break;
				}

				// We are at end of string value of identifier
				case CFSM_PROCESS_VALUE_STRING: {
					value->value = htval_stringe(
						dynastring_getstring(retval), value->value);

					// Insert value into section
					config_insert_value(currentSection,	value);

					// Allocate memory for new value
					value = malloc(sizeof(CONF_VALUE));
					value->name = NULL;
					value->value = htval_null();
					dynastring_clear(retval);
					break;
				}

				// We are at end of bool value of identifier
				case CFSM_PROCESS_VALUE_BOOL: {
					if (strcmp(dynastring_getstring(retval), "true") == 0) {
						value->value = htval_inte(1, value->value);
					} else {
						value->value = htval_inte(0, value->value);
					}

					// Insert value ito section
					config_insert_value(currentSection,	value);

					// Allocate memory for new value
					value = malloc(sizeof(CONF_VALUE));
					value->name = NULL;
					value->value = htval_null();
					dynastring_clear(retval);
					break;
				}

				// No action...
				case CFSM_NO_ACTION: {
					break;
				}

				default: {
					printError("config", "Unknown FSM result.");
					break;
				}
			}
			column++;
			if (cancelParsing) break;
		}

		// Last value is always pending...
		htval_free(value->value);
		free(value);
		dynastring_free(retval);

		fclose(f);

		return rootSection;
	} else {
		printError("config", "Unable to open file %s: %s", fileName,
			strerror(errno));
		return NULL;
	}
} // config_parse

/**
 * Validate section name.
 * @param name Name of section
 * @return True if section name is valid, false if invalid.
 */
bool config_validate_section(char *name) {
	size_t length = strlen(name);
	if (length == 0) return false;

	for (size_t i = 0; i < length; i++) {
		if (strchr(config_identifierAllowed, name[i]) == NULL) {
			return false;
		}
	}
	return true;
} // config_validate_section

/**
 * Validate value name
 * @param name Name of value
 * @param array Allow array identifier? If true, array identifiers ([]) will be
 *   allowed, if false, only plain value name is allowed.
 * @return True if section name is valid, false if invalid.
 */
bool config_validate_value(char *name, bool array) {
	enum {
		CVV_NAME,
		CVV_ARRAY,
		CVV_END
	} state = CVV_NAME;

	size_t length = strlen(name);
	if (length == 0) return false;

	for (size_t i = 0; i < length; i++) {
		switch (state) {
			case CVV_NAME:
				if (name[i] == '[') {
					if (array) {
						state = CVV_ARRAY;
					} else {
						return false;
					}
				} else {
					if (strchr(config_identifierAllowed, name[i]) == NULL) {
						return false;
					}
				}
				break;

			case CVV_ARRAY:
				if (name[i] == ']') {
					state = CVV_END;
				} else if (name[i] < '0' || name[i] > '9') {
					return false;
				}
				break;

			case CVV_END:
				return false;
				break;
		}
	}

	// It is invalid, when array identifier was not closed
	if (state == CVV_ARRAY) return false;

	return true;
} // config_validate_value

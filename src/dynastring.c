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
#include "dynastring.h"

/**
 * Init dynamic string
 * @return String.
 */
string dynastring_init() {
	string result = malloc(sizeof(sstring));
	if (result == NULL) return NULL;

	// Initial allocation
	result->data = malloc(64 * sizeof(char));
	result->length = 0;
	if (result->data != NULL) {
		result->allocated = 64;
		memset(result->data, '\0', result->allocated);
	} else {
		result->allocated = 0;
	}
	result->pointer = 0;
	return result;
} // dynastring_init

/**
 * Append char to dynamic string
 * @param s String
 * @param ch Char to append
 */
void dynastring_appendchar(string s, char ch) {
	static const int BLOCK_SIZE = 64;

	if (s == NULL) return;

	// Do the initial allocation, but it shouldn't be necessary.
	if (s->allocated == 0) {
		s->allocated = BLOCK_SIZE;
		s->data = malloc(s->allocated);
		memset(s->data, '\0', BLOCK_SIZE);
	}

	// Realloc string, if new char doesn't fit into buffer.
	if (s->length >= s->allocated) {
		s->allocated += BLOCK_SIZE;
		s->data = realloc(s->data, s->allocated);
		memset(s->data + s->allocated - BLOCK_SIZE, '\0', BLOCK_SIZE);
	}

	if (s->data != NULL) {
		for (size_t i = s->length; i > s->pointer; i--) {
			s->data[i] = s->data[i-1];
		}
		s->data[s->pointer++] = ch;
		s->length++;
	} else {
		// Memory allocation error
		s->allocated = 0;
		s->length = 0;
		s->pointer = 0;
	}
} // dynastring_append

/**
 * Append string to dynastring
 * @param s String
 * @param str String that should be appendded.
 */
void dynastring_appendstring(string s, char *ch) {
	if (ch != NULL) {
		for (size_t i = 0; i < strlen(ch); i++) {
			dynastring_appendchar(s, ch[i]);
		}
	}
} // dynastring_appendstring

/**
 * Append dynastring to dynastring.
 * @param s String
 * @param ch String that should be appended.
 */
void dynastring_append(string s, string ch) {
	dynastring_appendstring(s, dynastring_getstring(ch));
} // dynastring_append

/**
 * Clear string, so it can be reused again.
 * @param s String
 */
void dynastring_clear(string s) {
	if (s == NULL) return;

	if (s->allocated > 0) {
		// Empty string
		memset(s->data, '\0', s->allocated);
		s->length = 0;
		s->pointer = 0;
	}
} // dynastring_clear

/**
 * Free dynamic string.
 * @param s String
 */
void dynastring_free(string s) {
	if (s->allocated > 0) {
		free(s->data);
	}
	free(s);
} // dynastring_free

/**
 * Get C string from dynastring
 * @param s Dynastring
 * @return String
 */
char *dynastring_getstring(string s) {
	return s->data;
} // dynastring_getstring

/**
 * Get length of dynastring
 * @param s Dynastring
 * @return Length of string in number of chars.
 */
size_t dynastring_getlength(string s) {
	return s->length;
} // dynastring_getlength

/**
 * Get position of pointer in string
 * @param s Dynastring
 * @return Position of pointer in string as number of chars from begining of
 *   string.
 */
size_t dynastring_getpos(string s) {
	return s->pointer;
} // dynastring_getpos

/**
 * Seek pointer in dynastring
 * @param s Dynastring
 * @param offset Seek offset
 * @param mode Seek mode (SEEK_SET for set pointer to exact value, SEEK_CUR to
 *   seek from current position, SEEK_END for seek to end of string (offset
 *   is ignored in this case)).
 * @return Number of chars that has skipped from current position. Negative
 *   for left seek, positive for right seek.
 */
int dynastring_seek(string s, int offset, int mode) {
	switch (mode) {
		case SEEK_SET:
			if (offset < 0) offset = 0;
			if ((size_t)offset > s->length) offset = s->length;
			offset -= s->pointer;
			break;

		case SEEK_CUR:
			if ((int)s->pointer + (int)offset >= (int)s->length) {
				offset = s->length - s->pointer;
			}
			if ((int)s->pointer + offset < 0) {
				offset = s->pointer;
			}
			break;

		case SEEK_END:
			offset = s->length - s->pointer;
			break;
	}

	s->pointer += offset;
	return offset;
} // dynastring_seek

/**
 * Remove characters before/after pointer. Returns number of deleted characters.
 * @param s Dynastring
 * @param count Number of characters to delete. If negative, removes characters
 *   left to pointer (backspace), if positive, right to pointer (delete).
 */
int dynastring_delete(string s, int count) {
	if (count < 0) {
		// Backspace
		count *= -1;
		if ((int)s->pointer - count < 0) count = s->pointer;

		for (size_t i = s->pointer - count; i < s->length; i++) {
			s->data[i] = s->data[i+count];
		}
		s->length -= count;
		s->pointer -= count;

		// Remove deleted characters
		memset(s->data + s->length, '\0', count);

		return count;
	} else if (count > 0) {
		// Delete
		if (s->pointer + count >= s->length) count = s->length - s->pointer;
		for (int i = s->pointer + count; i < (int)s->length; i++) {
			s->data[i-count] = s->data[i];
		}
		s->length -= count;

		// Remove deleted characters
		memset(s->data + s->length, '\0', count);

		return count;
	}

	// count is 0, so return 0 deleted characters.
	return 0;
} // dynastring_delete

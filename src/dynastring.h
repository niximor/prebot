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

#ifndef _DYNASTRING_H
#define _DYNASTRING_H 1

#include <stdio.h>

/**
 * Dynamic string
 */
typedef struct {
	char *data;
	size_t length;
	size_t allocated;
	size_t pointer;
} sstring;
typedef sstring *string;

/**
 * Init dynamic string
 * @return String.
 */
extern string dynastring_init();

/**
 * Append char to dynamic string
 * @param s String
 * @param ch Char to append
 */
extern void dynastring_appendchar(string s, char ch);

/**
 * Append string to dynastring
 * @param s String
 * @param str String that should be appendded.
 */
extern void dynastring_appendstring(string s, char *ch);

/**
 * Append dynastring to string.
 * @param s String
 * @param str String that should be appended.
 */
extern void dynastring_append(string s, string str);

/**
 * Clear string, so it can be reused again.
 * @param s String
 */
extern void dynastring_clear(string s);

/**
 * Free dynamically allocated string, but it doesn't free the pointer, so
 * the string can be used again.
 * @param s String
 */
extern void dynastring_free(string s);

/**
 * Get C string from dynastring
 * @param s Dynastring
 * @return String
 */
extern char *dynastring_getstring(string s);

/**
 * Get length of dynastring
 * @param s Dynastring
 * @return Length of string in number of chars.
 */
extern size_t dynastring_getlength(string s);

/**
 * Get position of pointer in string
 * @param s Dynastring
 * @return Position of pointer in string as number of chars from begining of
 *   string.
 */
extern size_t dynastring_getpos(string s);

/**
 * Seek pointer in dynastring
 * @param s Dynastring
 * @param offset Seek offset
 * @param mode Seek mode (SEEK_SET for set pointer to exact value, SEEK_CUR to
 *   seek from current position, SEEK_END for seek to end of string (offset
 *   is ignored in this case)).
 * @return Number of chars that has skipped from actual position. Negative
 *   for left seek, positive for right seek.
 */
extern int dynastring_seek(string s, int offset, int mode);

/**
 * Remove characters before/after pointer. Returns number of deleted characters.
 * @param s Dynastring
 * @param count Number of characters to delete. If negative, removes characters
 *   left to pointer (backspace), if positive, right to pointer (delete).
 */
extern int dynastring_delete(string s, int count);

#endif

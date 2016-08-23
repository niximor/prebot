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
 *
 *  This function is modified version of islikeWildcard from XBSQL project,
 *  (C) 2001 by Mike Richardson.
 */

// Standard libraries
#include <stdbool.h>
#include <ctype.h>

// This library interface
#include "wildcard.h"

/**
 * Match string against wildcard string and return true, if string matches
 * and false if don't.
 * @param wildcard Wildcard string (* and ? wildcard chars are supported)
 * @param str String to match against wildcard
 * @param casesensitive Set to true to case sensitive matching
 * @return True if str matches wildcard, false otherwise
 */
bool wildmatch(const char *wildcard, const char *str, bool casesensitive) {
	while (*wildcard != '\0') {
		switch (*wildcard) {
			case '?':
				// Match any single character. Fail if the
				// string is exhausted, otherwise advance the
				// string and the pattern.
				if (*str++ == '\0') return false;
				wildcard += 1;
				break;

			case '*':
				// Match (a possibly empty) string. First try
				// a match with a null string (just advance the
				// pattern), otherwise advance the string (but
				// check for the end).
				if (wildmatch(&wildcard[1], str, casesensitive)) return true;
				if (*str++ == '\0') return false;
				break;

			default:
				// Anything else requires an exact match
				// between the string and the pattern.
				if (*str == '\0') return false;
				if (!casesensitive) {
					if (tolower(*str++) != tolower(*wildcard++)) return false;
				} else {
					if (*str++ != *wildcard++) return false;
				}
				break;
		}
	}

	// End of the pattern. There is a match if the string is also
	// exhausted.
	return	*str == '\0';
} // wildmatch

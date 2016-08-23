/**
 *  Functions for working with directory
 *
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

#ifndef _WILDCARD_H
# define _WILDCARD_H 1

#include <stdbool.h>

/**
 * Case sensitive wildcard match.
 * @param wildcard Wildcard string
 * @param str String to match against wildcard
 * @return True if str matches the wildcard, false otherwise
 */
#define wildmatch_cs(wildcard, str) wildmatch(wildcard, str, true)

/**
 * Case insensitive wildcard match
 * @param wildcard Wildcard string
 * @param str String to match against wildcard
 * @return True if str matches the wildcard, false otherwise
 */
#define wildmatch_ci(wildcard, str) wildmatch(wildcard, str, false)

/**
 * Match string against wildcard string and return true, if string matches
 * and false if don't.
 * @param wildcard Wildcard string (* and ? wildcard chars are supported)
 * @param str String to match against wildcard
 * @param casesensitive Set to true to case sensitive matching
 * @return True if str matches wildcard, false otherwise
 */
extern bool wildmatch(const char *wildcard, const char *str,
	bool casesensitive);

#endif

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

#ifndef _TOOLBOX_STRING_H
# define _TOOLBOX_STRING_H 1

#include <string.h>

/**
 * Compare two strings together
 */
#define eq(s1, s2) (strcmp(s1, s2) == 0)

/**
 * Compare two strings together, but at most first n characters
 */
#define eqn(s1, s2, n) (strncmp(s1, s2, n) == 0)

extern void strtolower(char *str);

#endif

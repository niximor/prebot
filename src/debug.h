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

#ifndef _DEBUG_H
# define _DEBUG_H 1

#define _GNU_SOURCE
#include <stdlib.h>

extern void *mymalloc(size_t size, char *file, int line);
extern void myfree(void *ptr, char *file, int line);

// Overwrite malloc and free functions to use our debug ones
#define malloc(size) mymalloc(size, __FILE__, __LINE__);
#define free(ptr) myfree(ptr, __FILE__, __LINE__);

#endif

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

#ifndef _IO_H
#define _IO_H 1

#include <stdio.h>
#include <stdlib.h>

/**
 * Redirect log (stderr) to file. If the file exists, it's content is
 * overwritten.
 * @param fileName Name of file where the log will be written.
 */
extern void redirLog(char *fileName);

/**
 * Redirect log (stderr) to file. If the file exists, new log is appended
 * to the end of existing data.
 * @param fileName Name of file where the log will be written.
 */
extern void redirLogAppend(char *fileName);

/**
 * Prints error message to stderr.
 * @param module Module name in which the error occured.
 * @param format Formatting of message
 */
extern void printError(char *module, const char *format, ...);

#endif

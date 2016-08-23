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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "io.h"

FILE *old_stderr = NULL;

/**
 * Redirect log (stderr) to file. Allows to specify mode for opening the file.
 * @param fileName Log file name
 * @param mode Mode for opening the file.
 */
bool redirLogMode(char *fileName, char *mode) {
    if (old_stderr == NULL) {
        old_stderr = stderr;
    }

    FILE *temp = fopen(fileName, mode);
    if (temp) {
        fclose(temp);
        stderr = freopen(fileName, mode, stderr);

        if (stderr == NULL) {
            stderr = old_stderr;
            printError("io", "Unable to open log file for writing: %s", strerror(errno));
            return false;
        } else {
            printError("io", "Successfully redirected output to %s.", fileName);
            return true;
        }
    } else {
        printError("io", "Unable to open log file for writing: %s", strerror(errno));
        return false;
    }
} // redirLogMode

/**
 * Redirect log (stderr) to file. If the file exists, it's content is
 * overwritten.
 * @param fileName Name of file where the log will be written.
 */
void redirLog(char *fileName) {
    redirLogMode(fileName, "w");
} // redirLog

/**
 * Redirect log (stderr) to file. If the file exists, new log is appended
 * to the end of existing data.
 * @param fileName Name of file where the log will be written.
 */
void redirLogAppend(char *fileName) {
    redirLogMode(fileName, "a");
} // redirLogAppend

/**
 * Prints error message to stderr.
 * @param module Module name in which the error occured.
 * @param format Formatting of message
 */
void printError(char *module, const char *format, ...) {
	time_t now = time(NULL);
	struct tm *now_human = localtime(&now);

	fprintf(stderr, "[%02d.%02d.%02d %02d:%02d:%02d] %s: ",
		now_human->tm_mday, now_human->tm_mon, now_human->tm_year+1900,
		now_human->tm_hour, now_human->tm_min, now_human->tm_sec, module);

	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	fprintf(stderr, "\n");
    fflush(stderr);
} // printError

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
 */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

// Standard includes
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

// Linux includes
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// Custom interface
#include "dirs.h"
#include "io.h"

/**
 * Get current directory name from file name. Return value is dynamically
 * allocated string, so don't forget to free it.
 * @param filename File name
 * @return Path extracted from filename.
 */
char *dirs_dirname(char *filename) {
	char *result = strdup(filename);

	// Try to find first / or \ from right, and trim everything
	// after that (back)slash.
	bool found = false;
	for (size_t i = strlen(result); i > 0; i--) {
		if (result[i-1] == '/' || result[i-1] == '\\') {
			result[i] = '\0';
			found = true;
			result = realloc(result, (strlen(result) + 1) * sizeof(char));
			break;
		}
	}

	// No / or \ was found in filename, so we expect directory to be
	// ./.
	if (!found) {
		free(result);
		result = strdup("./");
	}

	return result;
} // dirname

/**
 * Get file name without suffix from path.
 * @param path Path
 * @return Allocated string containing only file name without suffix.
 */
char *dirs_basename(char *path) {
	size_t dotpos = strlen(path);
	size_t slashpos = 0;

	for (size_t i = strlen(path); i > 0; i--) {
		if (path[i] == '.') {
			dotpos = i;
		}
		if (path[i-1] == '/' || path[i-1] == '\\') {
			slashpos = i;
			break;
		}
	}

	char *basename = malloc((dotpos - slashpos + 1) * sizeof(char));
	memcpy(basename, path + slashpos, dotpos - slashpos);
	basename[dotpos-slashpos] = '\0';
	return basename;
} // dirs_basename

/**
 * Compose full path name from directory and file parts. Return value is
 * dynamically allocated string, so don't forget to free it.
 * @param directory Directory part of the path
 * @param file File path of the path.
 */
char *dirs_makefullpath(char *directory, char *file) {
	if (directory == NULL) return file;
	if (file == NULL) return NULL;

	char *result;

	// If directory name contains / as last char, don't add another one.
	if (directory[strlen(directory)-1] == '/') {
		if (file[0] == '/') {
			asprintf(&result, "%s%s", directory, file+1);
		} else {
			asprintf(&result, "%s%s", directory, file);
		}
	} else {
		if (file[0] == '/') {
			asprintf(&result, "%s%s", directory, file);
		} else {
			asprintf(&result, "%s/%s", directory, file);
		}
	}

	return result;
} // dirs_makefullpath

/**
 * Filter hidden files for use by scandir function.
 * @param file dirent entry from directory listing
 * @return true when file should be contained in list, false otherwise.
 */
int dirs_filter_hidden(const struct dirent *file) {
	if (file->d_name[0] == '.') {
		return 0;
	} else {
		return 1;
	}
} // dirs_filter_hidden

/**
 * Load contents of directory into DirList structure.
 * @param directory Directory which to load.
 * @return DirList structure with list of files in directory, or NULL if
 *   cannot read the directory.
 */
DirList dirs_load(char *directory) {
	struct dirent **namelist;
	int namelistcount = scandir(directory, &namelist, dirs_filter_hidden,
		alphasort);

	if (namelistcount < 0) {
		printError("dirs", "Unable to load list of directory: %s",
			strerror(errno));

		return NULL;
	} else {
		DirList list = malloc(sizeof(struct sDirList));

		if (list != NULL) {
			list->files = malloc(namelistcount * sizeof(DirEntry));
			if (list->files != NULL) {
				list->name = strdup(directory);
				list->filesCount = namelistcount;
				for (int i = 0; i < namelistcount; i++) {
					DirEntry file = malloc(sizeof(struct sDirEntry));
					list->files[i] = file;

					file->name = strdup(namelist[i]->d_name);
					file->path = dirs_makefullpath(list->name, file->name);

					struct stat buf;
					if (stat(file->path, &buf) == 0) {
						file->uid = buf.st_uid;
						file->gid = buf.st_gid;
						file->size = buf.st_size;
						file->mode = buf.st_mode;
					} else {
						file->uid = 0;
						file->gid = 0;
						file->size = 0;
						file->mode = 0;
					}

					free(namelist[i]);
				}
			} else {
				free(namelist);
				free(list);
				return NULL;
			}
		}

		free(namelist);
		return list;
	}
} // dirs_load

/**
 * Free directory list
 * @param list Directory list
 */
void dirs_freelist(DirList list) {
	if (list == NULL) return;

	free(list->name);
	for (int i = 0; i < list->filesCount; i++) {
		free(list->files[i]->name);
		free(list->files[i]->path);
		free(list->files[i]);
	}
	free(list->files);
	free(list);
} // dirs_freelist

/**
 * Determines whether the file exists.
 * @return true if file exists, false otherwise.
 */
bool file_exists(char *path) {
	return access(path, F_OK) == 0;
} // file_exists

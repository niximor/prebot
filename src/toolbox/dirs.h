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

#ifndef _DIRS_H
# define _DIRS_H 1

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// Forward
typedef struct sDirList *DirList;
typedef struct sDirEntry *DirEntry;

/**
 * Entry of directory (file)
 */
struct sDirEntry {
	char *name;
	char *path;
	uid_t uid;
	gid_t gid;
	off_t size;
	mode_t mode;
}; // sDirEntry

/**
 * List of entrys in directory
 */
struct sDirList {
	char *name;				/**< Directory name */
	DirEntry *files;		/**< Files in directory */
	int filesCount;			/**< Number of files in files array */
}; // sDirList

/**
 * Get current directory name from file name. Return value is dynamically
 * allocated string, so don't forget to free it.
 * @param filename File name
 * @return Path extracted from filename.
 */
extern char *dirs_dirname(char *filename);

/**
 * Get file name without suffix from path.
 * @param path Path
 * @return Allocated string containing only file name without suffix.
 */
extern char *dirs_basename(char *path);

/**
 * Compose full path name from directory and file parts. Return value is
 * dynamically allocated string, so don't forget to free it.
 * @param directory Directory part of the path
 * @param file File path of the path.
 */
extern char *dirs_makefullpath(char *directory, char *file);

/**
 * Load contents of directory into DirList structure.
 * @param directory Directory which to load.
 * @return DirList structure with list of files in directory, or NULL if
 *   cannot read the directory.
 */
extern DirList dirs_load(char *directory);

/**
 * Free directory list
 * @param list Directory list
 */
extern void dirs_freelist(DirList list);

/**
 * Determines whether the file exists.
 * @return true if file exists, false otherwise.
 */
extern bool file_exists(char *path);

#endif

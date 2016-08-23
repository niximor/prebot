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

#ifndef _WORDWRAP_H
# define _WORDWRAP_H 1

#include <string.h>
#include <ctype.h>

#ifdef ww_start
# undef ww_start
#endif

#ifdef ww_end
# undef ww_end
#endif

/**
 * Start of word wrapping block. Everything between ww_start() and ww_end()
 * macros will be called for each output row with correctly created and filled
 * outputOffset and outputSize variables.
 * Remember, that word wrapping block cannot be nested.
 * @param inputString String on which to do wordwrap
 * @param screenSize Size of screen in chars
 * @param outputOffset Offset in original string where the print of line should
 *   begin.
 * @param outputSize Number of characters to print from outputOffset to end of
 *   line. Will always be smaller than screenSize.
 */
#define ww_start(inputString, screenSize, outputOffset, outputSize)           \
	size_t __ww_strLength = strlen(inputString);                              \
	size_t __ww_lastSpacePos = 0;                                             \
	size_t __ww_lastOutputPos = 0;                                            \
	for (size_t __ww_i = 0; __ww_i < __ww_strLength; __ww_i++) {              \
		if (isspace(inputString[__ww_i])) {                                   \
			__ww_lastSpacePos = __ww_i;                                       \
		}                                                                     \
                                                                              \
		if (__ww_i - __ww_lastOutputPos >= screenSize                         \
			|| __ww_i == __ww_strLength - 1) {                                \
                                                                              \
			if (__ww_i == __ww_strLength - 1) __ww_lastSpacePos = __ww_i + 1; \
                                                                              \
			size_t outputSize = 0;                                            \
			if ((int)__ww_lastSpacePos - (int)__ww_lastOutputPos < 0) {       \
				outputSize = screenSize;                                      \
				__ww_lastSpacePos += outputSize;                              \
			} else {                                                          \
				outputSize = __ww_lastSpacePos - __ww_lastOutputPos;          \
			}                                                                 \
                                                                              \
			size_t outputOffset = __ww_lastOutputPos;                         \
                                                                              \
			__ww_lastOutputPos = __ww_lastSpacePos + 1

/**
 * End of word wrapping block.
 */
#define ww_end()                                                              \
		}                                                                     \
	}

#endif

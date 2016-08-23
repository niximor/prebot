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

// Standard interfaces
#include <stdio.h>
#include <stdlib.h>

// This library interface
#include "tb_rand.h"

// Size of random buffer
#define TB_RAND_BUFFSIZE 1024
// Maximum randomly generated number
static const unsigned int TB_RAND_MAX = -1;

/**
 * Generate pseudo-random number in <min;max> interval.
 * @param min Minimum random number
 * @param max Maximum random number
 * @return Random number from <min;max> interval
 */
static int _tb_rand(int min, int max) {
	// Random buffer
	static unsigned int buffer[TB_RAND_BUFFSIZE];
	// Position in random buffer
	static size_t buffpos = TB_RAND_BUFFSIZE - 1;

	// Need to refresh buffer with new random numbers
	if (++buffpos == TB_RAND_BUFFSIZE) {
		// Usind /dev/urandom
		FILE *f = NULL;
		if ((f = fopen("/dev/urandom", "rb")) != NULL) {
			fread(buffer, sizeof(unsigned int), TB_RAND_BUFFSIZE, f);
			fclose(f);
			buffpos = 0;
		} else {
			// If /dev/urandom isn't available, use standard C functions
			for (size_t i = 0; i < TB_RAND_BUFFSIZE; i++) {
				buffer[i] = (rand() / RAND_MAX) * TB_RAND_MAX;
			}
			buffpos = 0;
		}
	}

	return (((double)buffer[buffpos] / (double)TB_RAND_MAX) *
		(double)(max - min + 1)) + min;
} // tb_rand

/**
 * Generate pseudo-random number in <min;max> interval
 * @param min Minimum random number
 * @param max Maximum random number
 * @return Random number from <min;max> interval
 */
int tb_rand(int min, int max) {
	return _tb_rand(min, max);
} // tb_rand

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

// Standard includes
#include <stdbool.h>

// Linux includes
#include <time.h>

// This library interface
#include "timers.h"

// My includes
#include <toolbox/linkedlist.h>
#include <io.h>

// Global timers instance
Timers timers_global;

/**
 * Init timers instance
 * @return Timers intsnce
 */
Timers timers_cinit() {
	Timers result = malloc(sizeof(struct sTimers));
	result->lastTest = time(NULL);
	ll_init(result);

	return result;
} // timers_cinit

/**
 * Add new timer
 * @param timers Timers intstance
 * @param type Timer type
 * @param timeout Timeout or time when the timer should go off
 * @param callback Callback called when timer timed out
 * @param customData Custom data pointer that can be used to deliver some data
 *   to callback function.
 * @return Created timer
 */
Timer timers_cadd(Timers timers, TimerType type, time_t timeout,
	TimerCallback callback, void *customData) {

	Timer timer = malloc(sizeof(struct sTimer));
	timer->type = type;
	timer->setTimeout = timeout;
	timer->currentTimeout = timeout;
	timer->callback = callback;
	timer->customData = customData;
	timer->timers = timers;

	// Add timer to linked list
	ll_append(timers, timer);

	return timer;
} // timers_cadd

/**
 * Remove timer
 * @param timer Timer that should be removed
 */
void timers_remove(Timer timer) {
	// Remove timer from linked list
	ll_remove(timer->timers, timer);

	// Free timer structure
	free(timer);
} // timers_remove

/**
 * Test timers - function to be called in main application loop
 * @param timers Timers instance
 */
void timers_ctest(Timers timers) {
	time_t now = time(NULL);
	ll_loop(timers, timer) {
		switch (timer->type) {
			case TM_TIMEOUT:
				timer->currentTimeout -= (now - timers->lastTest);
				if (timer->currentTimeout <= 0) {
					if (timer->callback != NULL && timer->callback(timer)) {
						timer->currentTimeout = timer->setTimeout;
					} else {
						timers_remove(timer);
					}
				}
				break;

			case TM_AT:
				if (timer->currentTimeout <= now) {
					if (timer->callback != NULL) {
						timer->callback(timer);
					}
					timers_remove(timer);
				}
				break;

			default:
				printError("timers", "Unknown timer type (%d).", timer->type);
				break;
		}
	}
	timers->lastTest = now;
} // timers_ctest

/**
 * Free timers instance
 * @param timers Timers instance
 */
void timers_cfree(Timers timers) {
	ll_loop(timers, timer) {
		free(timer);
	}
	free(timers);
} // timers_cfree

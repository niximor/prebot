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

#ifndef _TIMERS_H
# define _TIMERS_H 1

#include <stdbool.h>

// Forward
typedef struct sTimer *Timer;
typedef struct sTimers *Timers;

/**
 * Timer type
 */
typedef enum {
	TM_TIMEOUT,					/**< Specified number if seconds as timeout */
	TM_AT						/**< Specified exact time when timer should
									 go off */
} TimerType;

/**
 * Timer callback prototype
 * bool TimerCallback(Timer timer)
 * @param timer Timer that timed out
 * @return True if timer should be reset, false if timer should be removed.
 */
typedef bool (*TimerCallback)(Timer);

/**
 * Timer
 */
struct sTimer {
	Timer prev;					/**< Previous timer in chain */
	Timer next;					/**< Next timer in chain */
	TimerType type;				/**< Timer type */
	long int currentTimeout;	/**< Current timeout value */
	long int setTimeout;		/**< Setted timeout value for resetting
									 timer */
	TimerCallback callback;		/**< Callback */
	void *customData;			/**< Custom data that may be used to deliver
									 data to callback. */
	Timers timers;				/**< Timers instance that this timer is member
									 of. */
}; // sTimer

struct sTimers {
	Timer first;				/**< First timer in chain */
	Timer last;					/**< Last timer in chain */
	time_t lastTest;			/**< Last time timers has been tested */
}; // sTimers

// Global timers
extern Timers timers_global;

/**
 * Init global timers
 */
#define timers_init() (timers_global = timers_cinit())

/**
 * Add timer to global timers
 * @param type Timer type
 * @param timeout Timeout or time when timer should go off
 * @param callback Callback called when timer timed out
 * @return Created timer
 */
#define timers_add(type, timeout, callback, customData) \
	timers_cadd(timers_global, type, timeout, callback, customData)

/**
 * Test global timers - function to be called in main application loop
 */
#define timers_test() timers_ctest(timers_global)

/**
 * Free global timers instance
 */
#define timers_free() timers_cfree(timers_global)

/**
 * Init timers instance
 * @return Timers intsnce
 */
extern Timers timers_cinit();

/**
 * Add new timer
 * @param timers Timers intstance
 * @param type Timer type
 * @param timeout Timeout or time when the timer should go off
 * @param callback Callback called when timer timed out
 * @return Created timer
 */
extern Timer timers_cadd(Timers timers, TimerType type, time_t timeout,
	TimerCallback callback, void *customData);

/**
 * Remove timer
 * @param timer Timer that should be removed
 */
extern void timers_remove(Timer timer);

/**
 * Test timers - function to be called in main application loop
 * @param timers Timers instance
 */
extern void timers_ctest(Timers timers);

/**
 * Free timers instance
 * @param timers Timers instance
 */
extern void timers_cfree(Timers timers);

#endif

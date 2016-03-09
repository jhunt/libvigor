/*
  Copyright 2016 James Hunt <james@jameshunt.us>

  This file is part of libvigor.

  libvigor is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  libvigor is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with libvigor.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vigor.h>
#include "impl.h"

/*

    ######## #### ##     ## ########
       ##     ##  ###   ### ##
       ##     ##  #### #### ##
       ##     ##  ## ### ## ######
       ##     ##  ##     ## ##
       ##     ##  ##     ## ##
       ##    #### ##     ## ########

 */

int32_t time_s(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
}

int64_t time_ms(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int64_t) ((int64_t) tv.tv_sec * 1000 + (int64_t) tv.tv_usec / 1000);
}

const char *time_strf(const char *fmt, int32_t s)
{
	static char buf[1024];

	time_t ts = (time_t)s;
	struct tm tm;
	if (!localtime_r(&ts, &tm))
		return NULL;

	if (!fmt) fmt = "%x %X";
	size_t n = strftime(buf, sizeof(buf)-1, fmt, &tm);
	buf[n] = '\0';
	return buf;
}

int sleep_ms(int64_t ms)
{
	struct timespec ts;
	ts.tv_sec  = (time_t)(ms / 1000.0);
	ts.tv_nsec = (long)((ms - (ts.tv_sec * 1000)) * 1000.0 * 1000.0);
	return nanosleep(&ts, NULL);
}
void stopwatch_start(stopwatch_t *clock)
{
	gettimeofday(&clock->tv, NULL);
	clock->running = 1;
}

void stopwatch_stop(stopwatch_t *clock)
{
	clock->running = 0;
	struct timeval end, diff;
	if (gettimeofday(&end, NULL) != 0)
		return;

	if ((end.tv_usec - clock->tv.tv_usec) < 0) {
		diff.tv_sec  = end.tv_sec - clock->tv.tv_sec - 1;
		diff.tv_usec = 1000000 + end.tv_usec-clock->tv.tv_usec;
	} else {
		diff.tv_sec  = end.tv_sec-clock->tv.tv_sec;
		diff.tv_usec = end.tv_usec-clock->tv.tv_usec;
	}

	clock->tv.tv_sec  = diff.tv_sec;
	clock->tv.tv_usec = diff.tv_usec;
}

uint32_t stopwatch_s(const stopwatch_t *clock)
{
	return clock->tv.tv_sec;
}

uint64_t stopwatch_ms(const stopwatch_t *clock)
{
	return clock->tv.tv_sec  * 1000
	     + clock->tv.tv_usec / 1000;
}

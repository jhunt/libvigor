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

#include "test.h"

TESTS {
	alarm(5);
	setenv("TZ", "UTC", 1);

	subtest {
		stopwatch_t T;
		int64_t ms;

		STOPWATCH(&T, ms) {
			sleep_ms(120);
		}
		ok(stopwatch_s(&T) == 0, "0s elapsed in 120ms sleep");
		ok(stopwatch_ms(&T) >= 120, "at least 120ms elapsed");
		ok(stopwatch_ms(&T) <= 140, "no more than 140ms elapsed");
		is_int(ms, stopwatch_ms(&T), "milliseconds returned via STOPWATCH macro");

		STOPWATCH(&T, ms) {
			sleep_ms(980);
		}
		ok(stopwatch_s(&T) == 0, "0s elapsed in 980ms sleep");
		ok(stopwatch_ms(&T) >= 980, "at least 980ms elapsed");
		ok(stopwatch_ms(&T) <= 999, "no more than 999ms elapsed");
	}

	subtest {
		int32_t a, b;
		uint64_t c, d;

		a = time_s(); c = time_ms();
		sleep_ms(1100);
		b = time_s(); d = time_ms();

		ok(a < b, "time_s() returns the current time");
		ok(c < d, "time_ms() returns the current time (ms)");
	}

	subtest {
		int32_t ts = 1234567890;
		is_string(time_strf("[[%a, %b %d %Y at %H:%M:%S%P]]", ts),
			"[[Fri, Feb 13 2009 at 23:31:30pm]]",
			"time formatting works");
	}

	alarm(0);
	done_testing();
}

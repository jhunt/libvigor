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
	subtest { /* basic instantiation */
		hb_t heartbeat; memset(&heartbeat, 0x42, sizeof(heartbeat));
		ok(hb_init(&heartbeat, 1000, 4, 1.5, 2000), "Initialized a heartbeat structure");

		is_int(heartbeat.interval,  1000, "set interval properly");
		is_int(heartbeat.freshness, 4,    "set freshness properly");
		is_int(heartbeat.max_delay, 2000, "set max_delay properly");
		ok(heartbeat.backoff ==     1.5,  "set backoff properly");

		is_int(heartbeat.missed, 0, "'missed' is initially set to zero");
		is_int(heartbeat.delay,  1000, "'delay' is initially set to the interval");
	}

	subtest { /* aliveness check */
		hb_t heartbeat;
		hb_init(&heartbeat, 10, 1, 1.5, 40);

		ok( hb_alive(&heartbeat), "heartbeat is initially OK");
		ok( hb_miss(&heartbeat),  "heartbeat is still OK after 1 miss (per hb_miss())");
		ok( hb_alive(&heartbeat), "heartbeat is still OK after 1 miss (per hb_alive())");
		ok(!hb_miss(&heartbeat),  "heartbeat is not OK after 2 misses (per hb_miss())");
		ok(!hb_alive(&heartbeat), "heartbeat is not OK after 2 misses (per hb_alive())");
		ok(!hb_miss(&heartbeat),  "heartbeat is not OK after 3 misses (per hb_miss())");
		ok(!hb_alive(&heartbeat), "heartbeat is not OK after 3 misses (per hb_alive())");

		ok( hb_ping(&heartbeat),  "heartbeat ping");
		ok( hb_alive(&heartbeat), "heartbeat is OK after ping");
		ok( hb_miss(&heartbeat),  "heartbeat is still OK after 1 miss (per hb_miss())");
		ok( hb_alive(&heartbeat), "heartbeat is still OK after 1 miss (per hb_alive())");
		ok(!hb_miss(&heartbeat),  "heartbeat is not OK after 2 misses (per hb_miss())");
		ok(!hb_alive(&heartbeat), "heartbeat is not OK after 2 misses (per hb_alive())");
		ok(!hb_miss(&heartbeat),  "heartbeat is not OK after 3 misses (per hb_miss())");
		ok(!hb_alive(&heartbeat), "heartbeat is not OK after 3 misses (per hb_alive())");
	}

	subtest { /* delay / backoff algorithm */
		hb_t heartbeat;
		hb_init(&heartbeat, 10, 20, 2.0, 96);

		ok( hb_alive(&heartbeat), "heartbeat is initially OK");
		is_int(heartbeat.delay, 10, "initial delay is 10s");

		hb_miss(&heartbeat);
		is_int(heartbeat.delay, 20, "first miss causes 2x delay (10 -> 20)");

		hb_miss(&heartbeat);
		is_int(heartbeat.delay, 40, "second miss causes another 2x delay (20 -> 40)");

		hb_miss(&heartbeat);
		is_int(heartbeat.delay, 80, "third miss causes another 2x delay (40 -> 80)");

		hb_miss(&heartbeat);
		is_int(heartbeat.delay, 96, "fourth miss exceeds max_delay (80 -> 96)");

		hb_miss(&heartbeat);
		is_int(heartbeat.delay, 96, "fifth miss exceeds max_delay (no change)");

		hb_ping(&heartbeat);
		is_int(heartbeat.delay, 10, "ping resets delay to initial interval");
	}

	subtest { /* sleep / expiry */
		hb_t heartbeat;
		hb_init(&heartbeat, 100, 3, 1.5, 3000);

		ok( hb_alive(&heartbeat), "heartbeat is initially OK");

		int64_t t0, t1;
		t0 = time_ms();
		hb_sleep(&heartbeat);
		t1 = time_ms();
		ok(t1 - t0 >= 100, "at least 100ms pass during hb_sleep()");
	}

	alarm(0);
	done_testing();
}

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

	subtest {
		pid_t pid = fork();
		if (pid < 0)
			BAIL_OUT("Failed to fork!");

		if (pid == 0) {
			sleep_ms(500);
			kill(getppid(), SIGINT);
			exit(0);
		}

		signal_handlers();
		sleep(60);
		ok(signalled(), "signalled during our 60s nap");
		ok(!signalled(), "not signalled anymore");
	}

	alarm(0);
	done_testing();
}

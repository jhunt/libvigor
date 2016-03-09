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
		char  val[12] = {0};
		char *buf = vmalloc(12);
		isnt_null(buf, "vmalloc() returned a non-NULL pointer");
		ok(memcmp(val, buf, 12) == 0, "vmalloc() zeros out allocated memory");
	}

	subtest {
		char  val[12] = {0};
		char *buf = vcalloc(12, sizeof(char));
		isnt_null(buf, "vcalloc() returned a non-NULL pointer");
		ok(memcmp(val, buf, 12) == 0, "vcalloc() zeros out allocated memory");
	}

	subtest { /* out-of-memory condition */
		pid_t pid = fork();
		if (pid < 0) BAIL_OUT("Failed to fork; unable to run setrlimit/oom test");

		if (pid > 0) {
			int status;
			waitpid(pid, &status, 0);
			ok(WIFEXITED(status), "child process exited of its own free will");
			is_int(WEXITSTATUS(status), 42, "child OOMed and exited 42 (from vmalloc)");
		} else {
			/* artifically constrain our memory usage */
			struct rlimit rlim;
			if (getrlimit(RLIMIT_AS, &rlim) != 0) exit(0);
			rlim.rlim_cur = rlim.rlim_max = 10240000; /* ~10MiB */
			if (setrlimit(RLIMIT_AS, &rlim) != 0) exit(0);

			/* this should kill us */
			vmalloc(rlim.rlim_cur + 1);
			exit(0);
		}
	}

	alarm(0);
	done_testing();
}

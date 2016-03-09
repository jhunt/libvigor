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
		proc_t ps;
		is_int(proc_stat(getpid(), &ps), 0, "Got process status");
		is_int(ps.pid,  getpid(),  "PID returned in proc_t structure");
		is_int(ps.ppid, getppid(), "PPID returned in proc_t structure");

		if (!getenv("FAKEROOTKEY")) {
			/* don't run these under fakeroot; /proc/$$/status and
			   libfakeroot disagree about uid/gid/euid/egid...     */
			is_int(ps.uid,  getuid(),  "UID returned in proc_t structure");
			is_int(ps.gid,  getgid(),  "GID returned in proc_t structure");

			is_int(ps.euid, geteuid(), "EUID returned in proc_t structure");
			is_int(ps.egid, getegid(), "EGID returned in proc_t structure");
		}

		pid_t child = fork();
		if (child == 0) exit(0);
		waitpid(child, NULL, 0);
		is_int(proc_stat(child, &ps), -1,
			"Failed to get process status for a dead process");
	}

	alarm(0);
	done_testing();
}

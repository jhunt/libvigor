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
		struct stat st;
		unlink(TEST_TMP "/touched");
		ok(stat(TEST_TMP "/touched", &st) != 0,
			TEST_TMP "/touched does not exist yet");

		is_int(run("/bin/touch", TEST_TMP "/touched", NULL), 0,
			"`touch` command exited successfully");

		ok(stat(TEST_TMP "/touched", &st) == 0,
			TEST_TMP "/touched exists after run() call");
	}

	subtest { /* UID / GID tests */
		if (geteuid() != 0) {
			diag("skipping UID/GID tests");

		} else {
			mkdir("t/tmp", 0777);
			FILE *bin = fopen("t/tmp/ids", "w");
			if (!bin) BAIL_OUT("unable to create t/tmp/ids script!");
			fprintf(bin, "#!/usr/bin/perl\n$g=$);$g=~s/ .*//;\nprint $>,':',$g,$/;\n");
			fchmod(fileno(bin), 0777);
			fclose(bin);

			char buf[8192];
			runner_t runner = {
				.in  = NULL,
				.out = tmpfile(),
				.err = tmpfile(),
				.uid = 12345,
				.gid = 40404,
			};

			is_int(run2(&runner, "t/tmp/ids", NULL), 0,
				"Ran ID (t/tmp/ids) utility with valid IDs");
			rewind(runner.out);
			isnt_null(fgets(buf, 8191, runner.out), "Read line 1 of output");
			is_string(buf, "12345:40404\n", "t/tmp/ids output (line 1)");

			rewind(runner.err);
			is_null(fgets(buf, 8191, runner.err), "no stderr");
			ok(feof(runner.err), "stderr EOF");
		}
	}

	subtest { /* io pipes */
		mkdir("t/tmp", 0777);
		FILE *bin = fopen("t/tmp/calc", "w");
		if (!bin) BAIL_OUT("unable to create t/tmp/calc script!");
		fprintf(bin, "#/bin/bash\nread A; read B\necho $(expr $A \\* $B)\n");
		fchmod(fileno(bin), 0777);
		fclose(bin);

		runner_t runner = {
			.in  = tmpfile(),
			.out = tmpfile(),
			.err = tmpfile(),
			.uid = 0,
			.gid = 0,
		};
		fprintf(runner.in, "6\n9\n");
		rewind(runner.in);

		is_int(run2(&runner, "t/tmp/calc", NULL), 0,
			"Ran desk calculator (t/tmp/calc) utility");

		rewind(runner.out);
		char buf[8192];
		isnt_null(fgets(buf, 8191, runner.out), "Read line 1 of output");
		is_string(buf, "54\n", "t/tmp/calc output (line 1)");

		rewind(runner.err);
		is_null(fgets(buf, 8191, runner.err), "no stderr");
		ok(feof(runner.err), "stderr EOF");
	}

	alarm(0);
	done_testing();
}

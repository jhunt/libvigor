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
		lock_t L;
		lock_init(&L, TEST_TMP "/test.lock");

		ok(!L.valid, "initial lock is invalid");
		is_string(lock_info(&L), "<invalid lock file>",
			"string representation of an invalid lock");

		ok(lock_acquire(&L, 0) == 0, "Acquired lock");

		lock_t L2;
		lock_init(&L2, TEST_TMP "/test.lock");
		ok(lock_acquire(&L2, 0) != 0, "Failed to acquire a held lock");

		ok(lock_release(&L) == 0, "Released lock");
		ok(lock_release(&L) != 0, "Cannot release an un-held lock");

		ok(lock_acquire(&L2, 0) == 0, "Acquired newly-released lock");
	}

	alarm(0);
	done_testing();
}

/*
  Copyright 2014 James Hunt <james@jameshunt.us>

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
}

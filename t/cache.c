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

int global_counter = 0;
void destroyer(void *d)
{
	global_counter++;
}

TESTS {
	alarm(5);
	subtest { /* basic types */
		cache_t *cc = cache_new(4, 20);
		isnt_null(cc, "cache_new created a new cache");
		is_int(cc->max_len,  4, "Set cc->max_len properly");
		is_int(cc->expire,  20, "Set cc->expire properly");

		cache_purge(cc, 0);
		ok(1, "cache_purge() on an empty cache doesn't crash");

		cache_purge(cc, 1);
		ok(1, "cache_purge(force) on an empty cache doesn't crash");

		is_null(cache_get(cc, "xyzzy"),
				"No connection in the cache ident'd as 'xyzzy'");
		isnt_null(cache_set(cc, "xyzzy", (void*)42),
			"Inserted 'xyzzy'");
		isnt_null(cache_set(cc, "foobar", (void*)3),
			"Inserted 'foobar'");
		isnt_null(cache_get(cc, "xyzzy"),
			"Found 'xyzzy'");
		isnt_null(cache_get(cc, "foobar"),
			"Found 'foobar'");
		is_null(cache_get(cc, "FooBar"),
			"Cache retrieval is case-sensitive");
		isnt_null(cache_unset(cc, "foobar"),
			"Unset 'foobar'");
		is_null(cache_get(cc, "foobar"),
			"'foobar' is gone now");

		cache_purge(cc, 0);
		isnt_null(cache_get(cc, "xyzzy"),
			"'xyzzy' survives the first cache purge");
		int life = -2;
		is_int(cache_setopt(cc, VIGOR_CACHE_EXPIRY, &life), 0,
			"Set new minimum lifetime via cache_setopt");
		is_int(cc->expire, -2, "cc->expire updated");
		cache_purge(cc, 0);
		is_null(cache_get(cc, "xyzzy"),
			"'xyzzy' purged in the second cache purge");

		cache_free(cc);
	}

	subtest { /* destroy callback */
		cache_t *cc = cache_new(4, -1);
		isnt_null(cc, "cache_new created a new cache");
		is_int(cache_setopt(cc, VIGOR_CACHE_DESTRUCTOR, destroyer), 0,
			"Set destroyer() as the destroy callback");

		isnt_null(cache_set(cc, "key1", (void*)1), "key1 inserted");
		isnt_null(cache_set(cc, "key2", (void*)2), "key2 inserted");
		isnt_null(cache_set(cc, "key3", (void*)3), "key3 inserted");

		is_int(global_counter, 0, "global counter starts at 0");
		destroyer(NULL);
		is_int(global_counter, 1, "global counter increments on destroy()");
		cache_purge(cc, 0);
		is_int(global_counter, 4, "global counter fired 4 times");

		cache_free(cc);
	}

	subtest { /* for_each_cache_key */
		cache_t *cc = cache_new(4, 20);
		isnt_null(cc, "cache_new created a new cache");

		isnt_null(cache_set(cc, "xyzzy",  (void*)4), "Inserted 'xyzzy'");
		isnt_null(cache_set(cc, "foobar", (void*)3), "Inserted 'foobar'");

		char *key;
		int i = 0;
		for_each_cache_key(cc,key) {
			i++;
			ok(strcmp(key, "xyzzy") == 0 || strcmp(key, "foobar") == 0,
				"Found key 'xyzzy'/'foobar'");
		}
		is_int(i, 2, "found 2 keys");

		cache_free(cc);
	}

	subtest { /* resizing caches */
		cache_t *cc = cache_new(4, 20);
		is_int(cc->max_len,  4, "max_len is initially 4");
		is_int(cc->expire,  20, "expire is initially 20s");

		ok(cache_resize(&cc, 10) == 0, "resized cache from 4 to 10 entries");
		is_int(cc->max_len, 10, "max_len is now 10");
		is_int(cc->expire,  20, "expire is still 20s");

		ok(cache_resize(&cc, 1) != 0, "can't reduce cache size");

		cache_free(cc);
	}

	subtest { /* too much! */
		cache_t *cc = cache_new(4, 20);

		isnt_null(cache_set(cc, "key1", (void*)1), "key1 inserted");
		isnt_null(cache_set(cc, "key2", (void*)2), "key2 inserted");
		isnt_null(cache_set(cc, "key3", (void*)3), "key3 inserted");
		isnt_null(cache_set(cc, "key4", (void*)4), "key4 inserted");

		is_null(cache_set(cc, "key5", (void*)5), "key5 is too much");

		ok(cache_resize(&cc, 128) == 0, "extended cache from 4 slots to 128 slots");
		isnt_null(cache_set(cc, "key5", (void*)5), "key5 inserted (post-tune)");

		cache_free(cc);
	}

	subtest { /* touch */
		cache_t *cc = cache_new(4, 1);

		isnt_null(cache_set(cc, "key1", (void*)1), "key1 inserted");
		isnt_null(cache_set(cc, "key2", (void*)2), "key2 inserted");
		isnt_null(cache_set(cc, "key3", (void*)3), "key3 inserted");
		cache_touch(cc, "key1", time_s() + 301);
		cache_touch(cc, "key3",               300);

		cache_purge(cc, 0);
		isnt_null(cache_get(cc, "key1"), "key1 still alive and well");
		isnt_null(cache_get(cc, "key2"), "key2 still alive and well");
		  is_null(cache_get(cc, "key3"), "key3 expired (ts 300 is in the past)");

		diag("Sleeping for 3s");
		sleep_ms(3000);
		cache_purge(cc, 0);

		isnt_null(cache_get(cc, "key1"), "key1 still alive and well");
		  is_null(cache_get(cc, "key2"), "key2 expired after 1s");

		cache_free(cc);
	}

	subtest { /* full vs. empty */
		cache_t *cc = cache_new(2, 1);
		ok( cache_isempty(cc), "new cache isempty()");
		ok(!cache_isfull(cc), "new cache !isfull()");

		isnt_null(cache_set(cc, "key1", (void*)1), "key1 inserted");
		ok(!cache_isempty(cc), "half-full cache !isempty()");
		ok(!cache_isfull(cc), "half-full cache !isfull()");

		isnt_null(cache_set(cc, "key2", (void*)2), "key2 inserted");
		ok(!cache_isempty(cc), "full cache !isempty()");
		ok( cache_isfull(cc), "full cache !isfull()");

		cache_free(cc);
	}

	alarm(0);
	done_testing();
}

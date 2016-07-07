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
		hash_t *h;
		char *path = strdup("/some/path/some/where");
		char *name = strdup("staff");

		isnt_null(h = vmalloc(sizeof(hash_t)), "hash_new -> pointer");

		/* note: "path" and "group" both hash to 62 */
		is_null(hash_get(h, "path"), "can't get 'path' prior to set");
		is_null(hash_get(h, "name"), "can't get 'name' prior to set");

		ok(hash_set(h, "path", path) == path, "set h.path");
		ok(hash_set(h, "name", name) == name, "set h.name");

		is_string(hash_get(h, "path"), path, "get h.path");
		is_string(hash_get(h, "name"), name, "get h.name");

		hash_done(h, 0);
		free(h);
		free(path);
		free(name);
	}

	subtest {
		hash_t *h;
		char *path  = strdup("/some/path/some/where");
		char *group = strdup("staff");

		isnt_null(h = vmalloc(sizeof(hash_t)), "hash_new -> pointer");

		is_null(hash_get(h, "path"),  "get h.path (initially null)");
		is_null(hash_get(h, "group"), "get h.group (initially null)");

		ok(hash_set(h, "path",  path)  == path,  "set h.path");
		ok(hash_set(h, "group", group) == group, "set h.group");

		is_string(hash_get(h, "path"),  path,  "get h.path");
		is_string(hash_get(h, "group"), group, "get h.group");

		hash_done(h, 0);
		free(h);
		free(path);
		free(group);
	}

	subtest {
		hash_t *h;
		char *value1 = strdup("value1");
		char *value2 = strdup("value2");

		isnt_null(h = vmalloc(sizeof(hash_t)), "hash_new -> pointer");

		ok(hash_set(h, "key", value1) == value1, "first hash_set for overrides");
		is_string(hash_get(h, "key"), value1, "hash_get of first value");

		// hash_set returns previous value on override
		ok(hash_set(h, "key", value2) == value1, "second hash_set for overrides");
		is_string(hash_get(h, "key"), value2, "hash_get of second value");

		hash_done(h, 1);
		free(h);
		free(value1);
	}

	subtest {
		hash_t *h;
		char *foo = strdup("foo");
		char *bar = strdup("bar");

		isnt_null(h = vmalloc(sizeof(hash_t)), "hash_new -> pointer");

		/* note: "path" and "group" both hash to 62; this also tests collision. */
		ok(hash_set(h, "path", foo) == foo, "hash_set(path = foo)");
		ok(hash_get(h, "path") == foo, "hash_get(path) == foo");
		ok(hash_set(h, "group", bar) == bar, "hash_set(group = bar)");
		ok(hash_get(h, "group") == bar, "hash_get(group) == bar");

		ok(hash_unset(h, "path") == foo, "hash_unset(path) == foo");
		ok(hash_get(h, "path") == NULL, "hash_get(path) == NULL after unset call");
		ok(hash_get(h, "group") == bar, "hash_get(group) == bar after unset call");

		hash_done(h, 1);
		free(h);
		free(foo);
	}

	subtest {
		hash_t *h = NULL;

		is_null(hash_get(NULL, "test"), "hash_get NULL hash");

		isnt_null(h = vmalloc(sizeof(hash_t)), "hash_new -> pointer");
		hash_set(h, "test", "valid");

		is_null(hash_get(h, NULL), "hash_get NULL key");
		hash_done(h, 0);
		free(h);
	}

	subtest {
		hash_t *h;
		char *foo = strdup("foo");
		char *bar = strdup("bar");

		isnt_null(h = vmalloc(sizeof(hash_t)), "hash_new -> pointer");
		hash_set(h, "foo", foo);
		hash_set(h, "bar", bar);
		ok(hash_len(h) == 2, "hash length is 2 after 2 elements");
		hash_unset(h, "foo");
		ok(hash_len(h) == 1, "hash length is decrimetted after unset");

		hash_done(h, 0);
		free(h);
		free(foo);
		free(bar);
	}

	subtest {
		hash_t *h = vmalloc(sizeof(hash_t));
		char *key, *value;

		int saw_promise = 0;
		int saw_snooze  = 0;
		int saw_central = 0;
		int saw_bridge  = 0;

		hash_set(h, "promise", "esimorp");
		hash_set(h, "snooze",  "ezoons");
		hash_set(h, "central", "lartnec");
		hash_set(h, "bridge",  "egdirb");

		for_each_key_value(h, key, value) {
			if (strcmp(key, "promise") == 0) {
				saw_promise++;
				is_string(value, "esimorp", "h.promise");

			} else if (strcmp(key, "snooze") == 0) {
				saw_snooze++;
				is_string(value, "ezoons", "h.snooze");

			} else if (strcmp(key, "central") == 0) {
				saw_central++;
				is_string(value, "lartnec", "h.central");

			} else if (strcmp(key, "bridge") == 0) {
				saw_bridge++;
				is_string(value, "egdirb", "h.bridge");

			} else {
				fail("Unexpected value found during for_each_key_value");
			}
		}
		hash_done(h, 0);
		free(h);

		is_int(saw_promise, 1, "saw the promise key only once");
		is_int(saw_snooze,  1, "saw the snooze key only once");
		is_int(saw_central, 1, "saw the central key only once");
		is_int(saw_bridge,  1, "saw the bridge key only once");
	}

	subtest {
		hash_t a, b;
		memset(&a, 0, sizeof(a));
		memset(&b, 0, sizeof(b));

		hash_set(&a, "only-in-a", "this value");
		hash_set(&a, "common",    "shared from A");
		is_string(hash_get(&a, "only-in-a"), "this value",    "only-in-a == 'this value'");
		is_string(hash_get(&a, "common"),    "shared from A", "common    == 'shared from A'");
		is_null(hash_get(&a, "only-in-b"), "only-in-b not set in A pre-merge");

		hash_set(&b, "only-in-b", "that value");
		hash_set(&b, "common",    "shared from B");
		is_string(hash_get(&b, "only-in-b"), "that value",    "only-in-b == 'that value'");
		is_string(hash_get(&b, "common"),    "shared from B", "common    == 'shared from B'");
		is_null(hash_get(&b, "only-in-a"), "only-in-a not set in B pre-merge");

		hash_merge(&a, &b);
		is_string(hash_get(&a, "only-in-a"), "this value",    "only-in-a == 'this value'");
		is_string(hash_get(&a, "only-in-b"), "that value",    "only-in-b == 'that value'");
		is_string(hash_get(&a, "common"),    "shared from B", "common    == 'shared from B'");

		hash_done(&a, 0);
		hash_done(&b, 0);
	}

	subtest {
		hash_t h;
		memset(&h, 0, sizeof(h));

		char key[4096];
		memset(key, '.', 4096);
		key[0] = 'A';
		key[4095] = 'Z';

		hash_set(&h, key, "some value");
		char *k, *v;

		ok(hash_get(&h, key), "retrieved 4kb key");
		size_t n = 0;
		for_each_key_value(&h, k, v) {
			n++;
			is_string(k, key, "retrieved full key from keyval traversal");
			is_string(v, "some value", "retrieved value");
		}
		is_int(n, 1, "only found one key/value");

		hash_done(&h, 0);
	}

	alarm(0);
	done_testing();
}

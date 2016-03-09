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
		CONFIG(c);

		ok(!config_isset(&c, "foo"), "config[foo] not set yet");
		ok(!config_isset(&c, "bar"), "config[bar] not set yet");
		ok(!config_isset(&c, "baz"), "config[baz] not set yet");
		is_null(config_get(&c, "foo"), "config[foo] can't be retrieved");

		config_set(&c, "foo", "f00");
		ok( config_isset(&c, "foo"), "config[foo] is set now");
		ok(!config_isset(&c, "bar"), "config[bar] not set yet");
		ok(!config_isset(&c, "baz"), "config[baz] not set yet");
		is_string(config_get(&c, "foo"), "f00", "config[foo]'s first value");

		config_set(&c, "foo", "FOO!");
		ok( config_isset(&c, "foo"), "config[foo] is set");
		ok(!config_isset(&c, "bar"), "config[bar] not set yet");
		ok(!config_isset(&c, "baz"), "config[baz] not set yet");
		is_string(config_get(&c, "foo"), "FOO!", "config[foo]'s second value");

		config_set(&c, "bar", "fubar");
		ok( config_isset(&c, "foo"), "config[foo] is set");
		ok( config_isset(&c, "bar"), "config[bar] is set now");
		ok(!config_isset(&c, "baz"), "config[baz] not set yet");
		is_string(config_get(&c, "foo"), "FOO!",  "config[foo]'s second value");
		is_string(config_get(&c, "bar"), "fubar", "config[bar]'s first value");

		config_unset(&c, "bar");
		ok( config_isset(&c, "foo"), "config[foo] is set");
		ok(!config_isset(&c, "bar"), "config[bar] no longer set");
		ok(!config_isset(&c, "baz"), "config[baz] not set yet");
		is_null(config_get(&c, "bar"), "config[bar] can't be retrieved");

		config_done(&c);
	}

	subtest {
		CONFIG(c);
		config_set(&c, "foo", "bar");

		FILE *io = tmpfile();
		fprintf(io, "# this is a comment\n");
		fprintf(io, "path default\n");
		fprintf(io, "   \n");
		fprintf(io, "\tport 5155   \n");
		fprintf(io, "a b c d e  \n");
		fprintf(io, "#eof\n");

		rewind(io);
		is_int(config_read(&c, io), 0, "read from tmpfile");

		is_string(config_get(&c, "path"), "default", "config[path]");
		is_string(config_get(&c, "port"), "5155",    "config[port]");
		is_string(config_get(&c, "a"),    "b c d e", "config[a]");
		is_string(config_get(&c, "foo"),  "bar",     "config[foo] is still set");
		is_null(  config_get(&c, "debug"),           "config[debug] is not set");

		config_set(&c, "path", "/var");
		config_set(&c, "path", "/opt");
		config_set(&c, "debug", "yes");

		rewind(io);
		is_int(config_write(&c, io), 0, "wrote to tmpfile");
		config_done(&c);

		rewind(io);
		is_int(config_read(&c, io), 0, "read from tmpfile");
		is_string(config_get(&c, "path"), "/opt", "config[path] changed");
		is_string(config_get(&c, "port"), "5155", "config[port] is still 5515");
		is_string(config_get(&c, "debug"), "yes", "config[debug] is set");
		is_string(config_get(&c, "foo"),   "bar", "config[foo] is still set");
		config_done(&c);
	}

	subtest { /* comments */
		CONFIG(c);

		FILE *io = tmpfile();
		fprintf(io, "# this is a comment\n");
		fprintf(io, "a default          # trailing comment\n");
		fprintf(io, "b 4 8 15 16 23 42  # trailing comment\n");

		rewind(io);
		is_int(config_read(&c, io), 0, "read from tmpfile");

		is_string(config_get(&c, "a"), "default",         "config[a]");
		is_string(config_get(&c, "b"), "4 8 15 16 23 42", "config[b]");
		config_done(&c);
	}

	alarm(0);
	done_testing();
}

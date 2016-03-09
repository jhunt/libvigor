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

static int OLD_FD;
FILE* redirect(void)
{
	FILE *io = tmpfile();
	if (!io)
		BAIL_OUT("Failed to generate a tempfile for IO redirection");

	OLD_FD = dup(STDERR_FILENO);

	if (dup2(fileno(io), STDERR_FILENO) < 0)
		BAIL_OUT("Failed to reopen stderr for testing");

	return io;
}

void restore(FILE *io)
{
	rewind(io);
	if (dup2(OLD_FD, STDERR_FILENO) < 0)
		BAIL_OUT("Failed to restore stderr to previous stream");
	OLD_FD = -1;
}

#define WRAPPED_IO(io) for (io = redirect(); OLD_FD >= 0; restore(io))

TESTS {
	alarm(5);
	pid_t pid = getpid();
	FILE *io;

	subtest {
		WRAPPED_IO(io) {
			log_open("vigor", "stderr");
			log_level(LOG_DEBUG, NULL);

			logger(LOG_EMERG,   "this is an EMERGENCY %i", 1017);
			logger(LOG_ALERT,   "ALERT!! ALERT!!");
			logger(LOG_CRIT,    "this is CRITICAL");
			logger(LOG_ERR,     "this is an ERROR");
			logger(LOG_WARNING, "WARNING!! WARNING!!");
			logger(LOG_NOTICE,  "notice anything?");
			logger(LOG_INFO,    "this is an INFOmercial");
			logger(LOG_DEBUG,   "debug %x-%x", 57005, 48879);
		}

		char buf[8192], expect[8192];

		isnt_null(fgets(buf, 8192, io), "read line 1");
		snprintf(expect, 8192, "vigor[%i] %s\n", (int)pid, "this is an EMERGENCY 1017");
		is_string(buf, expect, "line 1 (LOG_EMERG)");

		isnt_null(fgets(buf, 8192, io), "read line 2");
		snprintf(expect, 8192, "vigor[%i] %s\n", (int)pid, "ALERT!! ALERT!!");
		is_string(buf, expect, "line 2 (LOG_ALERT)");

		isnt_null(fgets(buf, 8192, io), "read line 3");
		snprintf(expect, 8192, "vigor[%i] %s\n", (int)pid, "this is CRITICAL");
		is_string(buf, expect, "line 3 (LOG_CRIT)");

		isnt_null(fgets(buf, 8192, io), "read line 4");
		snprintf(expect, 8192, "vigor[%i] %s\n", (int)pid, "this is an ERROR");
		is_string(buf, expect, "line 4 (LOG_ERR)");

		isnt_null(fgets(buf, 8192, io), "read line 5");
		snprintf(expect, 8192, "vigor[%i] %s\n", (int)pid, "WARNING!! WARNING!!");
		is_string(buf, expect, "line 5 (LOG_WARNING)");

		isnt_null(fgets(buf, 8192, io), "read line 6");
		snprintf(expect, 8192, "vigor[%i] %s\n", (int)pid, "notice anything?");
		is_string(buf, expect, "line 6 (LOG_NOTICE)");

		isnt_null(fgets(buf, 8192, io), "read line 7");
		snprintf(expect, 8192, "vigor[%i] %s\n", (int)pid, "this is an INFOmercial");
		is_string(buf, expect, "line 7 (LOG_INFO)");

		isnt_null(fgets(buf, 8192, io), "read line 8");
		snprintf(expect, 8192, "vigor[%i] %s\n", (int)pid, "debug dead-beef");
		is_string(buf, expect, "line 8 (LOG_DEBUG)");

		is_null(fgets(buf, 8192, io), "EOF");
	}

	subtest { /* log level filtering */
		WRAPPED_IO(io) {
			log_open("vigor", "stderr");
			log_level(0, "warning");

			logger(LOG_DEBUG,   "LOG_DEBUG");
			logger(LOG_INFO,    "LOG_INFO");
			logger(LOG_NOTICE,  "LOG_NOTICE");
			logger(LOG_WARNING, "LOG_WARNING");
			logger(LOG_ERR,     "LOG_ERR");
			logger(LOG_CRIT,    "LOG_CRIT");
			logger(LOG_ALERT,   "LOG_ALERT");
			logger(LOG_EMERG,   "LOG_EMERG");
		}

		char buf[8192], expect[8192];

		isnt_null(fgets(buf, 8192, io), "read line");
		snprintf(expect, 8192, "vigor[%i] %s\n", (int)pid, "LOG_WARNING");
		is_string(buf, expect, "LOG_WARNING logged");
	}

	subtest { /* log level set + get */
		is_int(log_level_number("emerg"),     LOG_EMERG,   "emerg == LOG_EMERG");
		is_int(log_level_number("emergency"), LOG_EMERG,   "emergency == LOG_EMERG");
		is_int(log_level_number("alert"),     LOG_ALERT,   "alert == LOG_ALERT");
		is_int(log_level_number("crit"),      LOG_CRIT,    "crit == LOG_CRIT");
		is_int(log_level_number("critical"),  LOG_CRIT,    "critical == LOG_CRIT");
		is_int(log_level_number("err"),       LOG_ERR,     "err == LOG_ERR");
		is_int(log_level_number("error"),     LOG_ERR,     "error == LOG_ERR");
		is_int(log_level_number("warn"),      LOG_WARNING, "warn == LOG_WARNING");
		is_int(log_level_number("warning"),   LOG_WARNING, "warning == LOG_WARNING");
		is_int(log_level_number("notice"),    LOG_NOTICE,  "notice == LOG_NOTICE");
		is_int(log_level_number("info"),      LOG_INFO,    "info == LOG_INFO");
		is_int(log_level_number("debug"),     LOG_DEBUG,   "debug == LOG_DEBUG");
		is_int(log_level_number("whatever"),  -1,          "unknown log level == -1");

		is_string(log_level_name(LOG_EMERG),   "emergency", "LOG_EMERG == emergency");
		is_string(log_level_name(LOG_ALERT),   "alert",     "LOG_ALERT == alert");
		is_string(log_level_name(LOG_CRIT),    "critical",  "LOG_CRIT == critical");
		is_string(log_level_name(LOG_ERR),     "error",     "LOG_ERR == error");
		is_string(log_level_name(LOG_WARNING), "warning",   "LOG_WARNING == warning");
		is_string(log_level_name(LOG_NOTICE),  "notice",    "LOG_NOTICE == notice");
		is_string(log_level_name(LOG_INFO),    "info",      "LOG_INFO == info");
		is_string(log_level_name(LOG_DEBUG),   "debug",     "LOG_DEBUG == debug");
	}

	log_close();
	alarm(0);
	done_testing();
}

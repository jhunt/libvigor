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

#include <vigor.h>
#include "impl.h"

static int s_lock_details(lock_t *lock)
{
	lock->valid = 0;

	FILE *file = fopen(lock->path, "r");
	if (!file) return 1;

	int count = fscanf(file, "LOCK p%u u%u t%i\n",
		&lock->pid, &lock->uid, &lock->time);
	fclose(file);
	if (count != 3) return 1;

	lock->valid = 1;
	return 0;
}

/*

    ##        #######   ######  ##    ##  ######
    ##       ##     ## ##    ## ##   ##  ##    ##
    ##       ##     ## ##       ##  ##   ##
    ##       ##     ## ##       #####     ######
    ##       ##     ## ##       ##  ##         ##
    ##       ##     ## ##    ## ##   ##  ##    ##
    ########  #######   ######  ##    ##  ######

 */

void lock_init(lock_t *lock, const char *path)
{
	assert(lock); // LCOV_EXCL_LINE
	lock->path  = path;
	lock->valid =  0;
	lock->fd    = -1;
	lock->pid   =  0;
	lock->uid   =  0;
	lock->time  = -1;
}

int lock_acquire(lock_t *lock, int flags)
{
	assert(lock); // LCOV_EXCL_LINE
	assert(lock->path);

	lock->valid = 0;
	lock->fd = open(lock->path, O_CREAT|O_EXCL|O_WRONLY, 0444);
	if (lock->fd < 0) {
		/* lock exists; is it still viable? */
		if (s_lock_details(lock) == 0) {
			/* read lock details; check viability */

			proc_t ps;
			if (proc_stat(lock->pid, &ps) != 0
			&& (flags != VIGOR_LOCK_SKIPEUID || ps.euid != lock->uid)) {
				unlink(lock->path);
				lock->fd = open(lock->path, O_CREAT|O_EXCL|O_WRONLY, 0444);
			}
		}
	}

	if (lock->fd < 0)
		return -1;

	lock->pid  = getpid();
	lock->uid  = geteuid();
	lock->time = time_s();

	char *details = string("LOCK p%u u%u t%i\n", lock->pid, lock->uid, lock->time);
	size_t n = strlen(details);

	if (write(lock->fd, details, n) != n) {
		free(details);
		close(lock->fd);
		unlink(lock->path);
		return -1;
	}

	free(details);
	lock->valid = 1;
	return 0;
}

int lock_release(lock_t *lock)
{
	assert(lock); // LCOV_EXCL_LINE
	if (lock->fd < 0) return 0;

	close(lock->fd);
	return unlink(lock->path);
}

char* lock_info(lock_t *lock)
{
	assert(lock); // LCOV_EXCL_LINE

	static char buf[1024];
	if (lock->valid) {
		struct passwd *pw = getpwuid(lock->uid);
		snprintf(buf, 255, "PID %u, %s(%u); locked %s",
			lock->pid, (pw ? pw->pw_name : "<unknown>"), lock->uid,
			time_strf("%Y-%m-%d %H:%M:%S%z", lock->time));
	} else {
		snprintf(buf, 255, "<invalid lock file>");
	}
	return buf;
}

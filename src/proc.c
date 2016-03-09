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

/*

    ########  ########   #######   ######
    ##     ## ##     ## ##     ## ##    ##
    ##     ## ##     ## ##     ## ##
    ########  ########  ##     ## ##
    ##        ##   ##   ##     ## ##
    ##        ##    ##  ##     ## ##    ##
    ##        ##     ##  #######   ######

 */

int proc_stat(pid_t pid, proc_t *ps)
{
	assert(ps);      // LCOV_EXCL_LINE
	assert(pid > 0); // LCOV_EXCL_LINE

	char *path, line[8192];
	FILE *f;

	path = string("/proc/%u/status", pid);
	f = fopen(path, "r");
	free(path);
	if (!f) return -1;

	while (fgets(line, 8192, f)) {
		if (sscanf(line, "Pid:\t%u\n",     &ps->pid)) continue;
		if (sscanf(line, "PPid:\t%u\n",    &ps->ppid)) continue;
		if (sscanf(line, "State:\t%c ",    &ps->state)) continue;
		if (sscanf(line, "Uid:\t%u\t%u\t", &ps->uid, &ps->euid)) continue;
		if (sscanf(line, "Gid:\t%u\t%u\t", &ps->gid, &ps->egid)) continue;
	}
	fclose(f);
	return 0;
}

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

    ##     ## ######## ##     ##
    ###   ### ##       ###   ###
    #### #### ##       #### ####
    ## ### ## ######   ## ### ##
    ##     ## ##       ##     ##
    ##     ## ##       ##     ##
    ##     ## ######## ##     ##

 */

void* mem_vmalloc(size_t size, const char *func, const char *file, unsigned int line)
{
	void *buf = calloc(1, size);
	if (buf) return buf;

	logger(LOG_CRIT, "%s, %s:%u - malloc failed: %s",
	                 func, file, line, strerror(errno));
	exit(42);
}

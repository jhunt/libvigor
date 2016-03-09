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

#ifndef T_TEST_H
#define T_TEST_H

#include <ctap.h>
#include <errno.h>
#include <vigor.h>
#include <assert.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <unistd.h>
#include <pthread.h>
#include <netdb.h>

#ifndef TEST_DATA
#define TEST_DATA "t/data"
#endif

#ifndef TEST_TMP
#define TEST_TMP  "t/tmp"
#endif

#endif

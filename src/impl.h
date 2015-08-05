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

#ifndef LIBVIGOR_IMPL_H
#define LIBVIGOR_IMPL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <assert.h>
#include <errno.h>

#include <stdarg.h>
#include <unistd.h>
#include <signal.h>

#include <string.h>
#include <ctype.h>

#include <time.h>
#include <syslog.h>
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>
#include <pthread.h>

#include <sodium.h>

struct hash_bkt {
	size_t   len;
	char   **keys;
	char   **values;
};
struct hash_t {
	struct hash_bkt entries[64];
	ssize_t         bucket;
	ssize_t         offset;

	void (*free)(void *);
};

struct keyval {
	char *key;
	char *val;
	list_t l;
};

struct cachent {
	char    *ident;
	int32_t  last_seen;
	void    *data;
};
struct cache_t {
	size_t  __reserved1__;
	size_t  max_len;
	int32_t expire;

	void (*destroy_f)(void*);

	hash_t         *index;
	struct cachent  entries[];
};

struct lock_t {
	const char *path;

	int valid;
	int fd;

	pid_t   pid;
	uid_t   uid;
	int32_t time;
};

struct path_t {
	char     *buf;
	ssize_t   n;
	size_t    len;
};

struct pdu_t {
	void   *address;
	char   *peer;
	char   *type;
	int     len;
	list_t  frames;
};

#endif

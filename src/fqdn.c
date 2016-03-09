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

    ########  #######  ########  ##    ##
    ##       ##     ## ##     ## ###   ##
    ##       ##     ## ##     ## ####  ##
    ######   ##     ## ##     ## ## ## ##
    ##       ##  ## ## ##     ## ##  ####
    ##       ##    ##  ##     ## ##   ###
    ##        ##### ## ########  ##    ##

 */

/**
  Determine fully-qualified domain name of the local host.

  Using DNS and/or the /etc/hosts file, `fqdn()` determines the
  fully-qualified domain name of the local host.  This includes
  the node name (server name) as well as the full dotted domain
  name where this host resides.

  For example, on a box named 'igor' on the 'lab.example.com'
  domain, the FQDN will be 'igor.lab.example.com'.

  Returns a newly-allocated, NULL-terminated string containing
  the FQDN on success, and NULL on failure.
 */
char* fqdn(void)
{
	char nodename[1024];
	nodename[1023] = '\0';
	gethostname(nodename, 1023);

	struct addrinfo hints, *info, *p;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC; /*either IPV4 or IPV6*/
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_CANONNAME;

	int rc = getaddrinfo(nodename, NULL, &hints, &info);
	if (rc != 0) {
		logger(LOG_ERR, "Failed to lookup %s: %s", nodename, gai_strerror(rc));
		return NULL;
	}

	char *ret = NULL;
	for (p = info; p != NULL; p = p->ai_next) {
		if (!p->ai_canonname) continue;
		if (strcmp(p->ai_canonname, nodename) == 0) continue;
		ret = strdup(p->ai_canonname);
		break;
	}

	freeaddrinfo(info);
	return ret ? ret : strdup(nodename);
}

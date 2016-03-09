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

typedef struct {
	list_t      l;
	void       *socket;
	reactor_fn  fn;
	void       *data;
	int         index;
} reactor_item_t;

/*

    ########  ########    ###     ######  ########  #######  ########
    ##     ## ##         ## ##   ##    ##    ##    ##     ## ##     ##
    ##     ## ##        ##   ##  ##          ##    ##     ## ##     ##
    ########  ######   ##     ## ##          ##    ##     ## ########
    ##   ##   ##       ######### ##          ##    ##     ## ##   ##
    ##    ##  ##       ##     ## ##    ##    ##    ##     ## ##    ##
    ##     ## ######## ##     ##  ######     ##     #######  ##     ##

 */

reactor_t *reactor_new(void)
{
	reactor_t *r = vmalloc(sizeof(reactor_t));
	list_init(&r->reactors);
	return r;
}

void reactor_free(reactor_t *r)
{
	if (!r) return;

	reactor_item_t *item, *tmp;
	for_each_object_safe(item, tmp, &r->reactors, l)
		free(item);
	free(r->poller);
	free(r);
}

int reactor_set(reactor_t *r, void *socket, reactor_fn fn, void *data)
{
	assert(r);
	assert(socket);
	assert(fn);

	reactor_item_t *item = vmalloc(sizeof(reactor_item_t));
	list_init(&item->l);
	item->socket = socket;
	item->fn     = fn;
	item->data   = data;

	list_push(&r->reactors, &item->l);

	size_t n = list_len(&r->reactors);
	free(r->poller);
	r->poller = vmalloc(n * sizeof(zmq_pollitem_t));

	n = 0;
	for_each_object(item, &r->reactors, l) {
		item->index = n;
		r->poller[n].socket  = item->socket;
		r->poller[n].events  = ZMQ_POLLIN;
		n++;
	}

	return 0;
}

int reactor_go(reactor_t *r)
{
	assert(r);
	int rc;

	size_t n = list_len(&r->reactors);
	while ( (rc = zmq_poll(r->poller, n, -1)) >= 0) {
		reactor_item_t *item;
		for_each_object(item, &r->reactors, l) {
			if (r->poller[item->index].revents != ZMQ_POLLIN)
				continue;

			pdu_t *pdu = pdu_recv(item->socket);
			if (!pdu) continue;

			rc = (*(item->fn))(item->socket, pdu, item->data);
			pdu_free(pdu);

			if (rc == VIGOR_REACTOR_HALT) return 0;
		}
	}

	return 1;
}


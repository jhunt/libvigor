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

    ##       ####  ######  ########  ######
    ##        ##  ##    ##    ##    ##    ##
    ##        ##  ##          ##    ##
    ##        ##   ######     ##     ######
    ##        ##        ##    ##          ##
    ##        ##  ##    ##    ##    ##    ##
    ######## ####  ######     ##     ######

 */

int list_init(list_t *l)
{
	assert(l);
	l->next = l->prev = l;
	return 0;
}

int list_isempty(list_t *l)
{
	assert(l);
	return l->next == l;
}

size_t list_len(list_t *lst)
{
	assert(lst);
	size_t len = 0;
	list_t *n;

	for_each(n, lst)
		len++;

	return len;
}

int list_splice(list_t *prev, list_t *next)
{
	assert(prev);
	assert(next);

	next->prev = prev;
	prev->next = next;
	return 0;
}

int list_delete(list_t *n)
{
	assert(n);
	return list_splice(n->prev, n->next) == 0
	    && list_init(n)                  == 0 ? 0 : -1;
}

int list_replace(list_t *o, list_t *n)
{
	assert(o);
	assert(n);

	n->next = o->next;
	n->next->prev = n;

	n->prev = o->prev;
	n->prev->next = n;

	list_init(o);
	return 0;
}

int list_unshift(list_t *l, list_t *n)
{
	assert(l);
	assert(n);

	return list_splice(n, l->next) == 0
	    && list_splice(l, n)       == 0 ? 0 : -1;
}

int list_push(list_t *l, list_t *n)
{
	assert(l);
	assert(n);

	return list_splice(l->prev, n) == 0
	    && list_splice(n,       l) == 0 ? 0 : -1;
}

list_t *list_shift(list_t *l)
{
	assert(l);
	if (list_isempty(l))
		return NULL;

	list_t *n = l->next;
	if (list_splice(l, n->next) != 0)
		return NULL;
	return n;
}

list_t *list_pop(list_t *l)
{
	assert(l);
	if (list_isempty(l))
		return NULL;

	list_t *n = l->prev;
	if (list_splice(n->prev, l) != 0)
		return NULL;
	return n;
}

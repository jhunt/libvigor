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

struct number {
	int value;
	list_t entry;
};

static struct number *number(int v)
{
	struct number *n = calloc(1, sizeof(struct number));
	if (n) {
		list_init(&n->entry);
		n->value = v;
	}
	return n;
}
#define NUMBER(x,v) struct number *x = number(v)

static int head_number(list_t *l)
{
	if (list_isempty(l)) return 0;
	return list_head(l, struct number, entry)->value;
}

static int tail_number(list_t *l)
{
	if (list_isempty(l)) return 0;
	return list_tail(l, struct number, entry)->value;
}

static int sum(list_t *l)
{
	struct number *n;
	int sum = 0;

	for_each_object(n, l, entry)
		sum += n->value;
	return sum;
}

/*************************************************************************/

TESTS {
	alarm(5);
	subtest {
		NUMBER(n1, 4);
		NUMBER(n2, 8);
		NUMBER(n3, 15);
		LIST(l);

		ok(list_isempty(&l), "LIST is empty by default");
		is_int(list_len(&l), 0, "empty list has 0 elements");

		list_push(&l, &n2->entry);
		is_int(head_number(&l),  8, "head([8])");
		is_int(tail_number(&l),  8, "tail([8])");
		is_int(list_len(&l), 1, "len([8])");

		list_unshift(&l, &n1->entry);
		is_int(head_number(&l),  4, "head([4, 8])");
		is_int(tail_number(&l),  8, "tail([4, 8])");
		is_int(list_len(&l), 2, "len([4, 8])");

		list_push(&l, &n3->entry);
		is_int(head_number(&l),  4, "head([4, 8, 15])");
		is_int(tail_number(&l), 15, "tail([4, 8, 15])");
		is_int(list_len(&l), 3, "len([4, 8, 15])");

		list_delete(&n1->entry);
		is_int(head_number(&l),  8, "head([8, 15])");
		is_int(tail_number(&l), 15, "tail([8, 15])");
		is_int(list_len(&l), 2, "len([8, 15])");

		list_push(&l, &n1->entry);
		is_int(head_number(&l),  8, "head([8, 15, 4])");
		is_int(tail_number(&l),  4, "tail([8, 15, 4])");
		is_int(list_len(&l), 3, "len([8, 15, 4])");

		list_delete(&n3->entry);
		is_int(head_number(&l),  8, "head([8, 4])");
		is_int(tail_number(&l),  4, "tail([8, 4])");
		is_int(list_len(&l), 2, "len([8, 4])");

		list_delete(&n2->entry);
		is_int(head_number(&l),  4, "head([4])");
		is_int(tail_number(&l),  4, "tail([4])");
		is_int(list_len(&l), 1, "len([4])");

		free(n1); free(n2); free(n3);
	}

	subtest {
		LIST(l);
		NUMBER(n4,   4); list_push(&l,  &n4->entry);
		NUMBER(n8,   8); list_push(&l,  &n8->entry);
		NUMBER(n15, 15); list_push(&l, &n15->entry);
		NUMBER(n16, 16); list_push(&l, &n16->entry);
		NUMBER(n23, 23); list_push(&l, &n23->entry);
		NUMBER(n42, 42); list_push(&l, &n42->entry);

		struct number *N;
		int sum = 0;

		for_each_object(N, &l, entry) sum += N->value;
		is_int(sum, 108, "for_each_object-sum of [4, 8, 15, 16, 23, 42]");

		sum = 0;
		for_each_object_r(N, &l, entry) sum += N->value;
		is_int(sum, 108, "for_each_object_r-sum of [4, 8, 15, 16, 23, 42]");

		free(n4); free(n8); free(n15); free(n16); free(n23); free(n42);
	}

	subtest {
		LIST(a);
		LIST(b);

		NUMBER(n4,   4); list_push(&a,  &n4->entry);
		NUMBER(n8,   8); list_push(&a,  &n8->entry);
		NUMBER(n15, 15); list_push(&a, &n15->entry);

		NUMBER(n16, 16); list_push(&b, &n16->entry);
		NUMBER(n23, 23); list_push(&b, &n23->entry);
		NUMBER(n42, 42); list_push(&b, &n42->entry);

		is_int(sum(&a), 27, "sum([ 4,  8, 15])");
		is_int(sum(&b), 81, "sum([16, 23, 42])");

		list_replace(&n4->entry, &n16->entry);
		is_int(sum(&a), 39, "sum([16, 8, 15])");

		list_replace(&n15->entry, &n42->entry);
		is_int(sum(&a), 66, "sum([16, 8, 42])");

		list_replace(&n8->entry, &n23->entry);
		is_int(sum(&a), 81, "sum([16, 23, 42])");

		free(n4); free(n8); free(n15); free(n16); free(n23); free(n42);
	}

	subtest {
		LIST(l);
		NUMBER(n1, 1); list_push(&l, &n1->entry);
		NUMBER(n2, 2); list_push(&l, &n2->entry);
		NUMBER(n3, 4); list_push(&l, &n3->entry);
		NUMBER(n4, 8); list_push(&l, &n4->entry);
		is_int(sum(&l), 15, "sum([1, 2, 4, 8])");

		list_t *x;
		struct number *n;

		x = list_shift(&l); n = list_object(x, struct number, entry);
		is_int(n->value, 1, "shift([1, 2, 4, 8])");
		is_int(sum(&l), 14, "sum([2, 4, 8])");

		x = list_shift(&l); n = list_object(x, struct number, entry);
		is_int(n->value, 2, "shift([2, 4, 8])");
		is_int(sum(&l), 12, "sum([4, 8])");

		x = list_pop(&l); n = list_object(x, struct number, entry);
		is_int(n->value, 8, "pop([4, 8])");
		is_int(sum(&l), 4, "sum([4])");

		x = list_pop(&l); n = list_object(x, struct number, entry);
		is_int(n->value, 4, "pop([4])");
		is_int(sum(&l), 0, "sum([])");

		is_null(list_pop(&l), "pop([])");
		is_null(list_shift(&l), "shift([])");

		free(n1); free(n2); free(n3); free(n4);
	};

	alarm(0);
	done_testing();
}

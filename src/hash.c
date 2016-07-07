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

static uint8_t s_hash64(const char *s)
{
	unsigned int h = 81;
	unsigned char c;

	while ((c = *s++))
		h = ((h << 5) + h) + c;

	return h & ~0xc0;
}

static ssize_t s_hash_index(const struct hash_bkt *b, const char *k)
{
	ssize_t i;
	for (i = 0; i < b->len; i++)
		if (strcmp(b->keys[i], k) == 0)
			return i;
	return (ssize_t)-1;
}

static int s_hash_insert(struct hash_bkt *b, const char *k, void *v)
{
	if (!k) return 1;

	char ** new_k = realloc(b->keys,   (b->len + 1) * sizeof(char*));
	char ** new_v = realloc(b->values, (b->len + 1) * sizeof(void*));

	new_k[b->len] = strdup(k);
	new_v[b->len] = v;

	b->keys   = new_k;
	b->values = new_v;
	b->len++;

	return 0;
}

/*

    ##    ##     ###     ######   ##    ##  ########  ######
    ##    ##    ## ##   ##    ##  ##    ##  ##       ##    ##
    ##    ##   ##   ##  ##        ##    ##  ##       ##
    ########  ##     ##  ######   ########  ######    ######
    ##    ##  #########       ##  ##    ##  ##             ##
    ##    ##  ##     ## ##    ##  ##    ##  ##       ##    ##
    ##    ##  ##     ##  ######   ##    ##  ########  ######

 */

/**
  Release memory allocated to hash $h.

  If the $all parameter is non-zero, values stored in the hash
  will be freed as well.

  Note that this does not free the memory that $h resides in;
  this allows the hash_t object to be stack-allocated.
 */
void hash_done(hash_t *h, uint8_t all)
{
	ssize_t i, j;
	if (h) {
		for (i = 0; i < 64; i++) {
			for (j = 0; j < h->entries[i].len; j++) {
				free(h->entries[i].keys[j]);
				if (all) free(h->entries[i].values[j]);
			}
			free(h->entries[i].keys);
			free(h->entries[i].values);
		}
	}
}

/**
  Retrieve the value of $k from $h.

  If $k is not found in $h, NULL is returned.
 */
void* hash_get(const hash_t *h, const char *k)
{
	if (!h || !k) return NULL;

	const struct hash_bkt *b = &h->entries[s_hash64(k)];
	ssize_t i = s_hash_index(b, k);
	return (i < 0 ? NULL : b->values[i]);
}

/**
  Set the value of $k in hash $h to a new value, $v.

  If the key does not already exist in the hash, it will
  be inserted, and NULL will be returned.  If $k existed
  prior to this call, its value will be overwritten by $v,
  and the prior value will be returned (so that the caller
  can free it).
  */
void* hash_set(hash_t *h, const char *k, void *v)
{
	if (!h || !k) return NULL;

	struct hash_bkt *b = &h->entries[s_hash64(k)];
	ssize_t i = s_hash_index(b, k);

	if (i < 0) {
		if(s_hash_insert(b, k, v) == 0)
			h->len++;
		return v;
	}

	void *existing = b->values[i];
	b->values[i] = v;
	return existing;
}

/**
  Unset the key $k in hash $h.

  If the key isn't set, NULL will be returned.
  If it is, the value will be removed, and returned
  (so that the caller can free it).
 */
void* hash_unset(hash_t *h, const char *k) {
	if (!h || !k) return NULL;

	struct hash_bkt *b = &h->entries[s_hash64(k)];
	ssize_t i = s_hash_index(b, k);
	if (i < 0) {
		return NULL;
	}

	void *existing = b->values[i];
	ssize_t j;
	for (j = i + 1; j < b->len; i++, j++) {
		b->keys[i]   = b->keys[j];
		b->values[i] = b->values[j];
	}
	h->len--;
	b->len--;
	return existing;
}

/* internal use; external visibility for macro loops */
void* hash_next(hash_t *h, char **k, void **v)
{
	assert(h); // LCOV_EXCL_LINE

	char *tmp = NULL;
	if (k) *k = NULL;
	if (v) *v = NULL;
	while (h->bucket < 64) {
		struct hash_bkt *b = h->entries + h->bucket;
		if (h->offset >= b->len) {
			h->bucket++;
			h->offset = 0;
			continue;
		}
		tmp = b->keys[h->offset];
		if (k) *k = b->keys[h->offset];
		if (v) *v = b->values[h->offset];
		h->offset++;
		break;
	}
	return tmp;
}

/**
  Merge hash $b into $a, overwriting values in $a.
 */
void hash_merge(hash_t *a, hash_t *b)
{
	assert(a); // LCOV_EXCL_LINE;
	assert(b); // LCOV_EXCL_LINE;

	char *k; void *v;
	for_each_key_value(b, k, v)
		hash_set(a, k, v);
}

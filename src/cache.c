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

#include <vigor.h>
#include "impl.h"

/*

     ######     ###     ######  ##     ## ########
    ##    ##   ## ##   ##    ## ##     ## ##
    ##        ##   ##  ##       ##     ## ##
    ##       ##     ## ##       ######### ######
    ##       ######### ##       ##     ## ##
    ##    ## ##     ## ##    ## ##     ## ##
     ######  ##     ##  ######  ##     ## ########

*/

cache_t* cache_new(size_t len, int32_t min_life)
{
	cache_t *cc  = vmalloc(sizeof(cache_t)
	                     + sizeof(cache_entry_t) * len);
	cc->max_len  = len;
	cc->min_life = min_life;
	memset(&cc->index, 0, sizeof(hash_t));
	return cc;
}

int cache_tune(cache_t **cc, size_t len, int32_t min_life)
{
	errno = EINVAL;
	if (len      <= 0) len      = (*cc)->max_len;
	if (min_life <= 0) min_life = (*cc)->min_life;

	if (len < (*cc)->max_len)
		return -1;

	if (min_life < (*cc)->min_life)
		cache_purge((*cc), 0);
	(*cc)->min_life = min_life;

	if (len > (*cc)->max_len) {
		cache_t *new = cache_new(len, min_life);

		char *k; void *v;
		for_each_key_value(&(*cc)->index, k, v)
			hash_set(&new->index, k, v);

		new->destroy_f = (*cc)->destroy_f;
		cache_free((*cc));
		*cc = new;
	}

	return 0;
}

void cache_free(cache_t *cc)
{
	if (!cc) return;

	cache_purge(cc, 1);
	hash_done(&cc->index, 0);
	free(cc);
}

void cache_purge(cache_t *cc, int force)
{
	int32_t now = time_s();
	int i;
	for (i = 0; i < cc->max_len; i++) {
		if (!force &&
		     (cc->entries[i].last_seen == -1
		   || cc->entries[i].last_seen >= now - cc->min_life))
			continue;

		if (cc->entries[i].ident) {
			hash_set(&cc->index, cc->entries[i].ident, NULL);

			free(cc->entries[i].ident);
			cc->entries[i].ident = NULL;

			if (cc->destroy_f)
				(*cc->destroy_f)(cc->entries[i].data);
			cc->entries[i].data = NULL;

			cc->entries[i].last_seen = -1;
		}
	}
}

int cache_opt(cache_t *cc, int op, void *data)
{
	if (op == VIGOR_CACHE_DESTROY) {
		cc->destroy_f = data;
		return 0;
	}
	if (op == VIGOR_CACHE_MINLIFE) {
		cc->min_life = *(int*)(data) & 0xffffffff;
		return 0;
	}
	return 1;
}

static int s_cache_next(cache_t *cc)
{
	int i;
	for (i = 0; i < cc->max_len; i++)
		if (!cc->entries[i].ident)
			return i;
	return -1;
}

void* cache_get(cache_t *cc, const char *id)
{
	cache_entry_t *ent = hash_get(&cc->index, id);
	if (!ent) return NULL;

	int32_t now = time_s();
	if (ent->last_seen < now)
		ent->last_seen = time_s();

	return ent->data;
}

void* cache_set(cache_t *cc, const char *id, void *data)
{
	cache_entry_t *ent = hash_get(&cc->index, id);
	if (!ent) {
		int idx = s_cache_next(cc);
		if (idx < 0) return NULL;
		ent = &cc->entries[idx];
	}
	if (!ent->ident) {
		ent->ident = strdup(id);
		hash_set(&cc->index, id, ent);
	}
	ent->last_seen = time_s();
	return ent->data = data;
}

void* cache_unset(cache_t *cc, const char *id)
{
	cache_entry_t *ent = hash_get(&cc->index, id);
	if (!ent) return NULL;
	hash_set(&cc->index, id, NULL);

	free(ent->ident);
	ent->ident = NULL;

	ent->last_seen = -1;

	void *d = ent->data;
	ent->data = NULL;

	return d;
}

void cache_touch(cache_t *cc, const char *id, int32_t last)
{
	cache_entry_t *ent = hash_get(&cc->index, id);
	if (!ent) return;

	if (last <= 0) last = time_s();
	ent->last_seen = last;
}


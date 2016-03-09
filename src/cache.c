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

     ######     ###     ######  ##     ## ########
    ##    ##   ## ##   ##    ## ##     ## ##
    ##        ##   ##  ##       ##     ## ##
    ##       ##     ## ##       ######### ######
    ##       ######### ##       ##     ## ##
    ##    ## ##     ## ##    ## ##     ## ##
     ######  ##     ##  ######  ##     ## ########

*/

/**
  Create a new, general purpose cache.

  $len controls how many objects can be stored in the cache before
  it is full.  A full cache can be purged, thereby removing old, stale
  entries to make room for newer entries.

  $expire specifies how long (in seconds) until a cache entry is eligible
  to be purged.  Cache entries will not actually be purged until the
  @cache_purge function is called.

  The interplay between $len and $expire is important, and it serves
  to ensure that the cache is useful, does not churn too much, and isn't
  overly large.

  On success, returns a pointer to a new `cache_t` with the given $len
  and $expire parameters.

  To release the memory used by a cache, see @cache_free.
 */
cache_t* cache_new(size_t len, int32_t expire)
{
	cache_t *cc  = vmalloc(sizeof(cache_t)
	                     + sizeof(cache_entry_t) * len);
	cc->max_len  = len;
	cc->expire = expire;
	memset(&cc->index, 0, sizeof(hash_t));
	return cc;
}

/**
  Resize a cache.

  This function allows you to resize a cache on-the-fly, by
  increasing the amount of memory in which the cache resides.
  It is an error to try to reduce the size of a cache.

  On success, resizes $cc and returns 0.

  On failure, returns -1 and sets errno appropriately:

    - **`EINVAL`** - $len is less than the current cache size.

  The cache is guaranteed to remain unmodified on failure.
 */
int cache_resize(cache_t **cc, size_t len)
{
	errno = EINVAL;
	if (len < (*cc)->max_len)
		return -1;

	if (len > (*cc)->max_len) {
		cache_t *new = cache_new(len, (*cc)->expire);

		char *k; void *v;
		for_each_key_value(&(*cc)->index, k, v)
			hash_set(&new->index, k, v);

		new->destroy_f = (*cc)->destroy_f;
		cache_free((*cc));
		*cc = new;
	}

	return 0;
}

/**
  Release memory used by the cache $cc.

  It is _not_ an error to call cache_free with a NULL pointer.
 */
void cache_free(cache_t *cc)
{
	if (!cc) return;

	cache_purge(cc, 1);
	hash_done(&cc->index, 0);
	free(cc);
}

/**
  Purge expired cache entries.

  All cache entries that are expired (based on the global cache expiry)
  will be removed from the cache.  If a destructor function has been set
  (see @cache_opt), it will be called for each purged entry.

  The $force flag can be used to side-step the expiration logic and purge
  all entries in the cache.  This is akin to calling @cache_free, except
  that you can re-use the cache afterwards.
 */
void cache_purge(cache_t *cc, int force)
{
	int32_t now = time_s();
	int i;
	for (i = 0; i < cc->max_len; i++) {
		if (!force &&
		     (cc->entries[i].last_seen == -1
		   || cc->entries[i].last_seen >= now - cc->expire))
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

/**
  Set cache options.

  This function allows you to configure a cache, by specifying
  a the destructor function (used to free expired cache entries),
  setting the global cache expiry, etc.

  The following $op values are supported:

  - **`VIGOR_CACHE_DESTRUCTOR`** - A destructor function, used
    for freeing memory of expired cache entries.
  - **`VIGOR_CACHE_EXPIRY`** - The global cache expiry.

  The $data payload will be cast to the appropriate data type,
  based on the given $op.

  On success, returns 0.

  On failure, returns 1, and sets errno appropriately:

  - **`EINVAL`** - An unknown or unhandled $op value was specified.
 */
int cache_setopt(cache_t *cc, int op, const void *data)
{
	if (op == VIGOR_CACHE_DESTRUCTOR) {
		cc->destroy_f = data;
		return 0;
	}
	if (op == VIGOR_CACHE_EXPIRY) {
		cc->expire = *(int*)(data) & 0xffffffff;
		return 0;
	}
	errno = EINVAL;
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

/**
  Retrieve a value from the cache.

  Finds and returns the cached object, based on its cache key, $id.
  If the object is found in the cache, its entry will be updated with
  the current access timestamp (to stave off expiration).

  Returns a pointer to the cached object if it is found, NULL if not.
 */
void* cache_get(cache_t *cc, const char *id)
{
	cache_entry_t *ent = hash_get(&cc->index, id);
	if (!ent) return NULL;

	int32_t now = time_s();
	if (ent->last_seen < now)
		ent->last_seen = time_s();

	return ent->data;
}

/**
  Stores $data in a cache, using the key $id.

  Updates or inserts a $data object into the cache, storing it under
  the cache key $id.  The access timestamp of the entry will be set to
  the current timestamp.

  On success, returns $data.  On failure, returns NULL.

  Failure could indicate that the cache is full.  This function does
  not implicitly call @cache_purge; that is left to the caller.
 */
void* cache_set(cache_t *cc, const char *id, void *data)
{
	cache_entry_t *ent = hash_get(&cc->index, id);
	if (ent && ent->data != data && cc->destroy_f)
		(*cc->destroy_f)(ent->data);

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

/**
  Forcibly remove a cache entry.

  Removes the data object from the cache that is stored under the
  $id key.

  A pointer to the removed data object is returned to the caller if
  possible.  If nothing is stored under that key, returns NULL.
 */
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

	/* FIXME: it seems like we should have no return,
	          and just destroy the cache function... */
	return d;
}

/**
  Update the access timestamp of a cache entry.

  Finds the cache entry stored under the $id key and updates its
  access timestamp to $last.  This can be used to prematurely expire
  an entry, or keep it in cache without accessing it.
 */
void cache_touch(cache_t *cc, const char *id, int32_t last)
{
	cache_entry_t *ent = hash_get(&cc->index, id);
	if (!ent) return;

	if (last <= 0) last = time_s();
	ent->last_seen = last;
}

int cache_isfull(cache_t *cc)
{
	int i;
	for (i = 0; i < cc->max_len; i++)
		if (!cc->entries[i].ident)
			return 0;
	return 1;
}

int cache_isempty(cache_t *cc)
{
	int i;
	for (i = 0; i < cc->max_len; i++)
		if (cc->entries[i].ident)
			return 0;
	return 1;
}

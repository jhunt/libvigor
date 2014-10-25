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

#ifndef LIBVIGOR_VIGOR_H
#define LIBVIGOR_VIGOR_H

#define VIGOR_VERSION_MAJOR 1
#define VIGOR_VERSION_MINOR 0
#define VIGOR_VERSION_PATCH 0

#define VIGOR_MAKE_VERSION(major, minor, patch) \
          ((major) * 10000 + \
           (minor) *   100 + \
           (patch))
#define VIGOR_VERSION \
        VIGOR_MAKE_VERSION(VIGOR_VERSION_MAJOR, \
                           VIGOR_VERSION_MINOR, \
                           VIGOR_VERSION_PATCH)

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

#define vmalloc(l) mem_vmalloc((l), __func__, __FILE__, __LINE__)
void* mem_vmalloc(size_t, const char*, const char*, unsigned int);

/*
    ##       ####  ######  ########  ######
    ##        ##  ##    ##    ##    ##    ##
    ##        ##  ##          ##    ##
    ##        ##   ######     ##     ######
    ##        ##        ##    ##          ##
    ##        ##  ##    ##    ##    ##    ##
    ######## ####  ######     ##     ######
 */

typedef struct list list_t;
struct list {
	list_t *next;
	list_t *prev;
};

#define LIST(n) list_t n = { &(n), &(n) }
#define list_object(l,t,m) ((t*)((char*)(l) - offsetof(t,m)))
#define list_head(l,t,m) list_object((l)->next, t, m)
#define list_tail(l,t,m) list_object((l)->prev, t, m)

/** Iterate over a list */
#define for_each(pos, head) \
	for (pos = (head)->next; \
	     pos != (head);      \
	     pos = pos->next)
#define for_each_r(pos, head) \
	for (pos = (head)->prev; \
	     pos != (head);      \
	     pos = pos->prev)

/** Iterate over a list (safely) */
#define for_each_safe(pos, tmp, head) \
	for (pos = (head)->next, tmp = pos->next; \
	     pos != (head);                       \
	     pos = tmp, tmp = pos->next)
#define for_each_safe_r(pos, tmp, head) \
	for (pos = (head)->prev, tmp = pos->prev; \
	     pos != (head);                       \
	     pos = tmp, tmp = pos->prev)

#define list_next(o,m) list_object(o->m.next, typeof(*o), m)
#define list_prev(o,m) list_object(o->m.prev, typeof(*o), m)

/** Iterate over a list, accessing the objects */
#define for_each_object(pos, head, memb) \
	for (pos = list_object((head)->next, typeof(*pos), memb); \
	     &pos->memb != (head);                                 \
	     pos = list_object(pos->memb.next, typeof(*pos), memb))
#define for_each_object_r(pos, head, memb) \
	for (pos = list_object((head)->prev, typeof(*pos), memb); \
	     &pos->memb != (head);                                 \
	     pos = list_object(pos->memb.prev, typeof(*pos), memb))

/** Iterate over a list (safely), accessing the objects */
#define for_each_object_safe(pos, tmp, head, memb) \
	for (pos = list_object((head)->next, typeof(*pos), memb),   \
	     tmp = list_object(pos->memb.next, typeof(*pos), memb); \
	     &pos->memb != (head);                                   \
	     pos = tmp, tmp = list_object(tmp->memb.next, typeof(*tmp), memb))
#define for_each_object_safe_r(pos, tmp, head, memb) \
	for (pos = list_object((head)->prev, typeof(*pos), memb),   \
	     tmp = list_object(pos->memb.prev, typeof(*pos), memb); \
	     &pos->memb != (head);                                   \
	     pos = tmp, tmp = list_object(tmp->memb.prev, typeof(*tmp), memb))

int list_init(list_t *l);

int list_isempty(list_t *l);
size_t list_len(list_t *l);

int list_splice(list_t *prev, list_t *next);
int list_delete(list_t *n);
int list_replace(list_t *o, list_t *n);

int list_unshift(list_t *l, list_t *n);
int list_push   (list_t *l, list_t *n);

list_t* list_shift(list_t *l);
list_t* list_pop  (list_t *l);

/*
    ##    ##     ###     ######   ##    ##  ########  ######
    ##    ##    ## ##   ##    ##  ##    ##  ##       ##    ##
    ##    ##   ##   ##  ##        ##    ##  ##       ##
    ########  ##     ##  ######   ########  ######    ######
    ##    ##  #########       ##  ##    ##  ##             ##
    ##    ##  ##     ## ##    ##  ##    ##  ##       ##    ##
    ##    ##  ##     ##  ######   ##    ##  ########  ######
 */
typedef struct hash hash_t;
struct hash_bkt {
	size_t   len;
	char   **keys;
	char   **values;
};
struct hash {
	struct hash_bkt entries[64];
	ssize_t         bucket;
	ssize_t         offset;
};
int hash_done(hash_t *h, uint8_t all);
void* hash_get(const hash_t *h, const char *k);
void* hash_set(hash_t *h, const char *k, void *v);
void* hash_next(hash_t *h, char **k, void **v);
int hash_merge(hash_t *a, hash_t *b);

#define for_each_key_value(h,k,v) \
	for ((h)->offset = (h)->bucket = 0; \
	     hash_next((h), &(k), (void**)&(v)); )

/*
     ######   #######  ##    ## ######## ####  ######
    ##    ## ##     ## ###   ## ##        ##  ##    ##
    ##       ##     ## ####  ## ##        ##  ##
    ##       ##     ## ## ## ## ######    ##  ##   ####
    ##       ##     ## ##  #### ##        ##  ##    ##
    ##    ## ##     ## ##   ### ##        ##  ##    ##
     ######   #######  ##    ## ##       ####  ######
 */

#define config_t list_t
typedef struct keyval keyval_t;
struct keyval {
	char *key;
	char *val;
	list_t l;
};

int config_set  (config_t *cfg, const char *key, const char *val);
int config_unset(config_t *cfg, const char *key);
char* config_get(config_t *cfg, const char *key);
int config_isset(config_t *cfg, const char *key);
int config_read (config_t *cfg, FILE *io);
int config_write(config_t *cfg, FILE *io);
int config_uniq (config_t *dest, config_t *src);
int config_done (config_t *cfg);

/*

     ######     ###     ######  ##     ## ########
    ##    ##   ## ##   ##    ## ##     ## ##
    ##        ##   ##  ##       ##     ## ##
    ##       ##     ## ##       ######### ######
    ##       ######### ##       ##     ## ##
    ##    ## ##     ## ##    ## ##     ## ##
     ######  ##     ##  ######  ##     ## ########

*/

typedef struct {
	char    *ident;
	int32_t  last_seen;
	void    *data;
} cache_entry_t;

typedef struct {
	size_t  len;
	size_t  max_len;
	int32_t min_life;

	void (*destroy_f)(void*);

	hash_t      index;
	cache_entry_t  entries[];
} cache_t;

#define for_each_cache_key(cc,k) \
	for ((cc)->index.offset = (cc)->index.bucket = 0; \
	     hash_next(&(cc)->index, &(k), NULL); )

#define VIGOR_CACHE_DESTROY  1
#define VIGOR_CACHE_MINLIFE  2

cache_t* cache_new(size_t max, int32_t min_life);
int cache_tune(cache_t **cc, size_t max, int32_t min_life);
void cache_free(cache_t *cc);
void cache_purge(cache_t *cc, int force);
int   cache_opt(cache_t *cc, int op, void *value);
void* cache_get(cache_t *cc, const char *id);
void* cache_set(cache_t *cc, const char *id, void *data);
void* cache_unset(cache_t *cc, const char *id);
void cache_touch(cache_t *cc, const char *id, int32_t last);

/*
     ######  ######## ########  #### ##    ##  ######    ######
    ##    ##    ##    ##     ##  ##  ###   ## ##    ##  ##    ##
    ##          ##    ##     ##  ##  ####  ## ##        ##
     ######     ##    ########   ##  ## ## ## ##   ####  ######
          ##    ##    ##   ##    ##  ##  #### ##    ##        ##
    ##    ##    ##    ##    ##   ##  ##   ### ##    ##  ##    ##
     ######     ##    ##     ## #### ##    ##  ######    ######
 */

char* string(const char *fmt, ...);
char* interpolate(const char *s, hash_t *vars);

typedef struct {
	size_t   num;      /* number of actual strings */
	size_t   len;      /* number of memory slots for strings */
	char   **strings;  /* array of NULL-terminated strings */
} strings_t;
typedef int (*strings_cmp_fn)(const void*, const void*);

#define SPLIT_NORMAL  0
#define SPLIT_GREEDY  1

#define for_each_string(l,i) for ((i)=0; (i)<(l)->num; (i)++)

int STRINGS_ASC(const void *a, const void *b);
int STRINGS_DESC(const void *a, const void *b);

strings_t* strings_new(char** src);
strings_t* strings_dup(strings_t *orig);
void strings_free(strings_t *list);
void strings_sort(strings_t *list, strings_cmp_fn cmp);
void strings_uniq(strings_t *list);
int strings_search(const strings_t *list, const char *needle);
int strings_add(strings_t *list, const char *value);
int strings_add_all(strings_t *dst, const strings_t *src);
int strings_remove(strings_t *list, const char *value);
int strings_remove_all(strings_t *dst, strings_t *src);
strings_t* strings_intersect(const strings_t *a, const strings_t *b);
int strings_diff(strings_t *a, strings_t *b);
char* strings_join(strings_t *list, const char *delim);
strings_t* strings_split(const char *str, size_t len, const char *delim, int opt);

/*
    ######## #### ##     ## ########
       ##     ##  ###   ### ##
       ##     ##  #### #### ##
       ##     ##  ## ### ## ######
       ##     ##  ##     ## ##
       ##     ##  ##     ## ##
       ##    #### ##     ## ########
 */

int32_t time_s(void);
int64_t time_ms(void);
const char *time_strf(const char *fmt, int32_t s);
int sleep_ms(int64_t ms);

typedef struct {
	int running;
	struct timeval tv;
} stopwatch_t;

#define STOPWATCH(t, ms) for (stopwatch_start(t); (t)->running; stopwatch_stop(t), ms = stopwatch_ms(t))
void stopwatch_start(stopwatch_t *clock);
void stopwatch_stop(stopwatch_t *clock);
uint32_t stopwatch_s(const stopwatch_t *clock);
uint64_t stopwatch_ms(const stopwatch_t *clock);

/*

    ########  ########   #######   ######
    ##     ## ##     ## ##     ## ##    ##
    ##     ## ##     ## ##     ## ##
    ########  ########  ##     ## ##
    ##        ##   ##   ##     ## ##
    ##        ##    ##  ##     ## ##    ##
    ##        ##     ##  #######   ######

 */

typedef struct {
	pid_t   pid;
	pid_t  ppid;

	char  state;

	uid_t   uid, euid;
	gid_t   gid, egid;
} proc_t;

int proc_stat(pid_t pid, proc_t *ps);

/*

    ##        #######   ######  ##    ##  ######
    ##       ##     ## ##    ## ##   ##  ##    ##
    ##       ##     ## ##       ##  ##   ##
    ##       ##     ## ##       #####     ######
    ##       ##     ## ##       ##  ##         ##
    ##       ##     ## ##    ## ##   ##  ##    ##
    ########  #######   ######  ##    ##  ######

 */

#define VIGOR_LOCK_SKIPEUID 1
typedef struct {
	const char *path;

	int valid;
	int fd;

	pid_t   pid;
	uid_t   uid;
	int32_t time;
} lock_t;

void lock_init  (lock_t *lock, const char *path);
int lock_acquire(lock_t *lock, int flags);
int lock_release(lock_t *lock);
char* lock_info (lock_t *lock);

/*
    ##        #######   ######    ######
    ##       ##     ## ##    ##  ##    ##
    ##       ##     ## ##        ##
    ##       ##     ## ##   ####  ######
    ##       ##     ## ##    ##        ##
    ##       ##     ## ##    ##  ##    ##
    ########  #######   ######    ######
 */

void log_open (const char *ident, const char *facility);
void log_close(void);

int         log_level       (int level, const char *name);
const char* log_level_name  (int level);
int         log_level_number(const char *name);

void logger(int level, const char *fmt, ...);

/*

    ########     ###     ######  ########
    ##     ##   ## ##   ##    ## ##        ##   ##
    ##     ##  ##   ##  ##       ##         ## ##
    ########  ##     ##  ######  ######   #########
    ##     ## #########       ## ##         ## ##
    ##     ## ##     ## ##    ## ##        ##   ##
    ########  ##     ##  ######  ########

 */

int base16_encode(char *dst, size_t dlen, const void *src, size_t slen);
int base16_decode(void *dst, size_t dlen, const char *src, size_t slen);

char* base16_encodestr(const void *src, size_t len);
char* base16_decodestr(const char *src, size_t len);

/*

    ########     ###    ######## ##     ##
    ##     ##   ## ##      ##    ##     ##
    ##     ##  ##   ##     ##    ##     ##
    ########  ##     ##    ##    #########
    ##        #########    ##    ##     ##
    ##        ##     ##    ##    ##     ##
    ##        ##     ##    ##    ##     ##

 */

typedef struct {
	char     *buf;
	ssize_t   n;
	size_t    len;
} path_t;

path_t* path_new(const char *p);
void path_free(path_t *p);
const char *path(path_t *p);
int path_canon(path_t *p);
int path_push(path_t *p);
int path_pop(path_t *p);

/*
    ########  #######  ########  ##    ##
    ##       ##     ## ##     ## ###   ##
    ##       ##     ## ##     ## ####  ##
    ######   ##     ## ##     ## ## ## ##
    ##       ##  ## ## ##     ## ##  ####
    ##       ##    ##  ##     ## ##   ###
    ##        ##### ## ########  ##    ##
 */

char* fqdn(void);

/*
     ######  ####  ######   ##    ##    ###    ##        ######
    ##    ##  ##  ##    ##  ###   ##   ## ##   ##       ##    ##
    ##        ##  ##        ####  ##  ##   ##  ##       ##
     ######   ##  ##   #### ## ## ## ##     ## ##        ######
          ##  ##  ##    ##  ##  #### ######### ##             ##
    ##    ##  ##  ##    ##  ##   ### ##     ## ##       ##    ##
     ######  ####  ######   ##    ## ##     ## ########  ######
 */
void signal_handlers(void);
int signalled(void);

/*
    ########     ###    ##    ## ########
    ##     ##   ## ##   ###   ## ##     ##
    ##     ##  ##   ##  ####  ## ##     ##
    ########  ##     ## ## ## ## ##     ##
    ##   ##   ######### ##  #### ##     ##
    ##    ##  ##     ## ##   ### ##     ##
    ##     ## ##     ## ##    ## ########
 */

void seed_randomness(void);

#endif

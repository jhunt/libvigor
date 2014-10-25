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
     ######   #######  ##    ## ######## ####  ######
    ##    ## ##     ## ###   ## ##        ##  ##    ##
    ##       ##     ## ####  ## ##        ##  ##
    ##       ##     ## ## ## ## ######    ##  ##   ####
    ##       ##     ## ##  #### ##        ##  ##    ##
    ##    ## ##     ## ##   ### ##        ##  ##    ##
     ######   #######  ##    ## ##       ####  ######
 */

int config_set(config_t *cfg, const char *key, const char *val)
{
	keyval_t *kv;
	for_each_object(kv, cfg, l) {
		if (strcmp(kv->key, key) != 0)
			continue;
		free(kv->val);
		kv->val = strdup(val);
		return 0;
	}

	kv = malloc(sizeof(keyval_t));
	assert(kv);

	kv->key = strdup(key);
	kv->val = strdup(val);
	list_unshift(cfg, &kv->l);
	return 0;
}

int config_unset(config_t *cfg, const char *key)
{
	keyval_t *kv, *tmp;
	for_each_object_safe(kv, tmp, cfg, l) {
		if (strcmp(kv->key, key) != 0)
			continue;
		list_delete(&kv->l);
		free(kv->key);
		free(kv->val);
		free(kv);
	}
	return 0;
}

char * config_get(config_t *cfg, const char *key)
{
	keyval_t *kv;
	for_each_object(kv, cfg, l)
		if (strcmp(kv->key, key) == 0)
			return kv->val;
	return NULL;
}

int config_isset(config_t *cfg, const char *key)
{
	keyval_t *kv;
	for_each_object(kv, cfg, l)
		if (strcmp(kv->key, key) == 0)
			return 1;
	return 0;
}

int config_read(config_t *cfg, FILE *io)
{
	keyval_t *kv;
	char line[8192];
	while (fgets(line, 8191, io)) {
		/* FIXME: doesn't handle large lines (>8192) */
		/*
		   "   directive    value  \n"
		    ^  ^        ^   ^    ^
		    |  |        |   |    |
		    |  |        |   |    `--- d (= '\0')
		    |  |        |   `-------- c
		    |  |        `------------ b (= '\0')
		    |  `--------------------- a
		    `------------------------ line
		 */
		char *a, *b, *c, *d;
		for (a = line; *a &&  isspace(*a); a++);
		for (b = a;    *b && !isspace(*b); b++);
		for (c = b;    *c &&  isspace(*c); c++);
		for (d = c;    *d && !isspace(*d); d++);
		*b = *d = '\0';

		if (!*a) continue;
		if (*a == '#') continue;
		if (*a == '\n') continue;

		kv = malloc(sizeof(keyval_t));
		assert(kv);
		kv->key = strdup(a);
		kv->val = strdup(c);

		list_unshift(cfg, &kv->l);
	}
	return 0;
}

int config_write(config_t *cfg, FILE *io)
{
	int rc;

	config_t uniq;
	rc = list_init(&uniq);
	assert(rc == 0);

	rc = config_uniq(&uniq, cfg);
	assert(rc == 0);

	keyval_t *kv;
	for_each_object(kv, &uniq, l)
		fprintf(io, "%s %s\n", kv->key, kv->val);

	return 0;
}

int config_uniq(config_t *dest, config_t *src)
{
	keyval_t *kv, *tmp;
	for_each_object_safe(kv, tmp, src, l) {
		if (config_isset(dest, kv->key)) continue;
		config_set(dest, kv->key, kv->val);
	}
	return 0;
}

int config_done(config_t *cfg)
{
	keyval_t *kv, *tmp;
	for_each_object_safe(kv, tmp, cfg, l) {
		free(kv->key);
		free(kv->val);
		list_delete(&kv->l);
		free(kv);
	}
	return 0;
}


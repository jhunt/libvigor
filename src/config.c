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

     ######   #######  ##    ## ######## ####  ######
    ##    ## ##     ## ###   ## ##        ##  ##    ##
    ##       ##     ## ####  ## ##        ##  ##
    ##       ##     ## ## ## ## ######    ##  ##   ####
    ##       ##     ## ##  #### ##        ##  ##    ##
    ##    ## ##     ## ##   ### ##        ##  ##    ##
     ######   #######  ##    ## ##       ####  ######

 */

/**
  Set a configuration directive.

  Updates $cfg so that future requests for the value of $key will
  return $val.  Both $key and $val _must_ be strings; @config_set
  will create copies of them as needed, for its own memory management.

  Returns 0 on success.
 */
int config_set(config_t *cfg, const char *key, const char *val)
{
	assert(key);
	assert(val);

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

/**
  Deletes a configuration directive.

  Updates $cfg to remove all traces of $key.

  Returns 0 on success.
 */
int config_unset(config_t *cfg, const char *key)
{
	assert(cfg);
	assert(key);

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

/**
  Retrieve the value of a configuration directive.

  Looks through $cfg, and returns the value last set for $key
  by @config_set.  If $key isn't found, returns NULL.
 */
char * config_get(config_t *cfg, const char *key)
{
	assert(cfg);
	assert(key);

	keyval_t *kv;
	for_each_object(kv, cfg, l)
		if (strcmp(kv->key, key) == 0)
			return kv->val;
	return NULL;
}

/**
  Check if $key is defined in $cfg.

  Returns 1 if the given $key was found in the config object,
  and 0 if not.  This is designed to be used in conditionals.
 */
int config_isset(config_t *cfg, const char *key)
{
	assert(cfg);
	assert(key);

	keyval_t *kv;
	for_each_object(kv, cfg, l)
		if (strcmp(kv->key, key) == 0)
			return 1;
	return 0;
}

/**
  Read configuration from an input stream.

  This is usually used to read configuration directives and their
  values from a file.  Key/value pairs (whitespace-separated) are
  read from $io and set in $cfg.  Whitespace is (generally) ignored.
  Comments start with a '#' and continue to the end of the line.

  Returns 0 on success.
 */
int config_read(config_t *cfg, FILE *io)
{
	assert(cfg);
	assert(io);

	keyval_t *kv;
	char line[8192];
	while (fgets(line, 8191, io)) {
		/* FIXME: doesn't handle large lines (>8192) */
		/*
		   "   directive    value here  \n"
		    ^  ^        ^   ^         ^
		    |  |        |   |         |
		    |  |        |   |         `--- d (= '\0')
		    |  |        |   `------------- c
		    |  |        `----------------- b (= '\0')
		    |  `-------------------------- a
		    `----------------------------- line
		 */
		char *a, *b, *c, *d;

		/* strip comments */
		for (a = line; *a && *a != '#'; a++); *a = '\0';
		/* start of key token */
		for (a = line; *a &&  isspace(*a); a++);
		/* end of key token */
		for (b = a;    *b && !isspace(*b); b++);
		/* start of value */
		for (c = b;    *c &&  isspace(*c); c++);
		/* end of value */
		for (d = c; *d; d++); for (; d > c && (!*d || isspace(*d)); d--);
		if (*d) d++;
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

/**
  Write configuration to an output stream.

  Formats $cfg as key/value pairs (whitespace-separated) and prints them,
  one line apiece, to the $io stream.

  Returns 0 on success.
 */
int config_write(config_t *cfg, FILE *io)
{
	CONFIG(uniq);

	keyval_t *kv;
	for_each_object(kv, cfg, l) {
		if (!config_isset(&uniq, kv->key)) {
			config_set(&uniq, kv->key, kv->val);
			fprintf(io, "%s %s\n", kv->key, kv->val);
		}
	}

	config_done(&uniq);
	return 0;
}

/**
  Release memory used by the configuration.

  Note that this does not free the memory taken up by $cfg itself;
  this allows callers to define config_t objects on the stack.
 */
void config_done(config_t *cfg)
{
	assert(cfg);

	keyval_t *kv, *tmp;
	for_each_object_safe(kv, tmp, cfg, l) {
		free(kv->key);
		free(kv->val);
		list_delete(&kv->l);
		free(kv);
	}
}

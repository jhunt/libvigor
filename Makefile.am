#
# Makefile.am - libvigor Automake File
#

AUTOMAKE_OPTIONS = foreign subdir-objects
ACLOCAL_AMFLAGS  = -I build

############################################################

AM_CFLAGS = -Iinclude -Wall $(DEPS_CFLAGS)

############################################################

core_src :=
core_src += include/vigor.h
core_src += src/impl.h
core_src += src/basex.c
core_src += src/cache.c
core_src += src/config.c
core_src += src/fqdn.c
core_src += src/hash.c
core_src += src/list.c
core_src += src/lock.c
core_src += src/log.c
core_src += src/mem.c
core_src += src/path.c
core_src += src/proc.c
core_src += src/rand.c
core_src += src/signals.c
core_src += src/strings.c
core_src += src/time.c

lib_LTLIBRARIES = libvigor.la
libvigor_la_SOURCES = $(core_src)
libvigor_la_LDFLAGS = --version-info 1:0:0

include_HEADERS = include/vigor.h

CTAP_TESTS =

CTAP_TESTS += t/basex
t_basex_SOURCES = t/basex.c t/test.h
t_basex_LDFLAGS = libvigor.la

CTAP_TESTS += t/cache
t_cache_SOURCES = t/cache.c t/test.h
t_cache_LDFLAGS = libvigor.la

CTAP_TESTS += t/hash
t_hash_SOURCES = t/hash.c t/test.h
t_hash_LDFLAGS = libvigor.la

CTAP_TESTS += t/list
t_list_SOURCES = t/list.c t/test.h
t_list_LDFLAGS = libvigor.la

CTAP_TESTS += t/path
t_path_SOURCES = t/path.c t/test.h
t_path_LDFLAGS = libvigor.la

CTAP_TESTS += t/strings
t_strings_SOURCES = t/strings.c t/test.h
t_strings_LDFLAGS = libvigor.la

TESTS          = $(CTAP_TESTS)
BUILT_TESTS    = $(CTAP_TESTS)
check_PROGRAMS = $(CTAP_TESTS)

EXTRA_DIST  = README COPYING
EXTRA_DIST += bootstrap

############################################################

version:
	@echo $(VERSION)

distfile: dist
	@echo $(PACKAGE)-$(VERSION).tar.gz

manifest:
	@echo >&2 "nothin doin"

test: check

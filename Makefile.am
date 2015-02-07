#
# Makefile.am - libvigor Automake File
#

AUTOMAKE_OPTIONS = foreign subdir-objects
ACLOCAL_AMFLAGS  = -I build

############################################################

AM_CFLAGS = -Iinclude -Wall $(DEPS_CFLAGS)
AM_CFLAGS += @GCOV_CFLAGS@

############################################################

core_src :=
core_src += include/vigor.h
core_src += src/impl.h
core_src += src/basex.c
core_src += src/cache.c
core_src += src/config.c
core_src += src/cert.c
core_src += src/daemon.c
core_src += src/fqdn.c
core_src += src/ha.c
core_src += src/hash.c
core_src += src/list.c
core_src += src/lock.c
core_src += src/log.c
core_src += src/mem.c
core_src += src/mq.c
core_src += src/path.c
core_src += src/proc.c
core_src += src/rand.c
core_src += src/reactor.c
core_src += src/run.c
core_src += src/signals.c
core_src += src/strings.c
core_src += src/time.c

lib_LTLIBRARIES = libvigor.la
libvigor_la_SOURCES = $(core_src)
libvigor_la_LDFLAGS = --version-info 1:4:3

include_HEADERS = include/vigor.h

CTAP_TESTS =

CTAP_TESTS += t/basex
t_basex_SOURCES = t/basex.c t/test.h
t_basex_LDFLAGS = libvigor.la

CTAP_TESTS += t/cache
t_cache_SOURCES = t/cache.c t/test.h
t_cache_LDFLAGS = libvigor.la

CTAP_TESTS += t/cert
t_cert_SOURCES = t/cert.c t/test.h
t_cert_LDFLAGS = libvigor.la

CTAP_TESTS += t/config
t_config_SOURCES = t/config.c t/test.h
t_config_LDFLAGS = libvigor.la

CTAP_TESTS += t/ha
t_ha_SOURCES = t/ha.c t/test.h
t_ha_LDFLAGS = libvigor.la

CTAP_TESTS += t/hash
t_hash_SOURCES = t/hash.c t/test.h
t_hash_LDFLAGS = libvigor.la

CTAP_TESTS += t/list
t_list_SOURCES = t/list.c t/test.h
t_list_LDFLAGS = libvigor.la

CTAP_TESTS += t/lock
t_lock_SOURCES = t/lock.c t/test.h
t_lock_LDFLAGS = libvigor.la

CTAP_TESTS += t/log
t_log_SOURCES = t/log.c t/test.h
t_log_LDFLAGS = libvigor.la

CTAP_TESTS += t/mem
t_mem_SOURCES = t/mem.c t/test.h
t_mem_LDFLAGS = libvigor.la

CTAP_TESTS += t/mq
t_mq_SOURCES = t/mq.c t/test.h
t_mq_LDFLAGS = libvigor.la

CTAP_TESTS += t/path
t_path_SOURCES = t/path.c t/test.h
t_path_LDFLAGS = libvigor.la

CTAP_TESTS += t/proc
t_proc_SOURCES = t/proc.c t/test.h
t_proc_LDFLAGS = libvigor.la

CTAP_TESTS += t/reactor
t_reactor_SOURCES = t/reactor.c t/test.h
t_reactor_LDFLAGS = libvigor.la

CTAP_TESTS += t/run
t_run_SOURCES = t/run.c t/test.h
t_run_LDFLAGS = libvigor.la

CTAP_TESTS += t/signals
t_signals_SOURCES = t/signals.c t/test.h
t_signals_LDFLAGS = libvigor.la

CTAP_TESTS += t/strings
t_strings_SOURCES = t/strings.c t/test.h
t_strings_LDFLAGS = libvigor.la

CTAP_TESTS += t/time
t_time_SOURCES = t/time.c t/test.h
t_time_LDFLAGS = libvigor.la

TESTS          = $(CTAP_TESTS) t/memcheck
BUILT_TESTS    = $(CTAP_TESTS)
check_PROGRAMS = $(CTAP_TESTS)

EXTRA_DIST  = README COPYING
EXTRA_DIST += bootstrap
EXTRA_DIST += t/data
EXTRA_DIST += t/memcheck t/memchecker

test-data:
	rm -f t/tmp/check-data-install.stamp
	make t/tmp/check-data-install.stamp
check_DATA = t/tmp/check-data-install.stamp
t/tmp/check-data-install.stamp:
	mkdir -p t/tmp
	cp -a $(srcdir)/t/data t/tmp
	find t/tmp -type d ! -perm -200 -exec chmod u+w {} ';'
	touch $@

############################################################

version:
	@echo $(VERSION)

distfile: dist
	@echo $(PACKAGE)-$(VERSION).tar.gz

manifest:
	@echo >&2 "nothin doin"

test: check

.PHONY: docs
docs: clean-local-docs
	mkdir docs
	cdoc -R docs -s '(include|src)/' src/* include/*
clean-local-docs:
	rm -fr docs

clean-local: clean-local-check clean-gcov-files clean-local-cov clean-local-docs
clean-local-check:
	rm -fr t/tmp/*
	rm -f check-data-install.stamp
clean-gcov-files:
	@rm -fr coverage
	@find . -name "*.gcda" -o -name "*.gcno" -exec rm {} \;

if GCOV_ENABLED
coverage-clean:
	@lcov --directory . --zerocounters

coverage-report:
	@mkdir -p coverage
	@lcov --compat-libtool --directory . --base-directory . --capture --output-file coverage/app.info
	@genhtml -o coverage/ coverage/app.info

coverage:
	@make coverage-report

clean-local-cov:
	@make coverage-clean

check:
	@make coverage

else
coverage:
	@echo >&2 "nothin doin"
clean-local-cov:
endif

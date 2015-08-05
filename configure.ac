AC_INIT([libvigor], [2.0.0], [bugs@niftylogic.com])
AC_PREREQ(2.63)
AC_CONFIG_SRCDIR([include/vigor.h])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_MACRO_DIR([build])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_PROG_CC
AC_PROG_INSTALL

# AM_PROG_AR is used for linker stuff, but only by newer Automake
# On older Automakes (<1.12 IIRC) it just complains during ./bootstrap
# so we use the ifndef trick here to expand if expandable.
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

LT_INIT

AC_HAVE_LIBRARY(ctap,,
	AC_MSG_ERROR(Missing ctap testing library - see http://github.com/filefrog/ctap))

AC_CHECK_LIB(zmq,
	zmq_msg_send,,
	AC_MSG_ERROR(Missing a recent vintage 0MQ library))

AC_CHECK_LIB(sodium,
	crypto_box_keypair,,
	AC_MSG_ERROR(Missing sodium encryption library))

AC_CHECK_LIB(pthread,
	pthread_create,,
	AC_MSG_ERROR(Missing POSIX threads library))

AC_C_INLINE

AX_TEST_COVERAGE

AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

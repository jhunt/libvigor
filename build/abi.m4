changecom('#')
#
# AX_ABI_VERSION
#
# Quickly set the ABI version of the library
# and export for use in configuring rpm.spec
#
AC_DEFUN([AX_ABI_VERSION],
[
  AC_DEFINE([SOVERSION], ["$1:$2:$3"], [Set the ABI version.])
  AC_SUBST([SOVERSION],  [$1:$2:$3])

  AC_SUBST([SOFULL], [m4_eval([$1] - [$3])])
  AC_SUBST([SOLATEST], ["m4_eval([$1] - [$3]).$3.$2"])
])

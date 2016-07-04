AC_DEFUN([AC_CHECK_GETOPT],
[
  AC_CHECK_HEADER(getopt.h)
  AC_CHECK_DECLS([getopt])
  AC_MSG_CHECKING(for GNU getopt_long)
  AC_LINK_IFELSE([
  AC_LANG_PROGRAM(
  [[
      #include <string.h>
      #include <unistd.h>
      #define _GNU_SOURCE
      #include <getopt.h>
  ]],
  [[
      return getopt_long(0, NULL, NULL, NULL, NULL);
  ]]),
  ]
  AC_MSG_RESULT([yes])
  AC_DEFINE(HAVE_GETOPT_H, 1, [Define to 1 if you have the <getopt.h> header with the GNU `getopt_long' function.])
  have_getopt="yes",
  AC_MSG_RESULT([no])
  have_getopt="no")
  AM_CONDITIONAL(SYSTEM_GETOPT, test "x$have_getopt" = "xyes")
])

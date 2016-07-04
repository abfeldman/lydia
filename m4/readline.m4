AC_DEFUN([AC_CHECK_READLINE],
[
  AC_ARG_WITH([readline],
    [AS_HELP_STRING([--with-readline], [location of the readline library])],
	[ac_cv_with_readline=$withval],
	[ac_cv_with_readline=yes])

  if test $ac_cv_with_readline != no ; then
    AC_CHECK_LIB([readline], [readline], [ac_cv_lib_readline=yes], [ac_cv_lib_readline=no], [-ltermcap])
    AC_CHECK_HEADERS([readline/readline.h])
    AC_CHECK_HEADERS([readline/history.h])
    if test $ac_cv_lib_readline = yes ; then
      ac_cv_have_readline=yes
      READLINELIB="-lreadline -ltermcap"
      AC_DEFINE([HAVE_LIBREADLINE], 1, [Define to 1 if you have the GNU readline library installed])
    fi
  fi
  AC_SUBST(READLINELIB)
])

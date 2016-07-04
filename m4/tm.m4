AC_DEFUN([AC_CHECK_TM],
[
  AC_ARG_WITH([tm],
	[AS_HELP_STRING([--with-tm],[location of the Tm processor])],
	[ac_cv_with_tm=$withval],
	[ac_cv_with_tm=no])
  AC_ARG_VAR(TMROOT,[The root directory of Tm installation])
  for i in "/usr/local" "/usr" "/usr/local/tm" "/opt/tm" $ac_cv_with_tm $TMROOT ; do
    if test -x $i/bin/tm; then
      TMROOT=$i
    fi
  done

  if test "$TMROOT" = "" ; then
    AC_PATH_PROG(TM, tm$EXEEXT, no)
  else
    AC_PATH_PROG(TM, tm$EXEEXT, no, [$TMROOT/bin])
    tm_bindir=$TMROOT/bin
  fi
  AC_MSG_CHECKING(for Tm installation)
  if test $ac_cv_path_TM = no; then
    ac_cv_have_tm=no
  else
    ac_cv_have_tm=yes
  fi
  AC_MSG_RESULT($ac_cv_have_tm)
])

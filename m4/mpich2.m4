AC_DEFUN([AC_CHECK_MPICH2],
[
  if test $ac_cv_with_mpich2 = yes ; then
    CFLAGS="$CFLAGS -DMPI"
    CC=mpicc
  else
    CFLAGS="$CFLAGS -DMPI"
    CC=$ac_cv_with_mpich2/bin/mpicc
  fi

  AC_MSG_CHECKING(for MPICH2)
  AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <mpi.h>]])],
                [ac_cv_have_mpich2=yes],
                [ac_cv_have_mpich2=no],
                [ac_cv_have_mpich2=no])
  AC_MSG_RESULT($ac_cv_have_mpich2)
])

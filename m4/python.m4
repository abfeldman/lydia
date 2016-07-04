## --------------------------------- ##              -*- Autoconf -*-
## Check if Python is available.     ##
## --------------------------------- ##
##

dnl NOTE: AM_CHECK_PYTHON contains a workaround that has the side-effect of 
dnl setting $prefix, even if it isn't defined by the user. Be aware of this
dnl when doing autoconf tweaking.

AC_DEFUN([AC_CHECK_PYTHON],
[
 AM_PATH_PYTHON(2.4,, :)
 AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])

 # this is a workaround to eval the ${prefix} in $pythondir.
 test "x$prefix" = xNONE && prefix=$ac_default_prefix
 eval pythondir=$pythondir
 eval pkgpythondir=$pkgpythondir

 if test "$PYTHON" = : ; then
   ac_cv_have_python=no
 else
   ac_cv_have_python=yes
 fi
])
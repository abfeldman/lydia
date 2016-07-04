dnl Autoconf file for Lydia
AC_PREREQ([2.63])
AC_INIT([lydia],[2.4],[alex@llama.gs])
AC_CONFIG_SRCDIR([include/defs.h])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_AUX_DIR([auxdir])
AC_CONFIG_MACRO_DIR([m4])

dnl Checks for programs
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LEX
AC_PROG_YACC

AM_INIT_AUTOMAKE([foreign 1.7.9])

dnl install tool
AC_PROG_INSTALL

dnl ranlib
AC_PROG_RANLIB

dnl indent
AC_ARG_VAR(INDENT,[The GNU indent program])
if test "$INDENT" = "" ; then
  AC_PATH_PROG([INDENT],[indent],:)
fi

dnl Checks for libraries
AC_CHECK_LIB([m], [floor])

dnl Checks for header files
AC_HEADER_STDC
AC_CHECK_HEADERS([unistd.h], [break])

dnl Checks for type
dnl AC_TYPE_SIZE_T

dnl sets WORDS_BIGENDIAN
AC_C_BIGENDIAN

dnl Checks for structures
dnl checks for compiler characteristics
dnl check for warning flags
VL_PROG_CC_WARNINGS(ansi)
AC_CHECK_EXCESSIVE_PRECISION

dnl Checks for library functions
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_REPLACE_FUNCS([strdup strndup strnlen strsep])
AC_CHECK_DECLS([fileno, snprintf])

dnl Tm
AC_CHECK_TM

dnl getopt
AC_CHECK_GETOPT

dnl Python
dnl NOTE: AM_CHECK_PYTHON contains a workaround that has the side-effect of
dnl setting $prefix, even if it isn't defined by the user. Be aware of this
dnl when doing autoconf tweaking.
AC_CHECK_PYTHON

AC_ARG_ENABLE([gnu-install],
  [AS_HELP_STRING([--enable-gnu-install],[use canonical GNU installation layout])],
  [ac_cv_enable_gnu_install=$enableval],
  [ac_cv_enable_gnu_install=no])

dnl readline
AC_CHECK_READLINE

dnl profiling
AC_ARG_ENABLE([gprof],
    [AS_HELP_STRING([--enable-gprof], [use gprof profiling])],
    [ac_cv_enable_gprof=$enableval],
    [AC_CACHE_CHECK([use gprof profiling ], ac_cv_enable_gprof, ac_cv_enable_gprof=no)])

if test $ac_cv_enable_gprof = yes ; then
  AC_MSG_CHECKING(for gprof-compatible compiler)
  ac_cv_have_gprof_comp=no
  if test "$GCC" = yes || test "$GXX" = yes ; then
    ac_cv_have_gprof_comp=yes
    CFLAGS="$CFLAGS -pg"
    CXXFLAGS="$CXXFLAGS -pg"
    CPPFLAGS="$CPPFLAGS -DGPROF"
    LDFLAGS="$LDFLAGS -pg"
  fi
  AC_MSG_RESULT($ac_cv_have_gprof_comp)
fi

componentsdir='${pkgdatadir}/components'
examplesdir='${pkgdatadir}/examples'  

AC_SUBST(componentsdir)
AC_SUBST(examplesdir)

dnl MPICH2
AC_ARG_WITH([mpich2],
	[AS_HELP_STRING([--with-mpich2],[location of MPICH2])],
	[ac_cv_with_mpich2=$withval],
	[ac_cv_with_mpich2=no])
if test $ac_cv_with_mpich2 != no ; then
  AC_CHECK_MPICH2
  if test $ac_cv_have_mpich2 = no ; then
    AC_MSG_ERROR([MPICH2 specified but not working.])
  fi
fi

AC_CONFIG_FILES([Makefile
                 common/Makefile
                 lang/Makefile
                 lang/lc/Makefile
                 lang/lc/tests/Makefile
                 lang/oc/Makefile
                 lang/oc/tests/Makefile
                 converters/Makefile
                 converters/cnf2dnf/Makefile
                 converters/cnf2dnf/tests/Makefile
                 converters/lcm2wff/Makefile
                 converters/lcm2wff/tests/Makefile
                 converters/wff2cnf/Makefile
                 converters/wff2cnf/tests/Makefile
                 converters/wff2nnf/Makefile
                 converters/wff2nnf/tests/Makefile
                 converters/wff2obdd/Makefile
                 converters/wff2obdd/tests/Makefile
                 converters/cnf2obdd/Makefile
                 converters/cnf2obdd/tests/Makefile
                 converters/cnf2horn/Makefile
                 converters/cnf2horn/tests/Makefile
                 converters/obdd2dnf/Makefile
                 converters/obdd2dnf/tests/Makefile
                 converters/mvwff2tvwff/Makefile
                 converters/mvwff2tvwff/tests/Makefile
                 converters/mvcnf2tvcnf/Makefile
                 converters/mvwff2mvcnf/Makefile
                 converters/mvsmoothy/Makefile
                 converters/lcm2mvwff/Makefile
                 converters/lcm2mvwff/tests/Makefile
                 converters/smoothy/Makefile
                 converters/smoothy/tests/Makefile
                 solvers/Makefile
                 solvers/scotty/Makefile
                 solvers/scotty/tests/Makefile
                 solvers/gde/Makefile
                 solvers/gde/tests/Makefile
                 solvers/gotcha/Makefile
                 solvers/gotcha/tests/Makefile
                 solvers/cdas/Makefile
                 solvers/cdas/tests/Makefile
                 solvers/lsim/Makefile
                 solvers/lsim/tests/Makefile
                 solvers/safari/Makefile
                 solvers/safari/tests/Makefile
                 tools/Makefile
                 tools/cnf2dimacs/Makefile
                 tools/kb2lisp/Makefile
                 tools/kbstat/Makefile
                 tools/dimacs/Makefile
                 tools/dimacs/tests/Makefile
                 tools/squeezy/Makefile
                 tools/squeezy/tests/Makefile
                 tools/lyd2graphml/Makefile
                 tools/hwff2dmfp/Makefile
                 tools/cnf2table/Makefile
                 include/Makefile
                 doc/Makefile
                 components/Makefile
                 components/weak/Makefile
                 components/strong/Makefile
                 examples/Makefile
                 examples/ai/Makefile
                 examples/ai/sudoku/Makefile
                 examples/ai/mastermind/Makefile
                 examples/diagnosis/Makefile
                 examples/diagnosis/weak/Makefile
                 examples/diagnosis/strong/Makefile
                 examples/other/Makefile
                 examples/plane/Makefile
                 io/Makefile
                 encdec/Makefile
                 cones/Makefile
                 diag/Makefile
                 trie/Makefile
                 trie/tests/Makefile
                 graph/Makefile
                 interface/Makefile
                 testsuite/Makefile
                 util/Makefile
                 tms/Makefile
                 tms/ltms/Makefile
                 tms/atms/Makefile
                 tms/mvltms/Makefile
                 sat/Makefile
                 sat/bcp/Makefile
                 sat/lsss/Makefile
                 sat/minisat/Makefile])

AC_OUTPUT

echo
echo "Configuration:"
echo
echo "  installation prefix: ${prefix}"
echo
echo "  python support:      $ac_cv_have_python"
if test $ac_cv_have_python = no ; then
   echo
   echo "    WARNING:  Without Python, \"make check\" will have no effect."
fi
echo

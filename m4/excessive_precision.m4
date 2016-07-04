AC_DEFUN([AC_CHECK_EXCESSIVE_PRECISION],
[
AC_MSG_CHECKING([for FPU excessive precision])
old_CFLAGS=$CFLAGS
CFLAGS=""
AC_RUN_IFELSE(
[   AC_LANG_SOURCE([[
    double foo() 
    {
        double x = 123;
        double y = 2345.54;
        return x + 44 * y;
    }

    int main() 
    {
        return (foo() == foo());
    }
    ]])
],
[AC_MSG_RESULT([yes])
AC_DEFINE(EXCESSIVE_PRECISION, 1, [Define to 1 if the FPU exposes excessive precision])
AC_CHECK_SET_MATH_DOUBLE_PRECISION],
[AC_MSG_RESULT([no])],[AC_MSG_RESULT([no])])
CFLAGS=$old_CFLAGS
])

AC_DEFUN([AC_CHECK_SET_MATH_DOUBLE_PRECISION],
[
AC_MSG_CHECKING([for a way to set the FPU double precision])
AC_RUN_IFELSE(
[   AC_LANG_SOURCE([[
    #include <fpu_control.h>

    double foo() 
    {
        double x = 123;
        double y = 2345.54;
        return x + 44 * y;
    }

    int main() 
    {
        fpu_control_t fpu_control = 0x027f ;
        _FPU_SETCW(fpu_control);
        return (foo() != foo());
    }
]])
],
[AC_MSG_RESULT([yes])
AC_DEFINE(SET_MATH_DOUBLE_PRECISION, 1, [Define to 1 if we can set the FPU double precision])],
[AC_MSG_RESULT([no])],[AC_MSG_RESULT([no])])
])

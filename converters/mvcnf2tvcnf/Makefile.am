bin_PROGRAMS         = mvcnf2tvcnf

mvcnf2tvcnf_SOURCES  = mvcnf2tvcnf.c
mvcnf2tvcnf_LDADD    = $(top_builddir)/converters/wff2cnf/libwff2cnf.a \
                       $(top_builddir)/encdec/libencdec.a \
                       $(top_builddir)/common/libcommon.a \
                       $(top_builddir)/util/libutil.a

AM_CPPFLAGS          = -I$(top_builddir)/common \
                       -I$(top_srcdir)/converters/wff2cnf \
                       -I$(top_srcdir)/encdec \
                       -I$(top_srcdir)/util \
                       -I$(top_srcdir)/include \
                       -I$(top_srcdir)/common

include $(top_srcdir)/local.mk

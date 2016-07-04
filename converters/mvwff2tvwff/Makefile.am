SUBDIRS              = tests

bin_PROGRAMS         = mvwff2tvwff

mvwff2tvwff_SOURCES  = mvwff2tvwff.c
mvwff2tvwff_LDADD    = $(top_builddir)/encdec/libencdec.a \
                       $(top_builddir)/common/libcommon.a \
                       $(top_builddir)/util/libutil.a \
                       $(top_builddir)/converters/wff2cnf/libwff2cnf.a

AM_CPPFLAGS          = -I$(top_builddir)/common \
                       -I$(top_srcdir)/encdec \
                       -I$(top_srcdir)/util \
                       -I$(top_srcdir)/include \
                       -I$(top_srcdir)/common

include $(top_srcdir)/local.mk

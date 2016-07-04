SUBDIRS              = tests

bin_PROGRAMS         = dimacs

dimacs_SOURCES       = dimacs.c pp.c pp.h
dimacs_LDADD         = $(top_builddir)/sat/lsss/liblsss.a \
                       $(top_builddir)/common/libcommon.a \
                       $(top_builddir)/util/libutil.a \
                       $(top_builddir)/util/libstat.a

AM_CPPFLAGS         = -I$(top_builddir)/common \
                      -I$(top_srcdir)/include \
                      -I$(top_srcdir)/common \
                      -I$(top_srcdir)/util \
                      -I$(top_srcdir)/sat/lsss

include $(top_srcdir)/local.mk

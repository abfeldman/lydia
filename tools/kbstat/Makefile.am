bin_PROGRAMS    = kbstat

kbstat_SOURCES  = kbstat.c kbstat.h
kbstat_LDADD    = $(top_builddir)/common/libcommon.a \
                  $(top_builddir)/util/libutil.a

AM_CPPFLAGS     = -I$(top_builddir)/common \
                  -I$(top_srcdir)/common \
                  -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

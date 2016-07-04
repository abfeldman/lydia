SUBDIRS           = bcp lsss minisat

AM_CPPFLAGS       = -I$(top_builddir)/common \
                    -I$(top_srcdir)/common \
                    -I$(top_srcdir)/sat/bcp \
                    -I$(top_srcdir)/sat/lsss \
                    -I$(top_srcdir)/sat/minisat

lib_LIBRARIES     = libsat.a

include_HEADERS   = sat.h
libsat_a_SOURCES  = sat.c

include $(top_srcdir)/local.mk

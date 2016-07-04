bin_PROGRAMS            = cnf2table
lib_LIBRARIES           = libcnf2table.a

include_HEADERS         = cnf2table.h
libcnf2table_a_SOURCES  = cnf2table.c

cnf2table_SOURCES       = main.c

cnf2table_LDADD         = libcnf2table.a
cnf2table_LDADD        += $(top_builddir)/sat/libsat.a
cnf2table_LDADD        += $(top_builddir)/sat/lsss/liblsss.a
cnf2table_LDADD        += $(top_builddir)/sat/minisat/libsat_minisat.a
cnf2table_LDADD        += $(top_builddir)/sat/bcp/libsat_bcp.a
cnf2table_LDADD        += $(top_builddir)/io/libio.a
cnf2table_LDADD        += $(top_builddir)/util/libutil.a
cnf2table_LDADD        += $(top_builddir)/util/libstat.a
cnf2table_LDADD        += $(top_builddir)/common/libcommon.a
cnf2table_LDADD        += -lstdc++

AM_CPPFLAGS             = -I$(top_builddir)/common
AM_CPPFLAGS            += -I$(top_srcdir)/sat/lsss
AM_CPPFLAGS            += -I$(top_srcdir)/sat/bcp
AM_CPPFLAGS            += -I$(top_srcdir)/include
AM_CPPFLAGS            += -I$(top_srcdir)/common
AM_CPPFLAGS            += -I$(top_srcdir)/util
AM_CPPFLAGS            += -I$(top_srcdir)/sat
AM_CPPFLAGS            += -I$(top_srcdir)/io

include $(top_srcdir)/local.mk

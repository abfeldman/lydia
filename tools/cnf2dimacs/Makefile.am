bin_PROGRAMS       = cnf2dimacs

cnf2dimacs_SOURCES = cnf2dimacs.c pp.c pp.h
cnf2dimacs_LDADD   = $(top_builddir)/common/libcommon.a \
                     $(top_builddir)/util/libutil.a \
                     $(top_builddir)/io/libio.a

AM_CPPFLAGS        = -I$(top_builddir)/common \
                     -I$(top_srcdir)/common \
                     -I$(top_srcdir)/util \
                     -I$(top_srcdir)/io

include $(top_srcdir)/local.mk

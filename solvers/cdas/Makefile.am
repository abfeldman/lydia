SUBDIRS                = tests

bin_PROGRAMS           = cdas
lib_LIBRARIES          = libcdas.a

include_HEADERS        = cdas.h consistency.h
libcdas_a_SOURCES      = cdas.c cdas.h ds.c ds.h debug.c debug.h \
                         consistency.c consistency.h

cdas_SOURCES           = main.c

cdas_LDADD             = libcdas.a \
                         $(top_builddir)/sat/libsat.a \
                         $(top_builddir)/sat/lsss/liblsss.a \
                         $(top_builddir)/sat/minisat/libsat_minisat.a \
                         $(top_builddir)/sat/bcp/libsat_bcp.a \
                         $(top_builddir)/tms/ltms/libltms.a \
                         $(top_builddir)/util/libutil.a \
                         $(top_builddir)/util/libstat.a \
                         $(top_builddir)/trie/libtrie.a \
                         $(top_builddir)/diag/libdiag.a \
                         $(top_builddir)/diag/libobs.a \
                         $(top_builddir)/common/libcommon.a \
                         $(top_builddir)/encdec/libencdec.a \
                         $(top_builddir)/io/libio.a \
                         $(READLINELIB) \
                         -lstdc++

AM_CPPFLAGS            = -I$(top_builddir)/common \
                         -I$(top_srcdir)/sat/lsss \
                         -I$(top_srcdir)/tms/ltms \
                         -I$(top_srcdir)/sat \
                         -I$(top_srcdir)/include \
                         -I$(top_srcdir)/common \
                         -I$(top_srcdir)/trie \
                         -I$(top_srcdir)/diag \
                         -I$(top_srcdir)/util \
                         -I$(top_srcdir)/io

include $(top_srcdir)/local.mk

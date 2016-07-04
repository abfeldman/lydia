SUBDIRS             = tests

bin_PROGRAMS        = safari
lib_LIBRARIES       = libsafari.a

include_HEADERS     = safari.h
libsafari_a_SOURCES = safari.c ds.c ds.h consistency.c consistency.h

safari_SOURCES      = main.c

safari_LDADD        = libsafari.a \
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
                      $(top_builddir)/encdec/libencdec.a \
                      $(top_builddir)/common/libcommon.a \
                      $(top_builddir)/cones/libcones.a \
                      $(top_builddir)/io/libio.a \
                      $(READLINELIB) \
                      -lstdc++

AM_CPPFLAGS         = -I$(top_builddir)/common \
                      -I$(top_srcdir)/sat/lsss \
                      -I$(top_srcdir)/tms/ltms \
                      -I$(top_srcdir)/include \
                      -I$(top_srcdir)/common \
                      -I$(top_srcdir)/cones \
                      -I$(top_srcdir)/encdec \
                      -I$(top_srcdir)/trie \
                      -I$(top_srcdir)/diag \
                      -I$(top_srcdir)/util \
                      -I$(top_srcdir)/sat \
                      -I$(top_srcdir)/io

include $(top_srcdir)/local.mk

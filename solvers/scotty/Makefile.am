SUBDIRS              = tests

bin_PROGRAMS         = scotty
noinst_LIBRARIES     = libscotty.a

libscotty_a_SOURCES  = scotty.h scotty.c

scotty_SOURCES       = main.c

scotty_LDADD         = libscotty.a \
                       $(top_builddir)/io/libio.a \
                       $(top_builddir)/diag/libobs.a \
                       $(top_builddir)/diag/libdiag.a \
                       $(top_builddir)/trie/libtrie.a \
                       $(top_builddir)/util/libutil.a \
                       $(top_builddir)/util/libstat.a \
                       $(top_builddir)/encdec/libencdec.a \
                       $(top_builddir)/common/libcommon.a \
                       $(READLINELIB)

AM_CPPFLAGS          = -I$(top_builddir)/common \
                       -I$(top_srcdir)/include \
                       -I$(top_srcdir)/encdec \
                       -I$(top_srcdir)/common \
                       -I$(top_srcdir)/trie \
                       -I$(top_srcdir)/util \
                       -I$(top_srcdir)/diag \
                       -I$(top_srcdir)/io

include $(top_srcdir)/local.mk

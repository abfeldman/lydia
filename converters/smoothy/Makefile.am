SUBDIRS              = tests

bin_PROGRAMS         = smoothy
lib_LIBRARIES        = libsmoothy.a
include_HEADERS      = inline.h smoothy.h

libsmoothy_a_SOURCES = inline.c smoothy.c simplify.h simplify.c

smoothy_SOURCES      = main.c
smoothy_LDADD        = libsmoothy.a \
                       $(top_builddir)/common/libcommon.a \
                       $(top_builddir)/trie/libtrie.a \
                       $(top_builddir)/util/libutil.a \
                       $(top_builddir)/util/libstat.a

AM_CPPFLAGS          = -I$(top_builddir)/common \
                       -I$(top_srcdir)/common \
                       -I$(top_srcdir)/include \
                       -I$(top_srcdir)/trie \
                       -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

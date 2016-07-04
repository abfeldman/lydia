SUBDIRS               = tests

bin_PROGRAMS          = squeezy
lib_LIBRARIES         = libsqueezy.a

libsqueezy_a_SOURCES  = pi.c
libsqueezy_a_SOURCES += pi.h
libsqueezy_a_SOURCES += squeezy.c
libsqueezy_a_SOURCES += squeezy.h
libsqueezy_a_SOURCES += transv.c
libsqueezy_a_SOURCES += transv.h

squeezy_SOURCES       = main.c
squeezy_LDADD         = libsqueezy.a
squeezy_LDADD        += $(top_builddir)/common/libcommon.a
squeezy_LDADD        += $(top_builddir)/trie/libtrie.a
squeezy_LDADD        += $(top_builddir)/util/libutil.a

AM_CPPFLAGS           = -I$(top_builddir)/common
AM_CPPFLAGS          += -I$(top_srcdir)/include
AM_CPPFLAGS          += -I$(top_srcdir)/common
AM_CPPFLAGS          += -I$(top_srcdir)/trie
AM_CPPFLAGS          += -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

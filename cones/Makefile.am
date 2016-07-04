lib_LIBRARIES      = libcones.a

libcones_a_SOURCES = cones.h cones.c

include_HEADERS    = cones.h

AM_CPPFLAGS        = -I$(top_builddir)/common \
                     -I$(top_srcdir)/include \
                     -I$(top_srcdir)/encdec \
                     -I$(top_srcdir)/common \
                     -I$(top_srcdir)/trie \
                     -I$(top_srcdir)/util \
                     -I$(top_srcdir)/diag \
                     -I$(top_srcdir)/sat

include $(top_srcdir)/local.mk

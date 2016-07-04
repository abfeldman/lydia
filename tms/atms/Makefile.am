lib_LIBRARIES     = libatms.a
include_HEADERS   = atms.h
libatms_a_SOURCES = atms.c debug.h debug.c

AM_CPPFLAGS       = -I$(top_builddir)/common \
                    -I$(top_srcdir)/include \
                    -I$(top_srcdir)/encdec \
                    -I$(top_srcdir)/common \
                    -I$(top_srcdir)/trie \
                    -I$(top_srcdir)/io

include $(top_srcdir)/local.mk

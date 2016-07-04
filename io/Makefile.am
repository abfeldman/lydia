lib_LIBRARIES   = libio.a

libio_a_SOURCES = io.c io.h pp_variable.c pp_variable.h \
                   lreadline.c lreadline.h

include_HEADERS = io.h pp_variable.h

AM_CPPFLAGS     = -I$(top_builddir)/common \
                  -I$(top_srcdir)/include \
                  -I$(top_srcdir)/common \
                  -I$(top_srcdir)/trie \
                  -I$(top_srcdir)/diag \
                  -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

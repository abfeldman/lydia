lib_LIBRARIES     = libdiag.a libobs.a

libdiag_a_SOURCES = diag.h diag.c
libobs_a_SOURCES  = obs.h obs.c

include_HEADERS   = diag.h obs.h

AM_CPPFLAGS       = -I$(top_builddir)/common \
                    -I$(top_srcdir)/solvers/gotcha \
                    -I$(top_srcdir)/include \
                    -I$(top_srcdir)/encdec \
                    -I$(top_srcdir)/common \
                    -I$(top_srcdir)/util \
                    -I$(top_srcdir)/trie

include $(top_srcdir)/local.mk

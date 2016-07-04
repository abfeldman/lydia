bin_PROGRAMS     = kb2lisp

kb2lisp_SOURCES = kb2lisp.c pp.c pp.h
kb2lisp_LDADD   = $(top_builddir)/common/libcommon.a \
                  $(top_builddir)/util/libutil.a

AM_CPPFLAGS     = -I$(top_builddir)/common \
                  -I$(top_srcdir)/common \
                  -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

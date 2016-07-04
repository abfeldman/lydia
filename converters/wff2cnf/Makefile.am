SUBDIRS              = tests

bin_PROGRAMS         = wff2cnf
lib_LIBRARIES        = libwff2cnf.a

include_HEADERS      = wff2cnf.h
libwff2cnf_a_SOURCES = wff2cnf.c

wff2cnf_SOURCES      = main.c
wff2cnf_LDADD        = libwff2cnf.a \
                       $(top_builddir)/common/libcommon.a \
                       $(top_builddir)/util/libutil.a \
                       $(top_builddir)/io/libio.a

AM_CPPFLAGS          = -I$(top_builddir)/common \
                       -I$(top_srcdir)/common \
                       -I$(top_srcdir)/io \
                       -I$(top_srcdir)/include \
                       -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

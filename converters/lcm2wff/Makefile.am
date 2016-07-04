SUBDIRS              = tests

bin_PROGRAMS         = lcm2wff
lib_LIBRARIES        = liblcm2wff.a

include_HEADERS      = lcm2wff.h
liblcm2wff_a_SOURCES = walk.c walk.h lcm2wff.c

lcm2wff_SOURCES      = main.c
lcm2wff_LDADD        = $(top_builddir)/common/libcommon.a \
                       $(top_builddir)/util/libutil.a \
                       liblcm2wff.a

AM_CPPFLAGS          = -I$(top_builddir)/common \
                       -I$(top_srcdir)/common \
                       -I$(top_srcdir)/include \
                       -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

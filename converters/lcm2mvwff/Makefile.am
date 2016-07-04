SUBDIRS                = tests

bin_PROGRAMS           = lcm2mvwff
lib_LIBRARIES          = liblcm2mvwff.a

include_HEADERS        = lcm2mvwff.h
liblcm2mvwff_a_SOURCES = walk.c walk.h lcm2mvwff.c \
                         normalize.c normalize.h fold.c fold.h

lcm2mvwff_SOURCES      = main.c
lcm2mvwff_LDADD        = $(top_builddir)/common/libcommon.a \
                         $(top_builddir)/util/libutil.a \
                         liblcm2mvwff.a

AM_CPPFLAGS            = -I$(top_builddir)/common \
                         -I$(top_srcdir)/common \
                         -I$(top_srcdir)/include \
                         -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

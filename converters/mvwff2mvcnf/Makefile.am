bin_PROGRAMS             = mvwff2mvcnf
lib_LIBRARIES            = libmvwff2mvcnf.a

include_HEADERS          = mvwff2mvcnf.h
libmvwff2mvcnf_a_SOURCES = mvwff2mvcnf.c

mvwff2mvcnf_SOURCES      = main.c

mvwff2mvcnf_LDADD        = libmvwff2mvcnf.a \
                           $(top_builddir)/common/libcommon.a \
                           $(top_builddir)/util/libutil.a

AM_CPPFLAGS              = -I$(top_srcdir)/include \
                           -I$(top_builddir)/common \
                           -I$(top_srcdir)/common \
                           -I$(top_srcdir)/util

include $(top_srcdir)/local.mk


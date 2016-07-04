bin_PROGRAMS           = mvsmoothy
lib_LIBRARIES          = libmvsmoothy.a

include_HEADERS        = flatten_mvcnf.h flatten_mvdnf.h flatten_mvwff.h mvsmoothy.h
libmvsmoothy_a_SOURCES = flatten_mvcnf.c flatten_mvdnf.c flatten_mvwff.c mvsmoothy.c

mvsmoothy_SOURCES      = main.c

mvsmoothy_LDADD        = libmvsmoothy.a \
                         $(top_builddir)/common/libcommon.a \
                         $(top_builddir)/util/libutil.a \
                         $(top_builddir)/util/libstat.a \
                         $(top_builddir)/converters/smoothy/libsmoothy.a \
                         $(READLINELIB)

AM_CPPFLAGS            = -I$(top_builddir)/common \
                         -I$(top_srcdir)/include \
                         -I$(top_srcdir)/common \
                         -I$(top_srcdir)/util \
                         -I$(top_srcdir)/converters/smoothy

include $(top_srcdir)/local.mk


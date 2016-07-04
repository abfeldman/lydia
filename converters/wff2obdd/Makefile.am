SUBDIRS               = tests

bin_PROGRAMS          = wff2obdd
noinst_LIBRARIES      = libwff2obdd.a

libwff2obdd_a_SOURCES = wff2obdd.c wff2obdd.h

wff2obdd_SOURCES      = main.c
wff2obdd_LDADD        = libwff2obdd.a \
                        $(top_builddir)/common/libcommon.a \
                        $(top_builddir)/util/libutil.a \
                        $(top_builddir)/util/libstat.a \
                        $(TMLIB)

AM_CPPFLAGS           = -I$(top_builddir)/common \
                        -I$(top_srcdir)/common \
                        -I$(top_srcdir)/include \
                        -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

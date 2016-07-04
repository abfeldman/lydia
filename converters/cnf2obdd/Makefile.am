SUBDIRS               = tests

bin_PROGRAMS          = cnf2obdd
noinst_LIBRARIES      = libcnf2obdd.a

libcnf2obdd_a_SOURCES = cnf2obdd.c cnf2obdd.h

cnf2obdd_SOURCES      = main.c
cnf2obdd_LDADD        = libcnf2obdd.a \
                        $(top_builddir)/common/libcommon.a \
                        $(top_builddir)/util/libutil.a

AM_CPPFLAGS           = -I$(top_builddir)/common \
                        -I$(top_srcdir)/common \
                        -I$(top_srcdir)/include \
                        -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

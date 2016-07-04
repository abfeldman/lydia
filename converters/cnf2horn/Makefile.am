SUBDIRS               = tests

bin_PROGRAMS          = cnf2horn
lib_LIBRARIES         = libcnf2horn.a

include_HEADERS       = cnf2horn.h
libcnf2horn_a_SOURCES = cnf2horn.c

cnf2horn_SOURCES      = main.c
cnf2horn_LDADD        = libcnf2horn.a \
                        $(top_builddir)/common/libcommon.a \
                        $(top_builddir)/util/libutil.a \
                        $(top_builddir)/util/libstat.a

AM_CPPFLAGS           = -I$(top_builddir)/common \
                        -I$(top_srcdir)/include \
                        -I$(top_srcdir)/common \
                        -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

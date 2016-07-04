SUBDIRS               = tests

bin_PROGRAMS          = obdd2dnf
noinst_LIBRARIES      = libobdd2dnf.a

libobdd2dnf_a_SOURCES = obdd2dnf.c obdd2dnf.h

obdd2dnf_SOURCES      = main.c
obdd2dnf_LDADD        = libobdd2dnf.a \
                        $(top_builddir)/common/libcommon.a \
                        $(top_builddir)/util/libutil.a

AM_CPPFLAGS           = -I$(top_builddir)/common \
                        -I$(top_srcdir)/common \
                        -I$(top_srcdir)/include \
                        -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

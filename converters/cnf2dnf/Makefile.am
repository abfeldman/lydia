SUBDIRS              = tests

bin_PROGRAMS         = cnf2dnf
lib_LIBRARIES        = libcnf2dnf.a

include_HEADERS      = cnf2dnf.h
libcnf2dnf_a_SOURCES = cnf2dnf.c

cnf2dnf_SOURCES      = main.c
cnf2dnf_LDADD        = libcnf2dnf.a \
                       $(top_builddir)/sat/lsss/liblsss.a \
                       $(top_builddir)/common/libcommon.a \
                       $(top_builddir)/util/libutil.a \
                       $(top_builddir)/util/libstat.a

AM_CPPFLAGS         = -I$(top_srcdir)/include \
                      -I$(top_builddir)/common \
                      -I$(top_srcdir)/common \
                      -I$(top_srcdir)/util \
                      -I$(top_srcdir)/sat/lsss

include $(top_srcdir)/local.mk

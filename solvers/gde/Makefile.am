SUBDIRS           = tests

bin_PROGRAMS      = gde
noinst_LIBRARIES  = libgde.a

libgde_a_SOURCES  = gde.h gde.c

gde_SOURCES       = main.c

gde_LDADD         = libgde.a \
                    $(top_builddir)/io/libio.a \
                    $(top_builddir)/tms/atms/libatms.a \
                    $(top_builddir)/diag/libobs.a \
                    $(top_builddir)/diag/libdiag.a \
                    $(top_builddir)/trie/libtrie.a \
                    $(top_builddir)/util/libutil.a \
                    $(top_builddir)/util/libstat.a \
                    $(top_builddir)/encdec/libencdec.a \
                    $(top_builddir)/common/libcommon.a \
                    $(READLINELIB)

AM_CPPFLAGS       = -I$(top_builddir)/common \
                    -I$(top_srcdir)/tms/atms \
                    -I$(top_srcdir)/include \
                    -I$(top_srcdir)/encdec \
                    -I$(top_srcdir)/common \
                    -I$(top_srcdir)/trie \
                    -I$(top_srcdir)/util \
                    -I$(top_srcdir)/diag \
                    -I$(top_srcdir)/io

include $(top_srcdir)/local.mk

SUBDIRS           = tests

bin_PROGRAMS      = lsim
lib_LIBRARIES     = liblsim.a

include_HEADERS   = lsim.h
liblsim_a_SOURCES = lsim.c lsim.h

lsim_SOURCES      = main.c

lsim_LDADD        = liblsim.a \
                    $(top_builddir)/solvers/gotcha/libgotcha.a \
                    $(top_builddir)/cones/libcones.a \
                    $(top_builddir)/io/libio.a \
                    $(top_builddir)/sat/libsat.a \
                    $(top_builddir)/sat/lsss/liblsss.a \
                    $(top_builddir)/sat/minisat/libsat_minisat.a \
                    $(top_builddir)/sat/bcp/libsat_bcp.a \
                    $(top_builddir)/diag/libobs.a \
                    $(top_builddir)/diag/libdiag.a \
                    $(top_builddir)/trie/libtrie.a \
                    $(top_builddir)/encdec/libencdec.a \
                    $(top_builddir)/common/libcommon.a \
                    $(top_builddir)/converters/smoothy/libsmoothy.a \
                    $(top_builddir)/util/libutil.a \
                    $(top_builddir)/util/libstat.a \
                    $(READLINELIB) \
                    -lstdc++

AM_CPPFLAGS       = -I$(top_builddir)/common \
                    -I$(top_srcdir)/converters/smoothy \
                    -I$(top_srcdir)/solvers/gotcha \
                    -I$(top_srcdir)/sat/bcp \
                    -I$(top_srcdir)/include \
                    -I$(top_srcdir)/encdec \
                    -I$(top_srcdir)/common \
                    -I$(top_srcdir)/cones \
                    -I$(top_srcdir)/trie \
                    -I$(top_srcdir)/util \
                    -I$(top_srcdir)/diag \
                    -I$(top_srcdir)/io

include $(top_srcdir)/local.mk

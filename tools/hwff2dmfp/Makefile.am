bin_PROGRAMS      = hwff2dmfp

hwff2dmfp_SOURCES = hwff2dmfp.c
hwff2dmfp_LDADD   = $(top_builddir)/common/libcommon.a \
                    $(top_builddir)/util/libutil.a \
                    $(top_builddir)/graph/libgraph.a

AM_CPPFLAGS       = -I$(top_builddir)/common \
                    -I$(top_srcdir)/common \
                    -I$(top_srcdir)/include \
                    -I$(top_srcdir)/smoothy \
                    -I$(top_srcdir)/util \
                    -I$(top_srcdir)/graph

include $(top_srcdir)/local.mk

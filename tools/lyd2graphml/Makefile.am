bin_PROGRAMS        = cnf2graphml wff2graphml

cnf2graphml_SOURCES = cnf2graphml.c neato.h neato.c
cnf2graphml_LDADD   = $(top_builddir)/common/libcommon.a \
                      $(top_builddir)/util/libutil.a \
                      $(top_builddir)/graph/libgraph.a

wff2graphml_SOURCES = wff2graphml.c neato.h neato.c
wff2graphml_LDADD   = $(top_builddir)/common/libcommon.a \
                      $(top_builddir)/util/libutil.a \
                      $(top_builddir)/graph/libgraph.a

AM_CPPFLAGS         = -I$(top_builddir)/common \
                      -I$(top_srcdir)/common \
                      -I$(top_srcdir)/include \
                      -I$(top_srcdir)/smoothy \
                      -I$(top_srcdir)/util \
                      -I$(top_srcdir)/graph

include $(top_srcdir)/local.mk

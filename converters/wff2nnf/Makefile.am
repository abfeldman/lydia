SUBDIRS            = tests

bin_PROGRAMS       = wff2nnf
noinst_LIBRARIES   = libtvnnf.a

libtvnnf_a_SOURCES = tvnnf.c tvnnf.h

wff2nnf_SOURCES    = wff2nnf.c
wff2nnf_LDADD      = libtvnnf.a \
                     $(top_builddir)/common/libcommon.a \
                     $(top_builddir)/util/libutil.a

AM_CPPFLAGS     = -I$(top_builddir)/common \
                  -I$(top_srcdir)/common \
                  -I$(top_srcdir)/include \
                  -I$(top_srcdir)/util

include $(top_srcdir)/local.mk

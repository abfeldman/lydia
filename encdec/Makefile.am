lib_LIBRARIES        = libencdec.a

include_HEADERS      = enc.h dec.h
libencdec_a_SOURCES  = sparse.c sparse.h dense.c dense.h enc.c dec.c \
                       card_sort_terms.h card_sort_terms.c

AM_CPPFLAGS          = -I$(top_builddir)/common \
                       -I$(top_srcdir)/util \
                       -I$(top_srcdir)/include \
                       -I$(top_srcdir)/common \
                       -I$(top_srcdir)/converters/wff2cnf

include $(top_srcdir)/local.mk

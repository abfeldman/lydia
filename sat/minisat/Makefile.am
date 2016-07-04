lib_LIBRARIES             = libsat_minisat.a
include_HEADERS           = minisat.h
libsat_minisat_a_SOURCES  = minisat_alg.hpp minisat_heap.hpp minisat_sort.hpp \
                            minisat_types.hpp minisat_boxed_vec.hpp \
                            minisat_vec.hpp minisat_main.hpp
libsat_minisat_a_SOURCES += minisat.c minisat_main.cpp

AM_CPPFLAGS               = -I$(top_builddir)/common -I$(top_srcdir)/common

include $(top_srcdir)/local.mk

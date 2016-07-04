lib_LIBRARIES      = libutil.a libstat.a
libutil_a_SOURCES  = util.c util.h strndup.c strndup.h strnlen.c strnlen.h \
                     strsep.c strsep.h findbin.c findbin.h strdup.h \
                     rand_nr.c rand_nr.h

include_HEADERS    = util.h stat.h rand_nr.h

if !SYSTEM_GETOPT
libutil_a_SOURCES += getopt.c getopt.h getopt1.c getopt_init.c getopt_int.h
endif

libstat_a_SOURCES  = stat.c stat.h

AM_CPPFLAGS        = -I$(top_srcdir)/include

include $(top_srcdir)/local.mk

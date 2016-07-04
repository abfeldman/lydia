SUBDIRS              = tests

bin_PROGRAMS         = oc
lib_LIBRARIES        = liboc.a

include_HEADERS      = oc.h

liboc_a_SOURCES      = oc.c oc_parser.y oc_scanner.h oc_scanner.l \
                       error.h error.c dump.h dump.c typeinfer.h typeinfer.c \
                       rewrite.h rewrite.c expr.h expr.c

oc_SOURCES           = main.c

oc_LDADD             = liboc.a \
                       $(top_builddir)/common/libcommon.a \
                       $(top_builddir)/converters/smoothy/libsmoothy.a \
                       $(top_builddir)/util/libutil.a

AM_CPPFLAGS          = -I$(top_builddir)/common \
                       -I$(top_srcdir)/common \
                       -I$(top_srcdir)/converters/smoothy \
                       -I$(top_srcdir)/util \
                       -I$(top_srcdir)/include

TEMPLATES            = oc.ht oc.ct
DSFILES              = oc.ds
TFILES               = occonf.t
EXTRA_DIST           = $(TEMPLATES) $(DSFILES) $(TFILES)
MAINTAINERCLEANFILES = oc.h oc.c

AM_YFLAGS            = -d -pobs

oc.h: oc.ht oc.ds occonf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include oc.ds oc.ht > oc.h

oc.c: oc.ct oc.ds occonf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include oc.ds oc.ct > oc.c

oc.c: oc.h

include $(top_srcdir)/local.mk

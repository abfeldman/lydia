SUBDIRS              = tests

bin_PROGRAMS         = lc
lib_LIBRARIES        = liblc.a

include_HEADERS      = lc.h

liblc_a_SOURCES      = ast.h ast.c lc.c lc_parser.y \
                       lc_scanner.h lc_scanner.l error.h error.c \
                       typetbl.h typetbl.c symtbl.h symtbl.c \
                       attr.h attr.c typeinfer.h typeinfer.c \
                       rewrite.c rewrite.h dump.c dump.h \
                       lc_types.c lc_types.h evalbool.c evalbool.h \
                       evalint.c evalint.h evalfloat.c evalfloat.h \
                       evalenum.c evalenum.h search.h search.c \
                       array.c array.h expr.c expr.h varexpr.c varexpr.h \
                       applyexpr.c applyexpr.h iter.c iter.h

lc_SOURCES           = main.c

lc_LDADD             = liblc.a \
                       $(top_builddir)/common/libcommon.a \
                       $(top_builddir)/util/libutil.a

AM_CPPFLAGS          = -I$(top_builddir)/common \
                       -I$(top_srcdir)/common \
                       -I$(top_srcdir)/util \
                       -I$(top_srcdir)/include

TEMPLATES            = ast.ht ast.ct
DSFILES              = ast.ds
TFILES               = astconf.t
EXTRA_DIST           = $(TEMPLATES) $(DSFILES) $(TFILES)
MAINTAINERCLEANFILES = ast.h ast.c

AM_YFLAGS            = --defines=y.tab.h

ast.h: ast.ht ast.ds astconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include ast.ds ast.ht > ast.h

ast.c: ast.ct ast.ds astconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include ast.ds ast.ct > ast.c

ast.c: ast.h

include $(top_srcdir)/local.mk

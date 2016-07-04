bin_PROGRAMS  = lydia
lydia_SOURCES = lydia.c

lydia_LDADD   = $(top_builddir)/lang/lc/liblc.a \
                $(top_builddir)/lang/oc/liboc.a \
                $(top_builddir)/solvers/scotty/libscotty.a \
                $(top_builddir)/solvers/gotcha/libgotcha.a \
                $(top_builddir)/cones/libcones.a \
                $(top_builddir)/solvers/safari/libsafari.a \
                $(top_builddir)/solvers/cdas/libcdas.a \
                $(top_builddir)/solvers/lsim/liblsim.a \
                $(top_builddir)/encdec/libencdec.a \
                $(top_builddir)/converters/lcm2wff/liblcm2wff.a \
                $(top_builddir)/converters/wff2cnf/libwff2cnf.a \
                $(top_builddir)/converters/cnf2dnf/libcnf2dnf.a \
                $(top_builddir)/converters/wff2obdd/libwff2obdd.a \
                $(top_builddir)/converters/obdd2dnf/libobdd2dnf.a \
                $(top_builddir)/converters/lcm2mvwff/liblcm2mvwff.a \
                $(top_builddir)/converters/smoothy/libsmoothy.a \
                $(top_builddir)/sat/libsat.a \
                $(top_builddir)/sat/lsss/liblsss.a \
                $(top_builddir)/sat/minisat/libsat_minisat.a \
                $(top_builddir)/sat/bcp/libsat_bcp.a \
                $(top_builddir)/tms/ltms/libltms.a \
                $(top_builddir)/encdec/libencdec.a \
                $(top_builddir)/common/libcommon.a \
                $(top_builddir)/diag/libdiag.a \
                $(top_builddir)/trie/libtrie.a \
                $(top_builddir)/util/libutil.a \
                $(top_builddir)/util/libstat.a \
                $(top_builddir)/diag/libobs.a \
                $(top_builddir)/io/libio.a \
                $(READLINELIB) \
                -lstdc++

AM_CPPFLAGS   = -I$(top_builddir)/common \
                -I$(top_builddir)/lang/lc \
                -I$(top_builddir)/lang/oc \
                -I$(top_srcdir)/include \
                -I$(top_srcdir)/common \
                -I$(top_srcdir)/cones \
                -I$(top_srcdir)/util \
                -I$(top_srcdir)/diag \
                -I$(top_srcdir)/trie \
                -I$(top_srcdir)/io \
                -I$(top_srcdir)/encdec \
                -I$(top_srcdir)/lang/lc \
                -I$(top_srcdir)/lang/oc \
                -I$(top_srcdir)/tms/ltms \
                -I$(top_srcdir)/converters/lcm2wff \
                -I$(top_srcdir)/converters/wff2cnf \
                -I$(top_srcdir)/converters/cnf2dnf \
                -I$(top_srcdir)/converters/wff2obdd \
                -I$(top_srcdir)/converters/obdd2dnf \
                -I$(top_srcdir)/converters/lcm2mvwff \
                -I$(top_srcdir)/converters/smoothy \
                -I$(top_srcdir)/solvers/scotty \
                -I$(top_srcdir)/solvers/gotcha \
                -I$(top_srcdir)/solvers/safari \
                -I$(top_srcdir)/solvers/cdas \
                -I$(top_srcdir)/solvers/lsim

include $(top_srcdir)/local.mk

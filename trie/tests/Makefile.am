if HAVE_PYTHON
TEST_SCRIPT       = trie_tests.py
EXTRA_DIST        = $(TEST_SCRIPT) \
                    tests.xml
endif

noinst_PROGRAMS   = trie_test

trie_test_SOURCES = trie_test.c

trie_test_LDADD   = $(top_builddir)/trie/libtrie.a \
                    $(top_builddir)/graph/libgraph.a \
                    $(top_builddir)/common/libcommon.a \
                    $(top_builddir)/util/libutil.a

AM_CPPFLAGS       = -I$(top_builddir)/common \
                    -I$(top_srcdir)/include \
                    -I$(top_srcdir)/common \
                    -I$(top_srcdir)/graph \
                    -I$(top_srcdir)/trie

include $(top_srcdir)/local.mk

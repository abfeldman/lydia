SUBDIRS           = . tests

lib_LIBRARIES     = libtrie.a

libtrie_a_SOURCES = trie.h trie.c trie_io.h trie_io.c

include_HEADERS   = trie.h trie_io.h

AM_CPPFLAGS       = -I$(top_builddir)/include \
                    -I$(top_srcdir)/include \
                    -I$(top_srcdir)/common

include $(top_srcdir)/local.mk

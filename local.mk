
SRCDIR="$(srcdir)/"
TOPSRCDIR="$(top_srcdir)/"
TOPBUILDDIR="$(top_builddir)/"
SUBDIR="$(subdir)/"

TESTS_ENVIRONMENT = \
	SUBDIR=$(SUBDIR) \
	SRCDIR=$(SRCDIR) \
	TOPSRCDIR=$(TOPSRCDIR) \
	TOPBUILDDIR=$(TOPBUILDDIR)


# Copied from Makefile.in -- we want our additional check-stress
# and check-full targets to be able to recurse into
# subdirectories, just like the built-in 'check' targets.
check-stress-recursive check-full-recursive:
	@failcom='exit 1'; \
	for f in x $$MAKEFLAGS; do \
	  case $$f in \
	    *=* | --[!k]*);; \
	    *k*) failcom='fail=yes';; \
	  esac; \
	done; \
	dot_seen=no; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	  || eval $$failcom; \
	done; \
	if test "$$dot_seen" = "no"; then \
	  $(MAKE) $(AM_MAKEFLAGS) "$$target-am" || exit 1; \
	fi; test -z "$$fail"

check-stress: $(BUILT_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) check-stress-recursive

check-stress-am: all-am
	$(MAKE) $(AM_MAKEFLAGS) check-stress-local

check-local:
if HAVE_PYTHON
	@if test -f "$(SRCDIR)/$(TEST_SCRIPT)" ; then \
		$(TESTS_ENVIRONMENT) $(PYTHON) $(SRCDIR)/$(TEST_SCRIPT) ; \
	fi
endif

check-stress-local:
if HAVE_PYTHON
	@if test -f "stress.xml" ; then \
		$(TESTS_ENVIRONMENT) $(PYTHON) $(SRCDIR)/$(TEST_SCRIPT) --file stress.xml ; \
	fi
endif

check-full: check check-stress


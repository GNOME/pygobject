# Part of the gi subdirectory is handled with non-recursive make to avoid
# py-compile getting confused between gi/types.py and Python's standard
# types module.
SUBDIRS = . examples gi tests pygtkcompat

PLATFORM_VERSION = 3.0

CLEANFILES = \
	`find "$(builddir)" -type f -name "*.pyc" -print`

EXTRA_DIST = \
	autogen.sh \
	pygobject.doap \
	pygobject-$(PLATFORM_VERSION).pc.in \
	PKG-INFO \
	PKG-INFO.in \
	tools/pygi-convert.sh \
	m4/python.m4 \
	m4/introspection.m4 \
	setup.py \
	setup.cfg \
	MANIFEST.in \
	README.rst \
	docs

MAINTAINERCLEANFILES = \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/autoscan.log \
	$(srcdir)/compile \
	$(srcdir)/config.guess \
	$(srcdir)/config.h.in \
	$(srcdir)/config.sub \
	$(srcdir)/configure.scan \
	$(srcdir)/depcomp \
	$(srcdir)/install-sh \
	$(srcdir)/ltmain.sh \
	$(srcdir)/missing \
	$(srcdir)/mkinstalldirs \
	$(srcdir)/omf.make \
	$(srcdir)/xmldocs.make \
	$(srcdir)/gtk-doc.make \
	$(srcdir)/ChangeLog \
	$(srcdir)/py-compile \
	$(srcdir)/m4/libtool.m4 \
	$(srcdir)/m4/ltoptions.m4 \
	$(srcdir)/m4/ltsugar.m4 \
	$(srcdir)/m4/ltversion.m4 \
	$(srcdir)/m4/lt~obsolete.m4 \
	`find "$(srcdir)" -type f -name Makefile.in -print` \
	`find "$(builddir)" -type f -name "*.pyc" -print`

BUILT_EXTRA_DIST = 		\
	ChangeLog

nobase_pyexec_PYTHON = \
	gi/__init__.py \
	gi/types.py \
	gi/module.py \
	gi/importer.py \
	gi/pygtkcompat.py \
	gi/docstring.py \
	gi/_constants.py \
	gi/_propertyhelper.py \
	gi/_signalhelper.py \
	gi/_option.py \
	gi/_error.py \
	gi/_ossighelper.py

# if we build in a separate tree, we need to symlink the *.py files from the
# source tree; Python does not accept the extensions and modules in different
# paths
build_pylinks:
	for f in $(nobase_pyexec_PYTHON); do \
		[ -e $(builddir)/$$f ] || \
			$(LN_S) $(abs_srcdir)/$$f $(builddir)/$$f; \
	done

all-local: build_pylinks

check-local: build_pylinks

if WITH_COMMON
# pkg-config files
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pygobject-$(PLATFORM_VERSION).pc
endif

# python
pyexec_LTLIBRARIES =

EGG_NAME = $(PACKAGE)-$(PACKAGE_VERSION)-py$(PYTHON_VERSION)
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pyexecdir)
	cp $(top_builddir)/PKG-INFO $(DESTDIR)$(pyexecdir)/$(EGG_NAME).egg-info
uninstall-local:
	rm -f $(DESTDIR)$(pyexecdir)/$(EGG_NAME).egg-info

release-news:
	printf "%s - %s\n\n" "$(VERSION)" "`LC_TIME=C date -I`" > NEWS.tmp

	@echo -e "\n\n\nFor blogging, you can copy&paste this HTML formatted news:"
	@echo "-------------- 8< -------------"
	@last=`head -n1 $(top_srcdir)/NEWS | cut -f1 -d' '`; \
        echo "<ul>"; \
	for commit in `git rev-list $$last..`; do \
	   data=`git log --format="format:%s%n%an%n%b" $$commit^..$$commit`; \
	   echo "$$data" | perl -e '$$subj = <>; chomp $$subj; $$author = <>; chomp $$author; while (<>) { if (/bugzilla.gnome.org.*id=([0-9]+)/) { $$bug = $$1 }};  print "  <li>$$subj ($$author)"; if ($$bug) { print " (<a href=\"https://bugzilla.gnome.org/show_bug.cgi?id=$$bug\">#$$bug</a>)"; }; print "</li>\n"'; \
	   echo "$$data" | perl -e '$$subj = <>; chomp $$subj; $$author = <>; chomp $$author; while (<>) { if (/bugzilla.gnome.org.*id=([0-9]+)/) { $$bug = $$1 }};  print "* $$subj ($$author)"; if ($$bug) { print " (#$$bug)"; }; print "\n"' >> NEWS.tmp; \
        done
	@echo "</ul>"
	@echo "-------------- 8< -------------"

	@echo >> NEWS.tmp
	@cat $(top_srcdir)/NEWS >> NEWS.tmp
	@mv NEWS.tmp $(top_srcdir)/NEWS

.PHONY: ChangeLog release-news

ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git $(top_srcdir)/missing --run git log PYGOBJECT_2_17_0^^.. --stat) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> $@); \
	fi


distclean-local:
	if test $(srcdir) = .; then :; else \
	    rm -f $(BUILT_EXTRA_DIST); \
	fi

dist-hook: $(BUILT_EXTRA_DIST)
	files='$(BUILT_EXTRA_DIST)'; \
	for f in $$files; do \
	  if test -f $$f; then d=.; else d=$(srcdir); fi; \
	  rm -f $(distdir)/$$f && cp $$d/$$f $(distdir) || exit 1; done
	rm -rf "$(distdir)/docs/_build"

check.quality:
	(cd $(abs_top_srcdir) && $(PYTHON) -m flake8);

check.gdb:
	cd tests && $(MAKE) check.gdb

check.nemiver:
	cd tests && $(MAKE) check.nemiver

check.valgrind:
	cd tests && $(MAKE) check.valgrind

check.valgrindlog:
	cd tests && $(MAKE) check.valgrindlog

check.valgrindxml:
	cd tests && $(MAKE) check.valgrindxml

@CODE_COVERAGE_RULES@

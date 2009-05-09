ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = 1.7

SUBDIRS = docs codegen glib gobject gio examples tests

PLATFORM_VERSION = 2.0

CLEANFILES =
EXTRA_DIST = 				\
	MANIFEST.in 			\
	pygobject-$(PLATFORM_VERSION).pc.in \
	PKG-INFO 			\
	PKG-INFO.in 			\
	setup.py			\
	pygobject_postinstall.py	\
	pygtk.py			\
	dsextras.py			\
	ChangeLog.pre-2.18		\
	m4/as-ac-expand.m4		\
	m4/jhflags.m4			\
	m4/python.m4

BUILT_EXTRA_DIST = 		\
	ChangeLog

INCLUDES = -I$(top_srcdir)/gobject $(PYTHON_INCLUDES) $(GLIB_CFLAGS)

# pkg-config files
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pygobject-$(PLATFORM_VERSION).pc

# python modules
pkgpyexecdir = $(pyexecdir)/gtk-2.0
pkgpyexec_PYTHON = dsextras.py
pkgpyexec_LTLIBRARIES =

# python
pyexec_PYTHON = pygtk.py
pyexec_LTLIBRARIES =

# linker flags
common_ldflags = -module -avoid-version
if PLATFORM_WIN32
common_ldflags += -no-undefined
endif

# install pth file.
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pythondir)
	echo "gtk-$(PLATFORM_VERSION)" > $(DESTDIR)$(pyexecdir)/pygtk.pth
install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(pyexecdir)
	echo "gtk-$(PLATFORM_VERSION)" > $(DESTDIR)$(pyexecdir)/pygtk.pth
uninstall-local:
	rm -f $(DESTDIR)$(pythondir)/pygtk.pth $(DESTDIR)$(pyexecdir)/pygtk.pth

EGG_NAME = $(PACKAGE)-$(PACKAGE_VERSION)-py$(PYTHON_VERSION)-$(PLATFORM).egg
TMPDIR = `pwd`/$(PACKAGE)-$(PACKAGE_VERSION)

egg: $(top_srcdir)/gobject/gobject.la PKG-INFO
	rm -fr $(TMPDIR)
	echo $(EGG_NAME)
	mkdir -p $(TMPDIR)
	mkdir -p $(TMPDIR)/EGG-INFO
	make -C gobject install-pkgpyexecLTLIBRARIES pkgpyexecdir=$(TMPDIR)
	$(INSTALL) dsextras.py $(TMPDIR)
	rm -fr $(TMPDIR)/gobject.la
	$(INSTALL) PKG-INFO $(TMPDIR)/EGG-INFO
	echo "gobject.so" >> $(TMPDIR)/EGG-INFO/native_libs.txt
	echo "gobject" >> $(TMPDIR)/EGG-INFO/top_level.txt
	echo "dsextras.py" >> $(TMPDIR)/EGG-INFO/top_level.txt
	cd $(TMPDIR) && zip -rq $(EGG_NAME) .
	mv $(TMPDIR)/$(EGG_NAME) .
	rm -fr $(TMPDIR)

doc-dist:
	mkdir pygobject
	cp -r docs/html/*.html pygobject
	cp -r docs/html/*.sgml pygobject
	cp -r docs/html/*.devhelp pygobject
	cp -r docs/style.css pygobject
	tar cfz $(PACKAGE)-docs.tar.gz pygobject
	rm -fr pygobject

release-tag:
	@ROOT=`svn info --xml|egrep ^"<url>"|cut -d\> -f2|cut -d\< -f1`; \
	SVNVERSION=`echo $(VERSION)|sed s/\\\./_/g`;\
	  echo "* Tagging $(VERSION)"; \
          svn cp -m "Tag $(VERSION)" \
              $$ROOT \
              svn+ssh://johan@svn.gnome.org/svn/pygobject/tags/PYGOBJECT_$$SVNVERSION


.PHONY: ChangeLog

ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run git log PYGOBJECT_2_17_0^^.. --stat) | fmt --split-only > $@.tmp \
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

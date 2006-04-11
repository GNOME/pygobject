AUTOMAKE_OPTIONS = 1.7

SUBDIRS = gobject . examples tests 

PLATFORM_VERSION = 2.0

CLEANFILES = pygtk.py
EXTRA_DIST = 				\
	MANIFEST.in 			\
	pygobject-$(PLATFORM_VERSION).pc.in \
	PKG-INFO 			\
	PKG-INFO.in 			\
	setup.py			\
	pygtk.py.in			\
	dsextras.py

INCLUDES = $(PYTHON_INCLUDES) $(GLIB_CFLAGS) -I$(top_srcdir)/gobject

# pkg-config files
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pygobject-$(PLATFORM_VERSION).pc

# python modules
pkgpyexecdir = $(pyexecdir)/gtk-2.0
pkgpyexec_PYTHON = dsextras.py
pkgpyexec_LTLIBRARIES =

pygtk.py: pygtk.py.in
	sed -e s!\@pyexecdir\@!@pyexecdir@! \
		< $< > $@
pygtk.py: Makefile

# python
nodist_pyexec_PYTHON = pygtk.py
pyexec_LTLIBRARIES =
noinst_PYTHON = ltihooks.py

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

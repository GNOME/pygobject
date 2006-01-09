AUTOMAKE_OPTIONS = 1.7

SUBDIRS = gobject . examples tests 

PLATFORM_VERSION = 2.0

CLEANFILES = 
EXTRA_DIST = 				\
	MANIFEST.in 			\
	pygobject-$(PLATFORM_VERSION).pc.in \
	PKG-INFO 			\
	PKG-INFO.in 			\
	setup.py
INCLUDES = $(PYTHON_INCLUDES) $(GLIB_CFLAGS) -I$(top_srcdir)/gobject

# pkg-config files
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pygobject-$(PLATFORM_VERSION).pc

# extension modules
pkgpyexecdir = $(pyexecdir)/gtk-2.0
pkgpyexec_LTLIBRARIES =

# python
pyexec_PYTHON = pygtk.py
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

PLATFORM_VERSION = 2.0

pkgincludedir = $(includedir)/pygtk-$(PLATFORM_VERSION)
pkgpyexecdir = $(pyexecdir)

SUBDIRS = \
	repository \
	overrides

INCLUDES = -I$(top_srcdir)/gobject -I$(top_srcdir)/glib

pygidir = $(pkgpyexecdir)/gi
pygi_PYTHON = \
	types.py \
	module.py \
	importer.py \
	__init__.py

_gi_la_CFLAGS = \
	$(PYTHON_INCLUDES) \
	$(GI_CFLAGS)
_gi_la_LDFLAGS = \
	-module \
	-avoid-version \
	-export-symbols-regex "init_gi|PyInit__gi"
_gi_la_LIBADD = \
	$(GI_LIBS) \
	$(top_builddir)/glib/libpyglib-2.0-@PYTHON_BASENAME@.la
_gi_la_SOURCES = \
	pygi-repository.c \
	pygi-repository.h \
	pygi-info.c \
	pygi-info.h \
	pygi-invoke.c \
	pygi-invoke.h \
	pygi-foreign.c \
	pygi-foreign.h \
	pygi-foreign-gvariant.c \
	pygi-foreign-gvariant.h \
	pygi-struct.c \
	pygi-struct.h \
	pygi-argument.c \
	pygi-argument.h \
	pygi-type.c \
	pygi-type.h \
	pygi-boxed.c \
	pygi-boxed.h \
	pygi-closure.c \
	pygi-closure.h \
	pygi-callbacks.c \
	pygi-callbacks.h \
	pygi.h \
	pygi-private.h \
	pygi-property.c \
	pygi-property.h \
	pygi-signal-closure.c \
	pygi-signal-closure.h \
	pygobject-external.h \
	gimodule.c

_gi_cairo_la_CFLAGS = \
	$(PYTHON_INCLUDES) \
	$(GI_CFLAGS) \
	$(PYCAIRO_CFLAGS)
_gi_cairo_la_LDFLAGS = \
	-module \
	-avoid-version \
	-export-symbols-regex "init_gi_cairo|PyInit__gi_cairo"
_gi_cairo_la_LIBADD = \
	$(GI_LIBS) \
	$(PYCAIRO_LIBS)
_gi_cairo_la_SOURCES = pygi-foreign-cairo.c


pygi_LTLIBRARIES = _gi.la

if ENABLE_CAIRO
pygi_LTLIBRARIES += _gi_cairo.la
endif


# This is to ensure we have a symlink to the .so in the
# build directory, which the Python interpreter can load
# directly without having to know how to parse .la files.
.la.so:
	test -L $@ || $(LN_S) .libs/$@ $@

all-local: $(LTLIBRARIES:.la=.so)
check-local: $(LTLIBRARIES:.la=.so)
clean-local:
	rm -f $(LTLIBRARIES:.la=.so)


-include $(top_srcdir)/git.mk

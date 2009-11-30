PLATFORM_VERSION = 2.0

pkgincludedir = $(includedir)/pygtk-$(PLATFORM_VERSION)
pkgpyexecdir = $(pyexecdir)/gtk-2.0

SUBDIRS = \
	repository \
	overrides

pygidir = $(pkgpyexecdir)/gi
pygi_PYTHON = \
	types.py \
	module.py \
	importer.py \
	__init__.py

_gi_la_CFLAGS = \
	$(PYTHON_INCLUDES) \
	$(GNOME_CFLAGS)
_gi_la_LDFLAGS = \
	-module \
	-avoid-version \
	-export-symbols-regex init_gi
_gi_la_LIBADD = \
	$(GNOME_LIBS)
_gi_la_SOURCES = \
	pygi-repository.c \
	pygi-repository.h \
	pygi-info.c \
	pygi-info.h \
	pygi-struct.c \
	pygi-struct.h \
	pygi-argument.c \
	pygi-argument.h \
	pygi-type.c \
	pygi-type.h \
	pygi-boxed.c \
	pygi-boxed.h \
	pygi.h \
	pygi-private.h \
	pygobject-external.h \
	gimodule.c

pygi_LTLIBRARIES = _gi.la

.la.so:
	$(LN_S) .libs/$@ $@ || true

all: $(pygi_LTLIBRARIES:.la=.so)
clean-local:
	rm -f $(pygi_LTLIBRARIES:.la=.so)


AUTOMAKE_OPTIONS = 1.7
PLATFORM_VERSION = 2.0

pkgincludedir = $(includedir)/pygtk-$(PLATFORM_VERSION)
pkgpyexecdir = $(pyexecdir)/gtk-2.0

if BUILD_GI

SUBDIRS = repository overrides

INCLUDES = \
	-I$(top_srcdir)/gobject \
	$(PYTHON_INCLUDES) \
	$(PYGOBJECT_CFLAGS) \
	$(GOBJECT_INTROSPECTION_CFLAGS)

# headers
pkginclude_HEADERS = pygi.h

# gi Python scripts and modules
pygidir = $(pkgpyexecdir)/gi
pygi_PYTHON = \
	types.py \
	module.py \
	importer.py \
	__init__.py

# linker flags
common_ldflags = -module -avoid-version
if PLATFORM_WIN32
common_ldflags += -no-undefined
endif

# _gi module
_gi_la_CFLAGS = -O0
_gi_la_LDFLAGS = $(common_ldflags) -export-symbols-regex init_gi
_gi_la_LIBADD = $(GOBJECT_INTROSPECTION_LIBS)
_gi_la_SOURCES = \
	pygi-repository.c \
	pygi-repository.h \
	pygi-info.c \
	pygi-info.h \
	pygi-argument.c \
	pygi-argument.h \
	pygi.h \
	pygi-private.h \
	gimodule.c

pygi_LTLIBRARIES = _gi.la


.la.so:
	$(LN_S) .libs/$@ $@ || true

all: $(pygi_LTLIBRARIES:.la=.so)
clean-local:
	rm -f $(pygi_LTLIBRARIES:.la=.so)

endif # BUILD_GI

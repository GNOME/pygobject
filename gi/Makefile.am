AUTOMAKE_OPTIONS = 1.7

SUBDIRS = repository overrides

PLATFORM_VERSION = 2.0

INCLUDES = \
	-I$(top_srcdir)/gobject \
	$(PYTHON_INCLUDES) \
	$(PYGOBJECT_CFLAGS) \
	$(GOBJECT_INTROSPECTION_CFLAGS)

pkginclude_HEADERS = pygi.h

# gi extension modules
pkgpyexecdir = $(pyexecdir)/gtk-2.0/gi

# gi python scripts
pygidir = $(pkgpyexecdir)
pygi_PYTHON = \
	types.py \
	module.py \
	importer.py \
	__init__.py

# gi overrides extension modules
pygioverridesdir = $(pkgpyexecdir)/overrides

# gi overrides scripts
pygioverrides_PYTHON = \
	$(srcdir)/overrides/

# linker flags
common_ldflags = -module -avoid-version
if PLATFORM_WIN32
common_ldflags += -no-undefined
endif

# repo module
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

if BUILD_GOBJECT_INTROSPECTION
pygi_LTLIBRARIES = _gi.la

all: $(pygi_LTLIBRARIES:.la=.so)
clean-local:
	rm -f $(pygi_LTLIBRARIES:.la=.so)
.la.so:
	$(LN_S) .libs/$@ $@ || true
endif


AUTOMAKE_OPTIONS = 1.7
PLATFORM_VERSION = 2.0
INCLUDES = $(PYTHON_INCLUDES) $(GLIB_CFLAGS)

pkgincludedir = $(includedir)/pygtk-$(PLATFORM_VERSION)
pkginclude_HEADERS = pygobject.h

pkgpyexecdir = $(pyexecdir)/gtk-2.0

# gobject python scripts
pygobjectdir = $(pkgpyexecdir)/gobject
pygobject_PYTHON = __init__.py
pygobject_LTLIBRARIES = _gobject.la 


common_ldflags = -module -avoid-version
if PLATFORM_WIN32
common_ldflags += -no-undefined
endif


_gobject_la_CFLAGS = $(GLIB_CFLAGS)
_gobject_la_LDFLAGS = $(common_ldflags) -export-symbols-regex init_gobject
_gobject_la_LIBADD = $(GLIB_LIBS)
_gobject_la_SOURCES = \
	gobjectmodule.c \
	pygboxed.c \
	pygenum.c \
	pygflags.c \
	pygobject.c \
	pygobject.h \
	pygobject-private.h \
	pygmaincontext.c \
	pygmainloop.c \
	pygparamspec.c \
	pygpointer.c \
	pygiochannel.c \
	pygsource.c \
	pygtype.c

if PLATFORM_WIN32
_gobject_la_CFLAGS += -DPLATFORM_WIN32
endif

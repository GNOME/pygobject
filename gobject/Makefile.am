AUTOMAKE_OPTIONS = 1.7
INCLUDES = $(PYTHON_INCLUDES) $(GLIB_CFLAGS)

pkgincludedir = $(includedir)/pygtk-2.0
pkginclude_HEADERS = pygobject.h

pkgpyexecdir = $(pyexecdir)/gtk-2.0
pkgpyexec_LTLIBRARIES = gobject.la 

common_ldflags = -module -avoid-version
if PLATFORM_WIN32
common_ldflags += -no-undefined
endif

gobject_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initgobject
gobject_la_LIBADD = $(GLIB_LIBS)
gobject_la_SOURCES = \
	gobjectmodule.c \
	pygboxed.c \
	pygenum.c \
	pygenum.h \
	pygflags.c \
	pygflags.h \
	pygobject.c \
	pygobject.h \
	pygobject-private.h \
	pygtype.c

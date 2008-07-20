AUTOMAKE_OPTIONS = 1.7
INCLUDES = $(PYTHON_INCLUDES) $(GLIB_CFLAGS) -DPY_SSIZE_T_CLEAN

pkgincludedir = $(includedir)/pygtk-2.0
pkginclude_HEADERS = pyglib.h

lib_LTLIBRARIES = libpyglib-2.0.la

pkgpyexecdir = $(pyexecdir)/gtk-2.0

pyglibdir = $(pkgpyexecdir)/glib
pyglib_PYTHON = 	\
	__init__.py
pyglib_LTLIBRARIES = _glib.la

common_ldflags = -module -avoid-version
if PLATFORM_WIN32
common_ldflags += -no-undefined
endif

libpyglib_2_0_la_CFLAGS = $(GLIB_CFLAGS)
libpyglib_2_0_la_LIBADD = $(GLIB_LIBS) $(FFI_LIBS)
libpyglib_2_0_la_SOURCES = \
	pyglib.c	\
	pyglib.h	\
	pyglib-private.h

_glib_la_CFLAGS = $(GLIB_CFLAGS)
_glib_la_LDFLAGS = $(common_ldflags) -export-symbols-regex init_glib
_glib_la_LIBADD = $(GLIB_LIBS) libpyglib-2.0.la
_glib_la_SOURCES = 	 \
	glibmodule.c	 \
	pygspawn.c	 \
	pygspawn.h	 \
	pygmaincontext.c \
	pygmainloop.c

if PLATFORM_WIN32
_glib_la_CFLAGS += -DPLATFORM_WIN32
endif

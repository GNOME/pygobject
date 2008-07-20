AUTOMAKE_OPTIONS = 1.7
PLATFORM_VERSION = 2.0
INCLUDES = $(PYTHON_INCLUDES) $(GLIB_CFLAGS) -DPY_SSIZE_T_CLEAN

# Workaround for automake so we can build shared library which are not
# going to be installed
noinstshared_LTLIBRARIES = libpyglib.la _glib.la 
noinstshareddir = /tmp
install-noinstsharedLTLIBRARIES: # prevent it from being installed

common_ldflags = -module -avoid-version
if PLATFORM_WIN32
common_ldflags += -no-undefined
endif

libpyglib_la_CFLAGS = $(GLIB_CFLAGS)
libpyglib_la_LIBADD = $(GLIB_LIBS) $(FFI_LIBS)
libpyglib_la_SOURCES = \
	pyglib.c	\
	pyglib.h	\
	pyglib-private.h

_glib_la_CFLAGS = $(GLIB_CFLAGS)
_glib_la_LDFLAGS = $(common_ldflags) -export-symbols-regex init_glib
_glib_la_LIBADD = $(GLIB_LIBS) libpyglib.la
_glib_la_SOURCES = 	\
	glibmodule.c	\
	pygspawn.c	\
	pygspawn.h

if PLATFORM_WIN32
_glib_la_CFLAGS += -DPLATFORM_WIN32
endif

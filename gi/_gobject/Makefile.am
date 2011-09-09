AUTOMAKE_OPTIONS = 1.7
PLATFORM_VERSION = 3.0

pkgincludedir = $(includedir)/pygobject-$(PLATFORM_VERSION)
pkginclude_HEADERS = pygobject.h

extension_cppflags = \
	$(PYTHON_INCLUDES) \
	-DPY_SSIZE_T_CLEAN

extension_ldflags = \
	-module \
	-avoid-version

if OS_WIN32
extension_ldflags += \
	-no-undefined \
	-shrext ".pyd"
endif

pygobjectdir = $(pyexecdir)/gi/_gobject

pygobject_PYTHON = \
	__init__.py \
	constants.py \
	propertyhelper.py

pygobject_LTLIBRARIES = _gobject.la

_gobject_la_SOURCES = \
        gobjectmodule.c \
        pygboxed.c \
        pygboxed.h \
        pygenum.c \
        pygenum.h \
        pygflags.c \
        pygflags.h \
        pyginterface.c \
        pyginterface.h \
        pygobject.c \
        pygobject.h \
        pygobject-private.h \
        pygparamspec.c \
        pygparamspec.h \
        pygpointer.c \
        pygpointer.h \
        pygtype.c \
        pygtype.h
_gobject_la_CFLAGS = \
	$(extension_cppflags) \
	-I$(top_srcdir)/gi/_glib \
	-I$(top_srcdir)/gi \
	$(FFI_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GI_CFLAGS)
_gi_la_CPPFLAGS = \
	$(extension_cppflags)
_gobject_la_LIBADD = \
	$(GLIB_LIBS) \
	$(FFI_LIBS) \
	$(PYTHON_LIBS) \
	$(top_builddir)/gi/_glib/libpyglib-gi-2.0-@PYTHON_BASENAME@.la
_gobject_la_LDFLAGS = \
	$(extension_ldflags) \
	-export-symbols-regex "_gobject|PyInit__gobject"

if HAVE_LIBFFI
_gobject_la_SOURCES += ffi-marshaller.c ffi-marshaller.h
endif


all: $(pygobject_LTLIBRARIES:.la=.so)
check-local: $(pygobject_LTLIBRARIES:.la=.so)
clean-local:
	rm -f $(pygobject_LTLIBRARIES:.la=.so)
.la.so:
	$(LN_S) .libs/$@ $@ || true

-include $(top_srcdir)/git.mk

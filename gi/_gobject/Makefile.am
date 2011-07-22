AUTOMAKE_OPTIONS = 1.7
PLATFORM_VERSION = 2.0

pkgpyexecdir = $(pyexecdir)

# gobject python scripts
pygobjectdir = $(pkgpyexecdir)/gi/_gobject
pygobject_PYTHON = 	\
	__init__.py 	\
	constants.py	\
	propertyhelper.py
pygobject_LTLIBRARIES = _gobject.la 

common_ldflags = -module -avoid-version
if PLATFORM_WIN32
common_ldflags += -no-undefined
endif

_gobject_la_CFLAGS = \
	-I$(top_srcdir)/gi/_glib \
	-I$(top_srcdir)/gi \
	$(PYTHON_INCLUDES) \
	$(FFI_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GI_CFLAGS) \
	 -DPY_SSIZE_T_CLEAN

_gobject_la_LDFLAGS = $(common_ldflags) -export-symbols-regex "_gobject|PyInit__gobject"
_gobject_la_LIBADD = \
	$(GLIB_LIBS) \
	$(FFI_LIBS) \
	$(PYTHON_LIBS) \
	$(top_builddir)/gi/_glib/libpyglib-gi-2.0-@PYTHON_BASENAME@.la

_gobject_la_SOURCES =           \
        gobjectmodule.c         \
        pygboxed.c              \
        pygboxed.h              \
        pygenum.c               \
        pygenum.h               \
        pygflags.c              \
        pygflags.h              \
        pyginterface.c          \
        pyginterface.h          \
        pygobject.c             \
        pygobject.h             \
        pygobject-private.h     \
        pygparamspec.c          \
        pygparamspec.h          \
        pygpointer.c            \
        pygpointer.h            \
        pygtype.c               \
        pygtype.h

if HAVE_LIBFFI
_gobject_la_SOURCES += ffi-marshaller.c ffi-marshaller.h
endif

if PLATFORM_WIN32
_gobject_la_CFLAGS += -DPLATFORM_WIN32
endif


all: $(pygobject_LTLIBRARIES:.la=.so)
check-local: $(pygobject_LTLIBRARIES:.la=.so)
clean-local:
	rm -f $(pygobject_LTLIBRARIES:.la=.so)
.la.so:
	$(LN_S) .libs/$@ $@ || true

-include $(top_srcdir)/git.mk

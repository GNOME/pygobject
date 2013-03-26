AUTOMAKE_OPTIONS = 1.7
PLATFORM_VERSION = 3.0

pkgincludedir = $(includedir)/pygobject-$(PLATFORM_VERSION)
pkginclude_HEADERS = pygobject.h

extension_cppflags = \
	$(PYTHON_INCLUDES) \
	-DPY_SSIZE_T_CLEAN

extension_ldflags = \
	-module \
	-avoid-version \
	-shrext $(PYTHON_SO)

if OS_WIN32
# Windows requires Python modules to be explicitly linked to libpython.
# Extension modules are shared libaries (.dll), but need to be
# called .pyd for Python to load it as an extension module.
extension_libadd = \
	$(PYTHON_LIBS)

extension_ldflags += \
	-no-undefined
endif

pygobjectdir = $(pyexecdir)/gi/_gobject

pygobject_PYTHON = \
	__init__.py \
	constants.py \
	propertyhelper.py \
	signalhelper.py

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
	$(GLIB_CFLAGS) \
	$(GI_CFLAGS)
_gi_la_CPPFLAGS = \
	$(extension_cppflags)
_gobject_la_LIBADD = \
	$(extension_libadd) \
	$(GLIB_LIBS) \
	$(GI_LIBS) \
	$(top_builddir)/gi/_glib/libpyglib-gi-2.0-@PYTHON_BASENAME@.la
_gobject_la_LDFLAGS = \
	$(extension_ldflags) \
	-export-symbols-regex "_gobject|PyInit__gobject"

# if we build in a separate tree, we need to symlink the *.py files from the
# source tree; Python does not accept the extensions and modules in different
# paths
build_pylinks:
	for f in $(pygobject_PYTHON); do \
	    [ -e $(builddir)/$$f ] || $(LN_S) $(srcdir)/$$f $(builddir)/$$f; \
	done


all: $(pygobject_LTLIBRARIES:.la=$(PYTHON_SO)) build_pylinks
check-local: $(pygobject_LTLIBRARIES:.la=$(PYTHON_SO)) build_pylinks
clean-local:
	rm -f $(pygobject_LTLIBRARIES:.la=$(PYTHON_SO))
%$(PYTHON_SO): %.la
	$(LN_S) -f .libs/$@ $@

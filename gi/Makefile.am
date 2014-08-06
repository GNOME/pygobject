PLATFORM_VERSION = 3.0

SUBDIRS = \
	repository \
	overrides \
	_gobject

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

pkgincludedir = $(includedir)/pygobject-$(PLATFORM_VERSION)
if WITH_COMMON
pkginclude_HEADERS = pygobject.h
endif

pygidir = $(pyexecdir)/gi

pygi_LTLIBRARIES = _gi.la

_gi_la_SOURCES = \
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
	pygtype.h \
	glibmodule.c \
	pygoptioncontext.c \
	pygoptioncontext.h \
	pygoptiongroup.c \
	pygoptiongroup.h \
	pygspawn.c \
	pygspawn.h \
	pyglib.c \
	pyglib.h \
	pyglib-private.h \
	pyglib-python-compat.h \
	gimodule.c \
	pygi-repository.c \
	pygi-repository.h \
	pygi-info.c \
	pygi-info.h \
	pygi-foreign.c \
	pygi-foreign.h \
	pygi-foreign-api.h \
	pygi-struct.c \
	pygi-struct.h \
	pygi-source.c \
	pygi-source.h \
	pygi-argument.c \
	pygi-argument.h \
	pygi-type.c \
	pygi-type.h \
	pygi-boxed.c \
	pygi-boxed.h \
	pygi-closure.c \
	pygi-closure.h \
	pygi-ccallback.c \
	pygi-ccallback.h \
	pygi.h \
	pygi-private.h \
	pygi-property.c \
	pygi-property.h \
	pygi-signal-closure.c \
	pygi-signal-closure.h \
	pygobject-external.h \
	pygi-invoke.c \
	pygi-invoke.h \
	pygi-invoke-state-struct.h \
	pygi-cache.h \
	pygi-cache.c \
	pygi-marshal-cleanup.c \
	pygi-marshal-cleanup.h \
	pygi-basictype.c \
	pygi-basictype.h \
	pygi-list.c \
	pygi-list.h \
	pygi-array.c \
	pygi-array.h \
	pygi-error.c \
	pygi-error.h \
	pygi-object.c \
	pygi-object.h \
	pygi-value.c \
	pygi-value.h \
	pygi-enum-marshal.c \
	pygi-enum-marshal.h \
	pygi-struct-marshal.c \
	pygi-struct-marshal.h \
	pygi-hashtable.c \
	pygi-hashtable.h
_gi_la_CFLAGS = \
	$(extension_cppflags) \
	$(GLIB_CFLAGS) \
	$(GI_CFLAGS)
_gi_la_CPPFLAGS = \
	$(extension_cppflags)
_gi_la_LIBADD = \
	$(extension_libadd) \
	$(GLIB_LIBS) \
	$(GI_LIBS) \
	$(FFI_LIBS)
_gi_la_LDFLAGS = \
	$(extension_ldflags) \
	-export-symbols-regex "init_gi|PyInit__gi"

if ENABLE_CAIRO
pygi_LTLIBRARIES += _gi_cairo.la
endif

_gi_cairo_la_SOURCES = \
	pygi-foreign-cairo.c
_gi_cairo_la_CFLAGS = \
	$(GI_CFLAGS) \
	$(CAIRO_CFLAGS) \
	$(PYCAIRO_CFLAGS)
_gi_cairo_la_CPPFLAGS = \
	$(extension_cppflags)
_gi_cairo_la_LIBADD = \
	$(extension_libadd) \
	$(GI_LIBS) \
	$(CAIRO_LIBS) \
	$(PYCAIRO_LIBS)
_gi_cairo_la_LDFLAGS = \
	$(extension_ldflags) \
	-export-symbols-regex "init_gi_cairo|PyInit__gi_cairo"


# This is to ensure we have a symlink to the .so in the
# build directory, which the Python interpreter can load
# directly without having to know how to parse .la files.
%$(PYTHON_SO): %.la
	$(LN_S) -f .libs/$@ $@

all-local: $(LTLIBRARIES:.la=$(PYTHON_SO))

check-local: $(LTLIBRARIES:.la=$(PYTHON_SO))
clean-local:
	rm -f $(LTLIBRARIES:.la=$(PYTHON_SO))

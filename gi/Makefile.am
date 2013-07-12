SUBDIRS = \
	repository \
	overrides \
        _glib \
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

pygidir = $(pyexecdir)/gi

pygi_PYTHON = \
	__init__.py \
	types.py \
	module.py \
	importer.py \
	pygtkcompat.py \
	docstring.py

pygi_LTLIBRARIES = _gi.la

_gi_la_SOURCES = \
	pygi-repository.c \
	pygi-repository.h \
	pygi-info.c \
	pygi-info.h \
	pygi-foreign.c \
	pygi-foreign.h \
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
	gimodule.c \
	pygi-invoke.c \
	pygi-invoke.h \
	pygi-invoke-state-struct.h \
	pygi-cache.h \
	pygi-cache.c \
	pygi-marshal-from-py.c \
	pygi-marshal-from-py.h \
	pygi-marshal-to-py.c \
	pygi-marshal-to-py.h \
	pygi-marshal-cleanup.c \
	pygi-marshal-cleanup.h
_gi_la_CFLAGS = \
	$(GI_CFLAGS)
_gi_la_CPPFLAGS = \
	$(extension_cppflags) \
	-I$(top_srcdir)/gi/_glib \
	-I$(top_srcdir)/gi/_gobject
_gi_la_LIBADD = \
	$(extension_libadd) \
	$(GI_LIBS) \
	$(top_builddir)/gi/_glib/libpyglib-gi-2.0-@PYTHON_BASENAME@.la
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
	$(PYCAIRO_CFLAGS)
_gi_cairo_la_CPPFLAGS = \
	$(extension_cppflags) \
	-I$(top_srcdir)/gi/_glib \
	-I$(top_srcdir)/gi/_gobject
_gi_cairo_la_LIBADD = \
	$(extension_libadd) \
	$(GI_LIBS) \
	$(PYCAIRO_LIBS)
_gi_cairo_la_LDFLAGS = \
	$(extension_ldflags) \
	-export-symbols-regex "init_gi_cairo|PyInit__gi_cairo"


# This is to ensure we have a symlink to the .so in the
# build directory, which the Python interpreter can load
# directly without having to know how to parse .la files.
%$(PYTHON_SO): %.la
	$(LN_S) -f .libs/$@ $@

# if we build in a separate tree, we need to symlink the *.py files from the
# source tree; Python does not accept the extensions and modules in different
# paths
build_pylinks:
	for f in $(pygi_PYTHON); do \
	    [ -e $(builddir)/$$f ] || $(LN_S) $(srcdir)/$$f $(builddir)/$$f; \
	done

all-local: $(LTLIBRARIES:.la=$(PYTHON_SO)) build_pylinks

check-local: $(LTLIBRARIES:.la=$(PYTHON_SO)) build_pylinks
clean-local:
	rm -f $(LTLIBRARIES:.la=$(PYTHON_SO))

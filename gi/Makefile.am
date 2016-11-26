PLATFORM_VERSION = 3.0

SUBDIRS = \
	repository \
	overrides

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

include pygi-srcs.mak

_gi_la_SOURCES = $(pygi_module_sources)
_gi_la_CFLAGS = \
	$(CODE_COVERAGE_CFLAGS) \
	$(WARN_CFLAGS) \
	$(extension_cppflags) \
	$(GLIB_CFLAGS) \
	$(GI_CFLAGS)
_gi_la_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	$(extension_cppflags)
_gi_la_LIBADD = \
	$(CODE_COVERAGE_LIBS) \
	$(extension_libadd) \
	$(GLIB_LIBS) \
	$(GI_LIBS) \
	$(FFI_LIBS)
_gi_la_LDFLAGS = \
	$(WARN_LDFLAGS) \
	$(extension_ldflags) \
	-export-symbols-regex "init_gi|PyInit__gi"

if ENABLE_CAIRO
pygi_LTLIBRARIES += _gi_cairo.la
endif

_gi_cairo_la_SOURCES = $(pygi_cairo_module_sources)
_gi_cairo_la_CFLAGS = \
	$(CODE_COVERAGE_CFLAGS) \
	$(WARN_CFLAGS) \
	$(GI_CFLAGS) \
	$(CAIRO_CFLAGS) \
	$(PYCAIRO_CFLAGS)
_gi_cairo_la_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
	$(extension_cppflags)
_gi_cairo_la_LIBADD = \
	$(CODE_COVERAGE_LIBS) \
	$(extension_libadd) \
	$(GI_LIBS) \
	$(CAIRO_LIBS) \
	$(PYCAIRO_LIBS)
_gi_cairo_la_LDFLAGS = \
	$(WARN_LDFLAGS) \
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

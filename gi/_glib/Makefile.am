AUTOMAKE_OPTIONS = 1.7

extension_cppflags = \
	$(PYTHON_INCLUDES) \
	-DPY_SSIZE_T_CLEAN

extension_ldflags = \
	-module \
	-avoid-version

if OS_WIN32
# Windows requires Python modules to be explicitly linked to libpython.
# Extension modules are shared libaries (.dll), but need to be
# called .pyd for Python to load it as an extension module.
extension_libadd = \
	$(PYTHON_LIBS)

extension_ldflags += \
	-no-undefined \
	-shrext ".pyd"
endif


pyglibdir = $(pyexecdir)/gi/_glib

pyglib_PYTHON = \
	__init__.py \
	option.py

lib_LTLIBRARIES = libpyglib-gi-2.0-@PYTHON_BASENAME@.la

libpyglib_gi_2_0_@PYTHON_BASENAME@_la_SOURCES = \
	pyglib.c \
	pyglib.h \
	pyglib-private.h \
	pyglib-python-compat.h
libpyglib_gi_2_0_@PYTHON_BASENAME@_la_CPPFLAGS = \
	$(extension_cppflags)
libpyglib_gi_2_0_@PYTHON_BASENAME@_la_CFLAGS = \
	$(GLIB_CFLAGS)
libpyglib_gi_2_0_@PYTHON_BASENAME@_la_LIBADD = \
	$(extension_libadd) \
	$(FFI_LIBS) \
	$(GLIB_LIBS)

if OS_WIN32
libpyglib_gi_2_0_@PYTHON_BASENAME@_la_LDFLAGS = \
	-no-undefined
endif

pyglib_LTLIBRARIES = _glib.la

_glib_la_SOURCES = \
	glibmodule.c \
	pygiochannel.c \
	pygiochannel.h \
	pygoptioncontext.c \
	pygoptioncontext.h \
	pygoptiongroup.c \
	pygoptiongroup.h \
	pygmaincontext.c \
	pygmaincontext.h \
	pygmainloop.c \
	pygmainloop.h \
	pygsource.c \
	pygsource.h \
	pygspawn.c \
	pygspawn.h
_glib_la_CFLAGS = \
	$(GLIB_CFLAGS)
_glib_la_CPPFLAGS = \
	$(extension_cppflags)
_glib_la_LIBADD = \
	$(extension_libadd) \
	$(GLIB_LIBS) \
	libpyglib-gi-2.0-@PYTHON_BASENAME@.la
_glib_la_LDFLAGS = \
	$(extension_ldflags) \
	-export-symbols-regex "_glib|PyInit__glib"

if PLATFORM_WIN32
_glib_la_CFLAGS += -DPLATFORM_WIN32
endif


all: $(pyglib_LTLIBRARIES:.la=.so)
check-local: $(pyglib_LTLIBRARIES:.la=.so)
clean-local:
	rm -f $(pyglib_LTLIBRARIES:.la=.so)
.la.so:
	$(LN_S) .libs/$@ $@ || true


-include $(top_srcdir)/git.mk

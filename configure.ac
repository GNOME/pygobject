-*- mode: m4 -*-
AC_PREREQ([2.68])

# The version of python used is determined by the executable pointed to by the
# --with-python switch, or if that's not set, by the PYTHON environment
# variable.  For instance if your system installs Python 3 as python3 to
# configure to compile pygobject under Python 3 you would do this:
#   $ ./configure --with-python=python3
# or
#   $ PYTHON=python3 ./configure
#
# You can also build against a full interpreter path, such as
#   $ ./configure --with-python=~/my-patched-python/python

m4_define(python_min_ver, 2.7)
m4_define(python3_min_ver, 3.4)

dnl the pygobject version number
m4_define(pygobject_major_version, 3)
m4_define(pygobject_minor_version, 28)
m4_define(pygobject_micro_version, 1)
m4_define(pygobject_version, pygobject_major_version.pygobject_minor_version.pygobject_micro_version)

dnl versions of packages we require ...
m4_define(introspection_required_version, 1.46.0)
m4_define(pycairo_required_version, 1.11.1)
m4_define(glib_required_version, 2.38.0)
m4_define(gio_required_version, 2.38.0)
m4_define(libffi_required_version, 3.0)

AC_INIT([pygobject],[pygobject_version],
	[http://bugzilla.gnome.org/enter_bug.cgi?product=pygobject],
	[pygobject],[https://wiki.gnome.org/Projects/PyGObject/])

m4_ifndef([AX_IS_RELEASE], [AC_MSG_ERROR(['autoconf-archive' missing])])

AX_IS_RELEASE([minor-version])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([gi/gimodule.c])

AC_DEFINE(PYGOBJECT_MAJOR_VERSION, pygobject_major_version, [pygobject major version])
AC_SUBST(PYGOBJECT_MAJOR_VERSION, pygobject_major_version)
AC_DEFINE(PYGOBJECT_MINOR_VERSION, pygobject_minor_version, [pygobject minor version])
AC_SUBST(PYGOBJECT_MINOR_VERSION, pygobject_minor_version)
AC_DEFINE(PYGOBJECT_MICRO_VERSION, pygobject_micro_version, [pygobject micro version])
AC_SUBST(PYGOBJECT_MICRO_VERSION, pygobject_micro_version)

AC_CONFIG_HEADERS(config.h)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES(yes)])
AM_INIT_AUTOMAKE([1.13 foreign no-dist-gzip dist-xz])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_MSG_CHECKING([for build time linking with Python (Win32)])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    os_win32=yes
    link_python_libs=yes
    OS_EXT=dll
    ;;
  *-*-darwin*)
    os_win32=no
    link_python_libs=no
    OS_EXT=dylib
    ;;
  *)
    os_win32=no
    link_python_libs=no
    OS_EXT=so
    ;;
esac
AC_MSG_RESULT([$link_python_libs])
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")

AC_SUBST(OS_EXT)
AC_DEFINE_UNQUOTED(OS_EXT, "$OS_EXT", [Define shared library extension])

LT_PREREQ([2.2.6])
LT_INIT([dlopen win32-dll disable-static])

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC
AM_PROG_CC_C_O

# option to specify python interpreter to use; this just sets $PYTHON, so that
# we will fallback to reading $PYTHON if --with-python is not given, and
# python.m4 will get the expected input
AC_ARG_WITH(python,
  AS_HELP_STRING([--with-python=PATH],[Path to Python interpreter; searches $PATH if only a program name is given; if not given, searches for a few standard names such as "python3" or "python2"]),
  [PYTHON="$withval"], [])
if test x"$PYTHON" = xyes; then
	AC_MSG_ERROR([--with-python option requires a path or program argument])
fi
if test -n "$PYTHON" && ! which "$PYTHON"; then
	AC_MSG_ERROR([Python interpreter $PYTHON does not exist])
fi

# check that we have the minimum version of python necessary to build
JD_PATH_PYTHON(python_min_ver)

# check if we are building for python 3
AM_PYTHON_CHECK_VERSION([$PYTHON], [3.0],
			 build_py3k=true,
			 build_py3k=false)

# if building for python 3 make sure we have the minimum version supported
if test $build_py3k = true ; then
  AC_MSG_CHECKING([for $PYTHON >=] python3_min_ver)
  AM_PYTHON_CHECK_VERSION([$PYTHON], python3_min_ver,
			  [AC_MSG_RESULT(yes)],
			  [AC_MSG_ERROR(too old)])
fi

# - 'SO' for PyPy, CPython 2.7-3.2
# - 'EXT_SUFFIX' for CPython3.3+ (http://bugs.python.org/issue16754)
# - fallback to '.so'
PYTHON_SO=`$PYTHON -c "import distutils.sysconfig, sys; get = distutils.sysconfig.get_config_var; sys.stdout.write(get('EXT_SUFFIX') or get('SO') or '.so');"`
AC_SUBST(PYTHON_SO)

PYG_CHECK_PYTHON_HEADERS(, AC_MSG_ERROR([Python headers not found]))
if test "x$link_python_libs" = "xyes"; then
  PYG_CHECK_PYTHON_LIBS(, AC_MSG_ERROR([Python libs not found. Windows requires Python modules to be explicitly linked to libpython.]))
fi

dnl get rid of the -export-dynamic stuff from the configure flags ...
export_dynamic=`(./libtool --config; echo eval echo \\$export_dynamic_flag_spec) | sh`

dnl glib
AM_PATH_GLIB_2_0(glib_required_version,,[AC_MSG_ERROR(maybe you want the pygobject-2-4 branch?)],gobject $extra_mods)
if test -n "$export_dynamic"; then
  GLIB_LIBS=`echo $GLIB_LIBS | sed -e "s/$export_dynamic//"`
fi

PYTHON_VALGRIND_SUPP=`$PYTHON -c "import sys; sys.stdout.write('python' + sys.version.__getitem__(0))"`
AC_SUBST([PYTHON_VALGRIND_SUPP])

dnl libffi
PKG_CHECK_MODULES(FFI, libffi >= libffi_required_version)

AC_SUBST(FFI_CFLAGS)
AC_SUBST(FFI_LIBS)

dnl gio
PKG_CHECK_MODULES(GIO, gio-2.0 >= gio_required_version)

AC_ARG_ENABLE(cairo,
    AS_HELP_STRING([--enable-cairo],[Enable Cairo bindings using introspection information]),
    enable_cairo=$enableval,
    enable_cairo=yes)

PKG_CHECK_MODULES(GI,
    glib-2.0 >= glib_required_version
    gobject-introspection-1.0 >= introspection_required_version
)

GOBJECT_INTROSPECTION_CHECK(introspection_required_version)

GI_DATADIR=$($PKG_CONFIG --variable=gidatadir gobject-introspection-1.0)
AC_SUBST(GI_DATADIR)

if test "$enable_cairo" != no; then
    PKG_CHECK_MODULES(CAIRO, cairo cairo-gobject)

    if test $build_py3k = true; then
        PKG_CHECK_MODULES(PYCAIRO,
            py3cairo >= pycairo_required_version
        )
    else
        PKG_CHECK_MODULES(PYCAIRO,
            pycairo >= pycairo_required_version
        )
    fi
fi
AM_CONDITIONAL(ENABLE_CAIRO, test "$enable_cairo" = "yes")

AC_ARG_WITH(common,
    AS_HELP_STRING([--without-common],
        [For package maintainers: do not install Python version independent files]),
    with_common=$enableval,
    with_common=yes)
AM_CONDITIONAL(WITH_COMMON, test "$with_common" = "yes")

# Disable -Werror always, see
# https://mail.gnome.org/archives/desktop-devel-list/2017-April/msg00009.html
AX_COMPILER_FLAGS(,, [yes])
# Disable some warnings, either because they get generated by headers of
# dependencies we don't control or because it was hard to fix them.
# Feel free and try to get rid of them.
# For clang only:
AX_APPEND_COMPILE_FLAGS([-Wno-unknown-warning-option -Wno-incompatible-pointer-types-discards-qualifiers -Wno-cast-align])
# For gcc + clang:
AX_APPEND_COMPILE_FLAGS([-Wno-discarded-qualifiers -Wno-redundant-decls -Wno-switch-enum -Wno-undef])

AX_CODE_COVERAGE()

AC_CONFIG_FILES(
  Makefile
  pygobject-3.0.pc
  gi/Makefile
  gi/repository/Makefile
  gi/overrides/Makefile
  examples/Makefile
  tests/Makefile
  pygtkcompat/Makefile
  PKG-INFO)
AC_OUTPUT

AC_MSG_RESULT([
	pygobject $VERSION

	Is release:			$ax_is_release
	Using python interpreter:	$PYTHON
	cairo support:			$enable_cairo
	code coverage support:          $enable_code_coverage
])


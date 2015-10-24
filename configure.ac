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
m4_define(python3_min_ver, 3.1)

dnl the pygobject version number
m4_define(pygobject_major_version, 3)
m4_define(pygobject_minor_version, 18)
m4_define(pygobject_micro_version, 3)
m4_define(pygobject_version, pygobject_major_version.pygobject_minor_version.pygobject_micro_version)

dnl versions of packages we require ...
m4_define(introspection_required_version, 1.39.0)
m4_define(py2cairo_required_version, 1.2.0)
m4_define(py3cairo_required_version, 1.10.0)
m4_define(glib_required_version, 2.38.0)
m4_define(gio_required_version, 2.38.0)

AC_INIT([pygobject],[pygobject_version],
	[http://bugzilla.gnome.org/enter_bug.cgi?product=pygobject],
	[pygobject],[https://wiki.gnome.org/Projects/PyGObject/])
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4 -I .")
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
AM_INIT_AUTOMAKE([1.11.1 foreign no-dist-gzip dist-xz])

dnl put the ACLOCAL flags in the makefile
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_MSG_CHECKING([for build time linking with Python (Darwin and Win32)])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    os_win32=yes
    link_python_libs=yes
    OS_EXT=dll
    ;;
  *-*-darwin*)
    os_win32=no
    link_python_libs=yes
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

AM_CHECK_PYTHON_HEADERS(, AC_MSG_ERROR([Python headers not found]))
if test "x$link_python_libs" = "xyes"; then
  AM_CHECK_PYTHON_LIBS(, AC_MSG_ERROR([Python libs not found. Windows and Darwin require Python modules to be explicitly linked to libpython.]))
fi

AC_MSG_CHECKING([for PySignal_SetWakeupFd in Python.h])
old_CPPFLAGS=$CPPFLAGS
CPPFLAGS="-Wall -Werror $PYTHON_INCLUDES"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <Python.h>]], [[PySignal_SetWakeupFd(0);]])],
                  [setwakeupfd_ok=yes],[setwakeupfd_ok=no])
AC_MSG_RESULT($setwakeupfd_ok)
if test "x$setwakeupfd_ok" != xno; then
  AC_DEFINE(HAVE_PYSIGNAL_SETWAKEUPFD, 1,
                [Define to 1 if PySignal_SetWakeupFd is available])
fi
CPPFLAGS=$old_CPPFLAGS

PLATFORM=`$PYTHON -c "import sys; from distutils import util; sys.stdout.write(util.get_platform())"`
AC_SUBST(PLATFORM)

AC_ARG_ENABLE(thread,
  AS_HELP_STRING([--disable-thread],[Disable pygobject threading support]),,
  enable_thread=yes)

AM_CHECK_PYMOD(thread,,,enable_thread=check_for_py3_module)
if test "x$enable_thread" = xcheck_for_py3_module; then
  dnl Python 3 uses the _thread module so check for that
  AM_CHECK_PYMOD(_thread,,enable_thread=yes,enable_thread=no)
fi

AC_MSG_CHECKING(whether to enable threading in pygobject)
if test "x$enable_thread" != xno; then
  extra_mods=gthread
  THREADING_CFLAGS=
  AC_MSG_RESULT(yes)
else
  extra_mods=
  THREADING_CFLAGS="-DDISABLE_THREADING"
  AC_MSG_RESULT(no)
fi
AC_SUBST(THREADING_CFLAGS)
CPPFLAGS="${CPPFLAGS} $THREADING_CFLAGS"

dnl get rid of the -export-dynamic stuff from the configure flags ...
export_dynamic=`(./libtool --config; echo eval echo \\$export_dynamic_flag_spec) | sh`

dnl glib
AM_PATH_GLIB_2_0(glib_required_version,,[AC_MSG_ERROR(maybe you want the pygobject-2-4 branch?)],gobject $extra_mods)
if test -n "$export_dynamic"; then
  GLIB_LIBS=`echo $GLIB_LIBS | sed -e "s/$export_dynamic//"`
fi
PYTHON_BASENAME=`basename $PYTHON`
AC_SUBST([PYTHON_BASENAME])

AS_AC_EXPAND(DATADIR, $datadir)

dnl libffi
PKG_CHECK_MODULES(FFI, libffi >= 3.0)
LIBFFI_PC=libffi

AC_SUBST(FFI_CFLAGS)
AC_SUBST(FFI_LIBS)
AC_SUBST(LIBFFI_PC)

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

GI_DATADIR=$($PKG_CONFIG --variable=gidatadir gobject-introspection-1.0)
AC_SUBST(GI_DATADIR)

if test "$enable_cairo" != no; then
    PKG_CHECK_MODULES(CAIRO, cairo cairo-gobject)

    if test $build_py3k = true; then
        PKG_CHECK_MODULES(PYCAIRO,
            py3cairo >= py3cairo_required_version
        )
    else
        PKG_CHECK_MODULES(PYCAIRO,
            pycairo >= py2cairo_required_version
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

INTROSPECTION_SCANNER=`$PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0`
INTROSPECTION_COMPILER=`$PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0`

AC_SUBST(INTROSPECTION_SCANNER)
AC_SUBST(INTROSPECTION_COMPILER)

# compiler warnings, errors, required cflags, and code coverage support
GNOME_COMPILE_WARNINGS([maximum])
AC_MSG_CHECKING(for Gnome code coverage support)
m4_ifdef([GNOME_CODE_COVERAGE],
         [AC_MSG_RESULT(yes)
          GNOME_CODE_COVERAGE],
         [AC_MSG_RESULT(no)
          GNOME_CODE_COVERAGE_RULES=''
          AC_SUBST([GNOME_CODE_COVERAGE_RULES])
          enable_code_coverage="no"])

if test "x$GCC" = "xyes"; then
  JH_ADD_CFLAG([-Wall])
  JH_ADD_CFLAG([-Werror=unused-variable])
  JH_ADD_CFLAG([-fno-strict-aliasing])
  JH_ADD_CFLAG([-Werror=declaration-after-statement])

  case $host_os in
  solaris*)
    ;;
  *)
    JH_ADD_CFLAG([-std=c90])
    ;;
  esac

fi

CFLAGS="$CFLAGS $WARN_CFLAGS $CODE_COVERAGE_CFLAGS"
LDFLAGS="$LDFLAGS $CODE_COVERAGE_CFLAGS"

AC_CONFIG_FILES(
  Makefile
  pygobject-3.0.pc
  pygobject-3.0-uninstalled.pc
  gi/Makefile
  gi/repository/Makefile
  gi/overrides/Makefile
  gi/_gobject/Makefile
  examples/Makefile
  tests/Makefile
  pygtkcompat/Makefile
  PKG-INFO)
AC_OUTPUT

AC_MSG_RESULT([
	pygobject $VERSION

	Using python interpreter:	$PYTHON
	cairo support:			$enable_cairo
	thread support:			$enable_thread
	code coverage support:          $enable_code_coverage
])


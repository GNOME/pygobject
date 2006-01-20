-*- mode: m4 -*-
AC_PREREQ(2.52)

dnl the pygobject version number
m4_define(pygobject_major_version, 2)
m4_define(pygobject_minor_version, 9)
m4_define(pygobject_micro_version, 2)
m4_define(pygobject_version, pygobject_major_version.pygobject_minor_version.pygobject_micro_version)

dnl versions of packages we require ...
m4_define(glib_required_version, 2.8.0)

AC_INIT(pygobject, pygobject_version,
        [http://bugzilla.gnome.org/enter_bug.cgi?product=pygobject])
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4 -I .")

AC_DEFINE(PYGOBJECT_MAJOR_VERSION, pygobject_major_version, [pygobject major version])
AC_SUBST(PYGOBJECT_MAJOR_VERSION, pygobject_major_version)
AC_DEFINE(PYGOBJECT_MINOR_VERSION, pygobject_minor_version, [pygobject minor version])
AC_SUBST(PYGOBJECT_MINOR_VERSION, pygobject_minor_version)
AC_DEFINE(PYGOBJECT_MICRO_VERSION, pygobject_micro_version, [pygobject micro version])
AC_SUBST(PYGOBJECT_MICRO_VERSION, pygobject_micro_version)

AC_CONFIG_SRCDIR([gobject/gobjectmodule.c])
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE

dnl put the ACLOCAL flags in the makefile
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_MSG_CHECKING([for some Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    os_win32=yes
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")

AC_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AM_PATH_PYTHON(2.3.5)

AM_CHECK_PYTHON_HEADERS(,[AC_MSG_ERROR(could not find Python headers)])

PLATFORM=`$PYTHON -c "from distutils import util; print util.get_platform()"`
AC_SUBST(PLATFORM)

AC_ARG_ENABLE(thread,
  AC_HELP_STRING([--disable-thread], [Disable pygobject threading support]),,
  enable_thread=yes)

AM_CHECK_PYMOD(thread,,,enable_thread=no)

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

pygobject_CODEGEN_DEFINES=""
AC_SUBST([pygobject_CODEGEN_DEFINES])

dnl add required cflags ...
JH_ADD_CFLAG([-Wall])
JH_ADD_CFLAG([-std=c9x])
JH_ADD_CFLAG([-fno-strict-aliasing])

AC_CONFIG_FILES(
  Makefile
  pygobject-2.0.pc
  gobject/Makefile
  examples/Makefile
  tests/Makefile
  PKG-INFO)
AC_OUTPUT

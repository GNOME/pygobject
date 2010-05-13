AC_INIT(pygi, 0.5.2)

AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIR(m4)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES(yes)])

AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

AM_PROG_LIBTOOL

# Python
AM_PATH_PYTHON(2.5.2)

AC_PATH_TOOL(PYTHON_CONFIG, "python${PYTHON_VERSION}-config")
if test -z "$PYTHON_CONFIG"; then
	AC_PATH_TOOL(PYTHON_CONFIG, "python-config-${PYTHON_VERSION}")
	if test -z "$PYTHON_CONFIG"; then
		AC_MSG_ERROR(Python development tools not found)
	fi
fi
PYTHON_INCLUDES=`$PYTHON_CONFIG --includes`
AC_SUBST(PYTHON_INCLUDES)

save_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS+="${PYTHON_INCLUDES}"
AC_CHECK_HEADER(Python.h, , AC_MSG_ERROR(Python headers not found))
CPPFLAGS="${save_CPPFLAGS}"

# FFI
PKG_CHECK_MODULES(FFI, libffi >= 3.0)

# GNOME
PKG_CHECK_MODULES(GNOME,
	glib-2.0 >= 2.22.4
	gobject-introspection-1.0 >= 0.6.10
	pygobject-2.0 >= 2.20
	pycairo >= 1.0.2
)

INTROSPECTION_SCANNER=`$PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0`
INTROSPECTION_COMPILER=`$PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0`

AC_SUBST(INTROSPECTION_SCANNER) 
AC_SUBST(INTROSPECTION_COMPILER) 

AC_OUTPUT(
	Makefile
	examples/Makefile
	gi/Makefile
	gi/repository/Makefile
	gi/overrides/Makefile
	tests/Makefile
)

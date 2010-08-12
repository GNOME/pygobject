#!/bin/sh
# Run this to generate all the initial makefiles, etc.

PROJECT=PyGObject


if test ! -f configure.ac ; then
    echo "You must run this script in the top-level $PROJECT directory"
    exit 1
fi


DIE=0

have_libtool=false
if libtoolize --version < /dev/null > /dev/null 2>&1 ; then
    libtool_version=`libtoolize --version |
             head -1 |
             sed -e 's/^\(.*\)([^)]*)\(.*\)$/\1\2/g' \
                 -e 's/^[^0-9]*\([0-9.][0-9.]*\).*/\1/'`
    case $libtool_version in
        2.2*)
        have_libtool=true
        ;;
    esac
fi
if $have_libtool ; then : ; else
    echo
    echo "You must have libtool >= 2.2 installed to compile $PROJECT."
    echo "Install the appropriate package for your distribution,"
    echo "or get the source tarball at http://ftp.gnu.org/gnu/libtool/"
    DIE=1
fi

if autoconf --version < /dev/null > /dev/null 2>&1 ; then : ; else
    echo
    echo "You must have autoconf installed to compile $PROJECT."
    echo "Install the appropriate package for your distribution,"
    echo "or get the source tarball at http://ftp.gnu.org/gnu/autoconf/"
    DIE=1
fi

if automake-1.11 --version < /dev/null > /dev/null 2>&1 ; then
    AUTOMAKE=automake-1.11
    ACLOCAL=aclocal-1.11
else if automake-1.10 --version < /dev/null > /dev/null 2>&1 ; then
    AUTOMAKE=automake-1.10
    ACLOCAL=aclocal-1.10
else
    echo
    echo "You must have automake 1.10.x or 1.11.x installed to compile $PROJECT."
    echo "Install the appropriate package for your distribution,"
    echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
    DIE=1
fi
fi

if test "$DIE" -eq 1; then
    exit 1
fi


libtoolize --force || exit $?

$ACLOCAL -I m4 || exit $?

autoconf || exit $?

autoheader || exit $?

$AUTOMAKE --add-missing || exit $?


# NOCONFIGURE is used by gnome-common; support both
if ! test -z "$AUTOGEN_SUBDIR_MODE"; then
    NOCONFIGURE=1
fi

if test -z "$NOCONFIGURE"; then
    if test -z "$*"; then
        echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
    fi

    ./configure --enable-maintainer-mode $AUTOGEN_CONFIGURE_ARGS "$@" || exit $?

    echo
    echo "Now type 'make' to compile $PROJECT."
fi

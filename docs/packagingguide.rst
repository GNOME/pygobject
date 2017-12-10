Packaging Guide
===============

Some notes on how to package PyGObject

Source packages can be found at
https://ftp.gnome.org/pub/GNOME/sources/pygobject


Existing Packages:

* https://www.archlinux.org/packages/extra/x86_64/python-gobject
* https://packages.qa.debian.org/p/pygobject.html
* https://github.com/Alexpux/MINGW-packages/tree/master/mingw-w64-pygobject


Building::

    ./configure --with-python=${PYTHON} --prefix="${PREFIX}"
    make check # if you want to run the test suite
    make DESTDIR="${PKGDIR}" install

Runtime dependencies:

    * glib
    * libgirepository (gobject-introspection)
    * libffi
    * Python 2 or 3

    The overrides directory contains various files which includes various
    Python imports mentioning gtk, gdk etc. They are only used when the
    corresponding library is present, they are not direct dependencies.

Build dependencies:

    * The runtime dependencies
    * cairo (optional)
    * pycairo (optional)
    * pkg-config

    If autotools is used:

        * gnome-common for PyGObject < 3.26
        * autoconf-archive for PyGObject >= 3.26

    If setup.py is used:

        * setuptools

Test Suite dependencies:

    * The runtime dependencies
    * GTK+ 3 (optional)
    * pango (optional)
    * pycairo (optional)

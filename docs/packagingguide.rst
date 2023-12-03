Packaging Guide
===============

Some notes on how to package PyGObject

Source packages can be found at
https://download.gnome.org/sources/pygobject

Existing Packages:

* https://www.archlinux.org/packages/extra/x86_64/python-gobject
* https://tracker.debian.org/pkg/pygobject
* https://github.com/MSYS2/MINGW-packages/tree/master/mingw-w64-pygobject

Building::

    python3 setup.py build
    python3 setup.py test # if you want to run the test suite
    python3 setup.py install --prefix="${PREFIX}" --root="${PKGDIR}"

Runtime dependencies:

    * glib
    * libgirepository (gobject-introspection)
    * libffi
    * Python 3

    The overrides directory contains various files which includes various
    Python imports mentioning gtk, gdk etc. They are only used when the
    corresponding library is present, they are not direct dependencies.

Build dependencies:

    * The runtime dependencies
    * cairo (optional)
    * pycairo (optional)
    * pkg-config
    * setuptools (optional)

Test Suite dependencies:

    * The runtime dependencies
    * GTK 4 (optional)
    * pango (optional)
    * pycairo (optional)

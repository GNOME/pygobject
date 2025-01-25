Packaging Guide
===============

PyGObject uses Meson, here are some notes on how to package PyGObject.

Source packages can be found at
https://download.gnome.org/sources/pygobject

Existing Packages:

* https://archlinux.org/packages/extra/x86_64/python-gobject
* https://tracker.debian.org/pkg/pygobject
* https://github.com/MSYS2/MINGW-packages/tree/master/mingw-w64-pygobject

Building::

    meson setup --prefix /usr --buildtype=plain _build -Dc_args=... -Dc_link_args=...
    meson compile -C _build
    meson test -C _build
    DESTDIR=/path/to/staging/root meson install -C _build

Runtime dependencies:

    * glib
    * libgirepository-2.0 (shipped with GLib ≥ 2.80)
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

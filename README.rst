.. image:: https://gitlab.gnome.org/GNOME/pygobject/-/raw/main/docs/images/pygobject.svg?ref_type=heads
   :align: center
   :width: 400px
   :height: 98px

|

**PyGObject** is a Python package which provides bindings for `GObject
<https://docs.gtk.org/gobject/>`__ based libraries such as `GTK
<https://www.gtk.org/>`__, `GStreamer <https://gstreamer.freedesktop.org/>`__,
`WebKitGTK <https://webkitgtk.org/>`__, `GLib
<https://docs.gtk.org/glib/>`__, `GIO
<https://docs.gtk.org/gio/>`__ and many more.

It supports Linux, Windows, and macOS and works with **Python 3.9+** and
**PyPy3**. PyGObject, including this documentation, is licensed under the
**LGPLv2.1+**.

Homepage
--------

https://pygobject.gnome.org

Installation
------------

The latest version from PyGObject can be installed from `PyPI <https://pypi.org/project/PyGObject/>`__:

    pip install PyGObject

PyGObject is only distributed as source distribution, so you need a C compiler installed on your host.

Please have a look at our `Getting Started <https://pygobject.gnome.org/getting_started.html>`__ documentation
for OS specific installation instructions.

Development
~~~~~~~~~~~

Our website contains instructions on how to `set up a development environment
<https://pygobject.gnome.org/devguide/dev_environ.html>`__.

Default branch renamed to ``main``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The default development branch of PyGObject has been renamed
to ``main``. To update your local checkout, use::

    git checkout master
    git branch -m master main
    git fetch
    git branch --unset-upstream
    git branch -u origin/main
    git symbolic-ref refs/remotes/origin/HEAD refs/remotes/origin/main

Imports
=======

The toplevel ``gi`` package allows you to import the different libraries
namespaces and ensure specific versions of them.

The next code line will import the ``GTK`` and ``GLib`` libraries from the
``gi.repository`` module. ``gi.repository`` holds the libraries bindings.

.. code:: python

    from gi.repository import Gtk, GLib


If you want to ensure a specific version of a library you can use :func:`gi.require_version`.

.. code:: python

    import gi
    gi.require_version('Gtk', '4.0')
    gi.require_version('GLib', '2.0')
    from gi.repository import Gtk, GLib


Currently, when importing ``Gdk`` or ``Gtk``, their init functions
(``Gdk.init_check()`` and ``Gtk.init_check()`` respectively) will be
automatically called for backwards-compatibility reasons.
This prevents, among other things, to use ``Gtk.disable_setlocale()``, as
it shall be called before ``Gtk`` initialization.

This behavior may be dropped in the future, but in the meanwhile you can
use :func:`gi.disable_legacy_autoinit` before the import to skip the
auto-init.

.. code:: python

    import gi
    gi.disable_legacy_autoinit()
    from gi.repository import Gtk


To avoid `PEP8/E402 <https://www.flake8rules.com/rules/E402.html>`_ you can
use a try block.

.. code:: python

    import sys

    import gi
    try:
        gi.require_version('Gtk', '4.0')
        gi.require_version('Adw', '1')
        from gi.repository import Adw, Gtk
    except ImportError or ValueError as exc:
        print('Error: Dependencies not met.', exc)
        sys.exit(1)


.. seealso::
    For more detailed information of the methods provided by the ``gi`` module
    checkout :ref:`guide-api`.

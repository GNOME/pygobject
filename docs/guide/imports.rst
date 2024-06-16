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

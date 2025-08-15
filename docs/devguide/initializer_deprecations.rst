========================
Initializer Deprecations
========================

Starting with PyGObject 3.11, overridden object creation and initialization (``__new__`` and
``__init__`` respectively) that contain side effects or dispatching beyond standard object creation
with `g_object_newv <https://docs.gtk.org/gobject/ctor.Object.newv.html>`_
will issue deprecation warnings. Additionally, positional arguments and non-standard keyword
argument names will also show deprecation warnings.

Deprecations will be in the form of Gtk.PyGTKDeprecationWarning. These warnings are only issued in
development releases (odd minor version, 3.9, 3.11, etc...). Stable releases with even minor
versions will not show deprecations unless the "-Wd" command line option is specified when running
Python.

Updating calls which invoke these deprecations will work across all of the 3.x releases (backwards
and forwards compatible) with the exception of Gtk.RecentChooserDialog(). With RecentChooserDialog,
updating the "manager" argument to "recent_manager" (when used either positionally or as a keyword)
is not compatible with versions of PyGObject prior to 3.11. However, the "manager" keyword is
guaranteed to work across all versions of the 3.x series (even though a deprecation is printed).

Rational
========

1. Cut down class overrides for the benefit of performance. Having a large amount of class overrides
   causes each of the class types to be loaded and all of its GI methods added.

2. Avoid the potential for silent creation bugs like `GnomeBug 711487
   <http://bugzilla.gnome.org/show_bug.cgi?id=711487>`_ and inheritance confusion `GnomeBug 721226
   <http://bugzilla.gnome.org/show_bug.cgi?id=721226>`_.

3. Ensure consistency across all GObject creation. In essence, we would like to guarantee all
   objects can be created with the same technique: taking a variable number of keyword arguments
   mapped exactly to the classes property names as defined by using `g_object_newv
   <https://docs.gtk.org/gobject/ctor.Object.newv.html>`_.
   This will greatly help consistency, documentation, and discoverability/expectation once the
   API user understands property names are valid keyword arguments for object construction.

4. Additional side effects when creating objects belong in explicitly named function (e.g.
   Gtk.IconSet.new_from_pixbuf) as opposed to overloading initializers to dispatch functionality.

5. Dispatching/overloaded methods in general should be considered a sort of "anti-pattern". They
   add unnecessary support by requiring new methods of a similar class also be added to the
   dispatch. It also means we have to maintain custom documentation for the dispatching method.
   Additionally it can hurt readability when compared using explicitly named methods. Examples:

.. code-block:: python

    cursor = Gdk.Cursor(display, cursor_type)
    cursor = Gdk.Cursor(display, pixbuf, x, y)

The prior example at least uses variable names which give a clue to the reader. A worse example is
shown with badly named variables:

.. code-block:: python

    cursor = Gdk.Cursor(disp, c)
    cursor = Gdk.Cursor(disp, c, x, y)

Explicit object creation enforces better readability as well as discoverability of documentation
without having to dig into the dispatching method to figure out what is going on:

.. code-block:: python

    cursor = Gdk.Cursor.new_for_display(display, cursor_type)
    cursor = Gdk.Cursor.new_from_pixbuf(display, pixbuf, x, y)

References
==========

* Bugzilla ticket for this deprecation: `GnomeBug 705810
  <http://bugzilla.gnome.org/show_bug.cgi?id=705810>`_

* `Mailing List Discussion
  <https://mail.gnome.org/archives/python-hackers-list/2013-August/msg00005.html>`_

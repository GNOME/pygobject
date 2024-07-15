.. currentmodule:: gi.repository

Interfaces
==========
GObject interfaces are a way of ensuring that objects passed to C code have the
right capabilities.

When a GObject implements an interface it should implement an expected set of
methods, properties and signals. In the Python sense, an interface is another
class that is inherited.

For example in GTK, :class:`Gtk.Image` supports various sources for the image
that it will display.
Some of these sources can be a :class:`Gio.Icon` or a :class:`Gdk.Paintable`,
both are actually interfaces so you don't pass a direct instance of these,
instead you should use some of the gobjects that implement the interface.
For :class:`Gio.Icon` those can be :class:`Gio.ThemedIcon` that represents an
icon from the icon theme, or :class:`Gio.FileIcon` that represents an icon
created from an image file.

Another important interface of reference is :class:`Gio.ListModel`.
It represents a mutable list of :class:`GObject.Objects <GObject.Object>`.
If you want to implement :class:`Gio.ListModel` you must implement three methods,
these are :meth:`Gio.ListModel.get_item_type`, :meth:`Gio.ListModel.get_n_items`
and :meth:`Gio.ListModel.get_item`.
The interfaces methods that you should implement are exposed as
:ref:`virtual methods <virtual-methods>`.
With these methods any consumer can iterate the list and use the objects for any
purpose.

.. tip::
    A generic implementation of :class:`Gio.ListModel` is :class:`Gio.ListStore`.
    It allows you to set the :class:`GObject.Object` type that it will store and
    provides methods to append, insert, sort, find and remove gobjects.

Example
-------

In this examples we'll be implementing a :class:`Gio.ListModel` in Python.
It will store a custom :class:`GObject.Object` and provide some helper methods
for it.

.. literalinclude:: examples/listmodel.py
    :linenos:

GObject
=======

GObject is the foundation for object-oriented programming in the GNOME
libraries.
For example :class:`GObject.Object` is the base providing the common attributes
and methods for all object types in GTK and the other libraries in this guide.

The :class:`GObject.Object` class provides methods for object construction and
destruction, property access methods, and signal support.

This chapter will introduce some important aspects about the GObject
implementation in Python.

.. toctree::
   :maxdepth: 3
   :caption: Contents

   gobject/basics
   gobject/subclassing
   gobject/interfaces
   Weak References <https://pygobject.gnome.org/guide/api/weakrefs.html>

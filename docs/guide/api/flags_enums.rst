=============
Flags & Enums
=============

Flags are subclasses of :class:`GObject.GFlags` and represent bit fields where
some bits also have names:

.. code:: pycon

    >>> Gtk.DialogFlags.MODAL
    <flags GTK_DIALOG_MODAL of type Gtk.DialogFlags>
    >>> Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT
    <flags GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT of type Gtk.DialogFlags>
    >>> int(_)
    3
    >>> Gtk.DialogFlags(3)
    <flags GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT of type Gtk.DialogFlags>
    >>> isinstance(Gtk.DialogFlags.MODAL, Gtk.DialogFlags)
    True
    >>>

Bitwise operations on them will produce a value of the same type.


Enums are subclasses of :class:`GObject.GEnum` and represent a list of named
constants:

.. code:: pycon

    >>> Gtk.Align.CENTER
    <enum GTK_ALIGN_CENTER of type Gtk.Align>
    >>> int(Gtk.Align.CENTER)
    3
    >>> int(Gtk.Align.END)
    2
    >>> Gtk.Align(1)
    <enum GTK_ALIGN_START of type Gtk.Align>
    >>> isinstance(Gtk.Align.CENTER, Gtk.Align)
    True

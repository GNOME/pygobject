.. currentmodule:: gi.repository

=============
Flags & Enums
=============

Flags are subclasses of :class:`GObject.GFlags`, which in turn is a
subclass of the standard library :class:`enum.IntFlag`. They represent
bit fields where some bits also have names:

.. code:: pycon

    >>> Gtk.DialogFlags.MODAL
    <DialogFlags.MODAL: 1>
    >>> Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT
    <DialogFlags.MODAL|DESTROY_WITH_PARENT: 3>
    >>> int(_)
    3
    >>> Gtk.DialogFlags(3)
    <DialogFlags.MODAL|DESTROY_WITH_PARENT: 3>
    >>> isinstance(Gtk.DialogFlags.MODAL, Gtk.DialogFlags)
    True
    >>>

Bitwise operations on them will produce a value of the same type.


Enums are subclasses of :class:`GObject.GEnum`, which in turn is a
subclass of the standard library :class:`enum.IntEnum`. They represent
a list of named constants:

.. code:: pycon

    >>> Gtk.Align.CENTER
    <Align.CENTER: 3>
    >>> int(Gtk.Align.CENTER)
    3
    >>> int(Gtk.Align.END)
    2
    >>> Gtk.Align(1)
    <Align.START: 1>
    >>> isinstance(Gtk.Align.CENTER, Gtk.Align)
    True

Creating New Enums and Flags
----------------------------

New enumerations and flags types can be defined by subclassing
:class:`GObject.GEnum` or :class:`GObject.GFlags` in the same way as
standard library enumerations. A new GType is registered
automatically.

.. code:: pycon

    >>> from gi.repository import GObject
    >>> class E(GObject.GEnum):
    ...     ONE = 1
    ...     TWO = 2
    ...
    >>> E.ONE
    <E.ONE: 1>
    >>> E.ONE.value_name
    'ONE'
    >>> E.ONE.value_nick
    'one'
    >>> E.__gtype__
    <GType __main__+E (1014834640)>
    >>> E.__gtype__.name
    '__main__+E'

The GType name can be set explicitly by providing a ``__gtype_name__``
attribute:

.. code:: pycon

    >>> from gi.repository import GObject
    >>> class MyEnum(GObject.GEnum):
    ...     __gtype_name__ = "MyEnum"
    ...     ONE = 1
    ...
    >>> MyEnum.__gtype__
    <GType MyEnum (767309744)>

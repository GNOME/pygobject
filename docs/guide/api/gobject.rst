.. currentmodule:: gi.repository

==============
GObject.Object
==============

Compare to other types, :obj:`GObject.Object` has the best integration between
the GObject and Python type system.

1) It is possible to subclass a :obj:`GObject.Object`. Subclassing
   creates a new :obj:`GObject.GType` which is connected to the new Python
   type. This means you can use it with API which takes :obj:`GObject.GType`.
2) The Python wrapper instance for a :obj:`GObject.Object` is always the same.
   For the same C instance you will always get the same Python instance.


In addition :obj:`GObject.Object` has support for :any:`signals <signals>` and
:any:`properties <properties>`

.. toctree::
    :titlesonly:
    :maxdepth: 1
    :hidden:

    signals
    properties
    weakrefs


Examples
--------

Subclassing:

.. code:: pycon

    >>> from gi.repository import GObject
    >>> class A(GObject.Object):
    ...     pass
    ...
    >>> A()
    <__main__.A object at 0x7f9113fc3280 (__main__+A at 0x559d9861acc0)>
    >>> A.__gtype__
    <GType __main__+A (94135355573712)>
    >>> A.__gtype__.name
    '__main__+A'
    >>>

In case you want to specify the GType name we have to provide a
``__gtype_name__``:

.. code:: pycon

    >>> from gi.repository import GObject
    >>> class B(GObject.Object):
    ...     __gtype_name__ = "MyName"
    ...
    >>> B.__gtype__
    <GType MyName (94830143629776)>
    >>>

:obj:`GObject.Object` only supports single inheritance, this means you can
only subclass one :obj:`GObject.Object`, but multiple Python classes:

.. code:: pycon

    >>> from gi.repository import GObject
    >>> class MixinA(object):
    ...     pass
    ...
    >>> class MixinB(object):
    ...     pass
    ...
    >>> class MyClass(GObject.Object, MixinA, MixinB):
    ...     pass
    ...
    >>> instance = MyClass()


Here we can see how we create a :obj:`Gio.ListStore` for our new subclass and
that we get back the same Python instance we put into it:

.. code:: pycon

    >>> from gi.repository import GObject, Gio
    >>> class A(GObject.Object):
    ...     pass
    ...
    >>> store = Gio.ListStore.new(A)
    >>> instance = A()
    >>> store.append(instance)
    >>> store.get_item(0) is instance
    True
    >>>

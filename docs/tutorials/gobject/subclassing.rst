.. currentmodule:: gi.repository

Subclassing
===========

Before entering in detail, you should know some important points about GObject
subclassing:

1. It is possible to subclass a :class:`GObject.Object`. Subclassing creates a
   new :class:`GObject.GType` which is connected to the new Python type.
   This means you can use it with API which takes :class:`GObject.GType`.

2. :class:`GObject.Object` only supports single inheritance, this means you can
   only subclass one :class:`GObject.Object`, but multiple Python classes.

3. The Python wrapper instance for a GObject.Object is always the same. For the
   same C instance you will always get the same Python instance.


Inherit from GObject.GObject
----------------------------
A native GObject is accessible via :class:`GObject.Object`.
It is rarely instantiated directly, we generally use an inherited classes.
A :class:`Gtk.Widget` is an inherited class of a :class:`GObject.Object`.
It may be interesting to make an inherited class to create a new widget, like a
settings dialog.

To inherit from :class:`GObject.Object`, you must call `super().__init__`
in your constructor to initialize the gobjects you are inheriting, like in the
example below:

.. code:: python

    from gi.repository import GObject

    class MyObject(GObject.Object):

        def __init__(self):
            super().__init__()

You can also pass arguments to `super().__init__`, for example to change
some property of your parent gobject:

.. code:: python

    class MyWindow(Gtk.Window):

        def __init__(self):
            super().__init__(title='Custom title')


In case you want to specify the GType name we have to provide a
``__gtype_name__``:

.. code:: python

    class MyWindow(Gtk.Window):
        __gtype_name__ = 'MyWindow'

        def __init__(self):
            super().__init__()


Properties
----------
One of the nice features of GObject is its generic get/set mechanism for object
properties.
Any class that inherits from GObject.Object can define new properties.
Each property has a type that never changes (e.g. ``str``, ``float``,
``int``...).

Create new properties
^^^^^^^^^^^^^^^^^^^^^

A property is defined with a name and a type. Even if Python itself is
dynamically typed, you can't change the type of a property once it is defined. A
property can be created using :func:`GObject.Property`.

.. code:: python

    from gi.repository import GObject

    class MyObject(GObject.Object):

        foo = GObject.Property(type=str, default='bar')
        property_float = GObject.Property(type=float)

        def __init__(self):
            super().__init__()

Properties can also be read-only, if you want some properties to be readable but
not writable. To do so, you can add some flags to the property definition, to
control read/write access.
Flags are :attr:`GObject.ParamFlags.READABLE` (only read access for external
code),
:attr:`GObject.ParamFlags.WRITABLE` (only write access),
:attr:`GObject.ParamFlags.READWRITE` (public):

.. there is also construct things, but they
.. doesn't seem to be functional in Python

.. code:: python

    foo = GObject.Property(type=str, flags=GObject.ParamFlags.READABLE) # not writable
    bar = GObject.Property(type=str, flags=GObject.ParamFlags.WRITABLE) # not readable


You can also define new read-only properties with a new method decorated with
:func:`GObject.Property`:

.. code:: python

    from gi.repository import GObject

    class MyObject(GObject.Object):

        def __init__(self):
            super().__init__()

        @GObject.Property
        def readonly(self):
            return 'This is read-only.'

You can get this property using:

.. code-block:: python

    my_object = MyObject()
    print(my_object.readonly)
    print(my_object.get_property('readonly'))

The API of :func:`GObject.Property` is similar to the builtin
:py:class:`property`.
You can create property setters in a way similar to Python property:

.. code-block:: python

    class AnotherObject(GObject.Object):
        value = 0

        @GObject.Property
        def prop(self):
            """Read only property."""
            return 1

        @GObject.Property(type=int)
        def prop_int(self):
            """Read-write integer property."""
            return self.value

        @prop_int.setter
        def prop_int(self, value):
            self.value = value

There is also a way to define minimum and maximum values for numbers:

.. code-block:: python

    class AnotherObject(GObject.Object):
        value = 0

        @GObject.Property(type=int, minimum=0, maximum=100)
        def prop_int(self):
            """Integer property with min-max.'"""
            return self.value

        @prop_int.setter
        def prop_int(self, value):
            self.value = value


    my_object = AnotherObject()
    my_object.prop_int = 200  # This will fail

Alternatively you can use the more verbose `__gproperties__` class
attribute to define properties:

.. code:: python

    from gi.repository import GObject

    class MyObject(GObject.Object):

        __gproperties__ = {
            'int-prop': (
                int, # type
                'integer prop', # nick
                'A property that contains an integer', # blurb
                1, # min
                5, # max
                2, # default
                GObject.ParamFlags.READWRITE # flags
            ),
        }

        def __init__(self):
            super().__init__()
            self.int_prop = 2

        def do_get_property(self, prop):
            if prop.name == 'int-prop':
                return self.int_prop
            else:
                raise AttributeError('unknown property %s' % prop.name)

        def do_set_property(self, prop, value):
            if prop.name == 'int-prop':
                self.int_prop = value
            else:
                raise AttributeError('unknown property %s' % prop.name)


For this approach properties must be defined in the ``__gproperties__`` class
attribute, a dictionary, and handled in :meth:`GObject.Object.do_get_property`
and :meth:`GObject.Object.do_set_property`
:ref:`virtual methods <virtual-methods>`.

.. hint::
    Changes to custom properties are also signaled by the ``notify`` detailed
    signal. But remember that it will normalize your property name to hyphens
    instead of underscores, so you will write ``notify::prop-int`` and not
    ``notify::prop_int``.

Signals
-------
Each signal is registered in the type system together with the type on which it
can be emitted: users of the type are said to connect to the signal on a given
type instance when they register a function to be invoked upon the signal
emission. Users can also emit the signal by themselves or stop the emission of
the signal from within one of the functions connected to the signal.

Create new signals
^^^^^^^^^^^^^^^^^^

New signals can be created by using the :func:`GObject.Signal` decorator.
The decorated methods are the object method handlers, these will be called when
the signal is emitted.

The time at which the method handlers are invoked depends on the signal flags.
:attr:`GObject.SignalFlags.RUN_FIRST` indicates that this signal will invoke the
object method handler in the first emission stage.
Alternatives are :attr:`GObject.SignalFlags.RUN_LAST` (the method handler will be
invoked in the third emission stage) and :attr:`GObject.SignalFlags.RUN_CLEANUP`
(invoke the method handler in the last emission stage).

Signals can also have arguments, the number and type of each argument is defined
as a tuple of types.

Signals can be emitted using :meth:`GObject.Object.emit`.

.. code:: python

    from gi.repository import GObject

    class MyObject(GObject.Object):

        def __init__(self):
            super().__init__()

        @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST, arg_types=(int,))
        def arg_signal(self, number):
            """Called every time the signal is emitted"""
            print('number:', number)

        @GObject.Signal
        def noarg_signal(self):
            """Called every time the signal is emitted"""
            print('noarg_signal')


    my_object = MyObject()

    def signal_callback(object_, number):
        """Called every time the signal is emitted until disconnection"""
        print(object_, number)

    my_object.connect('arg_signal', signal_callback)
    my_object.emit('arg_signal', 100)  # emit the signal "arg_signal", with the
                                       # argument 100

    my_object.emit('noarg_signal')


Alternatively you can use the more verbose `__gsignals__` class
attribute to define signals.
When a new signal is created, a method handler can also be defined in the form
of ``do_signal_name``, it will be called each time the signal is emitted.

.. code:: python

    class MyObject(GObject.Object):
        __gsignals__ = {
            'my_signal': (
                GObject.SignalFlags.RUN_FIRST,  # flag
                None,  # return type
                (int,)  # arguments
            )
        }

        def do_my_signal(self, arg):
            print("method handler for `my_signal' called with argument", arg)

.. _virtual-methods:

Virtual Methods
---------------

GObject and its based libraries usually have gobjects that expose virtual
methods. These methods serve to override functionality of the base gobject or
to run code on a specific scenario.
In that case you should call the base gobject virtual method to preserve its
original behavior.

In PyGObject these methods are prefixed with ``do_``. Some examples are
:meth:`GObject.Object.do_get_property` or :meth:`Gio.Application.do_activate`.

.. important::
    The Python :py:class:`super` class only works for the immediate parent.
    If you want to chain some virtual method from a object that is more
    up in the hierarchy of the one you are subclassing you must call the method
    directly from the object class:
    ``SomeOject.method(self, args)``.

.. code:: python

    class SomeOject(OtherObject):
        ...

    class MyObject(SomeOject):

        def __init__(self):
            super().__init__()

        def do_virtual_method(self):
            # Call the original method to keep its original behavior
            super().do_virtual_method()

            # Run some extra code
            ...

        """This is a virtual method from SomeOject parent"""
        def do_other(self):
            OtherObject.do_other(self)  # We can't use super()
            ...

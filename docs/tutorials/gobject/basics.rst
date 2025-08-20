.. currentmodule:: gi.repository

Basics
======

.. hint::
    In this example, we will use GTK widgets to demonstrate GObject
    capabilities.

GObject Initialization
----------------------

GObjects are initialized like any other Python class.

.. code:: python

    label = Gtk.Label()

.. _basics-properties:

Properties
----------

GObject has a powerful properties system.

Properties describe the configuration and state of a gobject. Each gobject has
its own particular set of properties. For example, a GTK button has the property
``label`` which contains the text of the label widget inside the button.

You can specify the name and value of any number of properties as keyword
arguments when creating an instance of a gobject. To create a label aligned to
the right with the text "Hello World", use:

.. code:: python

    label = Gtk.Label(label='Hello World', halign=Gtk.Align.END)

which is equivalent to

.. code:: python

    label = Gtk.Label()
    label.set_label('Hello World')
    label.set_halign(Gtk.Align.END)

There are various ways of interacting with a gobject properties from Python, we
already have seen the two first ways, these are setting them on initialization
or using the getters and setters functions that the gobject might provide.

Other option is to use :class:`GObject.Object` builtin methods :meth:`GObject.Object.get_property`
and :meth:`GObject.Object.set_property`. Using these methods is more common when
you have created a :class:`GObject.Object` subclass and you don't have getters
and setters functions.

.. code:: python

    label = Gtk.Label()
    label.set_property('label', 'Hello World')
    label.set_property('halign', Gtk.Align.END)
    print(label.get_property('label'))

Instead of using getters and setters you can also get and set the gobject
properties through the ``props`` property such as ``label.props.label = 'Hello World'``.
This is equivalent to the more verbose methods that we saw before, and it's a
more pythonic way of interacting with properties.

To see which properties are available for a gobject you can ``dir`` the
``props`` property:

.. code:: python

    widget = Gtk.Box()
    print(dir(widget.props))

This will print to the console the list of properties that a :class:`Gtk.Box`
has.

Property Bindings
^^^^^^^^^^^^^^^^^

GObject provides a practical way to bind properties of two different gobjects.
This is done using the :meth:`GObject.Object.bind_property` method.

The behavior of this binding can be controlled by passing a
:class:`GObject.BindingFlags` of choice.
:attr:`GObject.BindingFlags.DEFAULT` will update the target property every time
the source property changes.
:attr:`GObject.BindingFlags.BIDIRECTIONAL` creates a bidirectional binding; if
either the property of the source or the property of the target changes, the
other is updated.
:attr:`GObject.BindingFlags.SYNC_CREATE` is similar to ``DEFAULT`` but it will
also synchronize the values of the source and target properties when creating
the binding.
:attr:`GObject.BindingFlags.INVERT_BOOLEAN` works only for boolean properties
and setting one property to ``True`` will result in the other being set to
``False`` and vice versa (this flag cannot be used when passing custom
transformation functions to :meth:`GObject.Object.bind_property`).

.. code:: python

    entry = Gtk.Entry()
    label = Gtk.Label()

    entry.bind_property('text', label, 'label', GObject.BindingFlags.DEFAULT)

In this example **entry** is our source object and ``text`` the source property
to bind.
**label** is the target object and the namesake property ``label`` is the
target property.
Every time someone changes the ``text`` property of the entry the label
``label`` will be updated as well with the same value.

Property Bindings with Transformations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Sometimes you may want to bind two properties that are incompatible, or you
simply need to apply some transformation between these values.
For these scenarios :meth:`GObject.Object.bind_property` also accepts custom
transformation functions that serve to this purpose.

The transformation functions take as first argument the :class:`GObject.Binding`
instance for this binding and as second argument the property value depending
on the direction of the transformation.
For ``transform_to`` this will be the value of the source property and for
``transform_from`` the value of the target property.
Each function should return the value to be set in the other object's property,
with the correct type.

In this example we'll do an ``int`` to ``bool`` type conversion between two
objects:

.. code:: python

    def transform_to(_binding, value):
       return bool(value)  # Return int converted to a bool

    def transform_from(_binding, value):
        return int(value)  # Return bool converted to a int

    source.bind_property(
        'int_prop',
        target,
        'bool_prop',
        GObject.BindingFlags.BIDIRECTIONAL,
        transform_to,
        transform_from
    )

.. _basics-signals:

Signals
-------

GObject signals are a system for registering callbacks for specific events.

A generic example is:

.. code:: python

    handler_id = gobject.connect('event', callback, data)

Firstly, *gobject* is an instance of a gobject we created earlier. Next, the
event we are interested in. Each gobject has its own particular events which
can occur.
This means that when the gobject emits the event, the signal is issued.
Thirdly, the *callback* argument is the name of the callback function.
It contains the code which runs when signals of the specified type are issued.
Finally, the *data* argument includes any data which should be passed when the
signal is issued. However, this argument is completely optional and can be left
out if not required.

The function returns a number that identifies this particular signal-callback
pair.
It is required to disconnect from a signal such that the callback function will
not be called during any future or currently ongoing emissions of the signal it
has been connected to:

.. code:: python

    gobject.disconnect(handler_id)

When creating the callback function for a signal, the arguments it accepts will
depend on the specific signal, but for a signal with no arguments it will look
like this:

.. code:: python

    def on_event(gobject, data):
        ...

    my_object.connect('event', on_event, data)

Where ``gobject`` is the object that triggered the signal and ``data`` is the
additional data that we previously passed to the :meth:`GObject.Object.connect`
method.
If the signal had arguments, they will come before the optional data argument.

The ``notify`` signal
^^^^^^^^^^^^^^^^^^^^^

When any of a GObject's properties change, it will emit the ``notify`` signal.
This is a "detailed" signal, meaning that you can listen to a subset of the
signal, in this case a specific property.
For example, you can connect to the signal in the form of
``notify::property-name``:

.. code:: python

    def callback(label, _pspec):
        print(f'The label prop changed to {label.props.label}')

    label = Gtk.Label()
    label.connect('notify::label', callback)


.. currentmodule:: gi.repository

==========
Properties
==========

Properties are part of a class and are defined through a
:obj:`GObject.ParamSpec`, which contains the type, name, value range and so
on.

To find all the registered properties of a class you can use the
:meth:`GObject.Object.list_properties` class method.

.. code:: pycon

    >>> Gio.Application.list_properties()
    [<GParamString 'application-id'>, <GParamFlags 'flags'>, <GParamString
    'resource-base-path'>, <GParamBoolean 'is-registered'>, <GParamBoolean
    'is-remote'>, <GParamUInt 'inactivity-timeout'>, <GParamObject
    'action-group'>, <GParamBoolean 'is-busy'>]
    >>> param = Gio.Application.list_properties()[0]
    >>> param.name
    'application-id'
    >>> param.owner_type
    <GType GApplication (94881584893168)>
    >>> param.value_type
    <GType gchararray (64)>
    >>>

The :obj:`GObject.Object` contructor takes multiple properties as keyword
arguments. Property names usually contain "-" for seperating words. In Python
you can either use "-" or "_". In this case variable names don't allow "-", so
we use "_".

.. code:: pycon

    >>> app = Gio.Application(application_id="foo.bar")

To get and set the property value see :meth:`GObject.Object.get_property` and
:meth:`GObject.Object.set_property`.

.. code:: pycon

    >>> app = Gio.Application(application_id="foo.bar")
    >>> app
    <Gio.Application object at 0x7f7499284fa0 (GApplication at 0x564b571e7c00)>
    >>> app.get_property("application_id")
    'foo.bar'
    >>> app.set_property("application_id", "a.b")
    >>> app.get_property("application-id")
    'a.b'
    >>>


Each instance also has a ``props`` attribute which exposes all properties
as instance attributes:

.. code:: pycon

    >>> from gi.repository import Gtk
    >>> button = Gtk.Button(label="foo")
    >>> button.props.label
    'foo'
    >>> button.props.label = "bar"
    >>> button.get_label()
    'bar'
    >>>


To track changes of properties, :obj:`GObject.Object` has a special ``notify``
signal with the property name as the detail string. Note that in this case you
have to give the real property name and replacing "-" with "_" wont work.

.. code:: pycon

    >>> app = Gio.Application(application_id="foo.bar")
    >>> def my_func(instance, param):
    ...     print("New value %r" % instance.get_property(param.name))
    ...
    >>> app.connect("notify::application-id", my_func)
    11L
    >>> app.set_property("application-id", "something.different")
    New value 'something.different'
    >>>

You can define your own properties using the :obj:`GObject.Property` decorator,
which can be used similarly to the builtin Python :any:`property` decorator:

.. function:: GObject.Property(type=None, default=None, nick='', blurb='', \
    flags=GObject.ParamFlags.READWRITE, minimum=None, maximum=None)

    :param GObject.GType type: Either a GType, a type with a GType or a
        Python type which maps to a default GType
    :param object default: A default value
    :param str nick: Property nickname
    :param str blurb: Short description
    :param GObject.ParamFlags flags: Property configuration flags
    :param object minimum: Minimum value, depends on the type
    :param object maximum: Maximum value, depends on the type


.. code:: python

    class AnotherObject(GObject.Object):
        value = 0

        @GObject.Property
        def prop_pyobj(self):
            """Read only property."""

            return object()

        @GObject.Property(type=int)
        def prop_gint(self):
            """Read-write integer property."""

            return self.value

        @prop_gint.setter
        def prop_gint(self, value):
            self.value = value

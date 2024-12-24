.. currentmodule:: gi.repository

=======
Signals
=======

GObject signals are a system for registering callbacks for specific events.

To find all signals of a class you can use the
:func:`GObject.signal_list_names` function:


.. code:: pycon

    >>> GObject.signal_list_names(Gio.Application)
    ('activate', 'startup', 'shutdown', 'open', 'command-line', 'handle-local-options')
    >>>


To connect to a signal, use :meth:`GObject.Object.connect`:

.. code:: pycon

    >>> app = Gio.Application()
    >>> def on_activate(instance):
    ...     print("Activated:", instance)
    ...
    >>> app.connect("activate", on_activate)
    17L
    >>> app.run()
    ('Activated:', <Gio.Application object at 0x7f1bbb304320 (GApplication at 0x5630f1faf200)>)
    0
    >>>

It returns number which identifies the connection during its lifetime and which
can be used to modify the connection.

For example it can be used to temporarily ignore signal emissions using
:meth:`GObject.Object.handler_block`:

.. code:: pycon

    >>> app = Gio.Application(application_id="foo.bar")
    >>> def on_change(*args):
    ...     print(args)
    ...
    >>> c = app.connect("notify::application-id", on_change)
    >>> app.props.application_id = "foo.bar"
    (<Gio.Application object at 0x7f1bbb304550 (GApplication at 0x5630f1faf2b0)>, <GParamString 'application-id'>)
    >>> with app.handler_block(c):
    ...     app.props.application_id = "no.change"
    ...
    >>> app.props.application_id = "change.again"
    (<Gio.Application object at 0x7f1bbb304550 (GApplication at 0x5630f1faf2b0)>, <GParamString 'application-id'>)
    >>>


You can define your own signals using the :obj:`GObject.Signal` decorator:


.. function:: GObject.Signal(name='', flags=GObject.SignalFlags.RUN_FIRST, \
    return_type=None, arg_types=None, accumulator=None, accu_data=None)

    :param str name: The signal name
    :param GObject.SignalFlags flags: Signal flags
    :param GObject.GType return_type: Return type
    :param list arg_types: List of :class:`GObject.GType` argument types
    :param accumulator: Accumulator function
    :type accumulator: :obj:`GObject.SignalAccumulator`
    :param object accu_data: User data for the accumulator


.. code:: python

    class MyClass(GObject.Object):

        @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST, return_type=bool,
                        arg_types=(object,),
                        accumulator=GObject.signal_accumulator_true_handled)
        def test(self, *args):
            print("Handler", args)

        @GObject.Signal
        def noarg_signal(self):
            print("noarg_signal")

    instance = MyClass()

    def test_callback(inst, obj):
        print "Handled", inst, obj
        return True

    instance.connect("test", test_callback)
    instance.emit("test", object())

    instance.emit("noarg_signal")

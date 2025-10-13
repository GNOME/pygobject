Weak References
===============

While Python has a builtin ``weakref`` module it only allows one to create
weak references to Python objects, but with PyGObject the Python object
"wrapping" a GObject and the GObject itself might not have the same lifetime.
The wrapper can get garbage collected and a new wrapper created again at a
later point.

If you want to get notified when the underlying GObject gets finalized use
:meth:`GObject.Object.weak_ref`:


.. method:: GObject.Object.weak_ref(callback, *user_data)

    Registers a callback to be called when the underlying GObject gets
    finalized. The callback will receive the given `user_data`.

    To unregister the callback call the ``unref()`` method of the returned
    GObjectWeakRef object.

    :param callback: A callback which will be called when the object
        is finalized
    :type callback: :func:`callable`
    :param user_data: User data that will be passed to the callback
    :returns: GObjectWeakRef

========================================
Tutorial - Adding a GBytes Marshaler
========================================

This tutorial is based on a request for adding a Python bytes to GBytes argument
marshaler for introspected functions (`GnomeBug 729541 <http://bugzilla.gnome.org/show_bug.cgi?id=729541>`_).

Working with GI Interactively
=============================

To get started, we can inspect a GI function which takes a GBytes argument interactively
to determine what the argument is in terms of GI (ipython is very helpful here):

PyGI exposes GI functions as custom callable objects which also implement the
`GIFunctionInfo API <https://docs.gtk.org/girepository/class.FunctionInfo.html>`_ and its base classes.

.. code-block:: python

    >>> from gi.repository import GLib
    >>> Gio.MemoryInputStream.new_from_bytes
    gi.FunctionInfo(new_from_bytes)

Get the GIArgInfo:

.. code-block:: python

    >>> Gio.MemoryInputStream.new_from_bytes.get_arguments()
    (gi.ArgInfo(bytes),)
    >>> arg, = _
    >>> arg
    gi.ArgInfo(bytes)

Determine argument type using:

.. code-block:: python

    >>> ty = arg.get_type()
    >>> ty
    gi.TypeInfo(type_type_instance)
    >>> ty.get_tag_as_string()
    'interface'

At this point we know the argument type tag is an "interface" or
`GI_TYPE_TAG_INTERFACE <https://docs.gtk.org/girepository/enum.TypeTag.html>`_
so `g_type_info_get_interface <https://docs.gtk.org/girepository/method.TypeInfo.get_interface.html>`_
should be valid for the `GITypeInfo <https://docs.gtk.org/girepository/class.TypeInfo.html>`_.

.. code-block:: python

    >>> iface = ty.get_interface()
    StructInfo(Bytes)

In this case get_interface() is giving us a
`GIStructInfo <https://docs.gtk.org/girepository/class.StructInfo.html>`_.
We can then verify a valid GType is available using `gi_registered_type_info_get_g_type() <https://docs.gtk.org/girepository/method.RegisteredTypeInfo.get_g_type.html>`_
(GIRegisteredTypeInfo is a base class of GIStructInfo):

.. code-block:: python

    >>> gtype = iface.get_g_type()
    >>> gtype
    <GType GBytes (12737104)>

We also have to find the fundamental GType for GBytes:

.. code-block:: python

    >>> gtype.fundamental
    <GType GBoxed (72)>

Mapping out the C Code
======================

We now have enough information to correlate to the various switch statements in
the PyGI caching system which will help us place our new marshaling code. Starting
with `pygi-cache.c:pygi_arg_cache_new <https://gitlab.gnome.org/GNOME/pygobject/-/blob/main/gi/pygi-cache.c#L173>`_
you can trace through `_arg_cache_new_for_interface <https://gitlab.gnome.org/GNOME/pygobject/-/blob/main/gi/pygi-cache.c#L136>`_
and finally land in `pygi-cache-struct.c:pygi_arg_struct_new_from_info <https://gitlab.gnome.org/GNOME/pygobject/-/blob/main/gi/pygi-cache-struct.c#L584>`_.

For this bug, we are looking to add a "from py" marshaling convenience.
So we could add a new conditional in `arg_struct_from_py_setup <https://gitlab.gnome.org/GNOME/pygobject/-/blob/main/gi/pygi-cache-struct.c#L541>`_ within the ``G_TYPE_BOXED`` conditional. However, note this text in the function:

Instead what we should really do is create a new from_py_marshaller
specifically for GBytes arguments which are baked in at cache setup time.
Essentially specializing GBytes as early as possible in
`pygi_arg_struct_to_py_marshal <https://gitlab.gnome.org/GNOME/pygobject/-/blob/main/gi/pygi-cache-struct.c#L435>`_
by setting ``arg_cache->from_py_marshaller``.

Marshaler Callbacks
===================

Relevant marshaler callbacks are declared in `pygi-cache.h <https://gitlab.gnome.org/GNOME/pygobject/-/blob/main/gi/pygi-cache.h#L50>`_
and we need an implementation of both ``PyGIMarshalFromPyFunc``.

.. code-block:: c

    typedef gboolean (*PyGIMarshalFromPyFunc) (PyGIInvokeState        *state,
                                               PyGICallableCache      *callable_cache,
                                               PyGIArgCache           *arg_cache,
                                               PyObject               *py_arg,
                                               GIArgument             *arg,
                                               PyGIMarshalCleanupData *cleanup_data);

PyGIMarshalFromPyFunc is called for each argument prior to executing the callee, the relevant bits are as follows:

* py_arg - This is the input PyObject the Python caller is passing to the GI function.
  We need to type check this and do a mini dispatch depending on the type
  (PyBytes or buffer protocol check, PyGIBoxed, and Py_None).
* arg - This is the target memory area marshaler will fill out. In this case arg->v_pointer
  will be assigned a pointer to a GBytes object.
* arg_cache->allow_none - If TRUE, py_arg can be Py_None and arg->v_pointer should be set to
  NULL, returning TRUE from the marshaling callback.
* arg_cache->transfer - Determines how memory should be managed for the argument.
* cleanup_data - This is an output argument that can be set to custom data which passed back
  to us. used for freeing relevant memory after the callee
  returns.
  Alongside the data we also provide two GDestroyNotify functions: one for the success case (everything is handled)
  and one for the failure case (didn't work, clean all up).
  In our case we should set the data to the GBytes pointer, the destroy function to NULL depending on transfer semantics.
  The destroy_failure function should always point to ``g_bytes_unref()``.

The cleanup functions are called after the callee finishes and to cleanup any temporary data
we created while the callee was running.

Transfer Semantics
==================

A py_arg input of type PyGIBoxed is a direct wrapping of an existing GBoxed. This is a fairly
simple case to deal with, we just need to extract the boxed pointer (pyg_boxed_get) and assign
it to arg->v_pointer. For GI_TRANSFER_EVERYTHING we also need to add a reference the callee can
own by calling g_bytes_ref on this pointer.

In the case where we are passed a PyBytes (or Python object implementing the buffer protocol),
we need to create a new GBytes which holds a pointer to the PyBytes data. Zero copy can easily
be achieved when transfer is GI_TRANSFER_NOTHING because a read-only buffer can be retrieved
from Python and passed to the GBytes constructor (without a free_func). We know the lifetime
of the PyBytes is valid at least until the callee completes. The trick here is we also need to
set *cleanup_data* to the newly created GBytes so our cleanup callback can free the GBytes.
Since we didn't set a free_func when constructing the GBytes, calling g_bytes_unref will not
touch our Python owned data.

For converting a PyBytes with transfer mode as GI_TRANSFER_EVERYTHING, we basically follow
the same as above with some extra tricks. Since the callee is intending to own the GBytes
we pass it, we must pass it something which is guaranteed to survive after our Python function
returns (must exist after our cleanup callback). The easiest technique here is to memcpy the result
of the PyBytes data and construct a GBytes using g_bytes_new_with_free_func, with a free_func of
g_free and user_data of the bytes (no need for setting cleanup_data because the C callee owns everything).

However, it is possible to achieve zero copy with PyBytes and GI_TRANSFER_EVERYTHING by creating a custom
free_func which calls Py_DECREF. However, this free_func must wrap any Python API calls with
PyGILState_Ensure/Release pairs:

.. code-block:: c

    void
    threaded_py_bytes_free (PyObject *py_bytes)
    {
        PyGILState_STATE state = PyGILState_Ensure ();
        Py_DECREF (py_bytes);
        PyGILState_Release (state);
    }

    gboolean marshal (...)
    {
        /* ... py_arg type and transfer checks ... */
        char *buf = NULL;
        Py_ssize_t length;
        PyBytes_AsStringAndSize (py_arg, &buf, &length);
        arg->v_pointer = g_bytes_new_with_free_func (buf, length, threaded_py_bytes_free, py_arg);
        cleanup_data = { NULL };
        return True;

The above zero copy implementation could also possibly be implemented using memoryviews for accessing a Py_buffer instead of requiring a PyBytes type as input.

Marshaler Implementation
========================

This section is left up to the reader as an exercise, remember to write tests!

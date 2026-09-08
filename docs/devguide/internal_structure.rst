Internal Structure
==================

PyGObject is written part in Python, and part in C. This can make it hard to find functionality.
This document should give you enough context so you can confidently start working on PyGObject.

PyGObject's responsibility is to marshal between Python and libraries exposed via
[GObject Introspection](https://docs.gtk.org/girepository/).

PyGObject can be deconstructed in the following functionalities:

- Object creation and lifecycle management. Objects can be created from Python, but also from
  GObject directly (e.g. by ``Gtk.Builder``).
- Marshalling happens for ``GValue`` and ``GIArguments`` to/from Python types.
  Asynchronous functions have some special handling. Marshallers are generally cached (``pygi-cache.h``).
- Overrides allow us to make GI API's more Pythonic or deal with version inconsistencies.
  Overrides are written in Python (preferred), but sometimes use functions written in C.
  Most notably :class:`~gi.repository.GObject.Object`.
- Hard coded wrappers (``GSource``, ``GError``, GIRepository classes, GObject (partly)).
- C extension API (``pygobject.h``, ``pygobject-types.h``).
  Some code, e.g. in ``pygboxed.c``, is solely for the extension API, and is not used internally.
- Foreign interface for pycairo interop. This is not available outside of PyGObject.

Object lifecycle
----------------

* Numeric types, boolean, unichar: converted to their appropriate Python type.
* ``GObject``-based types: Python object on demand, shared if the same object is returned from multiple calls, instance dict shared among instances.
* Simple types (``GTypeInstance``, but not `GObject``), boxed types, structs:
  Python object on demand, instance dict *not* shared.
* Arrays, lists, hash tables: elements are marshalled to their appropriate C type.
  Python lists and dicts are created.
  Modifying a list or dict coming from an introspected function does not change the original.

Marshallers
-----------

Marshalling happens for all interactions with libraries exposed through PyGObject:

- functions
- instance methods
- callbacks and closures (e.g. for signals)
- struct attributes
- properties and fields
- virtual functions (``do_*``)

Many types in the GLib/GObject ecosystem can be reference-counted.
Some types, like ``char*``, and ``GValue`` can not.
For those types its imporant we keep track of who owns that data at any point in time.

Ownership
~~~~~~~~~

When an introspected function is called with anything more complex than an integer,
the question of ownership arises. Who is resposible for (freeing) the data after a
function is called? For this, parameters are annotated with ownership rules (``transfer``).

In the Python to C scenario (``*_from_py``):

* *Transfer Nothing*: The translated data will not be owned by the C function. It is PyGObject's responsibility to free it after it's used.
* *Transfer Everything*: The C function/field will take ownership of that data. PyGObject doesn't have to free it.
* *Transfer Container*: A special case for types like lists, arrays and hash tables. The container structure will be owned
  by the C function, but the data is not. This construct is mostly used for data returned from C (e.g. a temporary array or hash table).
  Put differently: the container will be owned by the callee (same as Transfer Everything), but the elements will be owned by the caller
  (Transfer Nothing).

Translating from C to Python (``*_to_py``) does the opposite:

* *Transfer Everything*: Ownership is transferred to PyGObject. PyGObject has to free it.
* *Transfer Nothing*: Data remains owned by the called function/field. PyGObject should not free it, but may take a reference or copy.
* *Transfer Container*: The containing type (list, array, hash table) is for PyGObject to free. The container contents is not.

Although this sounds good, there's still room for error:

* Containers with transfer mode "container" can have a destroy function associated, which triggers when the container is unreffed.
  There's no way to query if a destroy function is attached.
* For container types it's unknown if unreferencing will properly clear the contained data.

For objects that have the possibility for reference counting (GObject, GArrays, GHashTables, GTypeInstance) PyGObject should use reference counting.
Boxed types have copy/free functions that work in a similar way.
For other types we have to revert to copying memory. This also applies to types like ``char*``, ``char**`` (primitive arrays).

Cleanup
~~~~~~~

After a call is done, either from Python to C, or from C to python, C variables created during marshalling
need to be cleaned up (see transfer rules above).

There's a few cases where cleanup is needed:

* A call was successful: clean all data that is now owned by PyGObject.
* "In" parameter marshalling fails: clean all data. No call is made, so all data still belongs to PyGObject
  and needs to be freed accordingly.
* Return value or "out" parameter marshalling fails: The call was successful, but we can't deal with the result.
  PyGObject should clean up the "in" and "out" parameters as if the call was successful.
  Already marshalled objects need to be clean up as well.

Note that a call that sets a :class:`~gi.repository.GLib.Error` is still a successful call.

Note that the cleanup data is not always the marshalled value, but can contain more information needed to
properly free an object. A good example is closures.

Collections
~~~~~~~~~~~

Collection types (``GArray``, ``GPtrArray``, ``GHashTable``, ``GList``, ``GSlist``) have special serializers.

Because those types contain other types, everything from an integer to an object, PyGObject collects the
cleanup data for each element that's part of the collection.

Arrays and has tables can be reference counted, lists can't. For lists it's quite evident PyGObject has to
keep track of cleanup data for each element in the list.

The case of arrays and hash tables is a bit more complicated. Currently no ``free_func`` is set, except
for UTF8 and filename types when the ownership type is *transfer everything*.
For hash tables, the key and value ownership type is changed to avoid double free.

The current approach is not ideal, since it leaks references for other pointer types than
UTF8 and filenames.

NB. There's no guarantee that the introspected library will use the appropriate ``_ref``
and ``_unref`` functions on arrays and hash tables.

Non-GI Marshallers
------------------

In Python it's possible to create your own types. Those are registered with the GObject type system and can have
properties and signals. Those types are not available as GI data, obviously, so this case is dealt differently.

This applies only to properties and signals.

Properties
----------

Property values are marshalled into ``GValue``s. Since ``GValue`` uses ``GType`` for type information,
the marshalling is slightly simpler.

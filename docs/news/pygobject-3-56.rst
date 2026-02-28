:date: Feb 27, 2026
:author: Arjan Molenaar

PyGObject 3.56.0
================

PyGObject 3.56.0 has just been released. Major features include: better integration with
GObject's lifecycle (``do_constructed``, ``do_dispose``), a simpler way to deal with
Python wrapper objects, and cleanup of legacy code.

GObject lifecycle methods
-------------------------

This release exposes two new GObject lifecycle hooks: ``do_constructed`` and ``do_dispose``.

* ``do_constructed`` is called after a GObject has been fully constructed.
* ``do_dispose`` is called when the underlying GObject (not just the Python wrapper) is disposed,
  for example via :obj:`~gi.repository.GObject.Object.run_dispose`.

Slots
-----

Slots (``__slots__``) have never been officially supported in PyGObject, even though it worked in some cases.
With the new wrapping model (see below), using ``__slots__`` can cause issues.
PyGObject now emits a warning when ``__slots__`` is used in classes derived from GObject.

Shallow wrapper objects
-----------------------

PyGObject's Python object tracking used to be complex. In 3.56, wrapper lifecycle management has been simplified.

Previously, PyGObject retained Python wrapper objects when instance attributes existed,
using `toggle references <https://docs.gtk.org/gobject/method.Object.add_toggle_ref.html>`__.
Now, wrapper objects are always discardable. PyGObject preserves the instance dictionary and reapplies it to
new wrapper instances as needed. This behavior is easier to reason about and enables features such as ``do_dispose``.

Legacy code removal
-------------------

In 3.56, marshalling for fields, properties, constants, and signal closures has been replaced by the existing
function/method call marshalling logic. Because that path already supports more cases, this change resolves
multiple long-standing issues.

PyGObject is getting rid of custom wrapper logic. In this release, ``pygtkcompat`` and wrappers for
``GLib.OptionContext/GLib.OptionGroup`` have been fully removed.

What's next
-----------

There is still more to improve. We need your help for that: if you spot un-pythonic behavior, create an issue.
Maybe you feel like fixing an issue even.

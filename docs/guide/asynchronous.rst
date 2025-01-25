.. currentmodule:: gi.repository

Asynchronous Programming
========================

Asynchronous programming is an essential paradigm for handling tasks like I/O
operations, ensuring that your application remains responsive. GLib supports
asynchronous operations alongside its synchronous counterparts. These
asynchronous functions typically have a ``_async`` suffix and execute tasks in
the background, invoking a callback or returning a future-like object upon
completion or cancellation.

PyGObject offers two primary ways to implement asynchronous programming:

1. **Using Python's `asyncio` module** (available since PyGObject 3.50)
2. **Using callbacks** (the traditional approach)

Asynchronous Programming with ``asyncio``
-----------------------------------------

.. attention::
   Asyncio support for PyGObject is **experimental**. Feel free
   to explore its integration, but note that the API is subject to change as
   potential issues are addressed.

Overview of `asyncio` Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

PyGObject integrates seamlessly with Python's :mod:`asyncio` module by
providing:

1. An :mod:`asyncio` event loop implementation, enabling normal operation of
Python's asynchronous code.
2. Awaitable objects for Gio's asynchronous functions, allowing ``await``
syntax within Python coroutines.

Event Loop Integration
~~~~~~~~~~~~~~~~~~~~~~

To use the :mod:`asyncio` event loop with GLib, set up the GLib-based event loop
policy:

.. code-block:: python

    import asyncio
    from gi.events import GLibEventLoopPolicy

    # Set up the GLib event loop
    policy = GLibEventLoopPolicy()
    asyncio.set_event_loop_policy(policy)

Now, fetch the event loop and submit tasks:

.. code-block:: python

    loop = policy.get_event_loop()


    async def do_some_work():
        await asyncio.sleep(2)
        print("Done working!")


    task = loop.create_task(do_some_work())

.. note::

    Keep a reference to the tasks you create, as :mod:`asyncio` only
    maintains weak references to them.

Gio Asynchronous Function Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the callback parameter of a Gio asynchronous function is omitted, PyGObject
automatically returns an awaitable object (similar to :class:`asyncio.Future`).
This allows you to use ``await`` and cancel operations from within a coroutine.

.. code-block:: python

    loop = policy.get_event_loop()


    async def list_files():
        f = Gio.file_new_for_path("/")
        for info in await f.enumerate_children_async(
            "standard::*", 0, GLib.PRIORITY_DEFAULT
        ):
            print(info.get_display_name())


    task = loop.create_task(list_files())

Example: Download Window with Async Feedback
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Here is a full example illustrating asynchronous programming with
:mod:`asyncio` for a download window with async feedback.

.. literalinclude:: download_asyncio.py
   :language: python

Key Considerations
~~~~~~~~~~~~~~~~~~

* Async tasks use :obj:`GLib.PRIORITY_DEFAULT`. For background tasks,
  consider using a lower priority to avoid affecting the responsiveness of
  your GTK UI. Please see the `PRIORITY_* GLib Constants
  <https://docs.gtk.org/glib/index.html#constants>` for other settings.
* Prefer starting your application using
  :obj:`Gio.Application` or :obj:`Gtk.Application` instead of
  :func:`asyncio.run`, which is incompatible.

Asynchronous Programming with Callbacks
---------------------------------------

The traditional callback approach is a robust alternative for asynchronous
programming in PyGObject. Consider this example of downloading a web page and
displaying its source in a text field. The operation can also be canceled
mid-execution.

.. literalinclude:: download_callback.py
   :language: python

Synchronous Comparison
~~~~~~~~~~~~~~~~~~~~~~

   Before exploring the asynchronous method, let’s review the simpler blocking
   approach:

.. code-block:: python

    file = Gio.File.new_for_uri(
        "https://developer.gnome.org/documentation/tutorials/beginners.html"
    )
    try:
        status, contents, etag_out = file.load_contents(None)
    except GLib.GError:
        print("Error!")
    else:
        print(contents)

Asynchronous Workflow with Callbacks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For asynchronous tasks, you’ll need:

1. A :class:`Gio.Cancellable` to allow task cancellation.
2. A :class:`Gio.AsyncReadyCallback` function to handle the result upon task
   completion.

The example setup includes:

* **Start Button**: The handler calls :meth:`Gio.File.load_contents_async`
  with a cancellable and a callback function.
* **Cancel Button**: The handler calls :meth:`Gio.Cancellable.cancel` to stop
  the operation.

Once the operation completes—whether successfully, due to an error, or because
it was canceled—the ``on_ready_callback()`` function is invoked. This callback
receives two arguments: the :class:`Gio.File` instance and a
:class:`Gio.AsyncResult` instance containing the operation's result.

To retrieve the result, call :meth:`Gio.File.load_contents_finish`. This method
behaves like :meth:`Gio.File.load_contents`, but since the operation has
already completed, it returns immediately without blocking.

After handling the result, call :meth:`Gio.Cancellable.reset` to prepare the
:class:`Gio.Cancellable` for reuse in future operations. This ensures that the
"Load" button can be clicked again to initiate another task. The application
enforces that only one operation is active at a time by disabling the "Load"
button during an ongoing task using :meth:`Gtk.Widget.set_sensitive`.

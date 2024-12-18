.. currentmodule:: gi.repository

Asynchronous Programming
========================

In addition to functions for blocking I/O glib also provides corresponding
asynchronous versions, usually with the same name plus a ``_async`` suffix.
These functions do the same operation as the synchronous ones but don't block
during their execution. Instead of blocking they execute the operation in the
background and call a callback once the operation is finished or got canceled.

Asynchronous programming can be done in two ways:

1. asyncio, also known as ``async``/``await``
2. callbacks

Callbacks is the classic approach. Since PyGObject 3.50 it's also possible to use Python's build in
:mod:`asyncio` module. 

Asynchronous Programming with ``asyncio``
-----------------------------------------

.. attention::
    Asyncio support for PyGObject is **experimental**.
    
    You can use the async integration (we encourage you to try!), but the API is not guaranteed stable;
    it might change if we find problems.

PyGObject has integration with the :mod:`asyncio` module. There are two parts
to this integration. First, PyGObject provides an :mod:`asyncio` event loop so that Python code using
:mod:`asyncio` will run normally. The second part to the integration is that Gio asynchronous functions
can return an awaitable object, which allows using ``await`` on them within a Python coroutine.

Event Loop integration
~~~~~~~~~~~~~~~~~~~~~~

To use the :mod:`asyncio` event loop integration in GLib

.. code-block:: python

    import asyncio
    from gi.events import GLibEventLoopPolicy

    # Setup the GLib event loop
    policy = GLibEventLoopPolicy()
    asyncio.set_event_loop_policy(policy)

At this point you can fetch the event loop and start submitting tasks:

.. code-block:: python

    loop = policy.get_event_loop()

    async def do_some_work():
        await asyncio.sleep(2)
        print("done working")

    task = loop.create_task(do_some_work())

.. note::

    Note that you'll need to keep a reference to the tasks you create, since the asyncio module will only
    maintain a weak reference.

Gio asynchronous function integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the callback parameter of an asynchronous Gio function is skipped, then PyGObject will automatically
return an awaitable object similar to a :obj:`asyncio.Future`. This can be used to `await` the operation
from within a coroutine as well as cancelling it.

.. code-block:: python

    loop = policy.get_event_loop()

    async def list_files():
        f = Gio.file_new_for_path("/")
        for info in await f.enumerate_children_async("standard::*", 0, GLib.PRIORITY_DEFAULT):
            print(info.get_display_name())

    task = loop.create_task(list_files())


Example
~~~~~~~

A download window with asynchronous feedback.

.. literalinclude:: download_asyncio.py
   :language: python


Points of attention
~~~~~~~~~~~~~~~~~~~

 * async tasks run with :obj:`GLib.PRIORITY_DEFAULT`. Background work is often done with a lower priority.
   E.g. when running async routines your GTK GUI may not update.
 * The preferred way to start an application is by using :obj:`Gio.Application` or :obj:`Gtk.Application`.
   :obj:`asyncio.run` will not work.

Asynchronous Programming with callbacks
---------------------------------------

The following example shows how to download a web page and display the 
source in a text field. In addition it's possible to abort the running 
operation.

.. literalinclude:: download_callback.py
   :language: python

The example uses the asynchronous version of :meth:`Gio.File.load_contents` to
load the content of an URI pointing to a web page, but first we look at the
simpler blocking alternative.


We create a :class:`Gio.File` instance for our URI and call
:meth:`Gio.File.load_contents`, which, if it doesn't raise an error, returns
the content of the web page we wanted.

.. code:: python

    file = Gio.File.new_for_uri("https://developer.gnome.org/documentation/tutorials/beginners.html")
    try:
        status, contents, etag_out = file.load_contents(None)
    except GLib.GError:
        print("Error!")
    else:
        print(contents)

In the asynchronous variant we need two more things:

* A :class:`Gio.Cancellable`, which we can use during the operation to 
  abort or cancel it.
* And a :func:`Gio.AsyncReadyCallback` callback function, which gets called
  once the operation is finished and we can collect the result.

The window contains two buttons for which we register ``clicked`` signal
handlers:

* The ``on_start_clicked()`` signal handler calls 
  :meth:`Gio.File.load_contents_async` with a :class:`Gio.Cancellable` 
  and ``on_ready_callback()`` as :func:`Gio.AsyncReadyCallback`.
* The ``on_cancel_clicked()`` signal handler calls 
  :meth:`Gio.Cancellable.cancel` to cancel the running operation.

Once the operation is finished, either because the result is available, an
error occurred or the operation was canceled, ``on_ready_callback()`` will be
called with the :class:`Gio.File` instance and a :class:`Gio.AsyncResult`
instance which holds the result.

To get the result we now have to call :meth:`Gio.File.load_contents_finish` 
which returns the same things as :meth:`Gio.File.load_contents` except in 
this case the result is already there and it will return immediately 
without blocking.

After all this is done we call :meth:`Gio.Cancellable.reset` so the 
:class:`Gio.Cancellable` can be re-used for new operations and we can click 
the "Load" button again. This works since we made sure that only one 
operation can be active at any time by deactivating the "Load" button using 
:meth:`Gtk.Widget.set_sensitive`.

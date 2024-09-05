.. currentmodule:: gi.repository

Asynchronous Programming (asyncio)
==================================

.. attention::
    Async support for PyGObject is **experimental**.
    
    You can use the async integration (we encourage you to try!), but the API is not guaranteed stable;
    it might change if we find problems.

Since PyGObject 3.50.0, PyGObject has integration with the :mod:`asyncio` module. There are two parts
to this integration. First, PyGObject provides an :mod:`asyncio` event loop so that Python code using
:mod:`asyncio` will run normally. The second part to the integration is that Gio asynchronous functions
can return an awaitable object, which allows using ``await`` on them within a Python coroutine.

Event Loop integration
----------------------

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
--------------------------------------

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


Examples
--------

TODO: Async load a file.

A GTK window with a counter.

.. literalinclude:: async_counter.py
   :language: python


Points of attention
-------------------

 * async tasks run with :obj:`GLib.PRIORITY_DEFAULT`. Background work is often done with a lower priority.
   E.g. when running async routines your GTK GUI may not update.
 * The preferred way to start an application is by using :obj:`Gio.Application` or :obj:`Gtk.Application`.
   :obj:`asyncio.run` will not work.

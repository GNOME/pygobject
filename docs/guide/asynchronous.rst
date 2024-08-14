.. currentmodule:: gi.repository

Asynchronous programming (asyncio)
==================================

.. attention::
    Async support for PyGObject is **experimental**.
    
    You can use the async integration (we encourage you to try!), but the API is not guaranteed stable;
    it might change if we find problems.

Since PyGObject 3.50.0, PyGObject has support for asynchronous programming using ``async`` and ``await``.

In Python this is implemented by the :mod:`asyncio` module. PyGObject can make the asyncio event loop
part of GLib's main loop.

To integrate asyncio in GLib

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
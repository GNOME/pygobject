===============
Multi Threading
===============

Operations which could potentially block should not be executed in the main
loop. The main loop is in charge of input processing and drawing and
blocking it results in the user interface freezing. For the user this means
not getting any feedback and not being able to pause or abort the operation
which causes the problem.

Such an operation might be:

* Loading external resources like an image file on the web
* Searching the local file system
* Writing, reading and copying files
* Calculations where the runtime depends on some external factor

The following examples show

* how Python threads, running in parallel to GTK, can interact with the UI
* how to use and control asynchronous I/O operations in glib

This page will discuss multi-threading. A separate page discusses :doc:asynchronous.

This example uses a Python thread to execute code in the background
while still showing feedback on the progress in a window.

.. code:: python

    import threading
    import time
    import gi

    gi.require_version('Gtk', '4.0')
    from gi.repository import GLib, Gtk, GObject


    class Application(Gtk.Application):

        def do_activate(self):
            window = Gtk.ApplicationWindow(application=self)
            self.progress = Gtk.ProgressBar(show_text=True)

            window.set_child(self.progress)
            window.present()

            thread = threading.Thread(target=self.example_target)
            thread.daemon = True
            thread.start()

        def update_progress(self, i):
            self.progress.pulse()
            self.progress.set_text(str(i))
            return False

        def example_target(self):
            for i in range(50):
                GLib.idle_add(self.update_progress, i)
                time.sleep(0.2)


    app = Application()
    app.run()


The example shows a simple window containing a progress bar. After everything
is set up it constructs a Python thread, passes it a function to execute,
starts the thread and the GTK main loop. After the main loop is started it is
possible to see the window and interact with it.

In the background ``example_target()`` gets executed and calls
:func:`GLib.idle_add` and :func:`time.sleep` in a loop. In this example
:func:`time.sleep` represents the blocking operation. :func:`GLib.idle_add`
takes the ``update_progress()`` function and arguments that will get passed to
the function and asks the main loop to schedule its execution in the main
thread. This is needed because GTK isn't thread safe; only one thread, the
main thread, is allowed to call GTK code at all times.


Threads: FAQ
------------

* I'm porting code from pygtk (GTK 2) to PyGObject (GTK 3). Has anything
  changed regarding threads?

  Short answer: No.

  Long answer: ``gtk.gdk.threads_init()``, ``gtk.gdk.threads_enter()`` and
  ``gtk.gdk.threads_leave()`` are now :func:`Gdk.threads_init`,
  :func:`Gdk.threads_enter` and :func:`Gdk.threads_leave`.
  ``gobject.threads_init()`` can be removed.

* I'm using :func:`Gdk.threads_init` and want to get rid of it. What do I
  need to do?

  * Remove any :func:`Gdk.threads_init()`, :func:`Gdk.threads_enter` and
    :func:`Gdk.threads_leave` calls. In case they get executed in a thread,
    move the GTK code into its own function and schedule it using
    :func:`GLib.idle_add`. Be aware that the newly created function will be
    executed some time later, so other stuff can happen in between.

  * Replace any call to ``Gdk.threads_add_*()`` with their GLib counterpart.
    For example :func:`GLib.idle_add` instead of :func:`Gdk.threads_add_idle`.

* What about signals and threads?

  Signals get executed in the context they are emitted from. In which context
  the object is created or where ``connect()`` is called from doesn't matter.
  In GStreamer, for example, some signals can be called from a different
  thread, see the respective signal documentation for when this is the case.
  In case you connect to such a signal you have to make sure to not call any
  GTK code or use :func:`GLib.idle_add` accordingly.

* What if I need to call GTK code in signal handlers emitted from a thread?

  In case you have a signal that is emitted from another thread and you need
  to call GTK code during and not after signal handling, you can push the
  operation with an :class:`threading.Event` object to the main loop and wait
  in the signal handler until the operation gets scheduled and the result is
  available. Be aware that if the signal is emitted from the main loop this
  will deadlock. See the following example

  .. code:: python

        # [...]

        toggle_button = Gtk.ToggleButton()

        def signal_handler_in_thread():

            def function_calling_gtk(event, result):
                result.append(toggle_button.get_active())
                event.set()

            event = threading.Event()
            result = []
            GLib.idle_add(function_calling_gtk, event, result)
            event.wait()
            toggle_button_is_active = result[0]
            print(toggle_button_is_active)

        # [...]

* What about the Python `GIL
  <https://en.wikipedia.org/wiki/Global_Interpreter_Lock>`__ ?

  Similar to I/O operations in Python, all PyGObject calls release the
  GIL during their execution and other Python threads can be executed
  during that time.

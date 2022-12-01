=====================
Threads & Concurrency
=====================

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


Threads
-------

The first example uses a Python thread to execute code in the background 
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

        def update_progess(self, i):
            self.progress.pulse()
            self.progress.set_text(str(i))
            return False

        def example_target(self):
            for i in range(50):
                GLib.idle_add(self.update_progess, i)
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
takes the ``update_progess()`` function and arguments that will get passed to
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


Asynchronous Operations
-----------------------

In addition to functions for blocking I/O glib also provides corresponding
asynchronous versions, usually with the same name plus a ``_async`` suffix.
These functions do the same operation as the synchronous ones but don't block
during their execution. Instead of blocking they execute the operation in the
background and call a callback once the operation is finished or got canceled.

The following example shows how to download a web page and display the 
source in a text field. In addition it's possible to abort the running 
operation.


.. code:: python

    import time
    import gi

    gi.require_version('Gtk', '4.0')
    from gi.repository import Gio, GLib, Gtk


    class DownloadWindow(Gtk.ApplicationWindow):

        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs, default_width=500, default_height=400,
                             title="Async I/O Example")

            self.cancellable = Gio.Cancellable()

            self.cancel_button = Gtk.Button(label="Cancel")
            self.cancel_button.connect("clicked", self.on_cancel_clicked)
            self.cancel_button.set_sensitive(False)

            self.start_button = Gtk.Button(label="Load")
            self.start_button.connect("clicked", self.on_start_clicked)

            textview = Gtk.TextView(vexpand=True)
            self.textbuffer = textview.get_buffer()
            scrolled = Gtk.ScrolledWindow()
            scrolled.set_child(textview)

            box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6,
                          margin_start=12, margin_end=12, margin_top=12, margin_bottom=12)
            box.append(self.start_button)
            box.append(self.cancel_button)
            box.append(scrolled)

            self.set_child(box)

        def append_text(self, text):
            iter_ = self.textbuffer.get_end_iter()
            self.textbuffer.insert(iter_, f"[{time.time()}] {text}\n")

        def on_start_clicked(self, button):
            button.set_sensitive(False)
            self.cancel_button.set_sensitive(True)
            self.append_text("Start clicked...")

            file_ = Gio.File.new_for_uri(
                "http://python-gtk-3-tutorial.readthedocs.org/")
            file_.load_contents_async(
                self.cancellable, self.on_ready_callback, None)

        def on_cancel_clicked(self, button):
            self.append_text("Cancel clicked...")
            self.cancellable.cancel()

        def on_ready_callback(self, source_object, result, user_data):
            try:
                succes, content, etag = source_object.load_contents_finish(result)
            except GLib.GError as e:
                self.append_text(f"Error: {e.message}")
            else:
                content_text = content[:100].decode("utf-8")
                self.append_text(f"Got content: {content_text}...")
            finally:
                self.cancellable.reset()
                self.cancel_button.set_sensitive(False)
                self.start_button.set_sensitive(True)


    class Application(Gtk.Application):

        def do_activate(self):
            window = DownloadWindow(application=self)
            window.present()


    app = Application()
    app.run()


The example uses the asynchronous version of :meth:`Gio.File.load_contents` to
load the content of an URI pointing to a web page, but first we look at the
simpler blocking alternative:

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

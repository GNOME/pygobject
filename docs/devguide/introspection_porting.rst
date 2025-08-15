============================
Introspection Porting Guide
============================

.. contents::
   :local:

Beginning
=========

Starting with PyGObject 3.1.92 there's a module called pygtkcompat which will make it easier to
port an application from PyGTK.

There are two different approaches to porting your application from PyGTK to gobject-introspection:

1. Using a PyGTK compatibility layer (pygtkcompat)
2. Using a shell script which converts the callsites (``pygi-convert.sh``)

There are advantages and disadvantages with both:

1. Using pygtkcompat makes it possible to focus on porting to Gtk 3.x while not changing anything
   else, users and developers can keep on using the software as before.
2. Using pygtkcompat allows you to support both Gtk 2.x and Gtk 3.x with the same code base
3. Using ``pygi-convert.sh`` you can avoid using any layers and use the supported API directly

Using pygtkcompat
-----------------

The pygtkcompat module provides a PyGTK compatible API on top of gobject-introspection which makes
it possible to run your application on top of **both** PyGTK and gobject-introspection at the same
time.

Before using it you should port your application to using latest API, available 2.24, for instance::

    widget.window should be widget.get_window()
    container.child should be container.get_child()
    widget.flags() & gtk.REALIZED should be container.get_realized()

and so on.

Once an application has been updated to the latest PyGTK API, you can then import pygtkcompat and
enable the parts you need. For instance, to enable PyGTK compatible API on top of the Gtk 3.0
typelib, use the following:

.. code-block:: python

    from gi import pygtkcompat

    pygtkcompat.enable()
    pygtkcompat.enable_gtk(version='3.0')

That's it, if you're lucky enough and you're not using any strange/weird apis you should be able
to run your application. If you want an app to be compatible with both PyGTK and PyGI, you can use
the following technique:

.. code-block:: python

    try:
        from gi import pygtkcompat
    except ImportError:
        pygtkcompat = None

    if pygtkcompat is not None:
        pygtkcompat.enable()
        pygtkcompat.enable_gtk(version='3.0')

    import gtk

.. note::

   Porting the application from 2 to 3 is best covered in
   https://docs.gtk.org/gtk3/migrating-2to3.html. That usually includes things
   such as change expose-event to draw and do drawing with cairo.

How does PyGI work?
===================

Initial GI support was added to pygobject in version 2.19.0 (August 2009), but the entire
GI/pygobject/annotations stack really only stabilized in version 2.28, so that in practice you
will need at least this version and the corresponding latest upstream releases of GTK and other
libraries you want to use.

pygobject provides a ``gi.repository`` module namespace which generates virtual Python modules
from installed typelibs on the fly. For example, if you have the GIR for GTK 3 installed, you
can do:

.. code-block:: console

    $ python -c 'from gi.repository import Gtk; print Gtk'
    <gi.module.DynamicModule 'Gtk' from '/usr/lib/girepository-1.0/Gtk-3.0.typelib'>

and use it just like any other Python module.

Absolutely unexpected first example:

.. code-block:: console

    $ python -c 'from gi.repository import Gtk; Gtk.MessageDialog(None, 0, Gtk.MessageType.INFO, Gtk.ButtonsType.CLOSE, "Hello World").run()'

Let's look at the corresponding C declaration:

.. code-block:: c

    GtkWidget* gtk_message_dialog_new (GtkWindow *parent, GtkDialogFlags flags, GtkMessageType type, GtkButtonsType buttons, const gchar *message_format, ...);

and the C call:

.. code-block:: c

    GtkMessageDialog* msg = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Hello World");
    msg.run()

So what do we see here?

1. The C API by and large remains valid in Python (and other languages using the GI bindings),
   in particular the structure, order, and data types of arguments. There are a few exceptions
   which are mostly due to the different way Python works, and in some cases to make it easier to
   write code in Python; see below for details. But this means that you can (and should) use the
   normal API documentation for the C API of the library. Devhelp is your friend!

2. As Python is a proper object oriented language, pygobject (and in fact the GI typelib already)
   expose a GObject API as proper classes, objects, methods, and attributes. I. e. in Python you
   write

   .. code-block:: python

       button = Gtk.Button()
       button.set_label("foo")

   instead of the C gobject syntax

   .. code-block:: c

       GtkWidget* button = gtk_button_new(...);
       gtk_button_set_label(button, "foo");

   The class names in the typelib (and thus in Python) are derived from the actual class names
   stated in the C library (like ``GtkButton``), except that the common namespace prefix (``Gtk``
   here) is stripped, as it becomes the name of the module.

3. Global constants would be a heavy namespace clutter in Python, and thus pygobject exposes them
   in a namespaced fashion as well. I. e. if the ``MessageDialog`` constructor expects a constant
   of type ``GtkMessageType``, then by above namespace split this becomes a Python class
   ``Gtk.MessageType`` with the individual constants as attributes, e. g. ``Gtk.MessageType.INFO``.

4. Data types are converted in a rather obvious fashion. E. g. when the C API expects an ``int*``
   array pointer, you can supply a normal Python array ``[0, 1, 2]``. A Python string ``"foo"``
   will match a ``gchar*``, Pythons ``None`` matches ``NULL``, etc. So the GObject API actually
   translates quite naturally into a real OO language like Python, and after some time of getting
   used to above transformation rules, you should have no trouble translating the C API
   documentation into their Python equivalents. When in doubt, you can always look for the precise
   names, data types, etc. in the .gir instead, which shows the API broken by class, method, enum,
   etc, with the exact names and namespaces as they are exposed in Python.

As I mentioned above, this is in no way restricted to GTK, GNOME, or UI. For example, if you
handle any kind of hardware and hotplugging, you almost certainly want to query udev, which
provides a nice glib integration (with signals) through the gudev library. This example lists all
block devices (i. e. hard drives, USB sticks, etc.):

.. code-block:: pycon

    >>> from gi.repository import GUdev
    >>> c = GUdev.Client()
    >>> for dev in c.query_by_subsystem("block"):
    ...     print dev.get_device_file()
    ...
    /dev/sda
    /dev/sda1
    /dev/sda2
    [...]

See `the GUDevClient documentation
<http://www.kernel.org/pub/linux/utils/kernel/hotplug/gudev/GUdevClient.html#g-udev-client-query-by-subsystem>`_
for the corresponding C API. GI is not even restricted to GObject, you can annotate any non-OO
function based API with it. E. g. there is already a ``/usr/share/gir-1.0/xlib-2.0.gir``
(although it's horribly incomplete). These will behave as normal functions in Python (or other
languages) as well.

Differences to the C API
========================

The structure of method arguments is by and large the same in C and in GI/Python. There are some
notable exceptions which you must be aware of:

Constructors
------------

The biggest one is constructors. There is actually two ways of calling one:

* Use the real constructor implementation from the library. Unlike in normal Python you need to
  explicitly specify the constructor name:

  .. code-block:: python

      Gtk.Button.new()
      Gtk.Button.new_with_label("foo")

* Use the standard GObject constructor and pass in the initial property values as named arguments:

  .. code-block:: python

      Gtk.Button(label="foo", use_underline=True)

The second is actually the recommended one, as it makes the meaning of the arguments more explicit,
and also underlines the GObject best practice that a constructor should do nothing more than to
initialize properties. But otherwise it's pretty much a matter of taste which one you use.

Passing arrays
--------------

Unlike C, higher level languages know how long an array is, while in the C API you need to specify
that explicitly, either by terminating them with ``NULL`` or explicitly giving the length of the
array in a separate argument. Which one is used is already specified in the annotations and thus
in the typelib, so Python can automatically provide the right format without the developer needing
to append an extra ``None`` or a separate len(my_array) argument.

For example, in C you have

.. code-block:: c

    gtk_icon_theme_set_search_path (GtkIconTheme *icon_theme, const gchar *path[], gint n_elements)

In Python you can just call this as

.. code-block:: python

    my_icon_theme.set_search_path(['/foo', '/bar'])

and don't need to worry about the array size.

Output arguments
----------------

C functions can't return more than one argument, so they often use pointers which the function
then fills out. Conversely, Python doesn't know about pointers, but can easily return more than
one value as a tuple. The annotations already describe which arguments are "out" arguments, so in
Python they become part of the return tuple: first one is the "real" return value, and then all
out arguments in the same order as they appear in the declaration. For example:

.. code-block:: c

    GdkWindow* gdk_window_get_pointer (GdkWindow *window, gint *x, gint *y, GdkModifierType *mask)

In Python you would call this like

.. code-block:: python

    x, y, mask = mywindow.get_pointer()

The Python help system shows the correct input, output and return argument expectations:

.. code-block:: pycon

    >>> from gi.repository import Gdk
    >>> help(Gdk.Window.get_pointer)
    Help on function get_pointer:

    get_pointer(*args, **kwargs)
        get_pointer(self) -> x:int, y:int, mask:Gdk.ModifierType

GDestroyNotify
--------------

Some GLib/GTK functions take a callback method and an extra ``user_data`` argument that is passed
to the callback. In C they often also take a ``GDestroyNotify`` function which is run once all
callbacks are done, in order to free the memory of user_data. As Python has automatic memory
management, pygobject will take care of all this by itself, so you simply don't specify the
GDestroyNotify argument. For example:

.. code-block:: c

    void gtk_enumerate_printers (GtkPrinterFunc func, gpointer user_data, GDestroyNotify destroy, gboolean wait)

In Python you call this as

.. code-block:: python

    Gtk.enumerate_printers(my_callback, my_user_data, True)

Non-introspectable functions/methods
------------------------------------

When you work with PyGI for a longer time, you'll inevitably stumble over a method that simply
doesn't exist in the bindings. These usually are marked with ``introspectable="0"`` in the GIR.

In the best case this is because there are some missing annotations in the library which don't
have a safe default, so GI disables these to prevent crashes. They usually come along with a
corresponding warning message from g-ir-scanner, and it's usually quite easy to fix these.

Another common case are functions which take a variable number of arguments, such as
``gtk_cell_area_add_with_properties()``. Varargs cannot be handled safely by libgirepository. In
these cases there are often alternatives available (such as ``gtk_cell_area_cell_set_property()``).
For other cases libraries now often have a ``..._v()`` counterpart which takes a list instead of
variable arguments.

Threads
=======

For using threads with PyGObject, please see: `Threads/Concurrency with Python and the GNOME
Platform <./Projects(2f)PyGObject(2f)Threading.html>`_

Overrides
=========

A specialty of pygobject is the possibility of replacing functions, methods, or classes of the
introspected library with custom code, called "overrides". As the goal is to stay very close to
the original API, they should be used and written sparsely, though. One major use case is to
provide replacements for unintrospectable methods. For example, ``Gtk.Menu.popup()`` is not
introspectable in GTK, but the GTK override implements this method in terms of
``Gtk.Menu.popup_for_device()``, so in this case the override actually helps to get closer to the
original API again. Another important case is automatic data type conversion, most prominently to
allow passing unicode objects to methods which expect an UTF-8 encoded ``gchar*``. This also
actually helps to prevent workarounds in application code and maintain a clean API. Thirdly,
overrides are useful if they help to massively ease development. For example it is quite laborious
to do GDBus calls or GVariant constructions with the native Gio/GLib API. pygobject offers
convenience overrides to make these a lot simpler and more Pythonic, but of course without
actually breaking the original API.

Overrides should be quite easy to understand. In general you should not even be required to know
about it, as most of them really just fix stuff to work as expected. :-)

Porting from PyGTK 2 to PyGI GTK 3
===================================

Note that this is really two migrations in one step, but is recommended as GTK2 still has a lot of
breakage with PyGI. It is recommended to port applications to PyGI/GTK+ 3 first, then port to
Python 3 as an additional step if Python 3 support is desired. Otherwise you can end up with a
conflated and hard to track set of changes. See the `GTK2 → GTK3 migration documentation
<https://docs.gtk.org/gtk3/migrating-2to3.html>`_.

If we compare the PyGTK vs. PyGI code for a "Hello" message box, we see that it's actually very
similar in structure:

.. code-block:: console

    $ python -c 'import gtk; gtk.MessageDialog(None, 0, gtk.MESSAGE_INFO, gtk.BUTTONS_CLOSE, "Hello World").run()'

vs.

.. code-block:: console

    $ python -c 'from gi.repository import Gtk; Gtk.MessageDialog(None, 0, Gtk.MessageType.INFO, Gtk.ButtonsType.CLOSE, "Hello World").run()'

So PyGTK also does the representation of the C functions as proper classes and methods, thus if
you port from PyGTK to PyGI, the structure by and large remains the same.

Step 1: The Great Renaming
--------------------------

The biggest part in terms of volume of code changed is basically just a renaming exercise. E. g.
``gtk.*`` now becomes ``Gtk.*``, and ``gtk.MESSAGE_INFO`` becomes ``Gtk.MessageType.INFO``.
Likewise, the imports need to be updated: ``import gtk`` becomes ``from gi.repository import Gtk``.

Fortunately this is is a mechanical task which can be automated. The `pygobject git tree
<https://gitlab.gnome.org/GNOME/pygobject>`_ has a script `pygi-convert.sh
<https://gitlab.gnome.org/GNOME/pygobject/blob/master/tools/pygi-convert.sh>`_ which is a long
list of perl -pe 's/old/new/' string replacements.

It's really blunt, but surprisingly effective, and for small applications chances are that it will
already produce something which actually runs. Note that this script is in no way finished, and
should be considered a collaborative effort amongst porters. So if you have something which should
be added there, please don't hesitate to open a bug or ping on IRC (#python on irc.gnome.org)).
We will be happy to improve the script.

When you just run ``pygi-convert.sh`` in your project tree, it will work on all ``*.py`` files.
If you have other Python code there which is named differently (such as ``bin/myprogram``), you
should run it once more with all these file names as argument.

* Make sure you don't keep using the static bindings for a library that we are using through
  introspection. That would cause wrappers from both bindings be mixed and compatibility issues
  would arise.

Step 2: Wash, rinse, repeat
---------------------------

Once the mechanical renamings are out of the way, the tedious and laborious part starts. As Python
does not have a concept of "compile-time check" and can't even check that called methods exist or
that you pass the right number of parameters, you now have to enter a loop of "start your
program", "click around until it breaks", "fix it", "goto 1".

The necessary changes here are really hard to generalize, as they highly depend on what your
program actually does, and this will also involve the GTK 2 → 3 parts. One thing that comes up a
lot are ``pack_start()``/``pack_end()`` calls. In PyGTK they have default values for ``expand``,
``start``, and ``padding`` attributes, but as GTK does not have them, you won't have them in PyGI
either (see `bgo#558620 - Add default values
<https://bugzilla.gnome.org/show_bug.cgi?id=558620>`_).

.. warning::

   Note that you can't do a migration halfway: If you try to import both ``gtk`` and
   ``gi.repository.Gtk``, you'll get nothing but program hangs and crashes, as you are trying to
   work with the same library in two different ways. You can mix static and GI bindings of
   *different* libraries though, such as ``dbus-python`` and ``gi.repository.Gtk``.

If your application uses plugins, you can use libpeas. It is a GObject plugins library that
support C, Python and Javascript languages though introspection.

Step 3: Packaging changes
-------------------------

After you have your code running with PyGI and committed it to your branch and released it, you
need to update the dependencies of your distro package for PyGI. You should grep your code for
"gi.repository" and collect a list of all imported typelibs, and then translate them into the
appropriate package name. For example, if you import "Gtk, Notify, Gudev" you need to add
dependencies to the packages which ship them:

* Debian/Ubuntu ship them in separate packages named ``gir<GI_ABI_version>-<libraryname>-<library_ABI_version>``,
  so in this example ``gir1.2-gtk-3.0``, ``gir1.2-notify-0.7``, and ``gir1.2-gudev-1.0``. You
  can find out with e. g. ``dpkg -S /usr/lib/girepository-1.0/Gtk-3.0.typelib``.
* Fedora ships the typelibs together with the shared libraries, so in this example ``gtk3``,
  ``libgudev1``, ``libnotify``. You can find out with e. g.
  ``rpm -qf /usr/lib/girepository-1.0/Gtk-3.0.typelib``.

At the same time you should drop the old static bindings, like python-gtk2, python-notify, etc.

Finally you should also bump the version of the pygobject dependency to (>= 2.28) to ensure that
you run with a reasonably bug free PyGI.

Examples
========

* pygobject's git tree has a very comprehensive `gtk-demo
  <https://gitlab.gnome.org/GNOME/pygobject/tree/master/examples/demo>`_ showing off pretty much
  all available GTK widgets in PyGI
* Examples of previously done pygtk → pyGI ports:

  * Apport: http://bazaar.launchpad.net/~apport-hackers/apport/trunk/revision/1801
  * Jockey: http://bazaar.launchpad.net/~jockey-hackers/jockey/trunk/revision/679
  * system-config-printer: https://git.fedorahosted.org/cgit/system-config-printer.git/log/?h=pygi
  * gtimelog: http://bazaar.launchpad.net/~gtimelog-dev/gtimelog/trunk/revision/181 (this is
    interesting because it makes the code work with *both* PyGTK and PyGI, whichever is available)

Comments
========

* What versions of the python packages do we need, and how do we get the for different
  distributions (gentoo, deb-based, rpm-based...?)
* Can we just fallback to import gtk, gdk.... if we detect old enough python packages, or do we
  need to perform other workarounds?

One of the biggest challenges in porting is that all of the constants have changed names. The
above ``pygi-convert.sh`` script gives a good idea about what the new names are, but if you have
any problems, refer to the relevant .gir file directly. For example, if your old code says
"gtk.TREE_VIEW_COLUMN_AUTOSIZE" and you're not sure what the new code should be, search in
/usr/share/gir-1.0/Gtk-2.0.gir for "COLUMN_AUTOSIZE", and you'll find some code that looks like
this:

.. code-block:: xml

        <enumeration name="TreeViewColumnSizing"
                     glib:type-name="GtkTreeViewColumnSizing"
                     glib:get-type="gtk_tree_view_column_sizing_get_type"
                     c:type="GtkTreeViewColumnSizing">
          <member name="grow_only"
                  value="0"
                  c:identifier="GTK_TREE_VIEW_COLUMN_GROW_ONLY"
                  glib:nick="grow-only"/>
          <member name="autosize"
                  value="1"
                  c:identifier="GTK_TREE_VIEW_COLUMN_AUTOSIZE"
                  glib:nick="autosize"/>
          <member name="fixed"
                  value="2"
                  c:identifier="GTK_TREE_VIEW_COLUMN_FIXED"
                  glib:nick="fixed"/>
        </enumeration>

Looking at that, you can determine the new constant name. It starts with "Gtk", then a period,
then the enumeration name ("TreeViewColumnSizing"), then another period, then the member name in
ALL CAPS ("AUTOSIZE"). So, this tells you to change "gtk.TREE_VIEW_COLUMN_AUTOSIZE" into
"Gtk.TreeViewColumnSizing.AUTOSIZE"

Problems
========

* Listening to signals: if you are listening a signal like "size-allocate" and when you print the
  allocation you get as the type GdkRectangle instead of CairoRectangleInt, the way to fix it is
  by overriding the signal instead of listening to it.

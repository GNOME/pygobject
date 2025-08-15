========================================
Analysis of Classes without Constructors
========================================

There are a number of classes in the GNOME APIs that are not intended for direct creation. The
issue is there is not an accurate way to generically determine this. GI bindings are general and
expose everything they can, this makes a number of classes available which crash when an attempt
is made to instantiate them. However, we can look at classes which do not explicitly implement
constructors as a hint. The following script and table attempts some analysis as to what classes
do not have constructors and why.

Script Used to Generate Initial Table
======================================

This script iterates a number of Python GI imported namespaces and looks for GObject.Object
based classes which do not provide a public constructor.

.. code-block:: python

    import inspect
    import gi
    from gi.repository import GObject, Gtk, Gdk, GdkPixbuf, Gio

    repo = gi.gi.Repository.get_default()
    modules = dict(GObject=GObject,
                   Gtk=Gtk, Gdk=Gdk,
                   GdkPixbuf=GdkPixbuf,
                   Gio=Gio)

    print("||'''Name'''||'''Bug'''||'''Alternative'''||'''Reason'''||")
    for modname, mod in sorted(modules.items()):
        for name in dir(mod):
            obj = getattr(mod, name)
            if not inspect.isclass(obj) or not issubclass(obj, GObject.Object):
                continue
            info = repo.find_by_name(modname, name)
            if isinstance(info, (gi.types.ObjectInfo, )) and not info.get_abstract():
                if not any(meth.is_constructor() for meth in info.get_methods()):
                    print('||`%s.%s` ||  ||  ||  ||' % (modname, name))

Classes without Constructors
=============================

====================================  ================================  ===============================================  ============================================================================================================
**Name**                              **Bug**                           **Alternative**                                  **Reason**
====================================  ================================  ===============================================  ============================================================================================================
``GObject.Binding``                   `675581`_                         GObject.Object.bind_property(...)                Non-generic semantics, requires construct only arguments.
``GObject.InitiallyUnowned``                                                                                             [1]
``Gdk.Display``                                                         Gdk.Display.get_default()                        Singleton
``Gdk.DisplayManager``                                                  Gdk.Display.get_default() or Gdk.Display.get()   Singleton
``Gdk.DragContext``                                                     Gdk.drag_begin()                                 [1]
``Gdk.Keymap``                                                          Gdk.Keymap.get_default()                         Singleton
``Gdk.Screen``                        `687792`_                         Screen.get_default()                             Singleton
``Gdk.Visual``                                                                                                           [1]
``GdkPixbuf.PixbufAnimationIter``                                                                                        [1]
``GdkPixbuf.PixbufSimpleAnimIter``                                                                                       [1]
``Gio.ApplicationCommandLine``                                                                                           [1] (Usually created by GApplication)
``Gio.DBusActionGroup``                                                 Gio.DBusActionGroup.get()                        [1]
``Gio.DBusMenuModel``                                                   Gio.DBusMenuModel.get()                          [1]
``Gio.DBusMethodInvocation``                                                                                             [1] Received as argument to the handle_method_call() function
``Gio.FileEnumerator``                                                  Gio.File.enumerate_children()                    [1]
``Gio.FileIOStream``                                                                                                     unknown
``Gio.FileInputStream``                                                                                                  unknown
``Gio.FileOutputStream``                                                                                                 unknown
``Gio.MemoryOutputStream``                                                                                               unknown
``Gio.ProxyAddressEnumerator``                                          Gio.SocketConnectable.enumerate()                [1]
``Gio.Resolver``                                                        Gio.Resolver                                     [1]
``Gio.SocketConnection``                                                                                                 unknown
``Gio.TcpConnection``                                                                                                    unknown
``Gio.TlsInteraction``                                                                                                   unknown
``Gio.UnixConnection``                                                                                                   unknown
``Gio.Vfs``                                                             Gio.Vfs.get_default() or Gio.Vfs.get_local()     Singleton
``Gio.VolumeMonitor``                 `647731`_                         Gio.VolumeMonitor.get()                          Singleton
``Gtk.AccelMap``                                                        Gtk.AccelMap.get()                               [1]
``Gtk.Accessible``                                                                                                       unknown
``Gtk.CellAreaContext``                                                 Gtk.CellAreaClass.create_context()               [1]
``Gtk.Clipboard``                                                       Gtk.Clipboard.get()                              [1]
``Gtk.FileChooserDialog``                                                                                                Generic creation works and is necessary. Constructor skipped do to variadic arguments.
``Gtk.MessageDialog``                                                                                                    Generic creation works and is necessary. Constructor skipped do to variadic arguments.
``Gtk.NumerableIcon``                                                                                                    Works. gtk_numerable_icon_new marked as function not constructor do to annotation problem?
``Gtk.PrintContext``                                                                                                     [1] Gtk.PrintContext objects gets passed to the ::begin-print, ::end-print, ::request-page-setup and ::draw-page signals on the Gtk.PrintOperation.
``Gtk.RecentChooserDialog``                                                                                              Generic creation works and is necessary. Constructor skipped do to variadic arguments.
``Gtk.Settings``                                                        Gtk.Settings.get_default()                       [1]
``Gtk.ThemingEngine``                                                                                                    [1]
``Gtk.Tooltip``                                                                                                          [1] Instance passed to "query-tooltip" signal handler
``Gtk.TreeModelFilter``                                                 Gtk.TreeModel.filter_new()                       [1]
``Xkl.Engine``                        `680202`_                         Xkl.Engine.get_instance(display)
====================================  ================================  ===============================================  ============================================================================================================

.. _675581: http://bugzilla.gnome.org/show_bug.cgi?id=675581
.. _687792: http://bugzilla.gnome.org/show_bug.cgi?id=687792
.. _647731: http://bugzilla.gnome.org/show_bug.cgi?id=647731
.. _680202: http://bugzilla.gnome.org/show_bug.cgi?id=680202

[1] - Generic creation works but the class might not be necessary in Python.

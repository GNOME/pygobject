# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2018 Nikita Churaev <lamefun.x0r@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
# USA

import sys
from gi.repository import GLib
from gi.repository import GObject
from gi.repository import Gio
from gi.repository import Gtk

__all__ = ['Template', 'Child']

def _template_connect_func(builder, obj, signal_name, handler_name,
                           connect_object, flags, cls):
    self = builder.get_object(cls.__gtype__.name)
    extra = () if connect_object is None else (connect_object,)
    if not hasattr(self, handler_name):
        raise RuntimeError('widget %r is missing the signal handler %r' %
                           (cls.__name__, handler_name))
    handler = getattr(self, handler_name)
    if flags & GObject.ConnectFlags.AFTER:
        obj.connect_after(signal_name, handler, *extra)
    else:
        obj.connect(signal_name, handler, *extra)

class Child(object):
    """
    This class is used to mark class members as Gtk.Widget template children.
    See the documentation for the Template class for more information.
    """
    pass

class Template(object):
    """
    The Template decorator allows you to use Gtk.Builder UI files as templates
    for your custom widgets:

    .. code-block:: python

        from gi.templates import Template, Child

        @Template.from_file('window.ui')
        class Window(Gtk.Window):
            __gtype_name__ = 'MyWindow'

            label = Child()

            def __init__(self, **kwargs):
                super().__init__(**kwargs)
                self.init_template()

            def button_clicked(self, button):
                self.label.set_text('Thanks!')

    Widget templates can be created using the Glade interface designer similarly
    to normal UI files. Here are the steps you need to take to create a widget
    template:

    * Ensure that the ID you have given in Glade to the object that you intend
      to be the template for your custom widget is exactly the same as your
      widget's ``__gtype_name__``.

    * Ensure that the type of the template object is exactly the same as your
      custom widget's parent class.

    * If you are using Glade 3.22, click the document properties button, check
      the "Composite template toplevel" box and choose the template widget.

    * If you are using an earlier version, open the "File" menu and click the
      "Properties" item to open the document properties dialog.

    Note that the ``__gtype_name__`` must be unique across the *whole* program
    and must not clash with any GType names used by the GObject-based libraries
    that it uses (for example, Gtk.Widget uses the GType name "Gtk.Widget").

    Here is what a widget template XML file should look like:

    .. code-block:: xml

        <?xml version="1.0" encoding="UTF-8"?>
        <interface>
          <requires lib="gtk+" version="3.20"/>
          <template class="MyWindow" parent="GtkWindow">
            <property name="title">My Window</property>
            <child>
              <object class="GtkBox" id="box">
                <property name="visible">True</property>
                <property name="orientation">vertical</property>
                <child>
                  <object class="GtkLabel" id="label">
                    <property name="visible">True</property>
                    <property name="label">Click the button!</property>
                  </object>
                </child>
                <child>
                  <object class="GtkButton" id="button">
                    <property name="label">Button</property>
                    <property name="visible">True</property>
                    <signal name="clicked" handler="button_clicked"/>
                  </object>
                </child>
              </object>
            </child>
          </template>
        </interface>

    The template objects are exposed using the Child class:

    .. code-block:: python

        button = Child()

    The custom widget's constructor must call ``self.init_template()`` after it
    calls the parent class constructor to initialize the widget and expose the
    template children as attributes:

    .. code-block:: python

        def __init__(self, **kwargs):
            super().__init__(**kwargs)
            self.init_template()
            self.button.set_label('Click here!')

    The signals handlers that are specified in the UI file are connected
    automatically in the same way as when using Gtk.Builder.connect_signals().

    The Template decorator supports loading templates from multiple sources:

    * ``@Template.from_file('path/to/file.ui')`` loads a template from a file.

    * ``@Template.from_resource('/path/to/template.ui')`` loads a template from
      a GIO resource. Note that the resource must already be loaded *before* the
      widget class is defined.

    * ``@Template.from_string('<interface>[..]</interface>')`` loads a template
      from a string which contains the template XML.

    * ``@Template(bytes)`` loads a template from a bytes or a GLib.Bytes object
      which contains the template XML in UTF-8.
    """

    def __init__(self, data):
        """Creates a template from UTF-8 XML bytes or GLib.Bytes."""
        if isinstance(data, bytes):
            self.data = GLib.Bytes(bytes)
        elif isinstance(data, GLib.Bytes):
            self.data = data
        else:
            raise ValueError("'data' must be bytes or GLib.Bytes")

    @staticmethod
    def from_file(path):
        """Loads a template from a file."""
        with open(path, 'rb') as f:
            return Template(GLib.Bytes(f.read()))

    @staticmethod
    def from_resource(path):
        """Loads a template from a GIO resource."""
        data = Gio.resources_lookup_data(path, Gio.ResourceLookupFlags.NONE)
        return Template(data)

    @staticmethod
    def from_string(string):
        """Creates a template from an XML string."""
        if sys.version_info >= (3, 0) or isinstance(string, unicode):
            return Template(string.encode('utf-8'))
        else:
            return Template(string)

    def __call__(self, cls):
        """Applies the template for to a widget class."""

        if not issubclass(cls, Gtk.Widget):
            raise TypeError('class %r is not a subclass of '
                            'Gtk.Widget' % (cls.__name__,))

        if getattr(cls, '__template_class__', None) is cls:
            raise ValueError('class %r already has a widget '
                             'template' % (cls.__name__,))

        if not hasattr(cls, '__gtype_name__'):
            raise ValueError('class %r is missing a '
                             '__gtype_name__' % (cls.__name__,))

        cls.set_template(self.data)
        cls.set_connect_func(_template_connect_func, cls)

        cls.__template_class__ = cls
        cls.__template_children__ = set()

        for name in dir(cls):
            attr = getattr(cls, name, None)
            if isinstance(attr, Child):
                cls.__template_children__.add(name)
                cls.bind_template_child_full(name, True, 0)

        def init_template(self):
            super(type(self), self).init_template()
            for name in self.__template_children__:
                child = self.get_template_child(cls, name)
                if child is None:
                    raise RuntimeError('widget template for class %r is '
                                       'missing an object with ID %r' %
                                       (cls.__name__, name))
                setattr(self, name, child)

        cls.init_template = init_template

        return cls

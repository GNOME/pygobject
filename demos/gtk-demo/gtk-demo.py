#!/bin/env python
# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2010 Red Hat, Inc., John (J5) Palmieri <johnp@redhat.com>
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



import pygtk
pygtk.require('2.0')
from gi.repository import Gtk, GLib, GObject, GdkPixbuf, Gio, Pango
import os
import glob

_DEMOCODEDIR = os.getcwd()

class Demo(GObject.GObject):
    __gtype_name__ = 'GtkDemo'

    def __init__(self, title = None, module = None, filename = None, children = None):
        super(Demo, self).__init__()
        self.title = title
        self.module = module
        self.filename = filename
        if children is None:
            children = []

        self.children = children

        self.isdir = False
        if module is None:
            self.isdir = True

class GtkDemoApp(object):
    def _quit(self, *args):
        Gtk.main_quit()

    def __init__(self):
        super(GtkDemoApp, self).__init__()

        self._demos = []
        self.load_demos()

        self.setup_default_icon()

        window = Gtk.Window(type=Gtk.WindowType.TOPLEVEL)
        window.set_title('PyGI GTK+ Code Demos')

        window.connect_after('destroy', self._quit)

        hbox = Gtk.HBox(homogeneous = False, spacing = 0)
        window.add(hbox)

        tree = self.create_tree()
        hbox.pack_start(tree, False, False, 0)

        notebook = Gtk.Notebook()
        hbox.pack_start(notebook, True, True, 0)

        (text_widget, info_buffer) = self.create_text(True)
        notebook.append_page(text_widget, Gtk.Label.new_with_mnemonic('_Info'))
        self.info_buffer = info_buffer

        tag = info_buffer.create_tag ('title', font = 'Sans 18')

        (text_widget, source_buffer) = self.create_text(True)
        notebook.append_page(text_widget, Gtk.Label.new_with_mnemonic('_Source'))

        self.source_buffer = source_buffer;
        tag = source_buffer.create_tag ('comment', foreground = 'DodgerBlue')
        tag = source_buffer.create_tag ('type', foreground = 'ForestGreen')
        tag = source_buffer.create_tag ('string',
                                        foreground = 'RosyBrown',
                                        weight = Pango.Weight.BOLD)
        tag = source_buffer.create_tag ('control', foreground = 'purple')
        tag = source_buffer.create_tag ('preprocessor',
                                        style = Pango.Style.OBLIQUE,
                                        foreground = 'burlywood4')
        tag = source_buffer.create_tag ('function' ,
                                        weight = Pango.Weight.BOLD,
                                        foreground = 'DarkGoldenrod4')

        self.source_buffer = source_buffer
        window.set_default_size (600, 400)
        window.show_all()

        self.selection_cb(self.tree_view.get_selection(),
                          self.tree_view.get_model())
        Gtk.main()

    def load_demos_from_list(self, file_list, demo_list):
        for f in file_list:
            base_name = os.path.basename(f)
            if base_name == '__init__.py':
                continue

            demo = None
            if os.path.isdir(f):
                children = []
                self.load_demos(f, children)
                demo = Demo(base_name, None, f, children)
            else:
                scrub_ext = f[0:-3]
                split_path = scrub_ext.split(os.sep)
                module_name = split_path[-1]
                base_module_name = '.'.join(split_path[:-1])
                _temp = __import__(base_module_name, globals(), locals(), [module_name], -1)
                module = getattr(_temp, module_name)

                try:
                    demo = Demo(module.title, module, f)
                except AttributeError, e:
                    raise AttributeError('(%s): %s' % (f, e.message))

            demo_list.append(demo)

    def load_demos(self, top_dir='demos', demo_list=None):
        if demo_list is None:
            demo_list = self._demos

        demo_file_list = []
        for filename in os.listdir(top_dir):
            fullname = os.path.join(top_dir, filename)
            if os.path.isdir(fullname):
                # make sure this is a module directory
                init_file = os.path.join(fullname, '__init__.py')
                if os.path.isfile(init_file):
                    demo_file_list.append(fullname)
                    continue

            if filename.endswith('.py'):
                demo_file_list.append(fullname)

        demo_file_list.sort(lambda a, b: cmp(a.lower(), b.lower()))

        self.load_demos_from_list(demo_file_list, demo_list)

    def find_file(self, base=''):
        dir = os.path.join('demos', 'data')
        logo_file = os.path.join(dir, 'gtk-logo-rgb.gif')
        base_file = os.path.join(dir, base)

        if (GLib.file_test(logo_file, GLib.FileTest.EXISTS) and
             GLib.file_test(base_file, GLib.FileTest.EXISTS)):
            return base_file
        else:
            filename = os.path.join(_DEMOCODEDIR, base)
            if GLib.file_test(filename, GLib.FileTest.EXISTS):
                return filename

            # can't find the file
            raise IOError('Cannot find demo data file "%s"' % base)

    def setup_default_icon(self):
        filename = self.find_file ('gtk-logo-rgb.gif')
        pixbuf = GdkPixbuf.Pixbuf.new_from_file(filename)
        transparent = pixbuf.add_alpha(True, 0xff, 0xff, 0xff)
        list = []
        list.append(transparent)
        Gtk.Window.set_default_icon_list(list)

    def selection_cb(self, selection, model):
        (success, m, treeiter) = selection.get_selected()
        if not success:
            return

        demo = model.get_value(treeiter, 1)

        title = demo.title

        if demo.isdir:
            return

        description = demo.module.description
        code = GLib.file_get_contents(demo.filename)[1]

        # output and style the title
        (start, end) = self.info_buffer.get_bounds()
        self.info_buffer.delete(start, end)
        (start, end) = self.source_buffer.get_bounds()
        self.source_buffer.delete(start, end)

        start = self.info_buffer.get_iter_at_offset(0)
        end = start.copy()
        self.info_buffer.insert(end, title)
        start = end.copy()
        start.backward_chars(len(title))
        self.info_buffer.apply_tag_by_name('title', start, end)
        self.info_buffer.insert(end, '\n\n')

        # output the description
        self.info_buffer.insert(end, description)

        # output the code
        start = self.source_buffer.get_iter_at_offset(0)
        end = start.copy()
        self.source_buffer.insert(end, code)

    def row_activated_cb(self, view, path, col, store):
        (success, treeiter) = store.get_iter(path)
        demo = store.get_value(treeiter, 1)
        demo.module.main(self)

    def create_tree(self):
        tree_store = Gtk.TreeStore(str, Demo, Pango.Style)
        tree_view = Gtk.TreeView()
        self.tree_view = tree_view
        tree_view.set_model(tree_store)
        selection = tree_view.get_selection()
        selection.set_mode(Gtk.SelectionMode.BROWSE)
        tree_view.set_size_request(200, -1)

        for demo in self._demos:
            children = demo.children
            parent = tree_store.append(None,
                                       (demo.title,
                                        demo,
                                        Pango.Style.NORMAL))
            if children:
                for child_demo in children:
                    tree_store.append(parent,
                                      (child_demo.title,
                                       child_demo,
                                       Pango.Style.NORMAL))

        cell = Gtk.CellRendererText()
        column = Gtk.TreeViewColumn(title = 'Widget (double click for demo)',
                                    cell_renderer = cell,
                                    text = 0,
                                    style = 2)

        first_iter = tree_store.get_iter_first()
        if first_iter is not None:
            selection.select_iter(first_iter)

        selection.connect('changed', self.selection_cb, tree_store)
        tree_view.connect('row_activated', self.row_activated_cb, tree_store)

        tree_view.append_column(column)

        tree_view.collapse_all()
        tree_view.set_headers_visible(False)
        scrolled_window = Gtk.ScrolledWindow(hadjustment = None,
                                             vadjustment = None)
        scrolled_window.set_policy(Gtk.PolicyType.NEVER,
                                   Gtk.PolicyType.AUTOMATIC)

        scrolled_window.add(tree_view)

        label = Gtk.Label(label = 'Widget (double click for demo)')

        box = Gtk.Notebook()
        box.append_page(scrolled_window, label)

        tree_view.grab_focus()

        return box

    def create_text(self, is_source):
        scrolled_window = Gtk.ScrolledWindow(hadjustment = None,
                                             vadjustment = None)
        scrolled_window.set_policy(Gtk.PolicyType.AUTOMATIC,
                                   Gtk.PolicyType.AUTOMATIC)
        scrolled_window.set_shadow_type(Gtk.ShadowType.IN)

        text_view = Gtk.TextView()
        buffer = Gtk.TextBuffer()

        text_view.set_buffer(buffer)
        text_view.set_editable(False)
        text_view.set_cursor_visible(False)

        scrolled_window.add(text_view)

        if is_source:
            font_desc = Pango.FontDescription('monospace')
            text_view.modify_font(font_desc)
            text_view.set_wrap_mode(Gtk.WrapMode.NONE)
        else:
            text_view.set_wrap_mode(Gtk.WrapMode.WORD)
            text_view.set_pixels_above_lines(2)
            text_view.set_pixels_below_lines(2)

        return(scrolled_window, buffer)

if __name__ == '__main__':
    demo = GtkDemoApp()

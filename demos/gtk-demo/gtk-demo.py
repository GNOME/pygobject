#!/usr/bin/env python
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


import os
import sys
import glob

from gi.repository import GLib, GObject, Gio, Pango, GdkPixbuf, Gtk



DEMOROOTDIR = os.path.abspath(os.path.dirname(__file__))
DEMOCODEDIR = os.path.join(DEMOROOTDIR, 'demos')
sys.path.insert(0, DEMOROOTDIR)


class Demo(GObject.GObject):
    __gtype_name__ = 'GtkDemo'

    def __init__(self, title, module, filename):
        super(Demo, self).__init__()

        self.title = title
        self.module = module
        self.filename = filename

    @classmethod
    def new_from_file(cls, path):
        relpath = os.path.relpath(path, DEMOROOTDIR)
        packagename = os.path.dirname(relpath).replace(os.sep, '.')
        modulename = os.path.basename(relpath)[0:-3]

        package = __import__(packagename, globals(), locals(), [modulename], -1)
        module = getattr(package, modulename)

        try:
            return cls(module.title, module, path)
        except AttributeError as e:
            raise AttributeError('(%s): %s' % (path, e.message))


class DemoTreeStore(Gtk.TreeStore):
    __gtype_name__ = 'GtkDemoTreeStore'

    def __init__(self, *args):
        #TODO: super does not seem to work here?
        #super(Gtk.TreeStore, self).__init__(str, Demo, Pango.Style)
        Gtk.TreeStore.__init__(self, str, Demo, Pango.Style)

        self._parent_nodes = {}

        for filename in self._list_dir(DEMOCODEDIR):
            fullpath = os.path.join(DEMOCODEDIR, filename)
            initfile = os.path.join(os.path.dirname(fullpath), '__init__.py')

            if fullpath != initfile \
            and os.path.isfile(initfile) \
            and fullpath.endswith('.py'):
                parentname = os.path.dirname(os.path.relpath(fullpath, DEMOCODEDIR))

                if parentname:
                    parent = self._get_parent_node(parentname)
                else:
                    parent = None

                demo = Demo.new_from_file(fullpath)
                self.append(parent, (demo.title, demo, Pango.Style.NORMAL))

    def _list_dir(self, path):
        demo_file_list = []

        for filename in os.listdir(path):
            fullpath = os.path.join(path, filename)

            if os.path.isdir(fullpath):
                demo_file_list.extend(self._list_dir(fullpath))
            elif os.path.isfile(fullpath):
                demo_file_list.append(fullpath)

        return sorted(demo_file_list, key=str.lower)

    def _get_parent_node(self, name):
        if not name in self._parent_nodes.keys():
            node = self.append(None, (name, None, Pango.Style.NORMAL))
            self._parent_nodes[name] = node

        return self._parent_nodes[name]


class GtkDemoWindow(Gtk.Window):
    __gtype_name__ = 'GtkDemoWindow'

    def __init__(self):
        super(GtkDemoWindow, self).__init__(type=Gtk.WindowType.TOPLEVEL)

        self.set_title('PyGI GTK+ Code Demos')
        self.set_default_size (600, 400)
        self.setup_default_icon()

        hbox = Gtk.HBox(homogeneous=False, spacing=0)
        self.add(hbox)

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
        self.show_all()

        self.selection_cb(self.tree_view.get_selection(),
                          self.tree_view.get_model())

    def find_file(self, base=''):
        dir = os.path.join(DEMOCODEDIR, 'data')
        logo_file = os.path.join(dir, 'gtk-logo-rgb.gif')
        base_file = os.path.join(dir, base)

        if (GLib.file_test(logo_file, GLib.FileTest.EXISTS) and
             GLib.file_test(base_file, GLib.FileTest.EXISTS)):
            return base_file
        else:
            filename = os.path.join(DEMOCODEDIR, base)

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
        sel = selection.get_selected()
        if sel == ():
            return

        treeiter = sel[1]
        title = model.get_value(treeiter, 0)
        demo = model.get_value(treeiter, 1)

        if demo is None:
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
        treeiter = store.get_iter(path)
        demo = store.get_value(treeiter, 1)
        if demo is not None:
            demo.module.main(self)

    def create_tree(self):
        tree_store = DemoTreeStore()
        tree_view = Gtk.TreeView()
        self.tree_view = tree_view
        tree_view.set_model(tree_store)
        selection = tree_view.get_selection()
        selection.set_mode(Gtk.SelectionMode.BROWSE)
        tree_view.set_size_request(200, -1)

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


def main():
    mainloop = GLib.MainLoop()

    demowindow = GtkDemoWindow()
    demowindow.connect('delete-event', quit, mainloop)
    demowindow.show()

    mainloop.run()

def quit(widget, event, mainloop):
    mainloop.quit()


if __name__ == '__main__':
    main()

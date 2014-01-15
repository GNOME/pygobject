# -*- Mode: Python; py-indent-offset: 4 -*-
# pygobject - Python bindings for the GObject library
# Copyright (C) 2014 Simon Feltman
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
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

title = "Tree Model with Large Data"
description = """
Implementation of the Gtk.TreeModel interface to create a custom model.
The demo uses a fake data store (it is not backed by a Python list) and is for
the purpose of showing how to override the TreeModel interfaces virtual methods.
"""

from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gtk


class Model(GObject.Object, Gtk.TreeModel):
    columns_types = (str, str)
    item_count = 100000
    item_data = 'abcdefghijklmnopqrstuvwxyz'

    def __init__(self):
        super(Model, self).__init__()

    def do_get_flags(self):
        return Gtk.TreeModelFlags.LIST_ONLY

    def do_get_n_columns(self):
        return len(self.columns_types)

    def do_get_column_type(self, n):
        return self.columns_types[n]

    def do_get_iter(self, path):
        # Return False and an empty iter when out of range
        index = path.get_indices()[0]
        if index < 0 or index >= self.item_count:
            return False, None

        it = Gtk.TreeIter()
        it.user_data = index
        return True, it

    def do_get_path(self, it):
        return Gtk.TreePath([it.user_data])

    def do_get_value(self, it, column):
        if column == 0:
            return str(it.user_data)
        elif column == 1:
            return self.item_data

    def do_iter_next(self, it):
        # Return False if there is not a next item
        next = it.user_data + 1
        if next >= self.item_count:
            return False

        # Set the iters data and return True
        it.user_data = next
        return True

    def do_iter_previous(self, it):
        prev = it.user_data - 1
        if prev < 0:
            return False

        it.user_data = prev
        return True

    def do_iter_children(self, parent):
        # If parent is None return the first item
        if parent is None:
            it = Gtk.TreeIter()
            it.user_data = 0
            return True, it
        return False, None

    def do_iter_has_child(self, it):
        return it is None

    def do_iter_n_children(self, it):
        # If iter is None, return the number of top level nodes
        if it is None:
            return self.item_count
        return 0

    def do_iter_nth_child(self, parent, n):
        if parent is not None or n >= self.item_count:
            return False, None
        elif parent is None:
            # If parent is None, return the nth iter
            it = Gtk.TreeIter()
            it.user_data = n
            return True, it

    def do_iter_parent(self, child):
        return False, None


def main(demoapp=None):
    model = Model()
    # Use fixed-height-mode to get better model load and display performance.
    view = Gtk.TreeView(fixed_height_mode=True, headers_visible=False)
    column = Gtk.TreeViewColumn()
    column.props.sizing = Gtk.TreeViewColumnSizing.FIXED

    renderer1 = Gtk.CellRendererText()
    renderer2 = Gtk.CellRendererText()
    column.pack_start(renderer1, expand=True)
    column.pack_start(renderer2, expand=True)
    column.add_attribute(renderer1, 'text', 0)
    column.add_attribute(renderer2, 'text', 1)
    view.append_column(column)

    scrolled = Gtk.ScrolledWindow()
    scrolled.add(view)

    window = Gtk.Window(title=title)
    window.set_size_request(480, 640)
    window.add(scrolled)
    window.show_all()
    GLib.timeout_add(10, lambda *args: view.set_model(model))
    return window


if __name__ == "__main__":
    window = main()
    window.connect('destroy', Gtk.main_quit)
    Gtk.main()

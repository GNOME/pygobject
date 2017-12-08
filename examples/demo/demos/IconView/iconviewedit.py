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

title = "Editing and Drag-and-Drop"
description = """The GtkIconView widget supports Editing and Drag-and-Drop.
This example also demonstrates using the generic GtkCellLayout interface to set
up cell renderers in an icon view.
"""

from gi.repository import Gtk, Gdk, GdkPixbuf


class IconviewEditApp:
    COL_TEXT = 0
    NUM_COLS = 1

    def __init__(self):
        self.window = Gtk.Window()
        self.window.set_title('Editing and Drag-and-Drop')
        self.window.set_border_width(8)
        self.window.connect('destroy', Gtk.main_quit)

        store = Gtk.ListStore(str)
        colors = ['Red', 'Green', 'Blue', 'Yellow']
        store.clear()
        for c in colors:
            store.append([c])

        icon_view = Gtk.IconView(model=store)
        icon_view.set_selection_mode(Gtk.SelectionMode.SINGLE)
        icon_view.set_item_orientation(Gtk.Orientation.HORIZONTAL)
        icon_view.set_columns(2)
        icon_view.set_reorderable(True)

        renderer = Gtk.CellRendererPixbuf()
        icon_view.pack_start(renderer, True)
        icon_view.set_cell_data_func(renderer,
                                     self.set_cell_color,
                                     None)

        renderer = Gtk.CellRendererText()
        icon_view.pack_start(renderer, True)
        renderer.props.editable = True
        renderer.connect('edited', self.edited, icon_view)
        icon_view.add_attribute(renderer, 'text', self.COL_TEXT)

        self.window.add(icon_view)

        self.window.show_all()

    def set_cell_color(self, cell_layout, cell, tree_model, iter_, icon_view):

        # FIXME return single element instead of tuple
        text = tree_model.get(iter_, self.COL_TEXT)[0]
        color = Gdk.color_parse(text)
        pixel = 0
        if color is not None:
            pixel = ((color.red >> 8) << 24 |
                     (color.green >> 8) << 16 |
                     (color.blue >> 8) << 8)

        pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, False, 8, 24, 24)
        pixbuf.fill(pixel)

        cell.props.pixbuf = pixbuf

    def edited(self, cell, path_string, text, icon_view):
        model = icon_view.get_model()
        path = Gtk.TreePath(path_string)

        iter_ = model.get_iter(path)
        model.set_row(iter_, [text])


def main(demoapp=None):
    IconviewEditApp()
    Gtk.main()


if __name__ == '__main__':
    main()

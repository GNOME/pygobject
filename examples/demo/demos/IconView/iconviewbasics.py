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

title = "Icon View Basics"
description = """The GtkIconView widget is used to display and manipulate
icons. It uses a GtkTreeModel for data storage, so the list store example might
be helpful. We also use the Gio.File API to get the icons for each file type.
"""


import os

from gi.repository import GLib, Gio, GdkPixbuf, Gtk


class IconViewApp:
    (COL_PATH,
     COL_DISPLAY_NAME,
     COL_PIXBUF,
     COL_IS_DIRECTORY,
     NUM_COLS) = range(5)

    def __init__(self, demoapp):
        self.pixbuf_lookup = {}

        self.demoapp = demoapp

        self.window = Gtk.Window()
        self.window.set_title('Gtk.IconView demo')
        self.window.set_default_size(650, 400)
        self.window.connect('destroy', Gtk.main_quit)

        vbox = Gtk.VBox()
        self.window.add(vbox)

        tool_bar = Gtk.Toolbar()
        vbox.pack_start(tool_bar, False, False, 0)

        up_button = Gtk.ToolButton(stock_id=Gtk.STOCK_GO_UP)
        up_button.set_is_important(True)
        up_button.set_sensitive(False)
        tool_bar.insert(up_button, -1)

        home_button = Gtk.ToolButton(stock_id=Gtk.STOCK_HOME)
        home_button.set_is_important(True)
        tool_bar.insert(home_button, -1)

        sw = Gtk.ScrolledWindow()
        sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN)
        sw.set_policy(Gtk.PolicyType.AUTOMATIC,
                      Gtk.PolicyType.AUTOMATIC)

        vbox.pack_start(sw, True, True, 0)

        # create the store and fill it with content
        self.parent_dir = '/'
        store = self.create_store()
        self.fill_store(store)

        icon_view = Gtk.IconView(model=store)
        icon_view.set_selection_mode(Gtk.SelectionMode.MULTIPLE)
        sw.add(icon_view)

        # connect to the 'clicked' signal of the "Up" tool button
        up_button.connect('clicked', self.up_clicked, store)

        # connect to the 'clicked' signal of the "home" tool button
        home_button.connect('clicked', self.home_clicked, store)

        self.up_button = up_button
        self.home_button = home_button

        # we now set which model columns that correspond to the text
        # and pixbuf of each item
        icon_view.set_text_column(self.COL_DISPLAY_NAME)
        icon_view.set_pixbuf_column(self.COL_PIXBUF)

        # connect to the "item-activated" signal
        icon_view.connect('item-activated', self.item_activated, store)
        icon_view.grab_focus()

        self.window.show_all()

    def sort_func(self, store, a_iter, b_iter, user_data):
        (a_name, a_is_dir) = store.get(a_iter,
                                       self.COL_DISPLAY_NAME,
                                       self.COL_IS_DIRECTORY)

        (b_name, b_is_dir) = store.get(b_iter,
                                       self.COL_DISPLAY_NAME,
                                       self.COL_IS_DIRECTORY)

        if a_name is None:
            a_name = ''

        if b_name is None:
            b_name = ''

        if (not a_is_dir) and b_is_dir:
            return 1
        elif a_is_dir and (not b_is_dir):
            return -1
        elif a_name > b_name:
            return 1
        elif a_name < b_name:
            return -1
        else:
            return 0

    def up_clicked(self, item, store):
        self.parent_dir = os.path.split(self.parent_dir)[0]
        self.fill_store(store)
        # de-sensitize the up button if we are at the root
        self.up_button.set_sensitive(self.parent_dir != '/')

    def home_clicked(self, item, store):
        self.parent_dir = GLib.get_home_dir()
        self.fill_store(store)

        # Sensitize the up button
        self.up_button.set_sensitive(True)

    def item_activated(self, icon_view, tree_path, store):
        iter_ = store.get_iter(tree_path)
        (path, is_dir) = store.get(iter_, self.COL_PATH, self.COL_IS_DIRECTORY)
        if not is_dir:
            return

        self.parent_dir = path
        self.fill_store(store)

        self.up_button.set_sensitive(True)

    def create_store(self):
        store = Gtk.ListStore(str, str, GdkPixbuf.Pixbuf, bool)

        # set sort column and function
        store.set_default_sort_func(self.sort_func)
        store.set_sort_column_id(-1, Gtk.SortType.ASCENDING)

        return store

    def file_to_icon_pixbuf(self, path):
        pixbuf = None

        # get the theme icon
        f = Gio.file_new_for_path(path)
        info = f.query_info(Gio.FILE_ATTRIBUTE_STANDARD_ICON,
                            Gio.FileQueryInfoFlags.NONE,
                            None)
        gicon = info.get_icon()

        # check to see if it is an image format we support
        for format in GdkPixbuf.Pixbuf.get_formats():
            for mime_type in format.get_mime_types():
                content_type = Gio.content_type_from_mime_type(mime_type)
                if content_type is not None:
                    break

            format_gicon = Gio.content_type_get_icon(content_type)
            if format_gicon.equal(gicon):
                gicon = f.icon_new()
                break

        if gicon in self.pixbuf_lookup:
            return self.pixbuf_lookup[gicon]

        if isinstance(gicon, Gio.ThemedIcon):
            names = gicon.get_names()
            icon_theme = Gtk.IconTheme.get_default()
            for name in names:
                try:
                    pixbuf = icon_theme.load_icon(name, 64, 0)
                    break
                except GLib.GError:
                    pass

            self.pixbuf_lookup[gicon] = pixbuf

        elif isinstance(gicon, Gio.FileIcon):
            icon_file = gicon.get_file()
            path = icon_file.get_path()
            pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_size(path, 72, 72)
            self.pixbuf_lookup[gicon] = pixbuf

        return pixbuf

    def fill_store(self, store):
        store.clear()
        for name in os.listdir(self.parent_dir):
            path = os.path.join(self.parent_dir, name)
            is_dir = os.path.isdir(path)
            pixbuf = self.file_to_icon_pixbuf(path)
            store.append((path, name, pixbuf, is_dir))


def main(demoapp=None):
    IconViewApp(demoapp)
    Gtk.main()


if __name__ == '__main__':
    main()

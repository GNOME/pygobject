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

title = "Search Entry"
description = """GtkEntry allows to display icons and progress information.
This demo shows how to use these features in a search entry.
"""

from gi.repository import Gtk, GObject

(PIXBUF_COL,
 TEXT_COL) = range(2)


class SearchboxApp:
    def __init__(self, demoapp):
        self.demoapp = demoapp

        self.window = Gtk.Dialog(title='Search Entry')
        self.window.add_buttons(Gtk.STOCK_CLOSE, Gtk.ResponseType.NONE)

        self.window.connect('response', lambda x, y: self.window.destroy())
        self.window.connect('destroy', Gtk.main_quit)

        content_area = self.window.get_content_area()

        vbox = Gtk.VBox(spacing=5)
        content_area.pack_start(vbox, True, True, 0)
        vbox.set_border_width(5)

        label = Gtk.Label()
        label.set_markup('Search entry demo')
        vbox.pack_start(label, False, False, 0)

        hbox = Gtk.HBox(homogeneous=False, spacing=10)
        hbox.set_border_width(0)
        vbox.pack_start(hbox, True, True, 0)

        # Create our entry
        entry = Gtk.Entry()
        hbox.pack_start(entry, False, False, 0)

        # Create the find and cancel buttons
        notebook = Gtk.Notebook()
        self.notebook = notebook
        notebook.set_show_tabs(False)
        notebook.set_show_border(False)
        hbox.pack_start(notebook, False, False, 0)

        find_button = Gtk.Button(label='Find')
        find_button.connect('clicked', self.start_search, entry)
        notebook.append_page(find_button, None)
        find_button.show()

        cancel_button = Gtk.Button(label='Cancel')
        cancel_button.connect('clicked', self.stop_search, entry)
        notebook.append_page(cancel_button, None)
        cancel_button.show()

        # Set up the search icon
        self.search_by_name(None, entry)

        # Set up the clear icon
        entry.set_icon_from_stock(Gtk.EntryIconPosition.SECONDARY,
                                  Gtk.STOCK_CLEAR)
        self.text_changed_cb(entry, None, find_button)

        entry.connect('notify::text', self.text_changed_cb, find_button)

        entry.connect('activate', self.activate_cb)

        # Create the menu
        menu = self.create_search_menu(entry)
        entry.connect('icon-press', self.icon_press_cb, menu)

        # FIXME: this should take None for the detach callback
        #        but our callback implementation does not allow
        #        it yet, so we pass in a noop callback
        menu.attach_to_widget(entry, self.detach)

        # add accessible alternatives for icon functionality
        entry.connect('populate-popup', self.entry_populate_popup)

        self.window.show_all()

    def detach(self, *args):
        pass

    def show_find_button(self):
        self.notebook.set_current_page(0)

    def show_cancel_button(self):
        self.notebook.set_current_page(1)

    def search_progress(self, entry):
        entry.progress_pulse()
        return True

    def search_progress_done(self, entry):
        entry.set_progress_fraction(0.0)

    def finish_search(self, button, entry):
        self.show_find_button()
        GObject.source_remove(self.search_progress_id)
        self.search_progress_done(entry)
        self.search_progress_id = 0

        return False

    def start_search_feedback(self, entry):
        self.search_progress_id = GObject.timeout_add(100,
                                                      self.search_progress,
                                                      entry)

        return False

    def start_search(self, button, entry):
        self.show_cancel_button()
        self.search_progress_id = GObject.timeout_add_seconds(1,
                                                              self.start_search_feedback,
                                                              entry)
        self.finish_search_id = GObject.timeout_add_seconds(15,
                                                            self.finish_search,
                                                            button)

    def stop_search(self, button, entry):
        GObject.source_remove(self.finish_search_id)
        self.finish_search(button, entry)

    def clear_entry_swapped(self, widget, entry):
        self.clear_entry(entry)

    def clear_entry(self, entry):
        entry.set_text('')

    def search_by_name(self, item, entry):
        entry.set_icon_from_stock(Gtk.EntryIconPosition.PRIMARY,
                                  Gtk.STOCK_FIND)
        entry.set_icon_tooltip_text(Gtk.EntryIconPosition.PRIMARY,
                                    'Search by name\n' +
                                    'Click here to change the search type')

    def search_by_description(self, item, entry):
        entry.set_icon_from_stock(Gtk.EntryIconPosition.PRIMARY,
                                  Gtk.STOCK_EDIT)
        entry.set_icon_tooltip_text(Gtk.EntryIconPosition.PRIMARY,
                                    'Search by description\n' +
                                    'Click here to change the search type')

    def search_by_file(self, item, entry):
        entry.set_icon_from_stock(Gtk.EntryIconPosition.PRIMARY,
                                  Gtk.STOCK_OPEN)
        entry.set_icon_tooltip_text(Gtk.EntryIconPosition.PRIMARY,
                                    'Search by file name\n' +
                                    'Click here to change the search type')

    def create_search_menu(self, entry):
        menu = Gtk.Menu()

        item = Gtk.ImageMenuItem.new_with_mnemonic('Search by _name')
        image = Gtk.Image.new_from_stock(Gtk.STOCK_FIND, Gtk.IconSize.MENU)
        item.set_image(image)
        item.set_always_show_image(True)
        item.connect('activate', self.search_by_name, entry)
        menu.append(item)

        item = Gtk.ImageMenuItem.new_with_mnemonic('Search by _description')
        image = Gtk.Image.new_from_stock(Gtk.STOCK_EDIT, Gtk.IconSize.MENU)
        item.set_image(image)
        item.set_always_show_image(True)
        item.connect('activate', self.search_by_description, entry)
        menu.append(item)

        item = Gtk.ImageMenuItem.new_with_mnemonic('Search by _file name')
        image = Gtk.Image.new_from_stock(Gtk.STOCK_OPEN, Gtk.IconSize.MENU)
        item.set_image(image)
        item.set_always_show_image(True)
        item.connect('activate', self.search_by_name, entry)
        menu.append(item)

        menu.show_all()

        return menu

    def icon_press_cb(self, entry, position, event, menu):
        if position == Gtk.EntryIconPosition.PRIMARY:
            menu.popup(None, None, None, None,
                       event.button, event.time)
        else:
            self.clear_entry(entry)

    def text_changed_cb(self, entry, pspec, button):
        has_text = entry.get_text_length() > 0
        entry.set_icon_sensitive(Gtk.EntryIconPosition.SECONDARY, has_text)
        button.set_sensitive(has_text)

    def activate_cb(self, entry, button):
        if self.search_progress_id != 0:
            return
        self.start_search(button, entry)

    def search_entry_destroyed(self, widget):
        if self.finish_search_id != 0:
            GObject.source_remove(self.finish_search_id)
        if self.search_progress_id != 0:
            GObject.source_remove(self.search_progress_id)

        self.window = None

    def entry_populate_popup(self, entry, menu):
        has_text = entry.get_text_length() > 0

        item = Gtk.SeparatorMenuItem()
        item.show()
        menu.append(item)

        item = Gtk.MenuItem.new_with_mnemonic("C_lear")
        item.show()
        item.connect('activate', self.clear_entry_swapped, entry)
        menu.append(item)
        item.set_sensitive(has_text)

        search_menu = self.create_search_menu(entry)
        item = Gtk.MenuItem.new_with_label('Search by')
        item.show()
        item.set_submenu(search_menu)
        menu.append(item)


def main(demoapp=None):
    SearchboxApp(demoapp)
    Gtk.main()


if __name__ == '__main__':
    main()

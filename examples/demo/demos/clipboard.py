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

title = "Clipboard"
description = """
GtkClipboard is used for clipboard handling. This demo shows how to
copy and paste text to and from the clipboard.

It also shows how to transfer images via the clipboard or via
drag-and-drop, and how to make clipboard contents persist after
the application exits. Clipboard persistence requires a clipboard
manager to run.
"""


from gi.repository import Gtk, Gdk


class ClipboardApp:
    def __init__(self):
        self.window = Gtk.Window()
        self.window.set_title('Clipboard demo')
        self.window.connect('destroy', lambda w: Gtk.main_quit())

        vbox = Gtk.VBox(homogeneous=False, spacing=0)
        vbox.set_border_width(8)
        self.window.add(vbox)

        label = Gtk.Label(label='"Copy" will copy the text\nin the entry to the clipboard')
        vbox.pack_start(label, False, False, 0)

        hbox = Gtk.HBox(homogeneous=False, spacing=4)
        hbox.set_border_width(8)
        vbox.pack_start(hbox, False, False, 0)

        # create first entry
        entry = Gtk.Entry()
        hbox.pack_start(entry, True, True, 0)

        # create button
        button = Gtk.Button.new_from_stock(Gtk.STOCK_COPY)
        hbox.pack_start(button, False, False, 0)
        button.connect('clicked', self.copy_button_clicked, entry)

        label = Gtk.Label(label='"Paste" will paste the text from the clipboard to the entry')
        vbox.pack_start(label, False, False, 0)

        hbox = Gtk.HBox(homogeneous=False, spacing=4)
        hbox.set_border_width(8)
        vbox.pack_start(hbox, False, False, 0)

        # create secondary entry
        entry = Gtk.Entry()
        hbox.pack_start(entry, True, True, 0)
        # create button
        button = Gtk.Button.new_from_stock(Gtk.STOCK_PASTE)
        hbox.pack_start(button, False, False, 0)
        button.connect('clicked', self.paste_button_clicked, entry)

        label = Gtk.Label(label='Images can be transferred via the clipboard, too')
        vbox.pack_start(label, False, False, 0)

        hbox = Gtk.HBox(homogeneous=False, spacing=4)
        hbox.set_border_width(8)
        vbox.pack_start(hbox, False, False, 0)

        # create the first image
        image = Gtk.Image(stock=Gtk.STOCK_DIALOG_WARNING,
                          icon_size=Gtk.IconSize.BUTTON)

        ebox = Gtk.EventBox()
        ebox.add(image)
        hbox.add(ebox)

        # make ebox a drag source
        ebox.drag_source_set(Gdk.ModifierType.BUTTON1_MASK,
                             None, Gdk.DragAction.COPY)
        ebox.drag_source_add_image_targets()
        ebox.connect('drag-begin', self.drag_begin, image)
        ebox.connect('drag-data-get', self.drag_data_get, image)

        # accept drops on ebox
        ebox.drag_dest_set(Gtk.DestDefaults.ALL,
                           None, Gdk.DragAction.COPY)
        ebox.drag_dest_add_image_targets()
        ebox.connect('drag-data-received', self.drag_data_received, image)

        # context menu on ebox
        ebox.connect('button-press-event', self.button_press, image)

        # create the second image
        image = Gtk.Image(stock=Gtk.STOCK_STOP,
                          icon_size=Gtk.IconSize.BUTTON)

        ebox = Gtk.EventBox()
        ebox.add(image)
        hbox.add(ebox)

        # make ebox a drag source
        ebox.drag_source_set(Gdk.ModifierType.BUTTON1_MASK,
                             None, Gdk.DragAction.COPY)
        ebox.drag_source_add_image_targets()
        ebox.connect('drag-begin', self.drag_begin, image)
        ebox.connect('drag-data-get', self.drag_data_get, image)

        # accept drops on ebox
        ebox.drag_dest_set(Gtk.DestDefaults.ALL,
                           None, Gdk.DragAction.COPY)
        ebox.drag_dest_add_image_targets()
        ebox.connect('drag-data-received', self.drag_data_received, image)

        # context menu on ebox
        ebox.connect('button-press-event', self.button_press, image)

        # tell the clipboard manager to make data persistent
        # FIXME: Allow sending strings a Atoms and convert in PyGI
        atom = Gdk.atom_intern('CLIPBOARD', True)
        clipboard = Gtk.Clipboard.get(atom)
        clipboard.set_can_store(None)

        self.window.show_all()

    def copy_button_clicked(self, button, entry):
        # get the default clipboard
        atom = Gdk.atom_intern('CLIPBOARD', True)
        clipboard = entry.get_clipboard(atom)

        # set the clipboard's text
        # FIXME: don't require passing length argument
        clipboard.set_text(entry.get_text(), -1)

    def paste_received(self, clipboard, text, entry):
        if text is not None:
            entry.set_text(text)

    def paste_button_clicked(self, button, entry):
        # get the default clipboard
        atom = Gdk.atom_intern('CLIPBOARD', True)
        clipboard = entry.get_clipboard(atom)

        # set the clipboard's text
        clipboard.request_text(self.paste_received, entry)

    def get_image_pixbuf(self, image):
        # FIXME: We should hide storage types in an override
        storage_type = image.get_storage_type()
        if storage_type == Gtk.ImageType.PIXBUF:
            return image.get_pixbuf()
        elif storage_type == Gtk.ImageType.STOCK:
            (stock_id, size) = image.get_stock()
            return image.render_icon(stock_id, size, None)

        return None

    def drag_begin(self, widget, context, data):
        pixbuf = self.get_image_pixbuf(data)
        Gtk.drag_set_icon_pixbuf(context, pixbuf, -2, -2)

    def drag_data_get(self, widget, context, selection_data, info, time, data):
        pixbuf = self.get_image_pixbuf(data)
        selection_data.set_pixbuf(pixbuf)

    def drag_data_received(self, widget, context, x, y, selection_data, info, time, data):
        if selection_data.get_length() > 0:
            pixbuf = selection_data.get_pixbuf()
            data.set_from_pixbuf(pixbuf)

    def copy_image(self, item, data):
        # get the default clipboard
        atom = Gdk.atom_intern('CLIPBOARD', True)
        clipboard = Gtk.Clipboard.get(atom)
        pixbuf = self.get_image_pixbuf(data)

        clipboard.set_image(pixbuf)

    def paste_image(self, item, data):
        # get the default clipboard
        atom = Gdk.atom_intern('CLIPBOARD', True)
        clipboard = Gtk.Clipboard.get(atom)
        pixbuf = clipboard.wait_for_image()

        if pixbuf is not None:
            data.set_from_pixbuf(pixbuf)

    def button_press(self, widget, event, data):
        if event.button != 3:
            return False

        self.menu = Gtk.Menu()

        item = Gtk.ImageMenuItem.new_from_stock(Gtk.STOCK_COPY, None)
        item.connect('activate', self.copy_image, data)
        item.show()
        self.menu.append(item)

        item = Gtk.ImageMenuItem.new_from_stock(Gtk.STOCK_PASTE, None)
        item.connect('activate', self.paste_image, data)
        item.show()
        self.menu.append(item)

        self.menu.popup(None, None, None, None, event.button, event.time)


def main(demoapp=None):
    ClipboardApp()
    Gtk.main()


if __name__ == '__main__':
    main()

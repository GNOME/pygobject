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

title = "Entry Buffer"
description = """
Gtk.EntryBuffer provides the text content in a Gtk.Entry.
"""


from gi.repository import Gtk


class EntryBufferApp:
    def __init__(self):
        self.window = Gtk.Dialog(title='Gtk.EntryBuffer')
        self.window.add_buttons(Gtk.STOCK_CLOSE, Gtk.ResponseType.NONE)
        self.window.connect('response', self.destroy)
        self.window.connect('destroy', lambda x: Gtk.main_quit())
        self.window.set_resizable(False)

        vbox = Gtk.VBox(homogeneous=False, spacing=0)
        self.window.get_content_area().pack_start(vbox, True, True, 0)
        vbox.set_border_width(5)

        label = Gtk.Label()
        label.set_markup('Entries share a buffer. Typing in one is reflected in the other.')
        vbox.pack_start(label, False, False, 0)

        # create a buffer
        buffer = Gtk.EntryBuffer()

        #create our first entry
        entry = Gtk.Entry(buffer=buffer)
        vbox.pack_start(entry, False, False, 0)

        # create the second entry
        entry = Gtk.Entry(buffer=buffer)
        entry.set_visibility(False)
        vbox.pack_start(entry, False, False, 0)

        self.window.show_all()

    def destroy(self, *args):
        self.window.destroy()
        Gtk.main_quit()


def main(demoapp=None):
    EntryBufferApp()
    Gtk.main()

if __name__ == '__main__':
    main()

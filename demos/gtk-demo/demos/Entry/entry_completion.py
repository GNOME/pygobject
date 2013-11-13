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

title = "Entry Completion"
description = """
Gtk.EntryCompletion provides a mechanism for adding support for
completion in Gtk.Entry.
"""


from gi.repository import Gtk


class EntryBufferApp:
    def __init__(self):
        self.window = Gtk.Dialog(title='Gtk.EntryCompletion')
        self.window.add_buttons(Gtk.STOCK_CLOSE, Gtk.ResponseType.NONE)
        self.window.connect('response', self.destroy)
        self.window.connect('destroy', lambda x: Gtk.main_quit())
        self.window.set_resizable(False)

        vbox = Gtk.VBox(homogeneous=False, spacing=0)
        self.window.get_content_area().pack_start(vbox, True, True, 0)
        vbox.set_border_width(5)

        label = Gtk.Label()
        label.set_markup('Completion demo, try writing <b>total</b> or <b>gnome</b> for example.')
        vbox.pack_start(label, False, False, 0)

        #create our entry
        entry = Gtk.Entry()
        vbox.pack_start(entry, False, False, 0)

        # create the completion object
        completion = Gtk.EntryCompletion()

        # assign the completion to the entry
        entry.set_completion(completion)

        # create tree model and use it as the completion model
        completion_model = self.create_completion_model()
        completion.set_model(completion_model)

        completion.set_text_column(0)

        self.window.show_all()

    def create_completion_model(self):
        store = Gtk.ListStore(str)

        store.append(['GNOME'])
        store.append(['total'])
        store.append(['totally'])

        return store

    def destroy(self, *args):
        self.window.destroy()
        Gtk.main_quit()


def main(demoapp=None):
    EntryBufferApp()
    Gtk.main()

if __name__ == '__main__':
    main()

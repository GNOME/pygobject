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

title = "Dialog and Message Boxes"
description = """
Dialog widgets are used to pop up a transient window for user feedback.
"""

from gi.repository import Gtk


class DialogsApp:
    def __init__(self):
        self.dialog_counter = 1

        self.window = Gtk.Window(title="Dialogs")
        self.window.set_border_width(8)
        self.window.connect('destroy', Gtk.main_quit)

        frame = Gtk.Frame(label="Dialogs")
        self.window.add(frame)

        vbox = Gtk.VBox(spacing=8)
        vbox.set_border_width(8)
        frame.add(vbox)

        # Standard message dialog
        hbox = Gtk.HBox(spacing=8)
        vbox.pack_start(hbox, False, False, 0)
        button = Gtk.Button.new_with_mnemonic("_Message Dialog")

        button.connect('clicked',
                       self._message_dialog_clicked)
        hbox.pack_start(button, False, False, 0)

        vbox.pack_start(Gtk.HSeparator(),
                        False, False, 0)

        # Interactive dialog
        hbox = Gtk.HBox(spacing=8)
        vbox.pack_start(hbox, False, False, 0)
        vbox2 = Gtk.VBox(spacing=0)
        button = Gtk.Button.new_with_mnemonic("_Interactive Dialog")

        button.connect('clicked',
                       self._interactive_dialog_clicked)
        hbox.pack_start(vbox2, False, False, 0)
        vbox2.pack_start(button, False, False, 0)

        table = Gtk.Table(n_rows=2, n_columns=2, homogeneous=False)
        table.set_row_spacings(4)
        table.set_col_spacings(4)
        hbox.pack_start(table, False, False, 0)

        label = Gtk.Label.new_with_mnemonic("_Entry 1")
        table.attach_defaults(label, 0, 1, 0, 1)

        self.entry1 = Gtk.Entry()
        table.attach_defaults(self.entry1, 1, 2, 0, 1)
        label.set_mnemonic_widget(self.entry1)

        label = Gtk.Label.new_with_mnemonic("E_ntry 2")

        table.attach_defaults(label, 0, 1, 1, 2)

        self.entry2 = Gtk.Entry()
        table.attach_defaults(self.entry2, 1, 2, 1, 2)
        label.set_mnemonic_widget(self.entry2)

        self.window.show_all()

    def _interactive_dialog_clicked(self, button):
        dialog = Gtk.Dialog(title='Interactive Dialog',
                            transient_for=self.window,
                            modal=True,
                            destroy_with_parent=True)
        dialog.add_buttons(Gtk.STOCK_OK, Gtk.ResponseType.OK,
                           "_Non-stock Button", Gtk.ResponseType.CANCEL)

        content_area = dialog.get_content_area()
        hbox = Gtk.HBox(spacing=8)
        hbox.set_border_width(8)
        content_area.pack_start(hbox, False, False, 0)

        stock = Gtk.Image(stock=Gtk.STOCK_DIALOG_QUESTION,
                          icon_size=Gtk.IconSize.DIALOG)

        hbox.pack_start(stock, False, False, 0)

        table = Gtk.Table(n_rows=2, n_columns=2, homogeneous=False)
        table.set_row_spacings(4)
        table.set_col_spacings(4)
        hbox.pack_start(table, True, True, 0)
        label = Gtk.Label.new_with_mnemonic("_Entry 1")
        table.attach_defaults(label, 0, 1, 0, 1)
        local_entry1 = Gtk.Entry()
        local_entry1.set_text(self.entry1.get_text())
        table.attach_defaults(local_entry1, 1, 2, 0, 1)
        label.set_mnemonic_widget(local_entry1)

        label = Gtk.Label.new_with_mnemonic("E_ntry 2")
        table.attach_defaults(label, 0, 1, 1, 2)

        local_entry2 = Gtk.Entry()
        local_entry2.set_text(self.entry2.get_text())
        table.attach_defaults(local_entry2, 1, 2, 1, 2)
        label.set_mnemonic_widget(local_entry2)

        hbox.show_all()

        response = dialog.run()
        if response == Gtk.ResponseType.OK:
            self.entry1.set_text(local_entry1.get_text())
            self.entry2.set_text(local_entry2.get_text())

        dialog.destroy()

    def _message_dialog_clicked(self, button):
        dialog = Gtk.MessageDialog(transient_for=self.window,
                                   modal=True,
                                   destroy_with_parent=True,
                                   message_type=Gtk.MessageType.INFO,
                                   buttons=Gtk.ButtonsType.OK,
                                   text="This message box has been popped up the following\nnumber of times:")
        dialog.format_secondary_text('%d' % self.dialog_counter)
        dialog.run()

        self.dialog_counter += 1
        dialog.destroy()


def main(demoapp=None):
    DialogsApp()
    Gtk.main()


if __name__ == '__main__':
    main()

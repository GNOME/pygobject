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

title = "Pickers"
description = """These widgets are mainly intended for use in preference
dialogs. They allow to select colors, fonts, files and directories.
"""

from gi.repository import Gtk


class PickersApp:
    def __init__(self):
        self.window = Gtk.Window(title='Pickers')
        self.window.connect('destroy', Gtk.main_quit)
        self.window.set_border_width(10)

        table = Gtk.Table(n_rows=4, n_columns=2, homogeneous=False)
        table.set_col_spacing(0, 10)
        table.set_row_spacings(3)
        self.window.add(table)
        table.set_border_width(10)

        label = Gtk.Label(label='Color:')
        label.set_alignment(0.0, 0.5)
        picker = Gtk.ColorButton()
        table.attach_defaults(label, 0, 1, 0, 1)
        table.attach_defaults(picker, 1, 2, 0, 1)

        label = Gtk.Label(label='Font:')
        label.set_alignment(0.0, 0.5)
        picker = Gtk.FontButton()
        table.attach_defaults(label, 0, 1, 1, 2)
        table.attach_defaults(picker, 1, 2, 1, 2)

        label = Gtk.Label(label='File:')
        label.set_alignment(0.0, 0.5)
        picker = Gtk.FileChooserButton.new('Pick a File',
                                           Gtk.FileChooserAction.OPEN)
        table.attach_defaults(label, 0, 1, 2, 3)
        table.attach_defaults(picker, 1, 2, 2, 3)

        label = Gtk.Label(label='Folder:')
        label.set_alignment(0.0, 0.5)
        picker = Gtk.FileChooserButton.new('Pick a Folder',
                                           Gtk.FileChooserAction.SELECT_FOLDER)
        table.attach_defaults(label, 0, 1, 3, 4)
        table.attach_defaults(picker, 1, 2, 3, 4)

        self.window.show_all()


def main(demoapp=None):
    PickersApp()
    Gtk.main()

if __name__ == '__main__':
    main()

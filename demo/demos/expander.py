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

title = "Expander"
description = """
GtkExpander allows to provide additional content that is initially hidden.
This is also known as "disclosure triangle".
"""

from gi.repository import Gtk


class ExpanderApp:
    def __init__(self):
        self.window = Gtk.Dialog(title="GtkExpander")
        self.window.add_buttons(Gtk.STOCK_CLOSE, Gtk.ResponseType.NONE)
        self.window.set_resizable(False)
        self.window.connect('response', lambda window, x: window.destroy())
        self.window.connect('destroy', Gtk.main_quit)

        content_area = self.window.get_content_area()
        vbox = Gtk.VBox(spacing=5)
        content_area.pack_start(vbox, True, True, 0)
        vbox.set_border_width(5)

        label = Gtk.Label(label='Expander demo. Click on the triangle for details.')
        vbox.pack_start(label, True, True, 0)

        expander = Gtk.Expander(label='Details')
        vbox.pack_start(expander, False, False, 0)

        label = Gtk.Label(label='Details can be shown or hidden')
        expander.add(label)

        self.window.show_all()


def main(demoapp=None):
    ExpanderApp()
    Gtk.main()

if __name__ == '__main__':
    main()

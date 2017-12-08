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

title = "Button Boxes"
description = """
The Button Box widgets are used to arrange buttons with padding.
"""


from gi.repository import Gtk


class ButtonBoxApp:
    def __init__(self):
        window = Gtk.Window()
        window.set_title('Button Boxes')
        window.connect('destroy', lambda x: Gtk.main_quit())
        window.set_border_width(10)

        main_vbox = Gtk.VBox(homogeneous=False, spacing=0)
        window.add(main_vbox)

        frame_horz = Gtk.Frame(label='Horizontal Button Boxes')
        main_vbox.pack_start(frame_horz, True, True, 10)

        vbox = Gtk.VBox(homogeneous=False, spacing=0)
        vbox.set_border_width(10)
        frame_horz.add(vbox)

        vbox.pack_start(
            self.create_bbox(True, 'Spread', 40, Gtk.ButtonBoxStyle.SPREAD),
            True, True, 0)

        vbox.pack_start(
            self.create_bbox(True, 'Edge', 40, Gtk.ButtonBoxStyle.EDGE),
            True, True, 5)

        vbox.pack_start(
            self.create_bbox(True, 'Start', 40, Gtk.ButtonBoxStyle.START),
            True, True, 5)

        vbox.pack_start(
            self.create_bbox(True, 'End', 40, Gtk.ButtonBoxStyle.END),
            True, True, 5)

        frame_vert = Gtk.Frame(label='Vertical Button Boxes')
        main_vbox.pack_start(frame_vert, True, True, 10)

        hbox = Gtk.HBox(homogeneous=False, spacing=0)
        hbox.set_border_width(10)
        frame_vert.add(hbox)

        hbox.pack_start(
            self.create_bbox(False, 'Spread', 30, Gtk.ButtonBoxStyle.SPREAD),
            True, True, 0)

        hbox.pack_start(
            self.create_bbox(False, 'Edge', 30, Gtk.ButtonBoxStyle.EDGE),
            True, True, 5)

        hbox.pack_start(
            self.create_bbox(False, 'Start', 30, Gtk.ButtonBoxStyle.START),
            True, True, 5)

        hbox.pack_start(
            self.create_bbox(False, 'End', 30, Gtk.ButtonBoxStyle.END),
            True, True, 5)

        window.show_all()

    def create_bbox(self, is_horizontal, title, spacing, layout):
        frame = Gtk.Frame(label=title)

        if is_horizontal:
            bbox = Gtk.HButtonBox()
        else:
            bbox = Gtk.VButtonBox()

        bbox.set_border_width(5)
        frame.add(bbox)

        bbox.set_layout(layout)
        bbox.set_spacing(spacing)

        # FIXME: GtkButton consturctor should take a stock_id
        button = Gtk.Button.new_from_stock(Gtk.STOCK_OK)
        bbox.add(button)

        button = Gtk.Button.new_from_stock(Gtk.STOCK_CANCEL)
        bbox.add(button)

        button = Gtk.Button.new_from_stock(Gtk.STOCK_HELP)
        bbox.add(button)

        return frame


def main(demoapp=None):
    ButtonBoxApp()
    Gtk.main()


if __name__ == '__main__':
    main()

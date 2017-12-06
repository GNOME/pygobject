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

title = "Color Selector"
description = """
 GtkColorSelection lets the user choose a color. GtkColorSelectionDialog is
 a prebuilt dialog containing a GtkColorSelection.
 """


from gi.repository import Gtk, Gdk


class ColorSelectorApp:
    def __init__(self):
        # FIXME: we should allow Gdk.Color to be allocated without parameters
        #        Also color doesn't seem to work
        self.color = Gdk.RGBA()
        self.color.red = 0
        self.color.blue = 1
        self.color.green = 0
        self.color.alpha = 1

        self.window = Gtk.Window()
        self.window.set_title('Color Selection')
        self.window.set_border_width(8)
        self.window.connect('destroy', lambda w: Gtk.main_quit())

        vbox = Gtk.VBox(homogeneous=False,
                        spacing=8)
        vbox.set_border_width(8)
        self.window.add(vbox)

        # create color swatch area
        frame = Gtk.Frame()
        frame.set_shadow_type(Gtk.ShadowType.IN)
        vbox.pack_start(frame, True, True, 0)

        self.da = Gtk.DrawingArea()
        self.da.connect('draw', self.draw_cb)

        # set a minimum size
        self.da.set_size_request(200, 200)
        # set the color
        self.da.override_background_color(0, self.color)
        frame.add(self.da)

        alignment = Gtk.Alignment(xalign=1.0,
                                  yalign=0.5,
                                  xscale=0.0,
                                  yscale=0.0)

        button = Gtk.Button(label='_Change the above color',
                            use_underline=True)
        alignment.add(button)
        vbox.pack_start(alignment, False, False, 0)

        button.connect('clicked', self.change_color_cb)

        self.window.show_all()

    def draw_cb(self, widget, cairo_ctx):
        style = widget.get_style_context()
        bg_color = style.get_background_color(0)
        Gdk.cairo_set_source_rgba(cairo_ctx, bg_color)
        cairo_ctx.paint()

        return True

    def change_color_cb(self, button):
        dialog = Gtk.ColorSelectionDialog(title='Changing color')
        dialog.set_transient_for(self.window)

        colorsel = dialog.get_color_selection()
        colorsel.set_previous_rgba(self.color)
        colorsel.set_current_rgba(self.color)
        colorsel.set_has_palette(True)

        response = dialog.run()

        if response == Gtk.ResponseType.OK:
            self.color = colorsel.get_current_rgba()
            self.da.override_background_color(0, self.color)

        dialog.destroy()


def main(demoapp=None):
    ColorSelectorApp()
    Gtk.main()

if __name__ == '__main__':
    main()

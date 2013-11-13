#!/usr/bin/env python
# coding=utf-8
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

title = "Rotated Text"
description = """This demo shows how to use PangoCairo to draw rotated and
transformed text.  The right pane shows a rotated GtkLabel widget. In both
cases, a custom PangoCairo shape renderer is installed to draw a red heard using
cairo drawing operations instead of the Unicode heart character.
"""

from gi.repository import Gtk, Pango, PangoCairo, Gdk
import cairo
import sys
import math

# Python 2 and 3 handle UTF8 differently
if sys.version_info < (3, 0):
    BYTES_TEXT = "I \xe2\x99\xa5 GTK+"
    UTF8_TEXT = unicode(BYTES_TEXT, 'UTF-8')
    BYTES_HEART = "\xe2\x99\xa5"
    HEART = unicode(BYTES_HEART, 'UTF-8')
else:
    UTF8_TEXT = "I ♥ GTK+"
    BYTES_TEXT = bytes(UTF8_TEXT, 'utf-8')
    HEART = "♥"
    BYTES_HEART = bytes(HEART, 'utf-8')


class RotatedTextApp:
    RADIUS = 150
    N_WORDS = 5
    FONT = "Serif 18"

    def __init__(self):

        white = Gdk.RGBA()
        white.red = 1.0
        white.green = 1.0
        white.blue = 1.0
        white.alpha = 1.0

        self.window = Gtk.Window(title="Rotated Text")
        self.window.set_default_size(4 * self.RADIUS, 2 * self.RADIUS)
        self.window.connect('destroy', Gtk.main_quit)

        box = Gtk.HBox()
        box.set_homogeneous(True)
        self.window.add(box)

        # add a drawing area
        da = Gtk.DrawingArea()
        box.add(da)

        # override the background color from the theme
        da.override_background_color(0, white)

        da.connect('draw', self.rotated_text_draw)

        label = Gtk.Label(label=UTF8_TEXT)
        box.add(label)
        label.set_angle(45)

        # Setup some fancy stuff on the label
        layout = label.get_layout()

        PangoCairo.context_set_shape_renderer(layout.get_context(),
                                              self.fancy_shape_renderer,
                                              None)
        attrs = self.create_fancy_attr_list_for_layout(layout)
        label.set_attributes(attrs)

        self.window.show_all()

    def fancy_shape_renderer(self, cairo_ctx, attr, do_path):
        x, y = cairo_ctx.get_current_point()
        cairo_ctx.translate(x, y)

        cairo_ctx.scale(float(attr.inc_rect.width) / Pango.SCALE,
                        float(attr.inc_rect.height) / Pango.SCALE)

        if int(attr.data) == 0x2665:  # U+2665 BLACK HEART SUIT
            cairo_ctx.move_to(0.5, 0.0)
            cairo_ctx.line_to(0.9, -0.4)
            cairo_ctx.curve_to(1.1, -0.8, 0.5, -0.9, 0.5, -0.5)
            cairo_ctx.curve_to(0.5, -0.9, -0.1, -0.8, 0.1, -0.4)
            cairo_ctx.close_path()

        if not do_path:
            cairo_ctx.set_source_rgb(1.0, 0.0, 0.0)
            cairo_ctx.fill()

    def create_fancy_attr_list_for_layout(self, layout):
        pango_ctx = layout.get_context()
        metrics = pango_ctx.get_metrics(layout.get_font_description(),
                                        None)
        ascent = metrics.get_ascent()

        logical_rect = Pango.Rectangle()
        logical_rect.x = 0
        logical_rect.width = ascent
        logical_rect.y = -ascent
        logical_rect.height = ascent

        # Set fancy shape attributes for all hearts
        attrs = Pango.AttrList()

        # FIXME: attr_shape_new_with_data isn't introspectable
        '''
        ink_rect = logical_rect
        p = BYTES_TEXT.find(BYTES_HEART)
        while (p != -1):
            attr = Pango.attr_shape_new_with_data(ink_rect,
                                                  logical_rect,
                                                  ord(HEART),
                                                  None)
            attr.start_index = p
            attr.end_index = p + len(BYTES_HEART)
            p = UTF8_TEXT.find(HEART, attr.end_index)

        attrs.insert(attr)
        '''
        return attrs

    def rotated_text_draw(self, da, cairo_ctx):
        # Create a cairo context and set up a transformation matrix so that the user
        # space coordinates for the centered square where we draw are [-RADIUS, RADIUS],
        # [-RADIUS, RADIUS].
        # We first center, then change the scale.
        width = da.get_allocated_width()
        height = da.get_allocated_height()
        device_radius = min(width, height) / 2.0
        cairo_ctx.translate(
            device_radius + (width - 2 * device_radius) / 2,
            device_radius + (height - 2 * device_radius) / 2)
        cairo_ctx.scale(device_radius / self.RADIUS,
                        device_radius / self.RADIUS)

        # Create a subtle gradient source and use it.
        pattern = cairo.LinearGradient(-self.RADIUS, -self.RADIUS, self.RADIUS, self.RADIUS)
        pattern.add_color_stop_rgb(0.0, 0.5, 0.0, 0.0)
        pattern.add_color_stop_rgb(1.0, 0.0, 0.0, 0.5)
        cairo_ctx.set_source(pattern)

        # Create a PangoContext and set up our shape renderer
        context = da.create_pango_context()
        PangoCairo.context_set_shape_renderer(context,
                                              self.fancy_shape_renderer,
                                              None)

        # Create a PangoLayout, set the text, font, and attributes */
        layout = Pango.Layout(context=context)
        layout.set_text(UTF8_TEXT, -1)
        desc = Pango.FontDescription(self.FONT)
        layout.set_font_description(desc)

        attrs = self.create_fancy_attr_list_for_layout(layout)
        layout.set_attributes(attrs)

        # Draw the layout N_WORDS times in a circle */
        for i in range(self.N_WORDS):
            # Inform Pango to re-layout the text with the new transformation matrix
            PangoCairo.update_layout(cairo_ctx, layout)

            width, height = layout.get_pixel_size()
            cairo_ctx.move_to(-width / 2, -self.RADIUS * 0.9)
            PangoCairo.show_layout(cairo_ctx, layout)

            # Rotate for the next turn
            cairo_ctx.rotate(math.pi * 2 / self.N_WORDS)

        return False


def main(demoapp=None):
    RotatedTextApp()
    Gtk.main()

if __name__ == '__main__':
    main()

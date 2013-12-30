#!/usr/bin/env python
# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2013 Gian Mario Tagliaretti <gianmt@gnome.org>
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

title = "CSS Theming/Multiple Backgrounds"
description = """
Gtk themes are written using CSS. Every widget is build of multiple items
that you can style very similarly to a regular website.
"""

from gi.repository import Gtk, Gdk, Pango, Gio, GLib


class CSSMultiplebgsApp:
    def __init__(self, demoapp):
        self.demoapp = demoapp

        self.window = Gtk.Window()
        self.window.set_title('CSS Multiplebgs')
        self.window.set_default_size(400, 300)
        self.window.set_border_width(10)
        self.window.connect('destroy', lambda w: Gtk.main_quit())

        overlay = Gtk.Overlay()
        overlay.add_events(Gdk.EventMask.ENTER_NOTIFY_MASK |
                           Gdk.EventMask.LEAVE_NOTIFY_MASK |
                           Gdk.EventMask.POINTER_MOTION_MASK)
        self.window.add(overlay)

        canvas = Gtk.DrawingArea()
        canvas.set_name("canvas")
        canvas.connect("draw", self.drawing_area_draw)
        overlay.add(canvas)

        button = Gtk.Button()
        button.add_events(Gdk.EventMask.ENTER_NOTIFY_MASK |
                          Gdk.EventMask.LEAVE_NOTIFY_MASK |
                          Gdk.EventMask.POINTER_MOTION_MASK)
        button.set_name("bricks-button")
        button.set_halign(Gtk.Align.CENTER)
        button.set_valign(Gtk.Align.CENTER)
        button.set_size_request(250, 84)
        overlay.add_overlay(button)

        paned = Gtk.Paned(orientation=Gtk.Orientation.VERTICAL)
        overlay.add_overlay(paned)

        # We need a filler so we get a handle
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        paned.add(box)

        buffer = Gtk.TextBuffer()
        buffer.create_tag(tag_name="warning", underline=Pango.Underline.SINGLE)
        buffer.create_tag(tag_name="error", underline=Pango.Underline.ERROR)

        provider = Gtk.CssProvider()

        buffer.connect("changed", self.css_text_changed, provider)
        provider.connect("parsing-error", self.show_parsing_error, buffer)

        textview = Gtk.TextView()
        textview.set_buffer(buffer)

        scrolled = Gtk.ScrolledWindow()
        scrolled.add(textview)
        paned.add(scrolled)

        bytes = Gio.resources_lookup_data("/css_multiplebgs/css_multiplebgs.css", 0)
        buffer.set_text(bytes.get_data().decode('utf-8'))

        self.apply_css(self.window, provider)
        self.window.show_all()

    def drawing_area_draw(self, widget, cairo_t):
        context = widget.get_style_context()
        Gtk.render_background(context, cairo_t, 0, 0,
                              widget.get_allocated_width(),
                              widget.get_allocated_height())

        Gtk.render_frame(context, cairo_t, 0, 0,
                         widget.get_allocated_width(),
                         widget.get_allocated_height())

    def apply_css(self, widget, provider):
        Gtk.StyleContext.add_provider(widget.get_style_context(),
                                      provider,
                                      Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        if isinstance(widget, Gtk.Container):
            widget.forall(self.apply_css, provider)

    def show_parsing_error(self, provider, section, error, buffer):
        start = buffer.get_iter_at_line_index(section.get_start_line(),
                                              section.get_start_position())

        end = buffer.get_iter_at_line_index(section.get_end_line(),
                                            section.get_end_position())

        # FIXME: this should return a GLib.GError instead it returns
        # FIXME: a GLib.Error object
        # FIXME: see https://bugzilla.gnome.org/show_bug.cgi?id=712519
        if error:
            tag_name = "error"
        else:
            tag_name = "warning"

        buffer.apply_tag_by_name(tag_name, start, end)

    def css_text_changed(self, buffer, provider):
        start = buffer.get_start_iter()
        end = buffer.get_end_iter()
        buffer.remove_all_tags(start, end)

        text = buffer.get_text(start, end, False)

        # Ignore CSS errors as they are shown by highlighting
        try:
            provider.load_from_data(text.encode('utf-8'))
        except GLib.GError as e:
            if e.domain != 'gtk-css-provider-error-quark':
                raise e

        Gtk.StyleContext.reset_widgets(Gdk.Screen.get_default())


def main(demoapp=None):
    CSSMultiplebgsApp(demoapp)
    Gtk.main()


if __name__ == '__main__':
    import os
    base_path = os.path.abspath(os.path.dirname(__file__))
    resource_path = os.path.join(base_path, '../data/demo.gresource')
    resource = Gio.Resource.load(resource_path)

    # FIXME: method register() should be without the underscore
    # FIXME: see https://bugzilla.gnome.org/show_bug.cgi?id=684319
    resource._register()
    main()

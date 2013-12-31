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

title = "CSS Basics"
description = """
Gtk themes are written using CSS. Every widget is build of multiple items
that you can style very similarly to a regular website.
"""

import os
from gi.repository import Gtk, Gdk, Pango, Gio, GLib


class CSSBasicsApp:
    def __init__(self, demoapp):
        self.demoapp = demoapp
        #: Store the last successful parsing of the css so we can revert
        #: this in case of an error.
        self.last_good_text = ''
        #: Set when we receive a parsing-error callback. This is needed
        #: to handle logic after a parsing-error callback which does not raise
        #: an exception with provider.load_from_data()
        self.last_error_code = 0

        self.window = Gtk.Window()
        self.window.set_title('CSS Basics')
        self.window.set_default_size(400, 300)
        self.window.set_border_width(10)
        self.window.connect('destroy', lambda w: Gtk.main_quit())

        self.infobar = Gtk.InfoBar()
        self.infolabel = Gtk.Label()
        self.infobar.get_content_area().pack_start(self.infolabel, False, False, 0)
        self.infobar.set_message_type(Gtk.MessageType.WARNING)

        scrolled = Gtk.ScrolledWindow()
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        box.pack_start(scrolled, expand=True, fill=True, padding=0)
        box.pack_start(self.infobar, expand=False, fill=True, padding=0)
        self.window.add(box)

        provider = Gtk.CssProvider()

        buffer = Gtk.TextBuffer()
        buffer.create_tag(tag_name="warning", underline=Pango.Underline.SINGLE)
        buffer.create_tag(tag_name="error", underline=Pango.Underline.ERROR)
        buffer.connect("changed", self.css_text_changed, provider)

        provider.connect("parsing-error", self.show_parsing_error, buffer)

        textview = Gtk.TextView()
        textview.set_buffer(buffer)
        scrolled.add(textview)

        bytes = Gio.resources_lookup_data("/css_basics/css_basics.css", 0)
        buffer.set_text(bytes.get_data().decode('utf-8'))

        self.apply_css(self.window, provider)
        self.window.show_all()
        self.infobar.hide()

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

        if error.code == Gtk.CssProviderError.DEPRECATED:
            tag_name = "warning"
        else:
            tag_name = "error"
        self.last_error_code = error.code

        self.infolabel.set_text(error.message)
        self.infobar.show_all()

        buffer.apply_tag_by_name(tag_name, start, end)

    def css_text_changed(self, buffer, provider):
        start = buffer.get_start_iter()
        end = buffer.get_end_iter()
        buffer.remove_all_tags(start, end)

        text = buffer.get_text(start, end, False).encode('utf-8')

        # Ignore CSS errors as they are shown by highlighting
        try:
            provider.load_from_data(text)
        except GLib.GError as e:
            if e.domain != 'gtk-css-provider-error-quark':
                raise e

        # If the parsing-error callback is ever run (even in the case of warnings)
        # load the last good css text that ran without any warnings. Otherwise
        # we may have a discrepancy in "last_good_text" vs the current buffer
        # causing section.get_start_position() to give back an invalid position
        # for the editor buffer.
        if self.last_error_code:
            provider.load_from_data(self.last_good_text)
            self.last_error_code = 0
        else:
            self.last_good_text = text
            self.infobar.hide()

        Gtk.StyleContext.reset_widgets(Gdk.Screen.get_default())


def main(demoapp=None):
    CSSBasicsApp(demoapp)
    Gtk.main()


if __name__ == '__main__':
    base_path = os.path.abspath(os.path.dirname(__file__))
    resource_path = os.path.join(base_path, '../data/demo.gresource')
    resource = Gio.Resource.load(resource_path)

    # FIXME: method register() should be without the underscore
    # FIXME: see https://bugzilla.gnome.org/show_bug.cgi?id=684319
    resource._register()
    main()

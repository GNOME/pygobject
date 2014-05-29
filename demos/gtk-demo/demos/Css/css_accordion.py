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

title = "CSS Accordion"
description = """
A simple accordion demo written using CSS transitions and multiple backgrounds.
"""


from gi.repository import Gtk, Gio


class CSSAccordionApp:
    def __init__(self):
        window = Gtk.Window()
        window.set_title('CSS Accordion')
        window.set_default_size(600, 300)
        window.set_border_width(10)
        window.connect('destroy', Gtk.main_quit)

        hbox = Gtk.Box(homogeneous=False, spacing=2,
                       orientation=Gtk.Orientation.HORIZONTAL)
        hbox.set_halign(Gtk.Align.CENTER)
        hbox.set_valign(Gtk.Align.CENTER)
        window.add(hbox)

        for label in ('This', 'Is', 'A', 'CSS', 'Accordion', ':-)'):
            hbox.add(Gtk.Button(label=label))

        bytes = Gio.resources_lookup_data("/css_accordion/css_accordion.css", 0)

        provider = Gtk.CssProvider()
        provider.load_from_data(bytes.get_data())

        self.apply_css(window, provider)

        window.show_all()

    def apply_css(self, widget, provider):
        Gtk.StyleContext.add_provider(widget.get_style_context(),
                                      provider,
                                      Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        if isinstance(widget, Gtk.Container):
            widget.forall(self.apply_css, provider)


def main(demoapp=None):
    CSSAccordionApp()
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

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

title = "Links"
description = """
GtkLabel can show hyperlinks. The default action is to call gtk_show_uri() on
their URI, but it is possible to override this with a custom handler.
"""

from gi.repository import Gtk


class LinksApp:
    def __init__(self):
        self.window = Gtk.Window()
        self.window.set_title('Links')
        self.window.set_border_width(12)
        self.window.connect('destroy', Gtk.main_quit)

        label = Gtk.Label(label="""Some <a href="http://en.wikipedia.org/wiki/Text"
title="plain text">text</a> may be marked up
as hyperlinks, which can be clicked
or activated via <a href="keynav">keynav</a>""")

        label.set_use_markup(True)
        label.connect("activate-link", self.activate_link)
        self.window.add(label)
        label.show()

        self.window.show()

    def activate_link(self, label, uri):
        if uri == 'keynav':
            parent = label.get_toplevel()
            markup = """The term <i>keynav</i> is a shorthand for
keyboard navigation and refers to the process of using
a program (exclusively) via keyboard input."""
            dialog = Gtk.MessageDialog(transient_for=parent,
                                       destroy_with_parent=True,
                                       message_type=Gtk.MessageType.INFO,
                                       buttons=Gtk.ButtonsType.OK,
                                       text=markup,
                                       use_markup=True)
            dialog.present()
            dialog.connect('response', self.response_cb)

            return True

    def response_cb(self, dialog, response_id):
        dialog.destroy()


def main(demoapp=None):
    LinksApp()
    Gtk.main()

if __name__ == '__main__':
    main()

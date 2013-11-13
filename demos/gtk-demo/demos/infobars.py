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

title = "Info Bars"
description = """
Info bar widgets are used to report important messages to the user.
"""

from gi.repository import Gtk


class InfobarApp:
    def __init__(self):
        self.window = Gtk.Window()
        self.window.set_title('Info Bars')
        self.window.set_border_width(8)
        self.window.connect('destroy', Gtk.main_quit)

        vbox = Gtk.VBox(spacing=0)
        self.window.add(vbox)

        bar = Gtk.InfoBar()
        vbox.pack_start(bar, False, False, 0)
        bar.set_message_type(Gtk.MessageType.INFO)
        label = Gtk.Label(label='This is an info bar with message type Gtk.MessageType.INFO')
        bar.get_content_area().pack_start(label, False, False, 0)

        bar = Gtk.InfoBar()
        vbox.pack_start(bar, False, False, 0)
        bar.set_message_type(Gtk.MessageType.WARNING)
        label = Gtk.Label(label='This is an info bar with message type Gtk.MessageType.WARNING')
        bar.get_content_area().pack_start(label, False, False, 0)

        bar = Gtk.InfoBar()
        bar.add_button(Gtk.STOCK_OK, Gtk.ResponseType.OK)
        bar.connect('response', self.on_bar_response)
        vbox.pack_start(bar, False, False, 0)
        bar.set_message_type(Gtk.MessageType.QUESTION)
        label = Gtk.Label(label='This is an info bar with message type Gtk.MessageType.QUESTION')
        bar.get_content_area().pack_start(label, False, False, 0)

        bar = Gtk.InfoBar()
        vbox.pack_start(bar, False, False, 0)
        bar.set_message_type(Gtk.MessageType.ERROR)
        label = Gtk.Label(label='This is an info bar with message type Gtk.MessageType.ERROR')
        bar.get_content_area().pack_start(label, False, False, 0)

        bar = Gtk.InfoBar()
        vbox.pack_start(bar, False, False, 0)
        bar.set_message_type(Gtk.MessageType.OTHER)
        label = Gtk.Label(label='This is an info bar with message type Gtk.MessageType.OTHER')
        bar.get_content_area().pack_start(label, False, False, 0)

        frame = Gtk.Frame(label="Info bars")
        vbox.pack_start(frame, False, False, 8)

        vbox2 = Gtk.VBox(spacing=8)
        vbox2.set_border_width(8)
        frame.add(vbox2)

        # Standard message dialog
        label = Gtk.Label(label='An example of different info bars')
        vbox2.pack_start(label, False, False, 0)

        self.window.show_all()

    def on_bar_response(self, info_bar, response_id):
        dialog = Gtk.MessageDialog(transient_for=self.window,
                                   modal=True,
                                   destroy_with_parent=True,
                                   message_type=Gtk.MessageType.INFO,
                                   buttons=Gtk.ButtonsType.OK,
                                   text='You clicked on an info bar')
        dialog.format_secondary_text('Your response has id %d' % response_id)
        dialog.run()
        dialog.destroy()


def main(demoapp=None):
    InfobarApp()
    Gtk.main()

if __name__ == '__main__':
    main()

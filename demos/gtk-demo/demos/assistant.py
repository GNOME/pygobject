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

title = "Assistant"
description = """
Demonstrates a sample multistep assistant. Assistants are used to divide
an operation into several simpler sequential steps, and to guide the user
through these steps.
"""


from gi.repository import Gtk


class AssistantApp:
    def __init__(self):
        self.assistant = Gtk.Assistant()
        self.assistant.set_default_size(-1, 300)

        self.create_page1()
        self.create_page2()
        self.create_page3()

        self.assistant.connect('cancel', self.on_close_cancel)
        self.assistant.connect('close', self.on_close_cancel)
        self.assistant.connect('apply', self.on_apply)
        self.assistant.connect('prepare', self.on_prepare)

        self.assistant.show()

    def on_close_cancel(self, assistant):
        assistant.destroy()
        Gtk.main_quit()

    def on_apply(self, assistant):
        # apply changes here; this is a fictional example so just do
        # nothing here
        pass

    def on_prepare(self, assistant, page):
        current_page = assistant.get_current_page()
        n_pages = assistant.get_n_pages()
        title = 'Sample assistant (%d of %d)' % (current_page + 1, n_pages)
        assistant.set_title(title)

    def on_entry_changed(self, widget):
        page_number = self.assistant.get_current_page()
        current_page = self.assistant.get_nth_page(page_number)
        text = widget.get_text()

        if text:
            self.assistant.set_page_complete(current_page, True)
        else:
            self.assistant.set_page_complete(current_page, False)

    def create_page1(self):
        box = Gtk.HBox(homogeneous=False,
                       spacing=12)
        box.set_border_width(12)
        label = Gtk.Label(label='You must fill out this entry to continue:')
        box.pack_start(label, False, False, 0)

        entry = Gtk.Entry()
        box.pack_start(entry, True, True, 0)
        entry.connect('changed', self.on_entry_changed)

        box.show_all()
        self.assistant.append_page(box)
        self.assistant.set_page_title(box, 'Page 1')
        self.assistant.set_page_type(box, Gtk.AssistantPageType.INTRO)

        pixbuf = self.assistant.render_icon(Gtk.STOCK_DIALOG_INFO,
                                            Gtk.IconSize.DIALOG,
                                            None)

        self.assistant.set_page_header_image(box, pixbuf)

    def create_page2(self):
        box = Gtk.VBox(homogeneous=False,
                       spacing=12)
        box.set_border_width(12)

        checkbutton = Gtk.CheckButton(label='This is optional data, you may continue even if you do not check this')
        box.pack_start(checkbutton, False, False, 0)

        box.show_all()

        self.assistant.append_page(box)
        self.assistant.set_page_complete(box, True)
        self.assistant.set_page_title(box, 'Page 2')

        pixbuf = self.assistant.render_icon(Gtk.STOCK_DIALOG_INFO,
                                            Gtk.IconSize.DIALOG,
                                            None)
        self.assistant.set_page_header_image(box, pixbuf)

    def create_page3(self):
        label = Gtk.Label(label='This is a confirmation page, press "Apply" to apply changes')
        label.show()
        self.assistant.append_page(label)
        self.assistant.set_page_complete(label, True)
        self.assistant.set_page_title(label, 'Confirmation')
        self.assistant.set_page_type(label, Gtk.AssistantPageType.CONFIRM)

        pixbuf = self.assistant.render_icon(Gtk.STOCK_DIALOG_INFO,
                                            Gtk.IconSize.DIALOG,
                                            None)
        self.assistant.set_page_header_image(label, pixbuf)


def main(demoapp=None):
    AssistantApp()
    Gtk.main()

if __name__ == '__main__':
    main()

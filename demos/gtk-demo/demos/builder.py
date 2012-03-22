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

title = "Builder"
description = """
Demonstrates an interface loaded from a XML description.
"""


import os

from gi.repository import Gtk


class BuilderApp:
    def __init__(self, demoapp):
        self.demoapp = demoapp

        self.builder = Gtk.Builder()
        if demoapp is None:
            filename = os.path.join('data', 'demo.ui')
        else:
            filename = demoapp.find_file('demo.ui')

        self.builder.add_from_file(filename)
        self.builder.connect_signals(self)

        window = self.builder.get_object('window1')
        window.connect('destroy', lambda x: Gtk.main_quit())
        window.show_all()

    def about_activate(self, action):
        about_dlg = self.builder.get_object('aboutdialog1')
        about_dlg.run()
        about_dlg.hide()

    def quit_activate(self, action):
        Gtk.main_quit()


def main(demoapp=None):
    BuilderApp(demoapp)
    Gtk.main()

if __name__ == '__main__':
    main()

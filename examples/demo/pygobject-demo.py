#!/usr/bin/env python3
# pygobject-demo.py
#
# Copyright 2018 Carlos Soriano <csoriano@redhat.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later

import gi
gi.require_version('Gtk', '4.0')
from gi.repository import Gtk, Gio

from typing import Dict, Any
import appwindow

class PyDemoApplication(Gtk.Application):
    __gtype_name__ = 'PyDemoApplication'

    def __init__(self) -> None:
        super().__init__(application_id='org.gnome.pygobject-demo',
                         flags=Gio.ApplicationFlags.FLAGS_NONE)

    def do_activate(self) -> None:
        win: Gtk.Window = self.get_active_window()
        if not win:
            win = PyDemoWindow(application=self)
        win.present()

class PyDemoWindow(Gtk.ApplicationWindow):
    __gtype_name__ = 'PyDemoWindow'

    def __init__(self, **kwargs: Dict[str, Any]) -> None:
        super().__init__(**kwargs)
        self.set_default_size(600, 400)
        self._main_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        self.add(self._main_box)
        self._sidebar = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self._main_box.add(self._sidebar)
        self._stack = Gtk.Stack()
        self._main_box.add(self._stack)

        button = Gtk.Button('Basics')
        button.connect("clicked", lambda _: self._stack.set_visible_child_name('basics'))
        button.vexpand = False
        button.valign = Gtk.Align.START
        self._sidebar.add(button)
        self._stack.add_named(appwindow.get_content(), 'basics')


def main(demoapp=None):
    app = PyDemoApplication()

    return app.run()


if __name__ == '__main__':
    main()

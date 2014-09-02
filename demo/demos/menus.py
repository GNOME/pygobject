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

title = "Menus"
description = """There are several widgets involved in displaying menus. The
GtkMenuBar widget is a menu bar, which normally appears horizontally at the top
of an application, but can also be layed out vertically. The GtkMenu widget is
the actual menu that pops up. Both GtkMenuBar and GtkMenu are subclasses of
GtkMenuShell; a GtkMenuShell contains menu items (GtkMenuItem). Each menu item
contains text and/or images and can be selected by the user. There are several
kinds of menu item, including plain GtkMenuItem, GtkCheckMenuItem which can be
checked/unchecked, GtkRadioMenuItem which is a check menu item that's in a
mutually exclusive group, GtkSeparatorMenuItem which is a separator bar,
GtkTearoffMenuItem which allows a GtkMenu to be torn off, and GtkImageMenuItem
which can place a GtkImage or other widget next to the menu text. A GtkMenuItem
can have a submenu, which is simply a GtkMenu to pop up when the menu item is
selected. Typically, all menu items in a menu bar have submenus. GtkUIManager
provides a higher-level interface for creating menu bars and menus; while you
can construct menus manually, most people don't do that. There's a separate demo
for GtkUIManager.
"""

from gi.repository import Gtk


class MenusApp:
    def __init__(self):
        self.window = Gtk.Window()
        self.window.set_title('Menus')
        self.window.connect('destroy', Gtk.main_quit)

        accel_group = Gtk.AccelGroup()
        self.window.add_accel_group(accel_group)
        self.window.set_border_width(0)

        box = Gtk.HBox()
        self.window.add(box)

        box1 = Gtk.VBox()
        box.add(box1)

        menubar = Gtk.MenuBar()
        box1.pack_start(menubar, False, True, 0)

        menuitem = Gtk.MenuItem(label='test\nline2')
        menuitem.set_submenu(self.create_menu(3, True))
        menubar.append(menuitem)

        menuitem = Gtk.MenuItem(label='foo')
        menuitem.set_submenu(self.create_menu(4, True))
        menuitem.set_right_justified(True)
        menubar.append(menuitem)

        box2 = Gtk.VBox(spacing=10)
        box2.set_border_width(10)
        box1.pack_start(box2, False, True, 0)

        button = Gtk.Button(label='Flip')
        button.connect('clicked', self.change_orientation, menubar)
        box2.pack_start(button, True, True, 0)

        button = Gtk.Button(label='Close')
        button.connect('clicked', lambda x: self.window.destroy())
        box2.pack_start(button, True, True, 0)
        button.set_can_default(True)

        self.window.show_all()

    def create_menu(self, depth, tearoff):
        if depth < 1:
            return None

        menu = Gtk.Menu()

        if tearoff:
            menuitem = Gtk.TearoffMenuItem()
            menu.append(menuitem)

        i = 0
        j = 1
        while i < 5:
            label = 'item %2d - %d' % (depth, j)
            # we should be adding this to a group but the group API
            # isn't bindable - we need something more like the
            # Gtk.RadioAction API
            menuitem = Gtk.RadioMenuItem(label=label)
            menu.append(menuitem)

            if i == 3:
                menuitem.set_sensitive(False)

            menuitem.set_submenu(self.create_menu(depth - 1, True))

            i += 1
            j += 1

        return menu

    def change_orientation(self, button, menubar):
        parent = menubar.get_parent()
        orientation = parent.get_orientation()
        parent.set_orientation(1 - orientation)

        if orientation == Gtk.Orientation.VERTICAL:
            menubar.props.pack_direction = Gtk.PackDirection.TTB
        else:
            menubar.props.pack_direction = Gtk.PackDirection.LTR


def main(demoapp=None):
    MenusApp()
    Gtk.main()

if __name__ == '__main__':
    main()

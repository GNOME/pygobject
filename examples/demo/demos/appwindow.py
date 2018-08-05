#!/usr/bin/env python3
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

title = "Application main window"
description = """
Demonstrates a typical application window with menubar, toolbar, statusbar.
"""

import os
import sys

######## EVIL #######
sys.path.remove('/usr/lib/python3.7/site-packages')
#####################

import gi
gi.require_version('Gtk', '4.0')
from gi.repository import Gtk


infobar = None
window = None
messagelabel = None
_demoapp = None


def widget_destroy(widget, button):
    widget.destroy()


def activate_action(action, user_data=None):
    global window

    name = action.get_name()
    _type = type(action)
    if name == 'DarkTheme':
        value = action.get_active()
        settings = Gtk.Settings.get_default()
        settings.set_property('gtk-application-prefer-dark-theme', value)
        return

    dialog = Gtk.MessageDialog(message_type=Gtk.MessageType.INFO,
                               buttons=Gtk.ButtonsType.CLOSE,
                               text='You activated action: "%s" of type %s' % (name, _type))

    # FIXME: this should be done in the constructor
    dialog.set_transient_for(window)
    dialog.connect('response', widget_destroy)
    dialog.show()


def activate_radio_action(action, current, user_data=None):
    global infobar
    global messagelabel

    name = current.get_name()
    _type = type(current)
    active = current.get_active()
    value = current.get_current_value()
    if active:
        text = 'You activated radio action: "%s" of type %s.\n Current value: %d' % (name, _type, value)
        messagelabel.set_text(text)
        infobar.set_message_type(Gtk.MessageType(value))
        infobar.show()


def update_statusbar(buffer, statusbar):
    statusbar.pop(0)
    count = buffer.get_char_count()

    iter = buffer.get_iter_at_mark(buffer.get_insert())
    row = iter.get_line()
    col = iter.get_line_offset()
    msg = 'Cursor at row %d column %d - %d chars in document' % (row, col, count)

    statusbar.push(0, msg)


def mark_set_callback(buffer, new_location, mark, data):
    update_statusbar(buffer, data)


def about_cb(widget, user_data=None):
    global window

    authors = ['John (J5) Palmieri',
               'Tomeu Vizoso',
               'and many more...']

    documentors = ['David Malcolm',
                   'Zack Goldberg',
                   'and many more...']

    license = """
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the Gnome Library; see the file COPYING.LIB.  If not,
write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.
"""
    dirname = os.path.abspath(os.path.dirname(__file__))
    filename = os.path.join(dirname, 'data', 'gtk-logo-rgb.gif')
    texture = Gtk.Texture.new_from_file(filename)

    about = Gtk.AboutDialog(parent=window,
                            program_name='GTK+ Code Demos',
                            version='0.1',
                            copyright='(C) 2010 The PyGI Team',
                            license=license,
                            website='http://live.gnome.org/PyGI',
                            comments='Program to demonstrate PyGI functions.',
                            authors=authors,
                            documenters=documentors,
                            logo=texture,
                            title='About GTK+ Code Demos')

    about.connect('response', widget_destroy)
    about.show()


action_entries = (
    ("FileMenu", None, "_File"),                # name, stock id, label
    ("OpenMenu", None, "_Open"),                # name, stock id, label
    ("PreferencesMenu", None, "_Preferences"),  # name, stock id, label
    ("ColorMenu", None, "_Color"),              # name, stock id, label
    ("ShapeMenu", None, "_Shape"),              # name, stock id, label
    ("HelpMenu", None, "_Help"),                # name, stock id, label
    ("New", "document-new",                     # name, stock id
     "_New", "<control>N",                      # label, accelerator
     "Create a new file",                       # tooltip
     activate_action),
    ("File1", None,                             # name, stock id
     "File1", None,                             # label, accelerator
     "Open first file",                         # tooltip
     activate_action),
    ("Save", "document-save",                    # name, stock id
     "_Save", "<control>S",                     # label, accelerator
     "Save current file",                       # tooltip
     activate_action),
    ("SaveAs", "document-save-as",              # name, stock id
     "Save _As...", None,                       # label, accelerator
     "Save to a file",                          # tooltip
     activate_action),
    ("Quit", "application-exit",                # name, stock id
     "_Quit", "<control>Q",                     # label, accelerator
     "Quit",                                    # tooltip
     activate_action),
    ("About", None,                             # name, stock id
     "_About", "<control>A",                    # label, accelerator
     "About",                                   # tooltip
     about_cb),
    ("Logo", "demo-gtk-logo",                   # name, stock id
     None, None,                                # label, accelerator
     "GTK+",                                    # tooltip
     activate_action),
)

toggle_action_entries = (
    ("Bold", "format-text-bold",                # name, stock id
     "_Bold", "<control>B",                     # label, accelerator
     "Bold",                                    # tooltip
     activate_action,
     True),                                     # is_active
    ("DarkTheme", None,                         # name, stock id
     "_Prefer Dark Theme", None,                # label, accelerator
     "Prefer Dark Theme",                       # tooltip
     activate_action,
     False),                                    # is_active
)

(COLOR_RED,
 COLOR_GREEN,
 COLOR_BLUE) = range(3)

color_action_entries = (
    ("Red", None,                               # name, stock id
     "_Red", "<control>R",                      # label, accelerator
     "Blood", COLOR_RED),                       # tooltip, value
    ("Green", None,                             # name, stock id
     "_Green", "<control>G",                    # label, accelerator
     "Grass", COLOR_GREEN),                     # tooltip, value
    ("Blue", None,                              # name, stock id
     "_Blue", "<control>B",                     # label, accelerator
     "Sky", COLOR_BLUE),                        # tooltip, value
)

(SHAPE_SQUARE,
 SHAPE_RECTANGLE,
 SHAPE_OVAL) = range(3)

shape_action_entries = (
    ("Square", None,                            # name, stock id
     "_Square", "<control>S",                   # label, accelerator
     "Square", SHAPE_SQUARE),                   # tooltip, value
    ("Rectangle", None,                         # name, stock id
     "_Rectangle", "<control>R",                # label, accelerator
     "Rectangle", SHAPE_RECTANGLE),             # tooltip, value
    ("Oval", None,                              # name, stock id
     "_Oval", "<control>O",                     # label, accelerator
     "Egg", SHAPE_OVAL),                        # tooltip, value
)

ui_info = """
<ui>
  <menubar name='MenuBar'>
    <menu action='FileMenu'>
      <menuitem action='New'/>
      <menuitem action='Open'/>
      <menuitem action='Save'/>
      <menuitem action='SaveAs'/>
      <separator/>
      <menuitem action='Quit'/>
    </menu>
    <menu action='PreferencesMenu'>
      <menuitem action='DarkTheme'/>
      <menu action='ColorMenu'>
    <menuitem action='Red'/>
    <menuitem action='Green'/>
    <menuitem action='Blue'/>
      </menu>
      <menu action='ShapeMenu'>
        <menuitem action='Square'/>
        <menuitem action='Rectangle'/>
        <menuitem action='Oval'/>
      </menu>
      <menuitem action='Bold'/>
    </menu>
    <menu action='HelpMenu'>
      <menuitem action='About'/>
    </menu>
  </menubar>
  <toolbar name='ToolBar'>
    <toolitem action='Open'>
      <menu action='OpenMenu'>
        <menuitem action='File1'/>
      </menu>
    </toolitem>
    <toolitem action='Quit'/>
    <separator action='Sep1'/>
    <toolitem action='Logo'/>
  </toolbar>
</ui>
"""


def _quit(*args):
    Gtk.main_quit()

def main(demoapp=None):
    global infobar
    global window
    global messagelabel
    global _demoapp

    _demoapp = demoapp

    window = Gtk.Window()
    window.set_title('Application Window')
    window.set_icon_name('gtk-open')
    window.set_default_size(200, 200)
    window.connect_after('destroy', _quit)
    button = Gtk.Button.new_with_label("Test")
    window.add(button)

    window.show()
    Gtk.main()


if __name__ == '__main__':
    main()

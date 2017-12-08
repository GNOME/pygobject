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

title = "List Store"
description = """
The GtkListStore is used to store data in list form, to be used later on by a
GtkTreeView to display it. This demo builds a simple GtkListStore and displays
it. See the Stock Browser demo for a more advanced example.
"""


from gi.repository import Gtk, GObject, GLib


class Bug:
    def __init__(self, is_fixed, number, severity, description):
        self.is_fixed = is_fixed
        self.number = number
        self.severity = severity
        self.description = description


# initial data we use to fill in the store
data = [Bug(False, 60482, "Normal", "scrollable notebooks and hidden tabs"),
        Bug(False, 60620, "Critical", "gdk_window_clear_area (gdkwindow-win32.c) is not thread-safe"),
        Bug(False, 50214, "Major", "Xft support does not clean up correctly"),
        Bug(True, 52877, "Major", "GtkFileSelection needs a refresh method. "),
        Bug(False, 56070, "Normal", "Can't click button after setting in sensitive"),
        Bug(True, 56355, "Normal", "GtkLabel - Not all changes propagate correctly"),
        Bug(False, 50055, "Normal", "Rework width/height computations for TreeView"),
        Bug(False, 58278, "Normal", "gtk_dialog_set_response_sensitive () doesn't work"),
        Bug(False, 55767, "Normal", "Getters for all setters"),
        Bug(False, 56925, "Normal", "Gtkcalender size"),
        Bug(False, 56221, "Normal", "Selectable label needs right-click copy menu"),
        Bug(True, 50939, "Normal", "Add shift clicking to GtkTextView"),
        Bug(False, 6112, "Enhancement", "netscape-like collapsable toolbars"),
        Bug(False, 1, "Normal", "First bug :=)")]


class ListStoreApp:
    (COLUMN_FIXED,
     COLUMN_NUMBER,
     COLUMN_SEVERITY,
     COLUMN_DESCRIPTION,
     COLUMN_PULSE,
     COLUMN_ICON,
     COLUMN_ACTIVE,
     COLUMN_SENSITIVE,
     NUM_COLUMNS) = range(9)

    def __init__(self):
        self.window = Gtk.Window()
        self.window.set_title('Gtk.ListStore Demo')
        self.window.connect('destroy', Gtk.main_quit)

        vbox = Gtk.VBox(spacing=8)
        self.window.add(vbox)

        label = Gtk.Label(label='This is the bug list (note: not based on real data, it would be nice to have a nice ODBC interface to bugzilla or so, though).')
        vbox.pack_start(label, False, False, 0)

        sw = Gtk.ScrolledWindow()
        sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN)
        sw.set_policy(Gtk.PolicyType.NEVER,
                      Gtk.PolicyType.AUTOMATIC)
        vbox.pack_start(sw, True, True, 0)

        self.create_model()
        treeview = Gtk.TreeView(model=self.model)
        treeview.set_rules_hint(True)
        treeview.set_search_column(self.COLUMN_DESCRIPTION)
        sw.add(treeview)

        self.add_columns(treeview)

        self.window.set_default_size(280, 250)
        self.window.show_all()

        self.window.connect('delete-event', self.window_closed)
        self.timeout = GLib.timeout_add(80, self.spinner_timeout)

    def window_closed(self, window, event):
        if self.timeout != 0:
            GLib.source_remove(self.timeout)

    def spinner_timeout(self):
        if self.model is None:
            return False

        iter_ = self.model.get_iter_first()
        pulse = self.model.get(iter_, self.COLUMN_PULSE)[0]
        if pulse == 999999999:
            pulse = 0
        else:
            pulse += 1

        self.model.set_value(iter_, self.COLUMN_PULSE, pulse)
        self.model.set_value(iter_, self.COLUMN_ACTIVE, True)

        return True

    def create_model(self):
        self.model = Gtk.ListStore(bool,
                                   GObject.TYPE_INT,
                                   str,
                                   str,
                                   GObject.TYPE_INT,
                                   str,
                                   bool,
                                   bool)

        col = 0
        for bug in data:
            if col == 1 or col == 3:
                icon_name = 'battery-critical-charging-symbolic'
            else:
                icon_name = ''
            if col == 3:
                is_sensitive = False
            else:
                is_sensitive = True

            self.model.append([bug.is_fixed,
                               bug.number,
                               bug.severity,
                               bug.description,
                               0,
                               icon_name,
                               False,
                               is_sensitive])
            col += 1

    def add_columns(self, treeview):
        model = treeview.get_model()

        # column for is_fixed toggle
        renderer = Gtk.CellRendererToggle()
        renderer.connect('toggled', self.is_fixed_toggled, model)

        column = Gtk.TreeViewColumn("Fixed?", renderer,
                                    active=self.COLUMN_FIXED)
        column.set_fixed_width(50)
        column.set_sizing(Gtk.TreeViewColumnSizing.FIXED)
        treeview.append_column(column)

        # column for severities
        renderer = Gtk.CellRendererText()
        column = Gtk.TreeViewColumn("Severity", renderer,
                                    text=self.COLUMN_SEVERITY)
        column.set_sort_column_id(self.COLUMN_SEVERITY)
        treeview.append_column(column)

        # column for description
        renderer = Gtk.CellRendererText()
        column = Gtk.TreeViewColumn("Description", renderer,
                                    text=self.COLUMN_DESCRIPTION)
        column.set_sort_column_id(self.COLUMN_DESCRIPTION)
        treeview.append_column(column)

        # column for spinner
        renderer = Gtk.CellRendererSpinner()
        column = Gtk.TreeViewColumn("Spinning", renderer,
                                    pulse=self.COLUMN_PULSE,
                                    active=self.COLUMN_ACTIVE)
        column.set_sort_column_id(self.COLUMN_PULSE)
        treeview.append_column(column)

        # column for symbolic icon
        renderer = Gtk.CellRendererPixbuf()
        renderer.props.follow_state = True
        column = Gtk.TreeViewColumn("Symbolic icon", renderer,
                                    icon_name=self.COLUMN_ICON,
                                    sensitive=self.COLUMN_SENSITIVE)
        column.set_sort_column_id(self.COLUMN_ICON)
        treeview.append_column(column)

    def is_fixed_toggled(self, cell, path_str, model):
        # get toggled iter
        iter_ = model.get_iter(path_str)
        is_fixed = model.get_value(iter_, self.COLUMN_FIXED)

        # do something with value
        is_fixed ^= 1

        model.set_value(iter_, self.COLUMN_FIXED, is_fixed)


def main(demoapp=None):
    ListStoreApp()
    Gtk.main()


if __name__ == '__main__':
    main()

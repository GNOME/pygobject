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

title = "Combo boxes"
description = """
The ComboBox widget allows to select one option out of a list.
The ComboBoxEntry additionally allows the user to enter a value
that is not in the list of options.

How the options are displayed is controlled by cell renderers.
 """


from gi.repository import Gtk, Gdk, GdkPixbuf, GLib, GObject


(PIXBUF_COL,
 TEXT_COL) = range(2)


class MaskEntry(Gtk.Entry):
    __gtype_name__ = 'MaskEntry'

    def __init__(self, mask=None):
        self.mask = mask
        super(MaskEntry, self).__init__()

        self.connect('changed', self.changed_cb)

        self.error_color = Gdk.RGBA()
        self.error_color.red = 1.0
        self.error_color.green = 0.9
        self.error_color_blue = 0.9
        self.error_color.alpha = 1.0

        # workaround since override_color doesn't accept None yet
        style_ctx = self.get_style_context()
        self.normal_color = style_ctx.get_color(0)

    def set_background(self):
        if self.mask:
            if not GLib.regex_match_simple(self.mask,
                                           self.get_text(), 0, 0):
                self.override_color(0, self.error_color)
                return

        self.override_color(0, self.normal_color)

    def changed_cb(self, entry):
        self.set_background()


class ComboboxApp:
    def __init__(self, demoapp):
        self.demoapp = demoapp

        self.window = Gtk.Window()
        self.window.set_title('Combo boxes')
        self.window.set_border_width(10)
        self.window.connect('destroy', lambda w: Gtk.main_quit())

        vbox = Gtk.VBox(homogeneous=False, spacing=2)
        self.window.add(vbox)

        frame = Gtk.Frame(label='Some stock icons')
        vbox.pack_start(frame, False, False, 0)

        box = Gtk.VBox(homogeneous=False, spacing=0)
        box.set_border_width(5)
        frame.add(box)

        model = self.create_stock_icon_store()
        combo = Gtk.ComboBox(model=model)
        box.add(combo)

        renderer = Gtk.CellRendererPixbuf()
        combo.pack_start(renderer, False)

        # FIXME: override set_attributes
        combo.add_attribute(renderer, 'pixbuf', PIXBUF_COL)
        combo.set_cell_data_func(renderer, self.set_sensitive, None)

        renderer = Gtk.CellRendererText()
        combo.pack_start(renderer, True)
        combo.add_attribute(renderer, 'text', TEXT_COL)
        combo.set_cell_data_func(renderer, self.set_sensitive, None)

        combo.set_row_separator_func(self.is_separator, None)
        combo.set_active(0)

        # a combobox demonstrating trees
        frame = Gtk.Frame(label='Where are we ?')
        vbox.pack_start(frame, False, False, 0)

        box = Gtk.VBox(homogeneous=False, spacing=0)
        box.set_border_width(5)
        frame.add(box)

        model = self.create_capital_store()
        combo = Gtk.ComboBox(model=model)
        box.add(combo)

        renderer = Gtk.CellRendererText()
        combo.pack_start(renderer, True)
        combo.add_attribute(renderer, 'text', 0)
        combo.set_cell_data_func(renderer, self.is_capital_sensistive, None)

        # FIXME: make new_from_indices work
        #        make constructor take list or string of indices
        path = Gtk.TreePath.new_from_string('0:8')
        treeiter = model.get_iter(path)
        combo.set_active_iter(treeiter)

        # A GtkComboBoxEntry with validation.

        frame = Gtk.Frame(label='Editable')
        vbox.pack_start(frame, False, False, 0)

        box = Gtk.VBox(homogeneous=False, spacing=0)
        box.set_border_width(5)
        frame.add(box)

        combo = Gtk.ComboBoxText.new_with_entry()
        self.fill_combo_entry(combo)
        box.add(combo)

        entry = MaskEntry(mask='^([0-9]*|One|Two|2\302\275|Three)$')

        Gtk.Container.remove(combo, combo.get_child())
        combo.add(entry)

        # A combobox with string IDs

        frame = Gtk.Frame(label='String IDs')
        vbox.pack_start(frame, False, False, 0)

        box = Gtk.VBox(homogeneous=False, spacing=0)
        box.set_border_width(5)
        frame.add(box)

        # FIXME: model is not setup when constructing Gtk.ComboBoxText()
        #        so we call new() - Gtk should fix this to setup the model
        #        in __init__, not in the constructor
        combo = Gtk.ComboBoxText.new()
        combo.append('never', 'Not visible')
        combo.append('when-active', 'Visible when active')
        combo.append('always', 'Always visible')
        box.add(combo)

        entry = Gtk.Entry()

        combo.bind_property('active-id',
                            entry, 'text',
                            GObject.BindingFlags.BIDIRECTIONAL)

        box.add(entry)
        self.window.show_all()

    def strip_underscore(self, s):
        return s.replace('_', '')

    def create_stock_icon_store(self):
        stock_id = (Gtk.STOCK_DIALOG_WARNING,
                    Gtk.STOCK_STOP,
                    Gtk.STOCK_NEW,
                    Gtk.STOCK_CLEAR,
                    None,
                    Gtk.STOCK_OPEN)

        cellview = Gtk.CellView()
        store = Gtk.ListStore(GdkPixbuf.Pixbuf, str)

        for id in stock_id:
            if id is not None:
                pixbuf = cellview.render_icon(id, Gtk.IconSize.BUTTON, None)
                item = Gtk.stock_lookup(id)
                label = self.strip_underscore(item.label)
                store.append((pixbuf, label))
            else:
                store.append((None, 'separator'))

        return store

    def set_sensitive(self, cell_layout, cell, tree_model, treeiter, data):
        """
        A GtkCellLayoutDataFunc that demonstrates how one can control
        sensitivity of rows. This particular function does nothing
        useful and just makes the second row insensitive.
        """

        path = tree_model.get_path(treeiter)
        indices = path.get_indices()

        sensitive = not(indices[0] == 1)

        cell.set_property('sensitive', sensitive)

    def is_separator(self, model, treeiter, data):
        """
        A GtkTreeViewRowSeparatorFunc that demonstrates how rows can be
        rendered as separators. This particular function does nothing
        useful and just turns the fourth row into a separator.
        """

        path = model.get_path(treeiter)

        indices = path.get_indices()
        result = (indices[0] == 4)

        return result

    def create_capital_store(self):
        capitals = (
            {'group': 'A - B', 'capital': None},
            {'group': None, 'capital': 'Albany'},
            {'group': None, 'capital': 'Annapolis'},
            {'group': None, 'capital': 'Atlanta'},
            {'group': None, 'capital': 'Augusta'},
            {'group': None, 'capital': 'Austin'},
            {'group': None, 'capital': 'Baton Rouge'},
            {'group': None, 'capital': 'Bismarck'},
            {'group': None, 'capital': 'Boise'},
            {'group': None, 'capital': 'Boston'},
            {'group': 'C - D', 'capital': None},
            {'group': None, 'capital': 'Carson City'},
            {'group': None, 'capital': 'Charleston'},
            {'group': None, 'capital': 'Cheyeene'},
            {'group': None, 'capital': 'Columbia'},
            {'group': None, 'capital': 'Columbus'},
            {'group': None, 'capital': 'Concord'},
            {'group': None, 'capital': 'Denver'},
            {'group': None, 'capital': 'Des Moines'},
            {'group': None, 'capital': 'Dover'},
            {'group': 'E - J', 'capital': None},
            {'group': None, 'capital': 'Frankfort'},
            {'group': None, 'capital': 'Harrisburg'},
            {'group': None, 'capital': 'Hartford'},
            {'group': None, 'capital': 'Helena'},
            {'group': None, 'capital': 'Honolulu'},
            {'group': None, 'capital': 'Indianapolis'},
            {'group': None, 'capital': 'Jackson'},
            {'group': None, 'capital': 'Jefferson City'},
            {'group': None, 'capital': 'Juneau'},
            {'group': 'K - O', 'capital': None},
            {'group': None, 'capital': 'Lansing'},
            {'group': None, 'capital': 'Lincon'},
            {'group': None, 'capital': 'Little Rock'},
            {'group': None, 'capital': 'Madison'},
            {'group': None, 'capital': 'Montgomery'},
            {'group': None, 'capital': 'Montpelier'},
            {'group': None, 'capital': 'Nashville'},
            {'group': None, 'capital': 'Oklahoma City'},
            {'group': None, 'capital': 'Olympia'},
            {'group': 'P - S', 'capital': None},
            {'group': None, 'capital': 'Phoenix'},
            {'group': None, 'capital': 'Pierre'},
            {'group': None, 'capital': 'Providence'},
            {'group': None, 'capital': 'Raleigh'},
            {'group': None, 'capital': 'Richmond'},
            {'group': None, 'capital': 'Sacramento'},
            {'group': None, 'capital': 'Salem'},
            {'group': None, 'capital': 'Salt Lake City'},
            {'group': None, 'capital': 'Santa Fe'},
            {'group': None, 'capital': 'Springfield'},
            {'group': None, 'capital': 'St. Paul'},
            {'group': 'T - Z', 'capital': None},
            {'group': None, 'capital': 'Tallahassee'},
            {'group': None, 'capital': 'Topeka'},
            {'group': None, 'capital': 'Trenton'}
        )

        parent = None

        store = Gtk.TreeStore(str)

        for item in capitals:
            if item['group']:
                parent = store.append(None, (item['group'],))
            elif item['capital']:
                store.append(parent, (item['capital'],))

        return store

    def is_capital_sensistive(self, cell_layout, cell, tree_model, treeiter, data):
        sensitive = not tree_model.iter_has_child(treeiter)
        cell.set_property('sensitive', sensitive)

    def fill_combo_entry(self, entry):
        entry.append_text('One')
        entry.append_text('Two')
        entry.append_text('2\302\275')
        entry.append_text('Three')


def main(demoapp=None):
    ComboboxApp(demoapp)
    Gtk.main()


if __name__ == '__main__':
    main()

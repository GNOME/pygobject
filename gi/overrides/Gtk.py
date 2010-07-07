# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2009 Johan Dahlin <johan@gnome.org>
#               2010 Simon van der Linden <svdlinden@src.gnome.org>
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

import gobject
from gi.repository import Gdk
from gi.repository import GObject
from ..types import override
from ..importer import modules

Gtk = modules['Gtk']
__all__ = []

class ActionGroup(Gtk.ActionGroup):
    def add_actions(self, entries, user_data=None):
        """
        The add_actions() method is a convenience method that creates a number
        of gtk.Action  objects based on the information in the list of action
        entry tuples contained in entries and adds them to the action group.
        The entry tuples can vary in size from one to six items with the
        following information:

            * The name of the action. Must be specified.
            * The stock id for the action. Optional with a default value of None
              if a label is specified.
            * The label for the action. This field should typically be marked
              for translation, see the set_translation_domain() method. Optional
              with a default value of None if a stock id is specified.
            * The accelerator for the action, in the format understood by the
              gtk.accelerator_parse() function. Optional with a default value of
              None.
            * The tooltip for the action. This field should typically be marked
              for translation, see the set_translation_domain() method. Optional
              with a default value of None.
            * The callback function invoked when the action is activated.
              Optional with a default value of None.

        The "activate" signals of the actions are connected to the callbacks and
        their accel paths are set to <Actions>/group-name/action-name.
        """
        try:
            iter(entries)
        except (TypeError):
            raise TypeError('entries must be iterable')

        def _process_action(name, stock_id=None, label=None, accelerator=None, tooltip=None, callback=None):
            action = Gtk.Action(name=name, label=label, tooltip=tooltip, stock_id=stock_id)
            if callback is not None:
                action.connect('activate', callback, user_data)

            self.add_action_with_accel(action, accelerator)

        for e in entries:
            # using inner function above since entries can leave out optional arguments
            _process_action(*e)

    def add_toggle_actions(self, entries, user_data=None):
        """
        The add_toggle_actions() method is a convenience method that creates a
        number of gtk.ToggleAction objects based on the information in the list
        of action entry tuples contained in entries and adds them to the action
        group. The toggle action entry tuples can vary in size from one to seven
        items with the following information:

            * The name of the action. Must be specified.
            * The stock id for the action. Optional with a default value of None
              if a label is specified.
            * The label for the action. This field should typically be marked
              for translation, see the set_translation_domain() method. Optional
              with a default value of None if a stock id is specified.
            * The accelerator for the action, in the format understood by the
              gtk.accelerator_parse() function. Optional with a default value of
              None.
            * The tooltip for the action. This field should typically be marked
              for translation, see the set_translation_domain() method. Optional
              with a default value of None.
            * The callback function invoked when the action is activated.
              Optional with a default value of None.
            * A flag indicating whether the toggle action is active. Optional
              with a default value of False.

        The "activate" signals of the actions are connected to the callbacks and
        their accel paths are set to <Actions>/group-name/action-name.
        """

        try:
            iter(entries)
        except (TypeError):
            raise TypeError('entries must be iterable')

        def _process_action(name, stock_id=None, label=None, accelerator=None, tooltip=None, callback=None, is_active=False):
            action = Gtk.ToggleAction(name=name, label=label, tooltip=tooltip, stock_id=stock_id)
            action.set_active(is_active)
            if callback is not None:
                action.connect('activate', callback, user_data)

            self.add_action_with_accel(action, accelerator)

        for e in entries:
            # using inner function above since entries can leave out optional arguments
            _process_action(*e)


    def add_radio_actions(self, entries, value=None, on_change=None, user_data=None):
        """
        The add_radio_actions() method is a convenience method that creates a
        number of gtk.RadioAction objects based on the information in the list
        of action entry tuples contained in entries and adds them to the action
        group. The entry tuples can vary in size from one to six items with the
        following information:

            * The name of the action. Must be specified.
            * The stock id for the action. Optional with a default value of None
              if a label is specified.
            * The label for the action. This field should typically be marked
              for translation, see the set_translation_domain() method. Optional
              with a default value of None if a stock id is specified.
            * The accelerator for the action, in the format understood by the
              gtk.accelerator_parse() function. Optional with a default value of
              None.
            * The tooltip for the action. This field should typically be marked
              for translation, see the set_translation_domain() method. Optional
              with a default value of None.
            * The value to set on the radio action. Optional with a default
              value of 0. Should be specified in applications.

        The value parameter specifies the radio action that should be set
        active. The "changed" signal of the first radio action is connected to
        the on_change callback (if specified and not None) and the accel paths
        of the actions are set to <Actions>/group-name/action-name.
        """
        try:
            iter(entries)
        except (TypeError):
            raise TypeError('entries must be iterable')

        first_action = None

        def _process_action(group_source, name, stock_id=None, label=None, accelerator=None, tooltip=None, entry_value=0):
            action = Gtk.RadioAction(name=name, label=label, tooltip=tooltip, stock_id=stock_id, value=entry_value)

            # FIXME: join_group is a patch to Gtk+ 3.0
            #        otherwise we can't effectively add radio actions to a
            #        group.  Should we depend on 3.0 and error out here
            #        or should we offer the functionality via a compat
            #        C module?
            if hasattr(action, 'join_group'):
                action.join_group(group_source)

            if value == entry_value:
                action.set_active(True)

            self.add_action_with_accel(action, accelerator)
            return action

        for e in entries:
            # using inner function above since entries can leave out optional arguments
            action = _process_action(first_action, *e)
            if first_action is None:
                first_action = action

        if first_action is not None and on_change is not None:
            first_action.connect('changed', on_change, user_data)

ActionGroup = override(ActionGroup)
__all__.append('ActionGroup')

class UIManager(Gtk.UIManager):
    def add_ui_from_string(self, buffer):
        if not isinstance(buffer, basestring):
            raise TypeError('buffer must be a string')

        length = len(buffer)

        return Gtk.UIManager.add_ui_from_string(self, buffer, length)

UIManager = override(UIManager)
__all__.append('UIManager')

class Builder(Gtk.Builder):

    def connect_signals(self, obj_or_map):
        def _full_callback(builder, gobj, signal_name, handler_name, connect_obj, flags, obj_or_map):
            handler = None
            if isinstance(obj_or_map, dict):
                handler = obj_or_map.get(handler_name, None)
            else:
                handler = getattr(obj_or_map, handler_name, None)

            if handler is None:
                raise AttributeError('Handler %s not found' % handler_name)

            if not callable(handler):
                raise TypeError('Handler %s is not a method or function' % handler_name)

            after = flags or GObject.ConnectFlags.AFTER
            if connect_obj is not None:
                if after:
                    gobj.connect_object_after(signal_name, handler, connect_obj)
                else:
                    gobj.connect_object(signal_name, handler, connect_obj)
            else:
                if after:
                    gobj.connect_after(signal_name, handler)
                else:
                    gobj.connect(signal_name, handler)

        self.connect_signals_full(_full_callback,
                                  obj_or_map);

    def add_from_string(self, buffer):
        if not isinstance(buffer, basestring):
            raise TypeError('buffer must be a string')

        length = len(buffer)

        return Gtk.Builder.add_from_string(self, buffer, length)

    def add_objects_from_string(self, buffer, object_ids):
        if not isinstance(buffer, basestring):
            raise TypeError('buffer must be a string')

        length = len(buffer)

        return Gtk.Builder.add_objects_from_string(self, buffer, length, object_ids)

Builder = override(Builder)
__all__.append('Builder')

class Dialog(Gtk.Dialog):

    def __init__(self, title=None, parent=None, flags=0, buttons=None):
        Gtk.Dialog.__init__(self)
        if title:
            self.set_title(title)
        if parent:
            self.set_transient_for(parent)
        if flags & Gtk.DialogFlags.MODAL:
            self.set_modal(True)
        if flags & Gtk.DialogFlags.DESTROY_WITH_PARENT:
            self.set_destroy_with_parent(True)
        if flags & Gtk.DialogFlags.NO_SEPARATOR:
            self.set_has_separator(False)
        if buttons:
            self.add_buttons(*buttons)

    def add_buttons(self, *args):
        """
        The add_buttons() method adds several buttons to the Gtk.Dialog using
        the button data passed as arguments to the method. This method is the
        same as calling the Gtk.Dialog.add_button() repeatedly. The button data
        pairs - button text (or stock ID) and a response ID integer are passed
        individually. For example:

        >>> dialog.add_buttons(Gtk.STOCK_OPEN, 42, "Close", Gtk.ResponseType.CLOSE)

        will add "Open" and "Close" buttons to dialog.
        """

        def buttons(b):
            while b:
                t, r = b[0:2]
                b = b[2:]
                yield t, r

        try:
            for text, response in buttons(args):
                self.add_button(text, response)
        except (IndexError):
            raise TypeError('Must pass an even number of arguments')

Dialog = override(Dialog)
__all__.append('Dialog')

class TextBuffer(Gtk.TextBuffer):
    def _get_or_create_tag_table(self):
        table = self.get_tag_table()
        if table is None:
            table = Gtk.TextTagTable()
            self.set_tag_table(table)

        return table

    def create_tag(self, tag_name=None, **properties):
        """
        @tag_name: name of the new tag, or None
        @properties: keyword list of properties and their values

        Creates a tag and adds it to the tag table of the TextBuffer.
        Equivalent to creating a Gtk.TextTag and then adding the
        tag to the buffer's tag table. The returned tag is owned by
        the buffer's tag table.

        If @tag_name is None, the tag is anonymous.

        If @tag_name is not None, a tag called @tag_name must not already
        exist in the tag table for this buffer.

        Properties are passed as a keyword list of names and values (e.g.
        foreground = 'DodgerBlue', weight = Pango.Weight.BOLD)

        Return value: a new tag
        """

        tag = Gtk.TextTag(name=tag_name, **properties)
        self._get_or_create_tag_table().add(tag)
        return tag

    def insert(self, iter, text):
        if not isinstance(text , basestring):
            raise TypeError('text must be a string, not %s' % type(text))

        length = len(text)
        Gtk.TextBuffer.insert(self, iter, text, length)

    def insert_at_cursor(self, text):
        if not isinstance(text , basestring):
            raise TypeError('text must be a string, not %s' % type(text))

        length = len(text)
        Gtk.TextBuffer.insert_at_cursor(self, text, length)

TextBuffer = override(TextBuffer)
__all__.append('TextBuffer')

class TreeModel(Gtk.TreeModel):
    def __len__(self):
        return self.iter_n_children(None)

TreeModel = override(TreeModel)
__all__.append('TreeModel')

class ListStore(Gtk.ListStore, TreeModel):
    def __init__(self, *column_types):
        Gtk.ListStore.__init__(self)
        self.set_column_types(column_types)

    def append(self, row):
        treeiter = Gtk.ListStore.append(self)

        n_columns = self.get_n_columns();
        if len(row) != n_columns:
            raise ValueError('row sequence has the incorrect number of elements')

        for i in range(n_columns):
            if row[i] is not None:
                self.set_value(treeiter, i, row[i])

        return treeiter

ListStore = override(ListStore)
__all__.append('ListStore')

class TreeStore(Gtk.TreeStore, TreeModel):

    def __init__(self, *column_types):
        Gtk.TreeStore.__init__(self)
        self.set_column_types(column_types)

    def append(self, parent, row):

        treeiter = Gtk.TreeStore.append(self, parent)

        n_columns = self.get_n_columns();
        if len(row) != n_columns:
            raise ValueError('row sequence has the incorrect number of elements')

        for i in xrange(n_columns):
            if row[i] is not None:
                self.set_value(treeiter, i, row[i])

        return treeiter

TreeStore = override(TreeStore)
__all__.append('TreeStore')

class TreeViewColumn(Gtk.TreeViewColumn):
    def __init__(self, title='',
                 cell_renderer=None,
                 **attributes):
        Gtk.TreeViewColumn.__init__(self, title=title)
        if cell_renderer:
            self.pack_start(cell_renderer, True)

        for (name, value) in attributes.iteritems():
            self.add_attribute(cell_renderer, name, value)

TreeViewColumn = override(TreeViewColumn)
__all__.append('TreeViewColumn')

import sys

initialized, argv = Gtk.init_check(sys.argv)
sys.argv = list(argv)
if not initialized:
    raise RuntimeError("Gtk couldn't be initialized")

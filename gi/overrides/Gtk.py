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

import sys
import gobject
from gi.repository import Gdk
from gi.repository import GObject
from ..overrides import override
from ..importer import modules

if sys.version_info >= (3, 0):
    _basestring = str
    _callable = lambda c: hasattr(c, '__call__')
else:
    _basestring = basestring
    _callable = callable

Gtk = modules['Gtk'].introspection_module
__all__ = []

class Widget(Gtk.Widget):

    def translate_coordinates(self, dest_widget, src_x, src_y):
        success, dest_x, dest_y = super(Widget, self).translate_coordinates(
            dest_widget, src_x, src_y)
        if success:
            return (dest_x, dest_y,)

Widget = override(Widget)
__all__.append('Widget')

class Container(Gtk.Container, Widget):

    def get_focus_chain(self):
        success, widgets = super(Container, self).get_focus_chain()
        if success:
            return widgets

Container = override(Container)
__all__.append('Container')

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
        if not isinstance(buffer, _basestring):
            raise TypeError('buffer must be a string')

        length = len(buffer)

        return Gtk.UIManager.add_ui_from_string(self, buffer, length)

UIManager = override(UIManager)
__all__.append('UIManager')

class ComboBox(Gtk.ComboBox, Container):

    def get_active_iter(self):
        success, aiter = super(ComboBox, self).get_active_iter()
        if success:
            return aiter

ComboBox = override(ComboBox)
__all__.append('ComboBox')

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

            if not _callable(handler):
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
        if not isinstance(buffer, _basestring):
            raise TypeError('buffer must be a string')

        length = len(buffer)

        return Gtk.Builder.add_from_string(self, buffer, length)

    def add_objects_from_string(self, buffer, object_ids):
        if not isinstance(buffer, _basestring):
            raise TypeError('buffer must be a string')

        length = len(buffer)

        return Gtk.Builder.add_objects_from_string(self, buffer, length, object_ids)

Builder = override(Builder)
__all__.append('Builder')


class Dialog(Gtk.Dialog, Container):

    def __init__(self,
                 title=None,
                 parent=None,
                 flags=0,
                 buttons=None,
                 **kwds):

        Gtk.Dialog.__init__(self, **kwds)
        if title:
            self.set_title(title)
        if parent:
            self.set_transient_for(parent)
        if flags & Gtk.DialogFlags.MODAL:
            self.set_modal(True)
        if flags & Gtk.DialogFlags.DESTROY_WITH_PARENT:
            self.set_destroy_with_parent(True)

        # NO_SEPARATOR has been removed from Gtk 3
        try:
            if flags & Gtk.DialogFlags.NO_SEPARATOR:
                self.set_has_separator(False)
        except AttributeError:
            pass

        if buttons is not None:
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
        def _button(b):
            while b:
                t, r = b[0:2]
                b = b[2:]
                yield t, r

        try:
            for text, response in _button(args):
                self.add_button(text, response)
        except (IndexError):
            raise TypeError('Must pass an even number of arguments')

Dialog = override(Dialog)
__all__.append('Dialog')

class MessageDialog(Gtk.MessageDialog, Dialog):
    def __init__(self,
                 parent=None,
                 flags=0,
                 type=Gtk.MessageType.INFO,
                 buttons=Gtk.ButtonsType.NONE,
                 message_format=None,
                 **kwds):

        if message_format != None:
            kwds['text'] = message_format
        Gtk.MessageDialog.__init__(self,
                                   buttons=buttons,
                                   **kwds)
        Dialog.__init__(self, parent=parent, flags=flags)

MessageDialog = override(MessageDialog)
__all__.append('MessageDialog')

class AboutDialog(Gtk.AboutDialog, Dialog):
    def __init__(self, **kwds):
        Gtk.AboutDialog.__init__(self, **kwds)
        Dialog.__init__(self)

AboutDialog = override(AboutDialog)
__all__.append('AboutDialog')

class ColorSelectionDialog(Gtk.ColorSelectionDialog, Dialog):
    def __init__(self, title=None, **kwds):
        Gtk.ColorSelectionDialog.__init__(self, **kwds)
        Dialog.__init__(self, title=title)

ColorSelectionDialog = override(ColorSelectionDialog)
__all__.append('ColorSelectionDialog')

class FileChooserDialog(Gtk.FileChooserDialog, Dialog):
    def __init__(self,
                 title=None,
                 parent=None,
                 action=Gtk.FileChooserAction.OPEN,
                 buttons=None,
                 **kwds):
        Gtk.FileChooserDialog.__init__(self,
                                       action=action,
                                       **kwds)
        Dialog.__init__(self, title=title, parent=parent, buttons=buttons)

FileChooserDialog = override(FileChooserDialog)
__all__.append('FileChooserDialog')

class FontSelectionDialog(Gtk.FontSelectionDialog, Dialog):
    def __init__(self, title=None, **kwds):
        Gtk.FontSelectionDialog.__init__(self, **kwds)
        Dialog.__init__(self, title=title)

FontSelectionDialog = override(FontSelectionDialog)
__all__.append('FontSelectionDialog')

class RecentChooserDialog(Gtk.RecentChooserDialog, Dialog):
    def __init__(self,
                 title=None,
                 parent=None,
                 manager=None,
                 buttons=None,
                 **kwds):

        Gtk.RecentChooserDialog.__init__(self, recent_manager=manager, **kwds)
        Dialog.__init__(self,
                        title=title,
                        parent=parent,
                        buttons=buttons)

RecentChooserDialog = override(RecentChooserDialog)
__all__.append('RecentChooserDialog')

class IconView(Gtk.IconView):

    def get_item_at_pos(self, x, y):
        success, path, cell = super(IconView, self).get_item_at_pos(x, y)
        if success:
            return (path, cell,)

    def get_visible_range(self):
        success, start_path, end_path = super(IconView, self).get_visible_range()
        if success:
            return (start_path, end_path,)

    def get_dest_item_at_pos(self, drag_x, drag_y):
        success, path, pos = super(IconView, self).get_dest_item_at_pos(drag_x, drag_y)
        if success:
            return path, pos

IconView = override(IconView)
__all__.append('IconView')

class IMContext(Gtk.IMContext):

    def get_surrounding(self):
        success, text, cursor_index = super(IMContext, self).get_surrounding()
        if success:
            return (text, cursor_index,)

IMContext = override(IMContext)
__all__.append('IMContext')

class RecentInfo(Gtk.RecentInfo):

    def get_application_info(self, app_name):
        success, app_exec, count, time = super(RecentInfo, self).get_application_info(app_name)
        if success:
            return (app_exec, count, time,)

RecentInfo = override(RecentInfo)
__all__.append('RecentInfo')

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
        if not isinstance(text , _basestring):
            raise TypeError('text must be a string, not %s' % type(text))

        length = len(text)
        Gtk.TextBuffer.insert(self, iter, text, length)

    def insert_at_cursor(self, text):
        if not isinstance(text , _basestring):
            raise TypeError('text must be a string, not %s' % type(text))

        length = len(text)
        Gtk.TextBuffer.insert_at_cursor(self, text, length)

    def get_selection_bounds(self):
        success, start, end = super(TextBuffer, self).get_selection_bounds(string,
            flags, limit)
        return (start, end)

TextBuffer = override(TextBuffer)
__all__.append('TextBuffer')

class TextIter(Gtk.TextIter):

    def forward_search(self, string, flags, limit):
        success, match_start, match_end = super(TextIter, self).forward_search(string,
            flags, limit)
        return (match_start, match_end,)

    def backward_search(self, string, flags, limit):
        success, match_start, match_end = super(TextIter, self).backward_search(string,
            flags, limit)
        return (match_start, match_end,)

TextIter = override(TextIter)
__all__.append('TextIter')

class TreeModel(Gtk.TreeModel):
    def __len__(self):
        return self.iter_n_children(None)

    def __bool__(self):
        return True

    # alias for Python 2.x object protocol
    __nonzero__ = __bool__

    def __getitem__(self, key):
        if isinstance(key, Gtk.TreeIter):
            return TreeModelRow(self, key)
        elif isinstance(key, int) and key < 0:
            index = len(self) + key
            if index < 0:
                raise IndexError("row index is out of bounds: %d" % key)
            try:
                aiter = self.get_iter(index)
            except ValueError:
                raise IndexError("could not find tree path '%s'" % key)
            return TreeModelRow(self, aiter)
        else:
            try:
                aiter = self.get_iter(key)
            except ValueError:
                raise IndexError("could not find tree path '%s'" % key)
            return TreeModelRow(self, aiter)

    def __iter__(self):
        return TreeModelRowIter(self, self.get_iter_first())

    def get_iter(self, path):
        if isinstance(path, Gtk.TreePath):
            pass
        elif isinstance(path, (int, str,)):
            path = self._tree_path_from_string(str(path))
        elif isinstance(path, tuple):
            path_str = ":".join(str(val) for val in path)
            path = self._tree_path_from_string(path_str)
        else:
            raise TypeError("tree path must be one of Gtk.TreeIter, Gtk.TreePath, \
                int, str or tuple, not %s" % type(path).__name__)

        success, aiter = super(TreeModel, self).get_iter(path)
        if not success:
            raise ValueError("invalid tree path '%s'" % path)
        return aiter

    def _tree_path_from_string(self, path):
        if len(path) == 0:
            raise TypeError("could not parse subscript '%s' as a tree path" % path)
        try:
            return TreePath.new_from_string(path)
        except TypeError:
            raise TypeError("could not parse subscript '%s' as a tree path" % path)

    def get_iter_first(self):
        success, aiter = super(TreeModel, self).get_iter_first()
        if success:
            return aiter

    def get_iter_from_string(self, path_string):
        success, aiter = super(TreeModel, self).get_iter_from_string(path_string)
        if not success:
            raise ValueError("invalid tree path '%s'" % path_string)
        return aiter

    def iter_next(self, aiter):
        next_iter = aiter.copy()
        success = super(TreeModel, self).iter_next(next_iter)
        if success:
            return next_iter

    def iter_children(self, aiter):
        success, child_iter = super(TreeModel, self).iter_children(aiter)
        if success:
            return child_iter

    def iter_nth_child(self, parent, n):
        success, child_iter = super(TreeModel, self).iter_nth_child(parent, n)
        if success:
            return child_iter

    def iter_parent(self, aiter):
        success, parent_iter = super(TreeModel, self).iter_parent(aiter)
        if success:
            return parent_iter

TreeModel = override(TreeModel)
__all__.append('TreeModel')

class TreeSortable(Gtk.TreeSortable, ):

    def get_sort_column_id(self):
        success, sort_column_id, order = super(TreeSortable, self).get_sort_column_id()
        if success:
            return (sort_column_id, order,)
        else:
            return (None, None,)

TreeSortable = override(TreeSortable)
__all__.append('TreeSortable')


class ListStore(Gtk.ListStore, TreeModel, TreeSortable):
    def __init__(self, *column_types):
        Gtk.ListStore.__init__(self)
        self.set_column_types(column_types)

    def append(self, row=None):
        treeiter = Gtk.ListStore.append(self)

        # TODO: Accept a dictionary for row
        # model.append(None,{COLUMN_ICON: icon, COLUMN_NAME: name})

        if row is not None:
            n_columns = self.get_n_columns();
            if len(row) != n_columns:
                raise ValueError('row sequence has the incorrect number of elements')

            for i in range(n_columns):
                if row[i] is not None:
                    self.set_value(treeiter, i, row[i])

        return treeiter

ListStore = override(ListStore)
__all__.append('ListStore')

class TreeModelRow(object):

    def __init__(self, model, iter_or_path):
        if not isinstance(model, Gtk.TreeModel):
            raise TypeError("expected Gtk.TreeModel, %s found" % type(model).__name__)
        self.model = model
        if isinstance(iter_or_path, Gtk.TreePath):
            self.iter = model.get_iter(iter_or_path)
        elif isinstance(iter_or_path, Gtk.TreeIter):
            self.iter = iter_or_path
        else:
            raise TypeError("expected Gtk.TreeIter or Gtk.TreePath, \
                %s found" % type(iter_or_path).__name__)

    @property
    def path(self):
        return self.model.get_path(self.iter)

    @property
    def next(self):
        return self.get_next()

    @property
    def parent(self):
        return self.get_parent()

    def get_next(self):
        next_iter = self.model.iter_next(self.iter)
        if next_iter:
            return TreeModelRow(self.model, next_iter)

    def get_parent(self):
        parent_iter = self.model.iter_parent(self.iter)
        if parent_iter:
            return TreeModelRow(self.model, parent_iter)

    def __getitem__(self, key):
        if isinstance(key, int):
            if key >= self.model.get_n_columns():
                raise IndexError("column index is out of bounds: %d" % key)
            elif key < 0:
                key = self._convert_negative_index(key)
            return self.model.get_value(self.iter, key)
        else:
            raise TypeError("indices must be integers, not %s" % type(key).__name__)

    def __setitem__(self, key, value):
        if isinstance(key, int):
            if key >= self.model.get_n_columns():
                raise IndexError("column index is out of bounds: %d" % key)
            elif key < 0:
                key = self._convert_negative_index(key)
            return self.model.set_value(self.iter, key, value)
        else:
            raise TypeError("indices must be integers, not %s" % type(key).__name__)

    def _convert_negative_index(self, index):
        new_index = self.model.get_n_columns() + index
        if new_index < 0:
            raise IndexError("column index is out of bounds: %d" % index)
        return new_index

    def iterchildren(self):
        child_iter = self.model.iter_children(self.iter)
        return TreeModelRowIter(self.model, child_iter)

__all__.append('TreeModelRow')

class TreeModelRowIter(object):

    def __init__(self, model, aiter):
        self.model = model
        self.iter = aiter

    def __next__(self):
        if not self.iter:
            raise StopIteration
        row = TreeModelRow(self.model, self.iter)
        self.iter = self.model.iter_next(self.iter)
        return row

    # alias for Python 2.x object protocol
    next = __next__

    def __iter__(self):
        return self

__all__.append('TreeModelRowIter')

class TreePath(Gtk.TreePath):

    def __str__(self):
        return self.to_string()

    def __lt__(self, other):
        return self.compare(other) < 0

    def __le__(self, other):
        return self.compare(other) <= 0

    def __eq__(self, other):
        return self.compare(other) == 0

    def __ne__(self, other):
        return self.compare(other) != 0

    def __gt__(self, other):
        return self.compare(other) > 0

    def __ge__(self, other):
        return self.compare(other) >= 0

TreePath = override(TreePath)
__all__.append('TreePath')

class TreeStore(Gtk.TreeStore, TreeModel, TreeSortable):

    def __init__(self, *column_types):
        Gtk.TreeStore.__init__(self)
        self.set_column_types(column_types)

    def append(self, parent, row=None):

        treeiter = Gtk.TreeStore.append(self, parent)

        # TODO: Accept a dictionary for row
        # model.append(None,{COLUMN_ICON: icon, COLUMN_NAME: name})

        if row is not None:
            n_columns = self.get_n_columns();
            if len(row) != n_columns:
                raise ValueError('row sequence has the incorrect number of elements')

            for i in range(n_columns):
                if row[i] is not None:
                    self.set_value(treeiter, i, row[i])

        return treeiter

TreeStore = override(TreeStore)
__all__.append('TreeStore')

class TreeView(Gtk.TreeView, Container):

    def get_path_at_pos(self, x, y):
        success, path, column, cell_x, cell_y = super(TreeView, self).get_path_at_pos(x, y)
        if success:
            return (path, column, cell_x, cell_y,)

    def get_dest_row_at_pos(self, drag_x, drag_y):
        success, path, pos = super(TreeView, self).get_dest_row_at_pos(drag_x, drag_y)
        if success:
            return (path, pos,)

TreeView = override(TreeView)
__all__.append('TreeView')

class TreeViewColumn(Gtk.TreeViewColumn):
    def __init__(self, title='',
                 cell_renderer=None,
                 **attributes):
        Gtk.TreeViewColumn.__init__(self, title=title)
        if cell_renderer:
            self.pack_start(cell_renderer, True)

        for (name, value) in attributes.items():
            self.add_attribute(cell_renderer, name, value)

    def cell_get_position(self, cell_renderer):
        success, start_pos, width = super(TreeViewColumn, self).cell_get_position(cell_renderer)
        if success:
            return (start_pos, width,)

TreeViewColumn = override(TreeViewColumn)
__all__.append('TreeViewColumn')

class TreeSelection(Gtk.TreeSelection):

    def get_selected(self):
        success, model, aiter = super(TreeSelection, self).get_selected()
        if success:
            return (model, aiter)
        else:
            return (model, None)

TreeSelection = override(TreeSelection)
__all__.append('TreeSelection')

class Button(Gtk.Button, Container):
    def __init__(self, label=None, stock=None, use_underline=False):
        if stock:
            label = stock
            use_stock = True
            use_underline = True
        else:
            use_stock = False
        Gtk.Button.__init__(self, label=label, use_stock=use_stock,
                            use_underline=use_underline)
Button = override(Button)
__all__.append('Button')

import sys

initialized, argv = Gtk.init_check(sys.argv)
sys.argv = list(argv)
if not initialized:
    raise RuntimeError("Gtk couldn't be initialized")

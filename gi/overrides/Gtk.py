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
from gi.repository import GObject
from ..overrides import override
from ..importer import modules

if sys.version_info >= (3, 0):
    _basestring = str
    _callable = lambda c: hasattr(c, '__call__')
else:
    _basestring = basestring
    _callable = callable

Gtk = modules['Gtk']._introspection_module
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

    def __len__(self):
        return len(self.get_children())

    def __contains__(self, child):
        return child in self.get_children()

    def __iter__(self):
        return iter(self.get_children())

    def __bool__(self):
        return True

    # alias for Python 2.x object protocol
    __nonzero__ = __bool__

    def get_focus_chain(self):
        success, widgets = super(Container, self).get_focus_chain()
        if success:
            return widgets

Container = override(Container)
__all__.append('Container')

class Editable(Gtk.Editable):

    def insert_text(self, text, position):
        pos = super(Editable, self).insert_text(text, -1, position)

        return pos

    def get_selection_bounds(self):
        success, start_pos, end_pos = super(Editable, self).get_selection_bounds()
        if success:
            return (start_pos, end_pos,)
        else:
            return tuple()

Editable = override(Editable)
__all__.append("Editable")

class Action(Gtk.Action):
    def __init__(self, name, label, tooltip, stock_id, **kwds):
        Gtk.Action.__init__(self, name=name, label=label, tooltip=tooltip, stock_id=stock_id, **kwds)

Action = override(Action)
__all__.append("Action")

class RadioAction(Gtk.RadioAction):
    def __init__(self, name, label, tooltip, stock_id, value, **kwds):
        Gtk.RadioAction.__init__(self, name=name, label=label, tooltip=tooltip, stock_id=stock_id, value=value, **kwds)

RadioAction = override(RadioAction)
__all__.append("RadioAction")

class ActionGroup(Gtk.ActionGroup):
    def __init__(self, name, **kwds):
        super(ActionGroup, self).__init__(name = name, **kwds)

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
            action = Action(name, label, tooltip, stock_id)
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
            action = Gtk.ToggleAction(name, label, tooltip, stock_id)
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
            action = RadioAction(name, label, tooltip, stock_id, entry_value)

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

    def insert_action_group(self, buffer, length=-1):
        return Gtk.UIManager.insert_action_group(self, buffer, length)

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

        self.connect_signals_full(_full_callback, obj_or_map)

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
                 _buttons_property=None,
                 **kwds):

        # buttons is overloaded by PyGtk so we have to do the same here
        # this breaks some subclasses of Dialog so add a _buttons_property
        # keyword to work around this
        if _buttons_property is not None:
            kwds['buttons'] = _buttons_property

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
                                   _buttons_property=buttons,
                                   message_type=type,
                                   **kwds)
        Dialog.__init__(self, parent=parent, flags=flags)

    def format_secondary_text(self, message_format):
        self.set_property('secondary-use-markup', False)
        self.set_property('secondary-text', message_format)

    def format_secondary_markup(self, message_format):
        self.set_property('secondary-use-markup', True)
        self.set_property('secondary-text', message_format)

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
        Dialog.__init__(self,
                        title=title,
                        parent=parent,
                        buttons=buttons)

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

    def create_mark(self, mark_name, where, left_gravity=False):
        return Gtk.TextBuffer.create_mark(self, mark_name, where, left_gravity)

    def set_text(self, text, length=-1):
        Gtk.TextBuffer.set_text(self, text, length)

    def insert(self, iter, text):
        if not isinstance(text , _basestring):
            raise TypeError('text must be a string, not %s' % type(text))

        length = len(text)
        Gtk.TextBuffer.insert(self, iter, text, length)

    def insert_with_tags(self, iter, text, *tags):
        start_offset = iter.get_offset()
        self.insert(iter, text)

        if not tags:
            return

        start = self.get_iter_at_offset(start_offset)

        for tag in tags:
            self.apply_tag(tag, start, iter)

    def insert_with_tags_by_name(self, iter, text, *tags):
        if not tags:
            return

        tag_objs = []

        for tag in tags:
            tag_obj = self.get_tag_table().lookup(tag)
            if not tag_obj:
                raise ValueError('unknown text tag: %s' % tag)
            tag_objs.append(tag_obj)

        self.insert_with_tags(iter, text, *tag_objs)

    def insert_at_cursor(self, text):
        if not isinstance(text , _basestring):
            raise TypeError('text must be a string, not %s' % type(text))

        length = len(text)
        Gtk.TextBuffer.insert_at_cursor(self, text, length)

    def get_selection_bounds(self):
        success, start, end = super(TextBuffer, self).get_selection_bounds()
        if success:
            return (start, end)
        else:
            return ()

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

    def begins_tag(self, tag=None):
        return super(TextIter, self).begins_tag(tag)

    def ends_tag(self, tag=None):
        return super(TextIter, self).ends_tag(tag)

    def toggles_tag(self, tag=None):
        return super(TextIter, self).toggles_tag(tag)

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
        if not isinstance(path, Gtk.TreePath):
            path = TreePath(path)

        success, aiter = super(TreeModel, self).get_iter(path)
        if not success:
            raise ValueError("invalid tree path '%s'" % path)
        return aiter

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

    def set_row(self, treeiter, row):
        # TODO: Accept a dictionary for row
        # model.append(None,{COLUMN_ICON: icon, COLUMN_NAME: name})

        n_columns = self.get_n_columns()
        if len(row) != n_columns:
            raise ValueError('row sequence has the incorrect number of elements')

        for i in range(n_columns):
            value = row[i]
            if value is None:
               continue  # None means skip this row

            self.set_value(treeiter, i, value)

    def _convert_value(self, treeiter, column, value):
            if value is None:
                return

            # we may need to convert to a basic type
            type_ = self.get_column_type(column)
            if type_ == GObject.TYPE_STRING:
                if isinstance(value, str):
                    value = str(value)
                elif sys.version_info < (3, 0):
                    if isinstance(value, unicode):
                        value = value.encode('UTF-8')
                    else:
                        raise ValueError('Expected string or unicode for column %i but got %s%s' % (column, value, type(value)))
                else:
                    raise ValueError('Expected a string for column %i but got %s' % (column, type(value)))
            elif type_ == GObject.TYPE_FLOAT or type_ == GObject.TYPE_DOUBLE:
                if isinstance(value, float):
                    value = float(value)
                else:
                    raise ValueError('Expected a float for column %i but got %s' % (column, type(value)))
            elif type_ == GObject.TYPE_LONG or type_ == GObject.TYPE_INT:
                if isinstance(value, int):
                    value = int(value)
                elif sys.version_info < (3, 0):
                    if isinstance(value, long):
                        value = long(value)
                    else:
                        raise ValueError('Expected an long for column %i but got %s' % (column, type(value)))
                else:
                    raise ValueError('Expected an integer for column %i but got %s' % (column, type(value)))
            elif type_ == GObject.TYPE_BOOLEAN:
                cmp_classes = [int]
                if sys.version_info < (3, 0):
                    cmp_classes.append(long)

                if isinstance(value, tuple(cmp_classes)):
                    value = bool(value)
                else:
                    raise ValueError('Expected a bool for column %i but got %s' % (column, type(value)))
            else:
                # use GValues directly to marshal to the correct type
                # standard object checks should take care of validation
                # so we don't have to do it here
                value_container = GObject.Value()
                value_container.init(type_)
                if type_ == GObject.TYPE_PYOBJECT:
                    value_container.set_boxed(value)
                    value = value_container
                elif type_ == GObject.TYPE_CHAR:
                    value_container.set_char(value)
                    value = value_container
                elif type_ == GObject.TYPE_UCHAR:
                    value_container.set_uchar(value)
                    value = value_container
                elif type_ == GObject.TYPE_UNICHAR:
                    cmp_classes = [str]
                    if sys.version_info < (3, 0):
                        cmp_classes.append(unicode)

                    if isinstance(value, tuple(cmp_classes)):
                        value = ord(value[0])

                    value_container.set_uint(value)
                    value = value_container
                elif type_ == GObject.TYPE_UINT:
                    value_container.set_uint(value)
                    value = value_container
                elif type_ == GObject.TYPE_ULONG:
                    value_container.set_ulong(value)
                    value = value_container
                elif type_ == GObject.TYPE_INT64:
                    value_container.set_int64(value)
                    value = value_container
                elif type_ == GObject.TYPE_UINT64:
                    value_container.set_uint64(value)
                    value = value_container

            return value

    def get(self, treeiter, *columns):
        n_columns = self.get_n_columns()

        values = []
        for col in columns:
            if not isinstance(col, int):
                raise TypeError("column numbers must be ints")

            if col < 0 or col >= n_columns:
                raise ValueError("column number is out of range")

            values.append(self.get_value(treeiter, col))

        return tuple(values)

TreeModel = override(TreeModel)
__all__.append('TreeModel')

class TreeSortable(Gtk.TreeSortable, ):

    def get_sort_column_id(self):
        success, sort_column_id, order = super(TreeSortable, self).get_sort_column_id()
        if success:
            return (sort_column_id, order,)
        else:
            return (None, None,)

    def set_sort_func(self, sort_column_id, sort_func, user_data=None):
        super(TreeSortable, self).set_sort_func(sort_column_id, sort_func, user_data)

    def set_default_sort_func(self, sort_func, user_data=None):
        super(TreeSortable, self).set_default_sort_func(sort_func, user_data)

TreeSortable = override(TreeSortable)
__all__.append('TreeSortable')

class ListStore(Gtk.ListStore, TreeModel, TreeSortable):
    def __init__(self, *column_types):
        Gtk.ListStore.__init__(self)
        self.set_column_types(column_types)

    def append(self, row=None):
        treeiter = Gtk.ListStore.append(self)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    def insert(self, position, row=None):
        treeiter = Gtk.ListStore.insert(self, position)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    def insert_before(self, sibling, row=None):
        treeiter = Gtk.ListStore.insert_before(self, sibling)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    def insert_after(self, sibling, row=None):
        treeiter = Gtk.ListStore.insert_after(self, sibling)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    def set_value(self, treeiter, column, value):
        value = self._convert_value(treeiter, column, value)
        Gtk.ListStore.set_value(self, treeiter, column, value)


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

    def __new__(cls, path=0):
        if isinstance(path, int):
            path = str(path)
        elif isinstance(path, tuple):
            path = ":".join(str(val) for val in path)

        if len(path) == 0:
            raise TypeError("could not parse subscript '%s' as a tree path" % path)
        try:
            return TreePath.new_from_string(path)
        except TypeError:
            raise TypeError("could not parse subscript '%s' as a tree path" % path)

    def __str__(self):
        return self.to_string()

    def __lt__(self, other):
        return not other is None and self.compare(other) < 0

    def __le__(self, other):
        return not other is None and self.compare(other) <= 0

    def __eq__(self, other):
        return not other is None and self.compare(other) == 0

    def __ne__(self, other):
        return other is None or self.compare(other) != 0

    def __gt__(self, other):
        return other is None or self.compare(other) > 0

    def __ge__(self, other):
        return other is None or self.compare(other) >= 0

TreePath = override(TreePath)
__all__.append('TreePath')

class TreeStore(Gtk.TreeStore, TreeModel, TreeSortable):

    def __init__(self, *column_types):
        Gtk.TreeStore.__init__(self)
        self.set_column_types(column_types)

    def append(self, parent, row=None):
        treeiter = Gtk.TreeStore.append(self, parent)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    def insert(self, parent, position, row=None):
        treeiter = Gtk.TreeStore.insert(self, parent, position)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    def insert_before(self, parent, sibling, row=None):
        treeiter = Gtk.TreeStore.insert_before(self, parent, sibling)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    def insert_after(self, parent, sibling, row=None):
        treeiter = Gtk.TreeStore.insert_after(self, parent, sibling)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    def set_value(self, treeiter, column, value):
        value = self._convert_value(treeiter, column, value)
        Gtk.TreeStore.set_value(self, treeiter, column, value)


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

    def _construct_target_list(self, targets):
        # FIXME: this should most likely be part of Widget or a global helper
        #        function
        target_entries = []
        for t in targets:
            entry = Gtk.TargetEntry.new(*t)
            target_entries.append(entry)
        return target_entries

    def enable_model_drag_source(self, start_button_mask, targets, actions):
        target_entries = self._construct_target_list(targets)
        super(TreeView, self).enable_model_drag_source(start_button_mask,
                                                       target_entries,
                                                       actions)

    def enable_model_drag_dest(self, targets, actions):
        target_entries = self._construct_target_list(targets)
        super(TreeView, self).enable_model_drag_dest(target_entries,
                                                     actions)

    def scroll_to_cell(self, path, column=None, use_align=False, row_align=0.0, col_align=0.0):
        if not isinstance(path, Gtk.TreePath):
            path = TreePath(path)
        super(TreeView, self).scroll_to_cell(path, column, use_align, row_align, col_align)


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

    def set_cell_data_func(self, cell_renderer, func, func_data=None):
        super(TreeViewColumn, self).set_cell_data_func(cell_renderer, func, func_data)

TreeViewColumn = override(TreeViewColumn)
__all__.append('TreeViewColumn')

class TreeSelection(Gtk.TreeSelection):

    def select_path(self, path):
        if not isinstance(path, Gtk.TreePath):
            path = TreePath(path)
        super(TreeSelection, self).select_path(path)

    def get_selected(self):
        success, model, aiter = super(TreeSelection, self).get_selected()
        if success:
            return (model, aiter)
        else:
            return (model, None)

    # for compatibility with PyGtk
    def get_selected_rows(self):
        rows, model = super(TreeSelection, self).get_selected_rows()
        return (model, rows)


TreeSelection = override(TreeSelection)
__all__.append('TreeSelection')

class Button(Gtk.Button, Container):
    def __init__(self, label=None, stock=None, use_underline=False, **kwds):
        if stock:
            label = stock
            use_stock = True
            use_underline = True
        else:
            use_stock = False
        Gtk.Button.__init__(self, label=label, use_stock=use_stock,
                            use_underline=use_underline, **kwds)
Button = override(Button)
__all__.append('Button')

class LinkButton(Gtk.LinkButton):
    def __init__(self, uri, label=None, **kwds):
        Gtk.LinkButton.__init__(self, uri=uri, label=label, **kwds)

LinkButton = override(LinkButton)
__all__.append('LinkButton')

class Label(Gtk.Label):
    def __init__(self, label=None, **kwds):
        Gtk.Label.__init__(self, label=label, **kwds)

Label = override(Label)
__all__.append('Label')

class Adjustment(Gtk.Adjustment):
    def __init__(self, *args, **kwds):
        arg_names = ('value', 'lower', 'upper',
                        'step_increment', 'page_increment', 'page_size')
        new_args = dict(zip(arg_names, args))
        new_args.update(kwds)
        Gtk.Adjustment.__init__(self, **new_args)

        # The value property is set between lower and (upper - page_size).
        # Just in case lower, upper or page_size was still 0 when value
        # was set, we set it again here.
        if 'value' in new_args:
            self.set_value(new_args['value'])

Adjustment = override(Adjustment)
__all__.append('Adjustment')

class Table(Gtk.Table, Container):
    def __init__(self, rows=1, columns=1, homogeneous=False, **kwds):
        if 'n_rows' in kwds:
            rows = kwds.pop('n_rows')

        if 'n_columns' in kwds:
            columns = kwds.pop('n_columns')
            
        Gtk.Table.__init__(self, n_rows=rows, n_columns=columns, homogeneous=homogeneous, **kwds)

    def attach(self, child, left_attach, right_attach, top_attach, bottom_attach, xoptions=Gtk.AttachOptions.EXPAND|Gtk.AttachOptions.FILL, yoptions=Gtk.AttachOptions.EXPAND|Gtk.AttachOptions.FILL, xpadding=0, ypadding=0):
        Gtk.Table.attach(self, child, left_attach, right_attach, top_attach, bottom_attach, xoptions, yoptions, xpadding, ypadding)

Table = override(Table)
__all__.append('Table')

class ScrolledWindow(Gtk.ScrolledWindow):
    def __init__(self, hadjustment=None, vadjustment=None, **kwds):
        Gtk.ScrolledWindow.__init__(self, hadjustment=hadjustment, vadjustment=vadjustment, **kwds)

ScrolledWindow = override(ScrolledWindow)
__all__.append('ScrolledWindow')

class Paned(Gtk.Paned):
    def pack1(self, child, resize=False, shrink=True):
        super(Paned, self).pack1(child, resize, shrink)

    def pack2(self, child, resize=True, shrink=True):
        super(Paned, self).pack2(child, resize, shrink)

Paned = override(Paned)
__all__.append('Paned')

if Gtk._version != '2.0':
    class Menu(Gtk.Menu):
        def popup(self, parent_menu_shell, parent_menu_item, func, data, button, activate_time):
            self.popup_for_device(None, parent_menu_shell, parent_menu_item, func, data, button, activate_time)
    Menu = override(Menu)
    __all__.append('Menu')

_Gtk_main_quit = Gtk.main_quit
@override(Gtk.main_quit)
def main_quit(*args):
    _Gtk_main_quit()

_Gtk_stock_lookup = Gtk.stock_lookup
@override(Gtk.stock_lookup)
def stock_lookup(*args):
    success, item = _Gtk_stock_lookup(*args)
    if not success:
        return None

    return item

initialized, argv = Gtk.init_check(sys.argv)
sys.argv = list(argv)
if not initialized:
    raise RuntimeError("Gtk couldn't be initialized")

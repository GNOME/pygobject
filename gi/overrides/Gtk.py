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

import collections
import sys
from gi.repository import GObject
from ..overrides import override, strip_boolean_result
from ..module import get_introspection_module
from gi import PyGIDeprecationWarning

if sys.version_info >= (3, 0):
    _basestring = str
    _callable = lambda c: hasattr(c, '__call__')
else:
    _basestring = basestring
    _callable = callable

Gtk = get_introspection_module('Gtk')

__all__ = []

if Gtk._version == '2.0':
    import warnings
    warn_msg = "You have imported the Gtk 2.0 module.  Because Gtk 2.0 \
was not designed for use with introspection some of the \
interfaces and API will fail.  As such this is not supported \
by the pygobject development team and we encourage you to \
port your app to Gtk 3 or greater. PyGTK is the recomended \
python module to use with Gtk 2.0"

    warnings.warn(warn_msg, RuntimeWarning)


def _construct_target_list(targets):
    """Create a list of TargetEntry items from a list of tuples in the form (target, flags, info)

    The list can also contain existing TargetEntry items in which case the existing entry
    is re-used in the return list.
    """
    target_entries = []
    for entry in targets:
        if not isinstance(entry, Gtk.TargetEntry):
            entry = Gtk.TargetEntry.new(*entry)
        target_entries.append(entry)
    return target_entries

__all__.append('_construct_target_list')


class Widget(Gtk.Widget):

    translate_coordinates = strip_boolean_result(Gtk.Widget.translate_coordinates)

    def render_icon(self, stock_id, size, detail=None):
        return super(Widget, self).render_icon(stock_id, size, detail)

    def drag_dest_set_target_list(self, target_list):
        if (target_list is not None) and (not isinstance(target_list, Gtk.TargetList)):
            target_list = Gtk.TargetList.new(_construct_target_list(target_list))
        super(Widget, self).drag_dest_set_target_list(target_list)

    def drag_source_set_target_list(self, target_list):
        if (target_list is not None) and (not isinstance(target_list, Gtk.TargetList)):
            target_list = Gtk.TargetList.new(_construct_target_list(target_list))
        super(Widget, self).drag_source_set_target_list(target_list)


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

    get_focus_chain = strip_boolean_result(Gtk.Container.get_focus_chain)


Container = override(Container)
__all__.append('Container')


class Editable(Gtk.Editable):

    def insert_text(self, text, position):
        return super(Editable, self).insert_text(text, -1, position)

    get_selection_bounds = strip_boolean_result(Gtk.Editable.get_selection_bounds, fail_ret=())


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
        super(ActionGroup, self).__init__(name=name, **kwds)

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
                if user_data is None:
                    action.connect('activate', callback)
                else:
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
                if user_data is None:
                    action.connect('activate', callback)
                else:
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
            if user_data is None:
                first_action.connect('changed', on_change)
            else:
                first_action.connect('changed', on_change, user_data)

ActionGroup = override(ActionGroup)
__all__.append('ActionGroup')


class UIManager(Gtk.UIManager):
    def add_ui_from_string(self, buffer):
        if not isinstance(buffer, _basestring):
            raise TypeError('buffer must be a string')

        length = len(buffer.encode('UTF-8'))

        return Gtk.UIManager.add_ui_from_string(self, buffer, length)

    def insert_action_group(self, buffer, length=-1):
        return Gtk.UIManager.insert_action_group(self, buffer, length)

UIManager = override(UIManager)
__all__.append('UIManager')


class ComboBox(Gtk.ComboBox, Container):
    get_active_iter = strip_boolean_result(Gtk.ComboBox.get_active_iter)

ComboBox = override(ComboBox)
__all__.append('ComboBox')


class Box(Gtk.Box):
    def __init__(self, homogeneous=False, spacing=0, **kwds):
        super(Box, self).__init__(**kwds)
        self.set_homogeneous(homogeneous)
        self.set_spacing(spacing)

Box = override(Box)
__all__.append('Box')


class SizeGroup(Gtk.SizeGroup):
    def __init__(self, mode=Gtk.SizeGroupMode.VERTICAL):
        super(SizeGroup, self).__init__(mode=mode)

SizeGroup = override(SizeGroup)
__all__.append('SizeGroup')


class MenuItem(Gtk.MenuItem):
    def __init__(self, label=None, **kwds):
        if label:
            super(MenuItem, self).__init__(label=label, **kwds)
        else:
            super(MenuItem, self).__init__(**kwds)

MenuItem = override(MenuItem)
__all__.append('MenuItem')


class Builder(Gtk.Builder):
    @staticmethod
    def _extract_handler_and_args(obj_or_map, handler_name):
        handler = None
        if isinstance(obj_or_map, collections.Mapping):
            handler = obj_or_map.get(handler_name, None)
        else:
            handler = getattr(obj_or_map, handler_name, None)

        if handler is None:
            raise AttributeError('Handler %s not found' % handler_name)

        args = ()
        if isinstance(handler, collections.Sequence):
            if len(handler) == 0:
                raise TypeError("Handler %s tuple can not be empty" % handler)
            args = handler[1:]
            handler = handler[0]

        elif not _callable(handler):
            raise TypeError('Handler %s is not a method, function or tuple' % handler)

        return handler, args

    def connect_signals(self, obj_or_map):
        """Connect signals specified by this builder to a name, handler mapping.

        Connect signal, name, and handler sets specified in the builder with
        the given mapping "obj_or_map". The handler/value aspect of the mapping
        can also contain a tuple in the form of (handler [,arg1 [,argN]])
        allowing for extra arguments to be passed to the handler. For example:
            builder.connect_signals({'on_clicked': (on_clicked, arg1, arg2)})
        """
        def _full_callback(builder, gobj, signal_name, handler_name, connect_obj, flags, obj_or_map):
            handler, args = self._extract_handler_and_args(obj_or_map, handler_name)

            after = flags & GObject.ConnectFlags.AFTER
            if connect_obj is not None:
                if after:
                    gobj.connect_object_after(signal_name, handler, connect_obj, *args)
                else:
                    gobj.connect_object(signal_name, handler, connect_obj, *args)
            else:
                if after:
                    gobj.connect_after(signal_name, handler, *args)
                else:
                    gobj.connect(signal_name, handler, *args)

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


# NOTE: This must come before any other Window/Dialog subclassing, to ensure
# that we have a correct inheritance hierarchy.


class Window(Gtk.Window):
    def __init__(self, type=Gtk.WindowType.TOPLEVEL, **kwds):
        if not initialized:
            raise RuntimeError("Gtk couldn't be initialized")

        # type is a construct-only property; if it is already set (e. g. by
        # GtkBuilder), do not try to set it again and just ignore it
        try:
            self.get_property('type')
            Gtk.Window.__init__(self, **kwds)
        except TypeError:
            Gtk.Window.__init__(self, type=type, **kwds)

Window = override(Window)
__all__.append('Window')


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
        if hasattr(Gtk.DialogFlags, "NO_SEPARATOR") and (flags & Gtk.DialogFlags.NO_SEPARATOR):
            self.set_has_separator(False)
            import warnings
            warnings.warn("Gtk.DialogFlags.NO_SEPARATOR has been depricated since Gtk+-3.0", PyGIDeprecationWarning)

        if buttons is not None:
            self.add_buttons(*buttons)

    action_area = property(lambda dialog: dialog.get_action_area())
    vbox = property(lambda dialog: dialog.get_content_area())

    def add_buttons(self, *args):
        """
        The add_buttons() method adds several buttons to the Gtk.Dialog using
        the button data passed as arguments to the method. This method is the
        same as calling the Gtk.Dialog.add_button() repeatedly. The button data
        pairs - button text (or stock ID) and a response ID integer are passed
        individually. For example:

           dialog.add_buttons(Gtk.STOCK_OPEN, 42, "Close", Gtk.ResponseType.CLOSE)

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
                 message_type=Gtk.MessageType.INFO,
                 buttons=Gtk.ButtonsType.NONE,
                 message_format=None,
                 **kwds):

        if message_format:
            kwds['text'] = message_format

        # type keyword is used for backwards compat with PyGTK
        if 'type' in kwds:
            import warnings
            warnings.warn("The use of the keyword type as a parameter of the Gtk.MessageDialog constructor has been depricated. Please use message_type instead.", PyGIDeprecationWarning)
            message_type = kwds.pop('type')

        Gtk.MessageDialog.__init__(self,
                                   _buttons_property=buttons,
                                   message_type=message_type,
                                   parent=parent,
                                   flags=flags,
                                   **kwds)

    def format_secondary_text(self, message_format):
        self.set_property('secondary-use-markup', False)
        self.set_property('secondary-text', message_format)

    def format_secondary_markup(self, message_format):
        self.set_property('secondary-use-markup', True)
        self.set_property('secondary-text', message_format)

MessageDialog = override(MessageDialog)
__all__.append('MessageDialog')


class AboutDialog(Gtk.AboutDialog):
    def __init__(self, **kwds):
        Gtk.AboutDialog.__init__(self, **kwds)

AboutDialog = override(AboutDialog)
__all__.append('AboutDialog')


class ColorSelectionDialog(Gtk.ColorSelectionDialog):
    def __init__(self, title=None, **kwds):
        Gtk.ColorSelectionDialog.__init__(self, title=title, **kwds)

ColorSelectionDialog = override(ColorSelectionDialog)
__all__.append('ColorSelectionDialog')


class FileChooserDialog(Gtk.FileChooserDialog):
    def __init__(self,
                 title=None,
                 parent=None,
                 action=Gtk.FileChooserAction.OPEN,
                 buttons=None,
                 **kwds):
        Gtk.FileChooserDialog.__init__(self,
                                       action=action,
                                       title=title,
                                       parent=parent,
                                       buttons=buttons,
                                       **kwds)
FileChooserDialog = override(FileChooserDialog)
__all__.append('FileChooserDialog')


class FontSelectionDialog(Gtk.FontSelectionDialog):
    def __init__(self, title=None, **kwds):
        Gtk.FontSelectionDialog.__init__(self, title=title, **kwds)

FontSelectionDialog = override(FontSelectionDialog)
__all__.append('FontSelectionDialog')


class RecentChooserDialog(Gtk.RecentChooserDialog):
    def __init__(self,
                 title=None,
                 parent=None,
                 manager=None,
                 buttons=None,
                 **kwds):

        Gtk.RecentChooserDialog.__init__(self,
                                         recent_manager=manager,
                                         title=title,
                                         parent=parent,
                                         buttons=buttons,
                                         **kwds)

RecentChooserDialog = override(RecentChooserDialog)
__all__.append('RecentChooserDialog')


class IconView(Gtk.IconView):

    def __init__(self, model=None, **kwds):
        Gtk.IconView.__init__(self, model=model, **kwds)

    get_item_at_pos = strip_boolean_result(Gtk.IconView.get_item_at_pos)
    get_visible_range = strip_boolean_result(Gtk.IconView.get_visible_range)
    get_dest_item_at_pos = strip_boolean_result(Gtk.IconView.get_dest_item_at_pos)

IconView = override(IconView)
__all__.append('IconView')


class ToolButton(Gtk.ToolButton):

    def __init__(self, stock_id=None, **kwds):
        Gtk.ToolButton.__init__(self, stock_id=stock_id, **kwds)

ToolButton = override(ToolButton)
__all__.append('ToolButton')


class IMContext(Gtk.IMContext):
    get_surrounding = strip_boolean_result(Gtk.IMContext.get_surrounding)

IMContext = override(IMContext)
__all__.append('IMContext')


class RecentInfo(Gtk.RecentInfo):
    get_application_info = strip_boolean_result(Gtk.RecentInfo.get_application_info)

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

    def insert(self, iter, text, length=-1):
        if not isinstance(text, _basestring):
            raise TypeError('text must be a string, not %s' % type(text))

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

    def insert_at_cursor(self, text, length=-1):
        if not isinstance(text, _basestring):
            raise TypeError('text must be a string, not %s' % type(text))

        Gtk.TextBuffer.insert_at_cursor(self, text, length)

    get_selection_bounds = strip_boolean_result(Gtk.TextBuffer.get_selection_bounds, fail_ret=())

TextBuffer = override(TextBuffer)
__all__.append('TextBuffer')


class TextIter(Gtk.TextIter):

    forward_search = strip_boolean_result(Gtk.TextIter.forward_search)
    backward_search = strip_boolean_result(Gtk.TextIter.backward_search)

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

    def _getiter(self, key):
        if isinstance(key, Gtk.TreeIter):
            return key
        elif isinstance(key, int) and key < 0:
            index = len(self) + key
            if index < 0:
                raise IndexError("row index is out of bounds: %d" % key)
            try:
                aiter = self.get_iter(index)
            except ValueError:
                raise IndexError("could not find tree path '%s'" % key)
            return aiter
        else:
            try:
                aiter = self.get_iter(key)
            except ValueError:
                raise IndexError("could not find tree path '%s'" % key)
            return aiter

    def _coerce_path(self, path):
        if isinstance(path, Gtk.TreePath):
            return path
        else:
            return TreePath(path)

    def __getitem__(self, key):
        aiter = self._getiter(key)
        return TreeModelRow(self, aiter)

    def __setitem__(self, key, value):
        row = self[key]
        self.set_row(row.iter, value)

    def __delitem__(self, key):
        aiter = self._getiter(key)
        self.remove(aiter)

    def __iter__(self):
        return TreeModelRowIter(self, self.get_iter_first())

    get_iter_first = strip_boolean_result(Gtk.TreeModel.get_iter_first)
    iter_children = strip_boolean_result(Gtk.TreeModel.iter_children)
    iter_nth_child = strip_boolean_result(Gtk.TreeModel.iter_nth_child)
    iter_parent = strip_boolean_result(Gtk.TreeModel.iter_parent)
    get_iter_from_string = strip_boolean_result(Gtk.TreeModel.get_iter_from_string,
                                                ValueError, 'invalid tree path')

    def get_iter(self, path):
        path = self._coerce_path(path)
        success, aiter = super(TreeModel, self).get_iter(path)
        if not success:
            raise ValueError("invalid tree path '%s'" % path)
        return aiter

    def iter_next(self, aiter):
        next_iter = aiter.copy()
        success = super(TreeModel, self).iter_next(next_iter)
        if success:
            return next_iter

    def iter_previous(self, aiter):
        prev_iter = aiter.copy()
        success = super(TreeModel, self).iter_previous(prev_iter)
        if success:
            return prev_iter

    def _convert_row(self, row):
        # TODO: Accept a dictionary for row
        # model.append(None,{COLUMN_ICON: icon, COLUMN_NAME: name})
        if isinstance(row, str):
            raise TypeError('Expected a list or tuple, but got str')

        n_columns = self.get_n_columns()
        if len(row) != n_columns:
            raise ValueError('row sequence has the incorrect number of elements')

        result = []
        columns = []
        for cur_col, value in enumerate(row):
            # do not try to set None values, they are causing warnings
            if value is None:
                continue
            result.append(self._convert_value(cur_col, value))
            columns.append(cur_col)
        return (result, columns)

    def set_row(self, treeiter, row):
        converted_row, columns = self._convert_row(row)
        for column in columns:
            value = row[column]
            if value is None:
                continue  # None means skip this row

            self.set_value(treeiter, column, value)

    def _convert_value(self, column, value):
        '''Convert value to a GObject.Value of the expected type'''

        if isinstance(value, GObject.Value):
            return value
        return GObject.Value(self.get_column_type(column), value)

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

    def filter_new(self, root=None):
        return super(TreeModel, self).filter_new(root)

    #
    # Signals supporting python iterables as tree paths
    #
    def row_changed(self, path, iter):
        return super(TreeModel, self).row_changed(self._coerce_path(path), iter)

    def row_inserted(self, path, iter):
        return super(TreeModel, self).row_inserted(self._coerce_path(path), iter)

    def row_has_child_toggled(self, path, iter):
        return super(TreeModel, self).row_has_child_toggled(self._coerce_path(path),
                                                            iter)

    def row_deleted(self, path):
        return super(TreeModel, self).row_deleted(self._coerce_path(path))

    def rows_reordered(self, path, iter, new_order):
        return super(TreeModel, self).rows_reordered(self._coerce_path(path),
                                                     iter, new_order)


TreeModel = override(TreeModel)
__all__.append('TreeModel')


class TreeSortable(Gtk.TreeSortable, ):

    get_sort_column_id = strip_boolean_result(Gtk.TreeSortable.get_sort_column_id, fail_ret=(None, None))

    def set_sort_func(self, sort_column_id, sort_func, user_data=None):
        super(TreeSortable, self).set_sort_func(sort_column_id, sort_func, user_data)

    def set_default_sort_func(self, sort_func, user_data=None):
        super(TreeSortable, self).set_default_sort_func(sort_func, user_data)

TreeSortable = override(TreeSortable)
__all__.append('TreeSortable')


class TreeModelSort(Gtk.TreeModelSort):
    def __init__(self, model, **kwds):
        Gtk.TreeModelSort.__init__(self, model=model, **kwds)

TreeModelSort = override(TreeModelSort)
__all__.append('TreeModelSort')


class ListStore(Gtk.ListStore, TreeModel, TreeSortable):
    def __init__(self, *column_types):
        Gtk.ListStore.__init__(self)
        self.set_column_types(column_types)

    def _do_insert(self, position, row):
        if row is not None:
            row, columns = self._convert_row(row)
            treeiter = self.insert_with_valuesv(position, columns, row)
        else:
            treeiter = Gtk.ListStore.insert(self, position)

        return treeiter

    def append(self, row=None):
        if row:
            return self._do_insert(-1, row)
        # gtk_list_store_insert() does not know about the "position == -1"
        # case, so use append() here
        else:
            return Gtk.ListStore.append(self)

    def prepend(self, row=None):
        return self._do_insert(0, row)

    def insert(self, position, row=None):
        return self._do_insert(position, row)

    # FIXME: sends two signals; check if this can use an atomic
    # insert_with_valuesv()

    def insert_before(self, sibling, row=None):
        treeiter = Gtk.ListStore.insert_before(self, sibling)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    # FIXME: sends two signals; check if this can use an atomic
    # insert_with_valuesv()

    def insert_after(self, sibling, row=None):
        treeiter = Gtk.ListStore.insert_after(self, sibling)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    def set_value(self, treeiter, column, value):
        value = self._convert_value(column, value)
        Gtk.ListStore.set_value(self, treeiter, column, value)

    def set(self, treeiter, *args):

        def _set_lists(columns, values):
            if len(columns) != len(values):
                raise TypeError('The number of columns do not match the number of values')
            for col_num, val in zip(columns, values):
                if not isinstance(col_num, int):
                    raise TypeError('TypeError: Expected integer argument for column.')
                self.set_value(treeiter, col_num, val)

        if args:
            if isinstance(args[0], int):
                columns = args[::2]
                values = args[1::2]
                _set_lists(columns, values)
            elif isinstance(args[0], (tuple, list)):
                if len(args) != 2:
                    raise TypeError('Too many arguments')
                _set_lists(args[0], args[1])
            elif isinstance(args[0], dict):
                columns = args[0].keys()
                values = args[0].values()
                _set_lists(columns, values)
            else:
                raise TypeError('Argument list must be in the form of (column, value, ...), ((columns,...), (values, ...)) or {column: value}.  No -1 termination is needed.')

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
    def previous(self):
        return self.get_previous()

    @property
    def parent(self):
        return self.get_parent()

    def get_next(self):
        next_iter = self.model.iter_next(self.iter)
        if next_iter:
            return TreeModelRow(self.model, next_iter)

    def get_previous(self):
        prev_iter = self.model.iter_previous(self.iter)
        if prev_iter:
            return TreeModelRow(self.model, prev_iter)

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
        elif isinstance(key, slice):
            start, stop, step = key.indices(self.model.get_n_columns())
            alist = []
            for i in range(start, stop, step):
                alist.append(self.model.get_value(self.iter, i))
            return alist
        else:
            raise TypeError("indices must be integers, not %s" % type(key).__name__)

    def __setitem__(self, key, value):
        if isinstance(key, int):
            if key >= self.model.get_n_columns():
                raise IndexError("column index is out of bounds: %d" % key)
            elif key < 0:
                key = self._convert_negative_index(key)
            self.model.set_value(self.iter, key, value)
        elif isinstance(key, slice):
            start, stop, step = key.indices(self.model.get_n_columns())
            indexList = range(start, stop, step)
            if len(indexList) != len(value):
                raise ValueError(
                    "attempt to assign sequence of size %d to slice of size %d"
                    % (len(value), len(indexList)))

            for i, v in enumerate(indexList):
                self.model.set_value(self.iter, v, value[i])
        else:
            raise TypeError("index must be an integer or slice, not %s" % type(key).__name__)

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
        elif not isinstance(path, _basestring):
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

    def __iter__(self):
        return iter(self.get_indices())

    def __len__(self):
        return self.get_depth()

    def __getitem__(self, index):
        return self.get_indices()[index]

TreePath = override(TreePath)
__all__.append('TreePath')


class TreeStore(Gtk.TreeStore, TreeModel, TreeSortable):

    def __init__(self, *column_types):
        Gtk.TreeStore.__init__(self)
        self.set_column_types(column_types)

    def _do_insert(self, parent, position, row):
        if row is not None:
            row, columns = self._convert_row(row)
            treeiter = self.insert_with_values(parent, position, columns, row)
        else:
            treeiter = Gtk.TreeStore.insert(self, parent, position)

        return treeiter

    def append(self, parent, row=None):
        return self._do_insert(parent, -1, row)

    def prepend(self, parent, row=None):
        return self._do_insert(parent, 0, row)

    def insert(self, parent, position, row=None):
        return self._do_insert(parent, position, row)

    # FIXME: sends two signals; check if this can use an atomic
    # insert_with_valuesv()

    def insert_before(self, parent, sibling, row=None):
        treeiter = Gtk.TreeStore.insert_before(self, parent, sibling)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    # FIXME: sends two signals; check if this can use an atomic
    # insert_with_valuesv()

    def insert_after(self, parent, sibling, row=None):
        treeiter = Gtk.TreeStore.insert_after(self, parent, sibling)

        if row is not None:
            self.set_row(treeiter, row)

        return treeiter

    def set_value(self, treeiter, column, value):
        value = self._convert_value(column, value)
        Gtk.TreeStore.set_value(self, treeiter, column, value)

    def set(self, treeiter, *args):

        def _set_lists(columns, values):
            if len(columns) != len(values):
                raise TypeError('The number of columns do not match the number of values')
            for col_num, val in zip(columns, values):
                if not isinstance(col_num, int):
                    raise TypeError('TypeError: Expected integer argument for column.')
                self.set_value(treeiter, col_num, val)

        if args:
            if isinstance(args[0], int):
                columns = args[::2]
                values = args[1::2]
                _set_lists(columns, values)
            elif isinstance(args[0], (tuple, list)):
                if len(args) != 2:
                    raise TypeError('Too many arguments')
                _set_lists(args[0], args[1])
            elif isinstance(args[0], dict):
                columns = args[0].keys()
                values = args[0].values()
                _set_lists(columns, values)
            else:
                raise TypeError('Argument list must be in the form of (column, value, ...), ((columns,...), (values, ...)) or {column: value}.  No -1 termination is needed.')

TreeStore = override(TreeStore)
__all__.append('TreeStore')


class TreeView(Gtk.TreeView, Container):

    def __init__(self, model=None):
        Gtk.TreeView.__init__(self)
        if model:
            self.set_model(model)

    get_path_at_pos = strip_boolean_result(Gtk.TreeView.get_path_at_pos)
    get_visible_range = strip_boolean_result(Gtk.TreeView.get_visible_range)
    get_dest_row_at_pos = strip_boolean_result(Gtk.TreeView.get_dest_row_at_pos)

    def enable_model_drag_source(self, start_button_mask, targets, actions):
        target_entries = _construct_target_list(targets)
        super(TreeView, self).enable_model_drag_source(start_button_mask,
                                                       target_entries,
                                                       actions)

    def enable_model_drag_dest(self, targets, actions):
        target_entries = _construct_target_list(targets)
        super(TreeView, self).enable_model_drag_dest(target_entries,
                                                     actions)

    def scroll_to_cell(self, path, column=None, use_align=False, row_align=0.0, col_align=0.0):
        if not isinstance(path, Gtk.TreePath):
            path = TreePath(path)
        super(TreeView, self).scroll_to_cell(path, column, use_align, row_align, col_align)

    def set_cursor(self, path, column=None, start_editing=False):
        if not isinstance(path, Gtk.TreePath):
            path = TreePath(path)
        super(TreeView, self).set_cursor(path, column, start_editing)

    def get_cell_area(self, path, column=None):
        if not isinstance(path, Gtk.TreePath):
            path = TreePath(path)
        return super(TreeView, self).get_cell_area(path, column)

    def insert_column_with_attributes(self, position, title, cell, **kwargs):
        column = TreeViewColumn()
        column.set_title(title)
        column.pack_start(cell, False)
        self.insert_column(column, position)
        column.set_attributes(cell, **kwargs)

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

    cell_get_position = strip_boolean_result(Gtk.TreeViewColumn.cell_get_position)

    def set_cell_data_func(self, cell_renderer, func, func_data=None):
        super(TreeViewColumn, self).set_cell_data_func(cell_renderer, func, func_data)

    def set_attributes(self, cell_renderer, **attributes):
        Gtk.CellLayout.clear_attributes(self, cell_renderer)

        for (name, value) in attributes.items():
            Gtk.CellLayout.add_attribute(self, cell_renderer, name, value)


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
    def __init__(self, label=None, stock=None, use_stock=False, use_underline=False, **kwds):
        if stock:
            label = stock
            use_stock = True
            use_underline = True
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

        # PyGTK compatiblity
        if 'page_incr' in new_args:
            new_args['page_increment'] = new_args.pop('page_incr')
        if 'step_incr' in new_args:
            new_args['step_increment'] = new_args.pop('step_incr')
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

    def attach(self, child, left_attach, right_attach, top_attach, bottom_attach, xoptions=Gtk.AttachOptions.EXPAND | Gtk.AttachOptions.FILL, yoptions=Gtk.AttachOptions.EXPAND | Gtk.AttachOptions.FILL, xpadding=0, ypadding=0):
        Gtk.Table.attach(self, child, left_attach, right_attach, top_attach, bottom_attach, xoptions, yoptions, xpadding, ypadding)

Table = override(Table)
__all__.append('Table')


class ScrolledWindow(Gtk.ScrolledWindow):
    def __init__(self, hadjustment=None, vadjustment=None, **kwds):
        Gtk.ScrolledWindow.__init__(self, hadjustment=hadjustment, vadjustment=vadjustment, **kwds)

ScrolledWindow = override(ScrolledWindow)
__all__.append('ScrolledWindow')


class HScrollbar(Gtk.HScrollbar):
    def __init__(self, adjustment=None, **kwds):
        Gtk.HScrollbar.__init__(self, adjustment=adjustment, **kwds)

HScrollbar = override(HScrollbar)
__all__.append('HScrollbar')


class VScrollbar(Gtk.VScrollbar):
    def __init__(self, adjustment=None, **kwds):
        Gtk.VScrollbar.__init__(self, adjustment=adjustment, **kwds)

VScrollbar = override(VScrollbar)
__all__.append('VScrollbar')


class Paned(Gtk.Paned):
    def pack1(self, child, resize=False, shrink=True):
        super(Paned, self).pack1(child, resize, shrink)

    def pack2(self, child, resize=True, shrink=True):
        super(Paned, self).pack2(child, resize, shrink)

Paned = override(Paned)
__all__.append('Paned')


class Arrow(Gtk.Arrow):
    def __init__(self, arrow_type, shadow_type, **kwds):
        Gtk.Arrow.__init__(self, arrow_type=arrow_type,
                           shadow_type=shadow_type,
                           **kwds)

Arrow = override(Arrow)
__all__.append('Arrow')


class IconSet(Gtk.IconSet):
    def __new__(cls, pixbuf=None):
        if pixbuf is not None:
            iconset = Gtk.IconSet.new_from_pixbuf(pixbuf)
        else:
            iconset = Gtk.IconSet.__new__(cls)
        return iconset

IconSet = override(IconSet)
__all__.append('IconSet')


class Viewport(Gtk.Viewport):
    def __init__(self, hadjustment=None, vadjustment=None, **kwds):
        Gtk.Viewport.__init__(self, hadjustment=hadjustment,
                              vadjustment=vadjustment,
                              **kwds)

Viewport = override(Viewport)
__all__.append('Viewport')


class TreeModelFilter(Gtk.TreeModelFilter):
    def set_visible_func(self, func, data=None):
        super(TreeModelFilter, self).set_visible_func(func, data)

    def set_value(self, iter, column, value):
        # Delegate to child model
        iter = self.convert_iter_to_child_iter(iter)
        self.get_model().set_value(iter, column, value)

TreeModelFilter = override(TreeModelFilter)
__all__.append('TreeModelFilter')

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

stock_lookup = strip_boolean_result(Gtk.stock_lookup)
__all__.append('stock_lookup')

initialized, argv = Gtk.init_check(sys.argv)
sys.argv = list(argv)

# -*- Mode: Python; py-indent-offset: 4 -*-
# coding: UTF-8
# vim: tabstop=4 shiftwidth=4 expandtab

from __future__ import absolute_import

import contextlib
import unittest
import time
import sys
import gc
import warnings

from .helper import ignore_gi_deprecation_warnings, capture_glib_warnings

import gi.overrides
import gi.types
from gi.repository import GLib, GObject

try:
    from gi.repository import Gtk, GdkPixbuf, Gdk
    PyGTKDeprecationWarning = Gtk.PyGTKDeprecationWarning
    Gtk_version = Gtk._version
except ImportError:
    Gtk = None
    Gtk_version = None
    PyGTKDeprecationWarning = None
    GdkPixbuf = None
    Gdk = None


def gtkver():
    if Gtk is None:
        return (0, 0, 0)
    return (Gtk.get_major_version(),
            Gtk.get_minor_version(),
            Gtk.get_micro_version())


GTK4 = (Gtk._version == "4.0")


@contextlib.contextmanager
def realized(widget):
    """Makes sure the widget is realized.

    view = Gtk.TreeView()
    with realized(view):
        do_something(view)
    """

    if isinstance(widget, Gtk.Window):
        toplevel = widget
    else:
        if Gtk._version == "4.0":
            toplevel = widget.get_parent_surface()
        else:
            toplevel = widget.get_parent_window()

    if toplevel is None:
        window = Gtk.Window()
        window.add(widget)

    widget.realize()
    while Gtk.events_pending():
        Gtk.main_iteration()
    assert widget.get_realized()
    yield widget

    if toplevel is None:
        window.remove(widget)
        window.destroy()

    while Gtk.events_pending():
        Gtk.main_iteration()


@unittest.skipUnless(Gtk, 'Gtk not available')
def test_freeze_child_notif():

    events = []

    def on_notify(widget, spec):
        events.append(spec.name)

    b = Gtk.Box()
    c = Gtk.Button()
    c.connect("child-notify", on_notify)
    c.freeze_child_notify()
    if GTK4:
        b.pack_start(c)
    else:
        b.pack_start(c, True, True, 0)
    b.child_set_property(c, "pack-type", Gtk.PackType.END)
    b.child_set_property(c, "pack-type", Gtk.PackType.START)
    c.thaw_child_notify()
    assert events.count("pack-type") == 1
    del events[:]

    with c.freeze_child_notify():
        b.child_set_property(c, "pack-type", Gtk.PackType.END)
        b.child_set_property(c, "pack-type", Gtk.PackType.START)

    assert events.count("pack-type") == 1


@unittest.skipUnless(Gtk, 'Gtk not available')
def test_wrapper_toggle_refs():
    class MyButton(Gtk.Button):
        def __init__(self, height):
            Gtk.Button.__init__(self)
            self._height = height

        def do_measure(self, orientation, for_size):
            if orientation == Gtk.Orientation.VERTICAL:
                return (self._height, self._height, -1, -1)
            else:
                return (0, 0, -1, -1)

        def do_get_preferred_height(self):
            return (self._height, self._height)

    height = 142
    w = Gtk.Window()
    b = MyButton(height)
    w.add(b)
    if not GTK4:
        b.show_all()
    del b
    gc.collect()
    gc.collect()
    if GTK4:
        # XXX: Why?
        assert w.get_preferred_size().minimum_size.height == height + 10
    else:
        assert w.get_preferred_size().minimum_size.height == height


@unittest.skipUnless(Gtk, 'Gtk not available')
@ignore_gi_deprecation_warnings
class TestGtk(unittest.TestCase):
    def test_container(self):
        box = Gtk.Box()
        self.assertTrue(isinstance(box, Gtk.Box))
        self.assertTrue(isinstance(box, Gtk.Container))
        self.assertTrue(isinstance(box, Gtk.Widget))
        self.assertTrue(box)
        label = Gtk.Label()
        label2 = Gtk.Label()
        box.add(label)
        box.add(label2)
        self.assertTrue(label in box)
        self.assertTrue(label2 in box)
        self.assertEqual(len(box), 2)
        self.assertTrue(box)
        labels = [x for x in box]
        self.assertEqual(labels, [label, label2])

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_actions(self):
        self.assertEqual(Gtk.Action, gi.overrides.Gtk.Action)
        action = Gtk.Action(name="test", label="Test", tooltip="Test Action", stock_id=Gtk.STOCK_COPY)
        self.assertEqual(action.get_name(), "test")
        self.assertEqual(action.get_label(), "Test")
        self.assertEqual(action.get_tooltip(), "Test Action")
        self.assertEqual(action.get_stock_id(), Gtk.STOCK_COPY)

        self.assertEqual(Gtk.RadioAction, gi.overrides.Gtk.RadioAction)
        action = Gtk.RadioAction(name="test", label="Test", tooltip="Test Action", stock_id=Gtk.STOCK_COPY, value=1)
        self.assertEqual(action.get_name(), "test")
        self.assertEqual(action.get_label(), "Test")
        self.assertEqual(action.get_tooltip(), "Test Action")
        self.assertEqual(action.get_stock_id(), Gtk.STOCK_COPY)
        self.assertEqual(action.get_current_value(), 1)

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_actiongroup(self):
        self.assertEqual(Gtk.ActionGroup, gi.overrides.Gtk.ActionGroup)

        action_group = Gtk.ActionGroup(name='TestActionGroup')
        callback_data = "callback data"

        def test_action_callback_data(action, user_data):
            self.assertEqual(user_data, callback_data)

        def test_radio_action_callback_data(action, current, user_data):
            self.assertEqual(user_data, callback_data)

        action_group.add_actions([
            ('test-action1', None, 'Test Action 1',
             None, None, test_action_callback_data),
            ('test-action2', Gtk.STOCK_COPY, 'Test Action 2',
             None, None, test_action_callback_data)], callback_data)
        action_group.add_toggle_actions([
            ('test-toggle-action1', None, 'Test Toggle Action 1',
             None, None, test_action_callback_data, False),
            ('test-toggle-action2', Gtk.STOCK_COPY, 'Test Toggle Action 2',
             None, None, test_action_callback_data, True)], callback_data)
        action_group.add_radio_actions([
            ('test-radio-action1', None, 'Test Radio Action 1'),
            ('test-radio-action2', Gtk.STOCK_COPY, 'Test Radio Action 2')], 1,
            test_radio_action_callback_data,
            callback_data)

        expected_results = [('test-action1', Gtk.Action),
                            ('test-action2', Gtk.Action),
                            ('test-toggle-action1', Gtk.ToggleAction),
                            ('test-toggle-action2', Gtk.ToggleAction),
                            ('test-radio-action1', Gtk.RadioAction),
                            ('test-radio-action2', Gtk.RadioAction)]

        for action in action_group.list_actions():
            a = (action.get_name(), type(action))
            self.assertTrue(a in expected_results)
            expected_results.remove(a)
            action.activate()

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_uimanager(self):
        self.assertEqual(Gtk.UIManager, gi.overrides.Gtk.UIManager)
        ui = Gtk.UIManager()
        ui.add_ui_from_string("""<ui>
    <menubar name="menubar1"></menubar>
</ui>
"""
)
        menubar = ui.get_widget("/menubar1")
        self.assertEqual(type(menubar), Gtk.MenuBar)

        ag = Gtk.ActionGroup(name="ag1")
        ui.insert_action_group(ag)
        ag2 = Gtk.ActionGroup(name="ag2")
        ui.insert_action_group(ag2)
        groups = ui.get_action_groups()
        self.assertEqual(ag, groups[-2])
        self.assertEqual(ag2, groups[-1])

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_uimanager_nonascii(self):
        ui = Gtk.UIManager()
        ui.add_ui_from_string(b'<ui><menubar name="menub\xc3\xa6r1" /></ui>'.decode('UTF-8'))
        mi = ui.get_widget("/menubær1")
        self.assertEqual(type(mi), Gtk.MenuBar)

    def test_window(self):
        # standard Window
        w = Gtk.Window()
        self.assertEqual(w.get_property('type'), Gtk.WindowType.TOPLEVEL)

        # type works as keyword argument
        w = Gtk.Window(type=Gtk.WindowType.POPUP)
        self.assertEqual(w.get_property('type'), Gtk.WindowType.POPUP)

        class TestWindow(Gtk.Window):
            __gtype_name__ = "TestWindow"

        # works from builder
        builder = Gtk.Builder()
        builder.add_from_string('''
<interface>
  <object class="GtkWindow" id="win">
    <property name="type">popup</property>
  </object>
  <object class="TestWindow" id="testwin">
  </object>
  <object class="TestWindow" id="testpop">
    <property name="type">popup</property>
  </object>
</interface>''')
        self.assertEqual(builder.get_object('win').get_property('type'),
                         Gtk.WindowType.POPUP)
        self.assertEqual(builder.get_object('testwin').get_property('type'),
                         Gtk.WindowType.TOPLEVEL)
        self.assertEqual(builder.get_object('testpop').get_property('type'),
                         Gtk.WindowType.POPUP)

    def test_dialog_classes(self):
        self.assertEqual(Gtk.Dialog, gi.overrides.Gtk.Dialog)
        self.assertEqual(Gtk.FileChooserDialog, gi.overrides.Gtk.FileChooserDialog)
        if not GTK4:
            self.assertEqual(Gtk.RecentChooserDialog, gi.overrides.Gtk.RecentChooserDialog)
            self.assertEqual(Gtk.ColorSelectionDialog, gi.overrides.Gtk.ColorSelectionDialog)
            self.assertEqual(Gtk.FontSelectionDialog, gi.overrides.Gtk.FontSelectionDialog)

    def test_dialog_base(self):
        dialog = Gtk.Dialog(title='Foo', modal=True)
        self.assertTrue(isinstance(dialog, Gtk.Dialog))
        self.assertTrue(isinstance(dialog, Gtk.Window))
        self.assertEqual('Foo', dialog.get_title())
        self.assertTrue(dialog.get_modal())

    def test_dialog_deprecations(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            dialog = Gtk.Dialog(title='Foo', flags=Gtk.DialogFlags.MODAL)
            self.assertTrue(dialog.get_modal())
            self.assertEqual(len(warn), 1)
            self.assertTrue(issubclass(warn[0].category, PyGTKDeprecationWarning))
            self.assertRegexpMatches(str(warn[0].message),
                                     '.*flags.*modal.*')

        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            dialog = Gtk.Dialog(title='Foo', flags=Gtk.DialogFlags.DESTROY_WITH_PARENT)
            self.assertTrue(dialog.get_destroy_with_parent())
            self.assertEqual(len(warn), 1)
            self.assertTrue(issubclass(warn[0].category, PyGTKDeprecationWarning))
            self.assertRegexpMatches(str(warn[0].message),
                                     '.*flags.*destroy_with_parent.*')

    def test_dialog_deprecation_stacklevels(self):
        # Test warning levels are setup to give the correct filename for
        # deprecations in different classes in the inheritance hierarchy.

        # Base class
        self.assertEqual(Gtk.Dialog, gi.overrides.Gtk.Dialog)
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            Gtk.Dialog(flags=Gtk.DialogFlags.MODAL)
            self.assertEqual(len(warn), 1)
            self.assertRegexpMatches(warn[0].filename, '.*test_overrides_gtk.*')

        # Validate overridden base with overridden sub-class.
        self.assertEqual(Gtk.MessageDialog, gi.overrides.Gtk.MessageDialog)
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            Gtk.MessageDialog(flags=Gtk.DialogFlags.MODAL)
            self.assertEqual(len(warn), 1)
            self.assertRegexpMatches(warn[0].filename, '.*test_overrides_gtk.*')

        # Validate overridden base with non-overridden sub-class.
        self.assertEqual(Gtk.AboutDialog, gi.repository.Gtk.AboutDialog)
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            Gtk.AboutDialog(flags=Gtk.DialogFlags.MODAL)
            self.assertEqual(len(warn), 1)
            self.assertRegexpMatches(warn[0].filename, '.*test_overrides_gtk.*')

    def test_dialog_add_buttons(self):
        # The overloaded "buttons" keyword gives a warning when attempting
        # to use it for adding buttons as was available in PyGTK.
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            dialog = Gtk.Dialog(title='Foo', modal=True,
                                buttons=('test-button1', 1))
            self.assertEqual(len(warn), 1)
            self.assertTrue(issubclass(warn[0].category, PyGTKDeprecationWarning))
            self.assertRegexpMatches(str(warn[0].message),
                                     '.*ButtonsType.*add_buttons.*')

        dialog.add_buttons('test-button2', 2, 'gtk-close', Gtk.ResponseType.CLOSE)
        button = dialog.get_widget_for_response(1)
        self.assertEqual('test-button1', button.get_label())
        button = dialog.get_widget_for_response(2)
        self.assertEqual('test-button2', button.get_label())
        button = dialog.get_widget_for_response(Gtk.ResponseType.CLOSE)
        self.assertEqual('gtk-close', button.get_label())

    def test_about_dialog(self):
        dialog = Gtk.AboutDialog()
        self.assertTrue(isinstance(dialog, Gtk.Dialog))
        self.assertTrue(isinstance(dialog, Gtk.Window))

        # AboutDialog is not sub-classed in overrides, make sure
        # the mro still injects the base class "add_buttons" override.
        self.assertTrue(hasattr(dialog, 'add_buttons'))

    def test_message_dialog(self):
        dialog = Gtk.MessageDialog(title='message dialog test',
                                   modal=True,
                                   buttons=Gtk.ButtonsType.OK,
                                   text='dude!')
        self.assertTrue(isinstance(dialog, Gtk.Dialog))
        self.assertTrue(isinstance(dialog, Gtk.Window))

        self.assertEqual('message dialog test', dialog.get_title())
        self.assertTrue(dialog.get_modal())
        text = dialog.get_property('text')
        self.assertEqual('dude!', text)

        dialog.format_secondary_text('2nd text')
        self.assertEqual(dialog.get_property('secondary-text'), '2nd text')
        self.assertFalse(dialog.get_property('secondary-use-markup'))

        dialog.format_secondary_markup('2nd markup')
        self.assertEqual(dialog.get_property('secondary-text'), '2nd markup')
        self.assertTrue(dialog.get_property('secondary-use-markup'))

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_color_selection_dialog(self):
        dialog = Gtk.ColorSelectionDialog(title="color selection dialog test")
        self.assertTrue(isinstance(dialog, Gtk.Dialog))
        self.assertTrue(isinstance(dialog, Gtk.Window))
        self.assertEqual('color selection dialog test', dialog.get_title())

    def test_file_chooser_dialog(self):
        # might cause a GVFS warning, do not break on this
        with capture_glib_warnings(allow_warnings=True):
            dialog = Gtk.FileChooserDialog(title='file chooser dialog test',
                                           action=Gtk.FileChooserAction.SAVE)

        self.assertTrue(isinstance(dialog, Gtk.Dialog))
        self.assertTrue(isinstance(dialog, Gtk.Window))
        self.assertEqual('file chooser dialog test', dialog.get_title())

        action = dialog.get_property('action')
        self.assertEqual(Gtk.FileChooserAction.SAVE, action)

    def test_file_chooser_dialog_default_action(self):
        # might cause a GVFS warning, do not break on this
        with capture_glib_warnings(allow_warnings=True):
            dialog = Gtk.FileChooserDialog(title='file chooser dialog test')

        action = dialog.get_property('action')
        self.assertEqual(Gtk.FileChooserAction.OPEN, action)

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_font_selection_dialog(self):
        dialog = Gtk.FontSelectionDialog(title="font selection dialog test")
        self.assertTrue(isinstance(dialog, Gtk.Dialog))
        self.assertTrue(isinstance(dialog, Gtk.Window))
        self.assertEqual('font selection dialog test', dialog.get_title())

    @unittest.skipIf(GTK4, "not in gtk4")
    def test_recent_chooser_dialog(self):
        test_manager = Gtk.RecentManager()
        dialog = Gtk.RecentChooserDialog(title='recent chooser dialog test',
                                         recent_manager=test_manager)
        self.assertTrue(isinstance(dialog, Gtk.Dialog))
        self.assertTrue(isinstance(dialog, Gtk.Window))
        self.assertEqual('recent chooser dialog test', dialog.get_title())

    class TestClass(GObject.GObject):
        __gtype_name__ = "GIOverrideTreeAPITest"

        def __init__(self, tester, int_value, string_value):
            super(TestGtk.TestClass, self).__init__()
            self.tester = tester
            self.int_value = int_value
            self.string_value = string_value

        def check(self, int_value, string_value):
            self.tester.assertEqual(int_value, self.int_value)
            self.tester.assertEqual(string_value, self.string_value)

    def test_buttons(self):
        self.assertEqual(Gtk.Button, gi.overrides.Gtk.Button)

        # test Gtk.Button
        button = Gtk.Button()
        self.assertTrue(isinstance(button, Gtk.Button))
        self.assertTrue(isinstance(button, Gtk.Container))
        self.assertTrue(isinstance(button, Gtk.Widget))

        if Gtk_version != "4.0":
            # Using stock items causes hard warning in devel versions of GTK+.
            with capture_glib_warnings(allow_warnings=True):
                button = Gtk.Button.new_from_stock(Gtk.STOCK_CLOSE)

            self.assertEqual(Gtk.STOCK_CLOSE, button.get_label())
            self.assertTrue(button.get_use_stock())
            self.assertTrue(button.get_use_underline())

            # test Gtk.Button use_stock
            button = Gtk.Button(label=Gtk.STOCK_CLOSE, use_stock=True,
                                use_underline=True)
            self.assertEqual(Gtk.STOCK_CLOSE, button.get_label())
            self.assertTrue(button.get_use_stock())
            self.assertTrue(button.get_use_underline())

        # test Gtk.LinkButton
        button = Gtk.LinkButton(uri='http://www.Gtk.org', label='Gtk')
        self.assertTrue(isinstance(button, Gtk.Button))
        self.assertTrue(isinstance(button, Gtk.Container))
        self.assertTrue(isinstance(button, Gtk.Widget))
        self.assertEqual('http://www.Gtk.org', button.get_uri())
        self.assertEqual('Gtk', button.get_label())

    def test_inheritance(self):
        for name in gi.overrides.Gtk.__all__:
            over = getattr(gi.overrides.Gtk, name)
            for element in dir(Gtk):
                try:
                    klass = getattr(Gtk, element)
                    info = klass.__info__
                except (NotImplementedError, AttributeError):
                    continue

                # Get all parent classes and interfaces klass inherits from
                if isinstance(info, gi.types.ObjectInfo):
                    classes = list(info.get_interfaces())
                    parent = info.get_parent()
                    while parent.get_name() != "Object":
                        classes.append(parent)
                        parent = parent.get_parent()
                    classes = [kl for kl in classes if kl.get_namespace() == "Gtk"]
                else:
                    continue

                for kl in classes:
                    if kl.get_name() == name:
                        self.assertTrue(issubclass(klass, over,),
                                        "%r does not inherit from override %r" % (klass, over,))

    def test_editable(self):
        self.assertEqual(Gtk.Editable, gi.overrides.Gtk.Editable)

        # need to use Gtk.Entry because Editable is an interface
        entry = Gtk.Entry()
        pos = entry.insert_text('HeWorld', 0)
        self.assertEqual(pos, 7)
        pos = entry.insert_text('llo ', 2)
        self.assertEqual(pos, 6)
        text = entry.get_chars(0, 11)
        self.assertEqual('Hello World', text)

    def test_label(self):
        label = Gtk.Label(label='Hello')
        self.assertTrue(isinstance(label, Gtk.Widget))
        self.assertEqual(label.get_text(), 'Hello')

    def adjustment_check(self, adjustment, value=0.0, lower=0.0, upper=0.0,
                         step_increment=0.0, page_increment=0.0, page_size=0.0):
        self.assertEqual(adjustment.get_value(), value)
        self.assertEqual(adjustment.get_lower(), lower)
        self.assertEqual(adjustment.get_upper(), upper)
        self.assertEqual(adjustment.get_step_increment(), step_increment)
        self.assertEqual(adjustment.get_page_increment(), page_increment)
        self.assertEqual(adjustment.get_page_size(), page_size)

    def test_adjustment(self):
        adjustment = Gtk.Adjustment(value=1, lower=0, upper=6, step_increment=4, page_increment=5, page_size=3)
        self.adjustment_check(adjustment, value=1, lower=0, upper=6, step_increment=4, page_increment=5, page_size=3)

        adjustment = Gtk.Adjustment(value=1, lower=0, upper=6, step_increment=4, page_increment=5)
        self.adjustment_check(adjustment, value=1, lower=0, upper=6, step_increment=4, page_increment=5)

        adjustment = Gtk.Adjustment(value=1, lower=0, upper=6, step_increment=4)
        self.adjustment_check(adjustment, value=1, lower=0, upper=6, step_increment=4)

        adjustment = Gtk.Adjustment(value=1, lower=0, upper=6)
        self.adjustment_check(adjustment, value=1, lower=0, upper=6)

        adjustment = Gtk.Adjustment()
        self.adjustment_check(adjustment)

        adjustment = Gtk.Adjustment(1, -1, 3, 0, 0, 0)
        self.adjustment_check(adjustment, value=1, lower=-1, upper=3)

        adjustment = Gtk.Adjustment(1, -1, 3, 0, 0, 0, value=2)
        self.adjustment_check(adjustment, value=2, lower=-1, upper=3)

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_table(self):
        table = Gtk.Table()
        self.assertTrue(isinstance(table, Gtk.Table))
        self.assertTrue(isinstance(table, Gtk.Container))
        self.assertTrue(isinstance(table, Gtk.Widget))
        self.assertEqual(table.get_size(), (1, 1))
        self.assertEqual(table.get_homogeneous(), False)

        table = Gtk.Table(n_rows=2, n_columns=3)
        self.assertEqual(table.get_size(), (2, 3))
        self.assertEqual(table.get_homogeneous(), False)

        table = Gtk.Table(n_rows=2, n_columns=3, homogeneous=True)
        self.assertEqual(table.get_size(), (2, 3))
        self.assertEqual(table.get_homogeneous(), True)

        label = Gtk.Label(label='Hello')
        self.assertTrue(isinstance(label, Gtk.Widget))
        table.attach(label, 0, 1, 0, 1)
        self.assertEqual(label, table.get_children()[0])

    def test_scrolledwindow(self):
        sw = Gtk.ScrolledWindow()
        self.assertTrue(isinstance(sw, Gtk.ScrolledWindow))
        self.assertTrue(isinstance(sw, Gtk.Container))
        self.assertTrue(isinstance(sw, Gtk.Widget))
        sb = sw.get_hscrollbar()
        self.assertEqual(sw.get_hadjustment(), sb.get_adjustment())
        sb = sw.get_vscrollbar()
        self.assertEqual(sw.get_vadjustment(), sb.get_adjustment())

    def test_widget_drag_methods(self):
        widget = Gtk.Button()

        # here we are not checking functionality, only that the methods exist
        # and except the right number of arguments

        widget.drag_check_threshold(0, 0, 0, 0)

        # drag_dest_ methods
        widget.drag_dest_set(Gtk.DestDefaults.DROP, None, Gdk.DragAction.COPY)
        widget.drag_dest_add_image_targets()
        widget.drag_dest_add_text_targets()
        widget.drag_dest_add_uri_targets()
        widget.drag_dest_get_track_motion()
        widget.drag_dest_set_track_motion(True)
        widget.drag_dest_get_target_list()
        widget.drag_dest_set_target_list(None)
        if GTK4:
            widget.drag_dest_set_target_list(Gdk.ContentFormats.new([]))
        else:
            widget.drag_dest_set_target_list(Gtk.TargetList.new([Gtk.TargetEntry.new('test', 0, 0)]))
        widget.drag_dest_unset()

        widget.drag_highlight()
        widget.drag_unhighlight()

        # drag_source_ methods
        widget.drag_source_set(Gdk.ModifierType.BUTTON1_MASK, None, Gdk.DragAction.MOVE)
        widget.drag_source_add_image_targets()
        widget.drag_source_add_text_targets()
        widget.drag_source_add_uri_targets()
        widget.drag_source_set_icon_name("_About")
        if not GTK4:
            widget.drag_source_set_icon_pixbuf(GdkPixbuf.Pixbuf())
            widget.drag_source_set_icon_stock(Gtk.STOCK_ABOUT)
        widget.drag_source_get_target_list()
        widget.drag_source_set_target_list(None)
        if GTK4:
            widget.drag_source_set_target_list(Gdk.ContentFormats.new([]))
        else:
            widget.drag_source_set_target_list(Gtk.TargetList.new([Gtk.TargetEntry.new('test', 0, 0)]))
        widget.drag_source_unset()

        # these methods cannot be called because they require a valid drag on
        # a real GdkWindow. So we only check that they exist and are callable.
        if not GTK4:
            self.assertTrue(hasattr(widget, 'drag_dest_set_proxy'))
        self.assertTrue(hasattr(widget, 'drag_get_data'))

    @unittest.skipIf(sys.platform == "darwin", "crashes")
    @unittest.skipIf(GTK4, "uses lots of gtk3 only api")
    def test_drag_target_list_gtk3(self):
        mixed_target_list = [Gtk.TargetEntry.new('test0', 0, 0),
                             ('test1', 1, 1),
                             Gtk.TargetEntry.new('test2', 2, 2),
                             ('test3', 3, 3)]

        def _test_target_list(targets):
            for i, target in enumerate(targets):
                self.assertTrue(isinstance(target, Gtk.TargetEntry))
                self.assertEqual(target.target, 'test' + str(i))
                self.assertEqual(target.flags, i)
                self.assertEqual(target.info, i)

        _test_target_list(Gtk._construct_target_list(mixed_target_list))

        widget = Gtk.Button()
        widget.drag_dest_set(Gtk.DestDefaults.DROP, None, Gdk.DragAction.COPY)
        widget.drag_dest_set_target_list(mixed_target_list)
        widget.drag_dest_get_target_list()

        widget.drag_source_set(Gdk.ModifierType.BUTTON1_MASK, None, Gdk.DragAction.MOVE)
        widget.drag_source_set_target_list(mixed_target_list)
        widget.drag_source_get_target_list()

        treeview = Gtk.TreeView()
        treeview.enable_model_drag_source(Gdk.ModifierType.BUTTON1_MASK,
                                          mixed_target_list,
                                          Gdk.DragAction.DEFAULT | Gdk.DragAction.MOVE)

        treeview.enable_model_drag_dest(mixed_target_list,
                                        Gdk.DragAction.DEFAULT | Gdk.DragAction.MOVE)

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_scrollbar(self):
        adjustment = Gtk.Adjustment()

        hscrollbar = Gtk.HScrollbar()
        vscrollbar = Gtk.VScrollbar()
        self.assertNotEqual(hscrollbar.props.adjustment, adjustment)
        self.assertNotEqual(vscrollbar.props.adjustment, adjustment)

        hscrollbar = Gtk.HScrollbar(adjustment=adjustment)
        vscrollbar = Gtk.VScrollbar(adjustment=adjustment)
        self.assertEqual(hscrollbar.props.adjustment, adjustment)
        self.assertEqual(vscrollbar.props.adjustment, adjustment)

    def test_iconview(self):
        # PyGTK compat
        iconview = Gtk.IconView()
        self.assertEqual(iconview.props.model, None)

        model = Gtk.ListStore(str)
        iconview = Gtk.IconView(model=model)
        self.assertEqual(iconview.props.model, model)

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_toolbutton(self):
        # PyGTK compat

        # Using stock items causes hard warning in devel versions of GTK+.
        with capture_glib_warnings(allow_warnings=True):
            button = Gtk.ToolButton()
            self.assertEqual(button.props.stock_id, None)

            button = Gtk.ToolButton(stock_id='gtk-new')
            self.assertEqual(button.props.stock_id, 'gtk-new')

        icon = Gtk.Image.new_from_stock(Gtk.STOCK_OPEN, Gtk.IconSize.SMALL_TOOLBAR)
        button = Gtk.ToolButton(label='mylabel', icon_widget=icon)
        self.assertEqual(button.props.label, 'mylabel')
        self.assertEqual(button.props.icon_widget, icon)

    def test_toolbutton_gtk4(self):
        icon = Gtk.Image.new()
        button = Gtk.ToolButton(label='mylabel', icon_widget=icon)
        self.assertEqual(button.props.label, 'mylabel')
        self.assertEqual(button.props.icon_widget, icon)

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_iconset(self):
        Gtk.IconSet()
        pixbuf = GdkPixbuf.Pixbuf()
        Gtk.IconSet.new_from_pixbuf(pixbuf)

    def test_viewport(self):
        vadjustment = Gtk.Adjustment()
        hadjustment = Gtk.Adjustment()

        viewport = Gtk.Viewport(hadjustment=hadjustment,
                                vadjustment=vadjustment)

        self.assertEqual(viewport.props.vadjustment, vadjustment)
        self.assertEqual(viewport.props.hadjustment, hadjustment)

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_stock_lookup(self):
        stock_item = Gtk.stock_lookup('gtk-ok')
        self.assertEqual(type(stock_item), Gtk.StockItem)
        self.assertEqual(stock_item.stock_id, 'gtk-ok')
        self.assertEqual(Gtk.stock_lookup('nosuchthing'), None)

    def test_gtk_main(self):
        # with no arguments
        GLib.idle_add(Gtk.main_quit)
        Gtk.main()

        # overridden function ignores its arguments
        GLib.idle_add(Gtk.main_quit, 'hello')
        Gtk.main()

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_widget_render_icon(self):
        button = Gtk.Button(label='OK')
        pixbuf = button.render_icon(Gtk.STOCK_OK, Gtk.IconSize.BUTTON)
        self.assertTrue(pixbuf is not None)


@unittest.skipUnless(Gtk, 'Gtk not available')
@unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
class TestWidget(unittest.TestCase):
    def test_style_get_property_gvalue(self):
        button = Gtk.Button()
        value = GObject.Value(int, -42)
        button.style_get_property('focus-padding', value)
        # Test only that the style property changed since we can't actuall
        # set it.
        self.assertNotEqual(value.get_int(), -42)

    def test_style_get_property_return_with_explicit_gvalue(self):
        button = Gtk.Button()
        value = GObject.Value(int, -42)
        result = button.style_get_property('focus-padding', value)
        self.assertIsInstance(result, int)
        self.assertNotEqual(result, -42)

    def test_style_get_property_return_with_implicit_gvalue(self):
        button = Gtk.Button()
        result = button.style_get_property('focus-padding')
        self.assertIsInstance(result, int)
        self.assertNotEqual(result, -42)

    def test_style_get_property_error(self):
        button = Gtk.Button()
        with self.assertRaises(ValueError):
            button.style_get_property('not-a-valid-style-property')


@unittest.skipIf(sys.platform == "darwin", "hangs")
@unittest.skipUnless(Gtk, 'Gtk not available')
class TestSignals(unittest.TestCase):
    def test_class_closure_override_with_aliased_type(self):
        class WindowWithSizeAllocOverride(Gtk.ScrolledWindow):
            __gsignals__ = {'size-allocate': 'override'}

            def __init__(self):
                Gtk.ScrolledWindow.__init__(self)
                self._alloc_called = False
                self._alloc_value = None
                self._alloc_error = None

            def do_size_allocate(self, *args):
                self._alloc_called = True
                self._alloc_value = args[0]

                try:
                    Gtk.ScrolledWindow.do_size_allocate(self, *args)
                except Exception as e:
                    self._alloc_error = e

        win = WindowWithSizeAllocOverride()
        rect = Gdk.Rectangle()
        rect.width = 100
        rect.height = 100

        with realized(win):
            win.show()
            win.get_preferred_size()
            if GTK4:
                win.size_allocate(rect, 0)
            else:
                win.size_allocate(rect)
            self.assertTrue(win._alloc_called)
            self.assertIsInstance(win._alloc_value, Gdk.Rectangle)
            self.assertTrue(win._alloc_error is None, win._alloc_error)

    @unittest.expectedFailure  # https://bugzilla.gnome.org/show_bug.cgi?id=735693
    def test_overlay_child_position(self):
        def get_child_position(overlay, widget, rect, user_data=None):
            rect.x = 1
            rect.y = 2
            rect.width = 3
            rect.height = 4
            return True

        overlay = Gtk.Overlay()
        overlay.connect('get-child-position', get_child_position)

        rect = Gdk.Rectangle()
        rect.x = -1
        rect.y = -1
        rect.width = -1
        rect.height = -1

        overlay.emit('get-child-position', None, rect)
        self.assertEqual(rect.x, 1)
        self.assertEqual(rect.y, 2)
        self.assertEqual(rect.width, 3)
        self.assertEqual(rect.height, 4)


@unittest.skipUnless(Gtk, 'Gtk not available')
class TestBuilder(unittest.TestCase):
    class SignalTest(GObject.GObject):
        __gtype_name__ = "GIOverrideSignalTest"
        __gsignals__ = {
            "test-signal": (GObject.SignalFlags.RUN_FIRST,
                            None,
                            []),
        }

    def test_add_from_string(self):
        builder = Gtk.Builder()
        builder.add_from_string(u"")
        builder.add_from_string("")

        def get_example(string):
            return u"""\
<interface>
  <menu id="appmenu">
    <section>
      <item>
        <attribute name="label">%s</attribute>
      </item>
    </section>
  </menu>
</interface>""" % string

        builder.add_from_string(get_example(u"ä" * 1000))

        builder = Gtk.Builder()
        builder.add_objects_from_string(u"", [''])
        builder.add_objects_from_string("", [''])
        builder.add_objects_from_string(get_example(u"ä" * 1000), [''])

    def test_extract_handler_and_args_object(self):
        class Obj():
            pass

        obj = Obj()
        obj.foo = lambda: None

        handler, args = Gtk._extract_handler_and_args(obj, 'foo')
        self.assertEqual(handler, obj.foo)
        self.assertEqual(len(args), 0)

    def test_extract_handler_and_args_dict(self):
        obj = {'foo': lambda: None}

        handler, args = Gtk._extract_handler_and_args(obj, 'foo')
        self.assertEqual(handler, obj['foo'])
        self.assertEqual(len(args), 0)

    def test_extract_handler_and_args_with_seq(self):
        obj = {'foo': (lambda: None, 1, 2)}

        handler, args = Gtk._extract_handler_and_args(obj, 'foo')
        self.assertEqual(handler, obj['foo'][0])
        self.assertSequenceEqual(args, [1, 2])

    def test_extract_handler_and_args_no_handler_error(self):
        obj = dict(foo=lambda: None)
        self.assertRaises(AttributeError,
                          Gtk._extract_handler_and_args,
                          obj, 'not_a_handler')

    def test_builder_with_handler_and_args(self):
        builder = Gtk.Builder()
        builder.add_from_string("""
            <interface>
              <object class="GIOverrideSignalTest" id="object_sig_test">
                  <signal name="test-signal" handler="on_signal1" />
                  <signal name="test-signal" handler="on_signal2" after="yes" />
              </object>
            </interface>
            """)

        args_collector = []

        def on_signal(*args):
            args_collector.append(args)

        builder.connect_signals({'on_signal1': (on_signal, 1, 2),
                                 'on_signal2': on_signal})

        objects = builder.get_objects()
        self.assertEqual(len(objects), 1)
        obj, = objects
        obj.emit('test-signal')

        self.assertEqual(len(args_collector), 2)
        self.assertSequenceEqual(args_collector[0], (obj, 1, 2))
        self.assertSequenceEqual(args_collector[1], (obj, ))

    def test_builder(self):
        self.assertEqual(Gtk.Builder, gi.overrides.Gtk.Builder)

        class SignalCheck:
            def __init__(self):
                self.sentinel = 0
                self.after_sentinel = 0

            def on_signal_1(self, *args):
                self.sentinel += 1
                self.after_sentinel += 1

            def on_signal_3(self, *args):
                self.sentinel += 3

            def on_signal_after(self, *args):
                if self.after_sentinel == 1:
                    self.after_sentinel += 1

        signal_checker = SignalCheck()
        builder = Gtk.Builder()

        # add object1 to the builder
        builder.add_from_string("""
<interface>
  <object class="GIOverrideSignalTest" id="object1">
      <signal name="test-signal" after="yes" handler="on_signal_after" />
      <signal name="test-signal" handler="on_signal_1" />
  </object>
</interface>
""")

        # only add object3 to the builder
        builder.add_objects_from_string("""
<interface>
  <object class="GIOverrideSignalTest" id="object2">
      <signal name="test-signal" handler="on_signal_2" />
  </object>
  <object class="GIOverrideSignalTest" id="object3">
      <signal name="test-signal" handler="on_signal_3" />
  </object>
  <object class="GIOverrideSignalTest" id="object4">
      <signal name="test-signal" handler="on_signal_4" />
  </object>
</interface>
""", ['object3'])

        # hook up signals
        builder.connect_signals(signal_checker)

        # call their notify signals and check sentinel
        objects = builder.get_objects()
        self.assertEqual(len(objects), 2)
        for obj in objects:
            obj.emit('test-signal')

        self.assertEqual(signal_checker.sentinel, 4)
        self.assertEqual(signal_checker.after_sentinel, 2)


@ignore_gi_deprecation_warnings
@unittest.skipUnless(Gtk, 'Gtk not available')
class TestTreeModel(unittest.TestCase):
    def test_tree_model_sort(self):
        self.assertEqual(Gtk.TreeModelSort, gi.overrides.Gtk.TreeModelSort)
        model = Gtk.TreeStore(int, bool)
        model_sort = Gtk.TreeModelSort(model=model)
        self.assertEqual(model_sort.get_model(), model)

    def test_tree_store(self):
        self.assertEqual(Gtk.TreeStore, gi.overrides.Gtk.TreeStore)
        self.assertEqual(Gtk.ListStore, gi.overrides.Gtk.ListStore)
        self.assertEqual(Gtk.TreeModel, gi.overrides.Gtk.TreeModel)
        self.assertEqual(Gtk.TreeViewColumn, gi.overrides.Gtk.TreeViewColumn)

        class TestPyObject(object):
            pass

        test_pyobj = TestPyObject()
        test_pydict = {1: 1, "2": 2, "3": "3"}
        test_pylist = [1, "2", "3"]
        tree_store = Gtk.TreeStore(int,
                                   'gchararray',
                                   TestGtk.TestClass,
                                   GObject.TYPE_PYOBJECT,
                                   object,
                                   object,
                                   object,
                                   bool,
                                   bool,
                                   GObject.TYPE_UINT,
                                   GObject.TYPE_ULONG,
                                   GObject.TYPE_INT64,
                                   GObject.TYPE_UINT64,
                                   GObject.TYPE_UCHAR,
                                   GObject.TYPE_CHAR)

        parent = None
        for i in range(97):
            label = 'this is child #%d' % i
            testobj = TestGtk.TestClass(self, i, label)
            parent = tree_store.append(parent, (i,
                                                label,
                                                testobj,
                                                testobj,
                                                test_pyobj,
                                                test_pydict,
                                                test_pylist,
                                                i % 2,
                                                bool(i % 2),
                                                i,
                                                GLib.MAXULONG,
                                                GLib.MININT64,
                                                0xffffffffffffffff,
                                                254,
                                                b'a'
                                                ))
        # test set
        parent = tree_store.append(parent)
        i = 97
        label = 'this is child #%d' % i
        testobj = TestGtk.TestClass(self, i, label)
        tree_store.set(parent,
                       0, i,
                       2, testobj,
                       1, label,
                       3, testobj,
                       4, test_pyobj,
                       5, test_pydict,
                       6, test_pylist,
                       7, i % 2,
                       8, bool(i % 2),
                       9, i,
                       10, GLib.MAXULONG,
                       11, GLib.MININT64,
                       12, 0xffffffffffffffff,
                       13, 254,
                       14, b'a')

        parent = tree_store.append(parent)
        i = 98
        label = 'this is child #%d' % i
        testobj = TestGtk.TestClass(self, i, label)
        tree_store.set(parent, {0: i,
                                2: testobj,
                                1: label,
                                3: testobj,
                                4: test_pyobj,
                                5: test_pydict,
                                6: test_pylist,
                                7: i % 2,
                                8: bool(i % 2),
                                9: i,
                                10: GLib.MAXULONG,
                                11: GLib.MININT64,
                                12: 0xffffffffffffffff,
                                13: 254,
                                14: b'a'})

        parent = tree_store.append(parent)
        i = 99
        label = 'this is child #%d' % i
        testobj = TestGtk.TestClass(self, i, label)
        tree_store.set(parent, (0, 2, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14),
                               (i,
                                testobj,
                                label,
                                testobj,
                                test_pyobj,
                                test_pydict,
                                test_pylist,
                                i % 2,
                                bool(i % 2),
                                i,
                                GLib.MAXULONG,
                                GLib.MININT64,
                                0xffffffffffffffff,
                                254,
                                b'a'))

        # len gets the number of children in the root node
        # since we kept appending to the previous node
        # there should only be one child of the root
        self.assertEqual(len(tree_store), 1)

        # walk the tree to see if the values were stored correctly
        parent = None
        i = 0

        treeiter = tree_store.iter_children(parent)
        while treeiter:
            i = tree_store.get_value(treeiter, 0)
            s = tree_store.get_value(treeiter, 1)
            obj = tree_store.get_value(treeiter, 2)
            obj.check(i, s)
            obj2 = tree_store.get_value(treeiter, 3)
            self.assertEqual(obj, obj2)

            pyobj = tree_store.get_value(treeiter, 4)
            self.assertEqual(pyobj, test_pyobj)
            pydict = tree_store.get_value(treeiter, 5)
            self.assertEqual(pydict, test_pydict)
            pylist = tree_store.get_value(treeiter, 6)
            self.assertEqual(pylist, test_pylist)

            bool_1 = tree_store.get_value(treeiter, 7)
            bool_2 = tree_store.get_value(treeiter, 8)
            self.assertEqual(bool_1, bool_2)
            self.assertTrue(isinstance(bool_1, bool))
            self.assertTrue(isinstance(bool_2, bool))

            uint_ = tree_store.get_value(treeiter, 9)
            self.assertEqual(uint_, i)
            ulong_ = tree_store.get_value(treeiter, 10)
            self.assertEqual(ulong_, GLib.MAXULONG)
            int64_ = tree_store.get_value(treeiter, 11)
            self.assertEqual(int64_, GLib.MININT64)
            uint64_ = tree_store.get_value(treeiter, 12)
            self.assertEqual(uint64_, 0xffffffffffffffff)
            uchar_ = tree_store.get_value(treeiter, 13)
            self.assertEqual(ord(uchar_), 254)
            char_ = tree_store.get_value(treeiter, 14)
            self.assertEqual(char_, 'a')

            parent = treeiter
            treeiter = tree_store.iter_children(parent)

        self.assertEqual(i, 99)

    def test_tree_store_signals(self):
        tree_store = Gtk.TreeStore(int, bool)

        def on_row_inserted(tree_store, tree_path, tree_iter, signal_list):
            signal_list.append('row-inserted')

        def on_row_changed(tree_store, tree_path, tree_iter, signal_list):
            signal_list.append('row-changed')

        signals = []
        tree_store.connect('row-inserted', on_row_inserted, signals)
        tree_store.connect('row-changed', on_row_changed, signals)

        # adding rows with and without data should only call one signal
        tree_store.append(None, (0, False))
        self.assertEqual(signals, ['row-inserted'])

        signals.pop()
        tree_store.append(None)
        self.assertEqual(signals, ['row-inserted'])

        signals.pop()
        tree_store.prepend(None, (0, False))
        self.assertEqual(signals, ['row-inserted'])

        signals.pop()
        tree_store.prepend(None)
        self.assertEqual(signals, ['row-inserted'])

        signals.pop()
        tree_store.insert(None, 1, (0, False))
        self.assertEqual(signals, ['row-inserted'])

        signals.pop()
        tree_store.insert(None, 1)
        self.assertEqual(signals, ['row-inserted'])

        # One set one signal
        signals.pop()
        tree_iter = tree_store.append(None, (10, False))
        tree_store.set(tree_iter, (0, 1), (20, True))
        self.assertEqual(signals, ['row-inserted', 'row-changed'])

    def test_list_store(self):
        class TestPyObject(object):
            pass

        test_pyobj = TestPyObject()
        test_pydict = {1: 1, "2": 2, "3": "3"}
        test_pylist = [1, "2", "3"]

        list_store = Gtk.ListStore(int, str, 'GIOverrideTreeAPITest', object, object, object, bool, bool)
        for i in range(1, 93):
            label = 'this is row #%d' % i
            testobj = TestGtk.TestClass(self, i, label)
            list_store.append((i,
                               label,
                               testobj,
                               test_pyobj,
                               test_pydict,
                               test_pylist,
                               i % 2,
                               bool(i % 2)))

        i = 93
        label = u'this is row #93'
        treeiter = list_store.append()
        list_store.set_value(treeiter, 0, i)
        list_store.set_value(treeiter, 1, label)
        list_store.set_value(treeiter, 2, TestGtk.TestClass(self, i, label))
        list_store.set_value(treeiter, 3, test_pyobj)
        list_store.set_value(treeiter, 4, test_pydict)
        list_store.set_value(treeiter, 5, test_pylist)
        list_store.set_value(treeiter, 6, 1)
        list_store.set_value(treeiter, 7, True)

        # test prepend
        label = 'this is row #0'
        list_store.prepend((0,
                            label,
                            TestGtk.TestClass(self, 0, label),
                            test_pyobj,
                            test_pydict,
                            test_pylist,
                            0,
                            False))

        # test automatic unicode->str conversion
        i = 94
        label = u'this is row #94'
        treeiter = list_store.append((i,
                                      label,
                                      TestGtk.TestClass(self, i, label),
                                      test_pyobj,
                                      test_pydict,
                                      test_pylist,
                                      0,
                                      False))

        # add sorted items out of order to test insert* apis
        # also test sending in None to not set a column
        i = 97
        label = 'this is row #97'
        treeiter = list_store.append((None,
                                      None,
                                      None,
                                      test_pyobj,
                                      None,
                                      test_pylist,
                                      1,
                                      None))

        list_store.set_value(treeiter, 0, i)
        list_store.set_value(treeiter, 1, label)
        list_store.set_value(treeiter, 2, TestGtk.TestClass(self, i, label))
        list_store.set_value(treeiter, 4, test_pydict)
        list_store.set_value(treeiter, 7, True)

        # this should append
        i = 99
        label = 'this is row #99'
        list_store.insert(9999, (i,
                                 label,
                                 TestGtk.TestClass(self, i, label),
                                 test_pyobj,
                                 test_pydict,
                                 test_pylist,
                                 1,
                                 True))

        i = 96
        label = 'this is row #96'
        list_store.insert_before(treeiter, (i,
                                            label,
                                            TestGtk.TestClass(self, i, label),
                                            test_pyobj,
                                            test_pydict,
                                            test_pylist,
                                            0,
                                            False))

        i = 98
        label = 'this is row #98'
        list_store.insert_after(treeiter, (i,
                                           label,
                                           TestGtk.TestClass(self, i, label),
                                           test_pyobj,
                                           test_pydict,
                                           test_pylist,
                                           0,
                                           False))

        i = 95
        label = 'this is row #95'
        list_store.insert(95, (i,
                               label,
                               TestGtk.TestClass(self, i, label),
                               test_pyobj,
                               test_pydict,
                               test_pylist,
                               1,
                               True))

        i = 100
        label = 'this is row #100'
        treeiter = list_store.append()
        list_store.set(treeiter,
                       1, label,
                       0, i,
                       2, TestGtk.TestClass(self, i, label),
                       3, test_pyobj,
                       4, test_pydict,
                       5, test_pylist,
                       6, 0,
                       7, False)
        i = 101
        label = 'this is row #101'
        treeiter = list_store.append()
        list_store.set(treeiter, {1: label,
                                  0: i,
                                  2: TestGtk.TestClass(self, i, label),
                                  3: test_pyobj,
                                  4: test_pydict,
                                  5: test_pylist,
                                  6: 1,
                                  7: True})
        i = 102
        label = 'this is row #102'
        treeiter = list_store.append()
        list_store.set(treeiter, (1, 0, 2, 3, 4, 5, 6, 7),
                                 (label,
                                  i,
                                  TestGtk.TestClass(self, i, label),
                                  test_pyobj,
                                  test_pydict,
                                  test_pylist,
                                  0,
                                  False))

        self.assertEqual(len(list_store), 103)

        # walk the list to see if the values were stored correctly
        i = 0
        treeiter = list_store.get_iter_first()

        counter = 0
        while treeiter:
            i = list_store.get_value(treeiter, 0)
            self.assertEqual(i, counter)
            s = list_store.get_value(treeiter, 1)
            obj = list_store.get_value(treeiter, 2)
            obj.check(i, s)

            pyobj = list_store.get_value(treeiter, 3)
            self.assertEqual(pyobj, test_pyobj)
            pydict = list_store.get_value(treeiter, 4)
            self.assertEqual(pydict, test_pydict)
            pylist = list_store.get_value(treeiter, 5)
            self.assertEqual(pylist, test_pylist)

            bool_1 = list_store.get_value(treeiter, 6)
            bool_2 = list_store.get_value(treeiter, 7)
            self.assertEqual(bool_1, bool_2)
            self.assertTrue(isinstance(bool_1, bool))
            self.assertTrue(isinstance(bool_2, bool))

            treeiter = list_store.iter_next(treeiter)

            counter += 1

        self.assertEqual(i, 102)

    def test_list_store_sort(self):
        def comp1(model, row1, row2, user_data):
            v1 = model[row1][1]
            v2 = model[row2][1]

            # make "m" smaller than anything else
            if v1.startswith('m') and not v2.startswith('m'):
                return -1
            if v2.startswith('m') and not v1.startswith('m'):
                return 1
            return (v1 > v2) - (v1 < v2)

        list_store = Gtk.ListStore(int, str)
        list_store.set_sort_func(2, comp1, None)
        list_store.append((1, 'apples'))
        list_store.append((3, 'oranges'))
        list_store.append((2, 'mango'))

        # not sorted yet, should be original order
        self.assertEqual([list(i) for i in list_store],
                         [[1, 'apples'], [3, 'oranges'], [2, 'mango']])

        # sort with our custom function
        list_store.set_sort_column_id(2, Gtk.SortType.ASCENDING)
        self.assertEqual([list(i) for i in list_store],
                         [[2, 'mango'], [1, 'apples'], [3, 'oranges']])

        list_store.set_sort_column_id(2, Gtk.SortType.DESCENDING)
        self.assertEqual([list(i) for i in list_store],
                         [[3, 'oranges'], [1, 'apples'], [2, 'mango']])

    def test_list_store_signals(self):
        list_store = Gtk.ListStore(int, bool)

        def on_row_inserted(list_store, tree_path, tree_iter, signal_list):
            signal_list.append('row-inserted')

        def on_row_changed(list_store, tree_path, tree_iter, signal_list):
            signal_list.append('row-changed')

        signals = []
        list_store.connect('row-inserted', on_row_inserted, signals)
        list_store.connect('row-changed', on_row_changed, signals)

        # adding rows with and without data should only call one signal
        list_store.append((0, False))
        self.assertEqual(signals, ['row-inserted'])

        signals.pop()
        list_store.append()
        self.assertEqual(signals, ['row-inserted'])

        signals.pop()
        list_store.prepend((0, False))
        self.assertEqual(signals, ['row-inserted'])

        signals.pop()
        list_store.prepend()
        self.assertEqual(signals, ['row-inserted'])

        signals.pop()
        list_store.insert(1, (0, False))
        self.assertEqual(signals, ['row-inserted'])

        signals.pop()
        list_store.insert(1)
        self.assertEqual(signals, ['row-inserted'])

        # One set one signal
        signals.pop()
        tree_iter = list_store.append((10, False))
        list_store.set(tree_iter, (0, 1), (20, True))
        self.assertEqual(signals, ['row-inserted', 'row-changed'])

    def test_list_store_insert_before(self):
        store = Gtk.ListStore(object)
        signals = []

        def on_row_inserted(store, tree_path, tree_iter, signal_list):
            signal_list.append('row-inserted')

        def on_row_changed(store, tree_path, tree_iter, signal_list):
            signal_list.append('row-changed')

        store.connect('row-inserted', on_row_inserted, signals)
        store.connect('row-changed', on_row_changed, signals)

        iter_ = store.append([0])
        assert store.get_value(iter_, 0) == 0
        assert signals == ['row-inserted']
        del signals[:]

        # append empty
        iter_ = store.insert_before(None)
        assert store.get_path(iter_).get_indices() == [1]
        assert store.get_value(iter_, 0) is None
        assert signals == ['row-inserted']
        del signals[:]

        # insert empty
        iter_ = store.insert_before(iter_)
        assert store.get_path(iter_).get_indices() == [1]
        assert store.get_value(iter_, 0) is None
        assert signals == ['row-inserted']
        del signals[:]

        # append non-empty
        iter_ = store.insert_before(None, [1234])
        assert store.get_path(iter_).get_indices() == [3]
        assert store.get_value(iter_, 0) == 1234
        assert signals == ['row-inserted']
        del signals[:]

        # insert non-empty
        iter_ = store.insert_before(iter_, [4321])
        assert store.get_path(iter_).get_indices() == [3]
        assert store.get_value(iter_, 0) == 4321
        assert signals == ['row-inserted']
        del signals[:]

        assert [r[0] for r in store] == [0, None, None, 4321, 1234]

    def test_list_store_insert_after(self):
        store = Gtk.ListStore(object)
        signals = []

        def on_row_inserted(store, tree_path, tree_iter, signal_list):
            signal_list.append('row-inserted')

        def on_row_changed(store, tree_path, tree_iter, signal_list):
            signal_list.append('row-changed')

        store.connect('row-inserted', on_row_inserted, signals)
        store.connect('row-changed', on_row_changed, signals)

        iter_ = store.append([0])
        assert store.get_value(iter_, 0) == 0
        assert signals == ['row-inserted']
        del signals[:]

        # prepend empty
        iter_ = store.insert_after(None)
        assert store.get_path(iter_).get_indices() == [0]
        assert store.get_value(iter_, 0) is None
        assert signals == ['row-inserted']
        del signals[:]

        # insert empty
        iter_ = store.insert_after(iter_)
        assert store.get_path(iter_).get_indices() == [1]
        assert store.get_value(iter_, 0) is None
        assert signals == ['row-inserted']
        del signals[:]

        # prepend non-empty
        iter_ = store.insert_after(None, [1234])
        assert store.get_path(iter_).get_indices() == [0]
        assert store.get_value(iter_, 0) == 1234
        assert signals == ['row-inserted']
        del signals[:]

        # insert non-empty
        iter_ = store.insert_after(iter_, [4321])
        assert store.get_path(iter_).get_indices() == [1]
        assert store.get_value(iter_, 0) == 4321
        assert signals == ['row-inserted']
        del signals[:]

        assert [r[0] for r in store] == [1234, 4321, None, None, 0]

    def test_tree_store_insert_before(self):
        store = Gtk.TreeStore(object)
        signals = []

        def on_row_inserted(store, tree_path, tree_iter, signal_list):
            signal_list.append('row-inserted')

        def on_row_changed(store, tree_path, tree_iter, signal_list):
            signal_list.append('row-changed')

        store.connect('row-inserted', on_row_inserted, signals)
        store.connect('row-changed', on_row_changed, signals)

        parent = store.append(None, [-1])
        assert signals == ['row-inserted']
        del signals[:]

        iter_ = store.append(parent, [0])
        assert store.get_path(iter_).get_indices() == [0, 0]
        assert store.get_value(iter_, 0) == 0
        assert signals == ['row-inserted']
        del signals[:]

        # append empty
        iter_ = store.insert_before(parent, None)
        assert store.get_path(iter_).get_indices() == [0, 1]
        assert store.get_value(iter_, 0) is None
        assert signals == ['row-inserted']
        del signals[:]

        # insert empty
        iter_ = store.insert_before(parent, iter_)
        assert store.get_path(iter_).get_indices() == [0, 1]
        assert store.get_value(iter_, 0) is None
        assert signals == ['row-inserted']
        del signals[:]

        # append non-empty
        iter_ = store.insert_before(parent, None, [1234])
        assert store.get_path(iter_).get_indices() == [0, 3]
        assert store.get_value(iter_, 0) == 1234
        assert signals == ['row-inserted']
        del signals[:]

        # insert non-empty
        iter_ = store.insert_before(parent, iter_, [4321])
        assert store.get_path(iter_).get_indices() == [0, 3]
        assert store.get_value(iter_, 0) == 4321
        assert signals == ['row-inserted']
        del signals[:]

        def func(model, path, iter_, rows):
            rows.append((path.get_indices(), model[iter_][:]))

        rows = []
        store.foreach(func, rows)
        assert rows == [
            ([0], [-1]), ([0, 0], [0]), ([0, 1], [None]), ([0, 2], [None]),
            ([0, 3], [4321]), ([0, 4], [1234])]

    def test_tree_store_insert_after(self):
        store = Gtk.TreeStore(object)
        signals = []

        def on_row_inserted(store, tree_path, tree_iter, signal_list):
            signal_list.append('row-inserted')

        def on_row_changed(store, tree_path, tree_iter, signal_list):
            signal_list.append('row-changed')

        store.connect('row-inserted', on_row_inserted, signals)
        store.connect('row-changed', on_row_changed, signals)

        parent = store.append(None, [-1])
        assert signals == ['row-inserted']
        del signals[:]

        iter_ = store.append(parent, [0])
        assert store.get_path(iter_).get_indices() == [0, 0]
        assert store.get_value(iter_, 0) == 0
        assert signals == ['row-inserted']
        del signals[:]

        # append empty
        iter_ = store.insert_after(parent, None)
        assert store.get_path(iter_).get_indices() == [0, 0]
        assert store.get_value(iter_, 0) is None
        assert signals == ['row-inserted']
        del signals[:]

        # insert empty
        iter_ = store.insert_after(parent, iter_)
        assert store.get_path(iter_).get_indices() == [0, 1]
        assert store.get_value(iter_, 0) is None
        assert signals == ['row-inserted']
        del signals[:]

        # append non-empty
        iter_ = store.insert_after(parent, None, [1234])
        assert store.get_path(iter_).get_indices() == [0, 0]
        assert store.get_value(iter_, 0) == 1234
        assert signals == ['row-inserted']
        del signals[:]

        # insert non-empty
        iter_ = store.insert_after(parent, iter_, [4321])
        assert store.get_path(iter_).get_indices() == [0, 1]
        assert store.get_value(iter_, 0) == 4321
        assert signals == ['row-inserted']
        del signals[:]

        def func(model, path, iter_, rows):
            rows.append((path.get_indices(), model[iter_][:]))

        rows = []
        store.foreach(func, rows)

        assert rows == [
            ([0], [-1]), ([0, 0], [1234]), ([0, 1], [4321]),
            ([0, 2], [None]), ([0, 3], [None]), ([0, 4], [0])]

    def test_tree_path(self):
        p1 = Gtk.TreePath()
        p2 = Gtk.TreePath.new_first()
        self.assertEqual(p1, p2)
        self.assertEqual(str(p1), '0')
        self.assertEqual(len(p1), 1)
        p1 = Gtk.TreePath(2)
        p2 = Gtk.TreePath.new_from_string('2')
        self.assertEqual(p1, p2)
        self.assertEqual(str(p1), '2')
        self.assertEqual(len(p1), 1)
        p1 = Gtk.TreePath('1:2:3')
        p2 = Gtk.TreePath.new_from_string('1:2:3')
        self.assertEqual(p1, p2)
        self.assertEqual(str(p1), '1:2:3')
        self.assertEqual(len(p1), 3)
        p1 = Gtk.TreePath((1, 2, 3))
        p2 = Gtk.TreePath.new_from_string('1:2:3')
        self.assertEqual(p1, p2)
        self.assertEqual(str(p1), '1:2:3')
        self.assertEqual(len(p1), 3)
        self.assertNotEqual(p1, None)
        self.assertTrue(p1 > None)
        self.assertTrue(p1 >= None)
        self.assertFalse(p1 < None)
        self.assertFalse(p1 <= None)

        self.assertEqual(tuple(p1), (1, 2, 3))
        self.assertEqual(p1[0], 1)
        self.assertEqual(p1[1], 2)
        self.assertEqual(p1[2], 3)
        self.assertRaises(IndexError, p1.__getitem__, 3)

    def test_tree_path_empty(self):
        p1 = Gtk.TreePath.new()
        assert str(p1) == ""

    def test_tree_model(self):
        tree_store = Gtk.TreeStore(int, str)

        self.assertTrue(tree_store)
        self.assertEqual(len(tree_store), 0)
        self.assertEqual(tree_store.get_iter_first(), None)

        def get_by_index(row, col=None):
            if col:
                return tree_store[row][col]
            else:
                return tree_store[row]

        self.assertRaises(TypeError, get_by_index, None)
        self.assertRaises(TypeError, get_by_index, "")
        self.assertRaises(TypeError, get_by_index, ())

        self.assertRaises(IndexError, get_by_index, "0")
        self.assertRaises(IndexError, get_by_index, 0)
        self.assertRaises(IndexError, get_by_index, (0,))

        self.assertRaises(ValueError, tree_store.get_iter, "0")
        self.assertRaises(ValueError, tree_store.get_iter, 0)
        self.assertRaises(ValueError, tree_store.get_iter, (0,))

        self.assertRaises(ValueError, tree_store.get_iter_from_string, "0")

        for row in tree_store:
            self.fail("Should not be reached")

        class DerivedIntType(int):
            pass

        class DerivedStrType(str):
            pass

        for i in range(100):
            label = 'this is row #%d' % i
            parent = tree_store.append(None, (DerivedIntType(i), DerivedStrType(label),))
            self.assertNotEqual(parent, None)
            for j in range(20):
                label = 'this is child #%d of node #%d' % (j, i)
                child = tree_store.append(parent, (j, label,))
                self.assertNotEqual(child, None)

        self.assertTrue(tree_store)
        self.assertEqual(len(tree_store), 100)

        self.assertEqual(tree_store.iter_previous(tree_store.get_iter(0)), None)

        for i, row in enumerate(tree_store):
            self.assertEqual(row.model, tree_store)
            self.assertEqual(row.parent, None)

            self.assertEqual(tree_store[i].path, row.path)
            self.assertEqual(tree_store[str(i)].path, row.path)
            self.assertEqual(tree_store[(i,)].path, row.path)

            self.assertEqual(tree_store[i][0], i)
            self.assertEqual(tree_store[i][1], "this is row #%d" % i)

            aiter = tree_store.get_iter(i)
            self.assertEqual(tree_store.get_path(aiter), row.path)

            aiter = tree_store.get_iter(str(i))
            self.assertEqual(tree_store.get_path(aiter), row.path)

            aiter = tree_store.get_iter((i,))
            self.assertEqual(tree_store.get_path(aiter), row.path)

            self.assertEqual(tree_store.iter_parent(aiter), row.parent)

            next = tree_store.iter_next(aiter)
            if i < len(tree_store) - 1:
                self.assertEqual(tree_store.get_path(next), row.next.path)
                self.assertEqual(tree_store.get_path(tree_store.iter_previous(next)),
                                 tree_store.get_path(aiter))
            else:
                self.assertEqual(next, None)

            self.assertEqual(tree_store.iter_n_children(row.iter), 20)

            child = tree_store.iter_children(row.iter)
            for j, childrow in enumerate(row.iterchildren()):
                child_path = tree_store.get_path(child)
                self.assertEqual(childrow.path, child_path)
                self.assertEqual(childrow.parent.path, row.path)
                self.assertEqual(childrow.path, tree_store[child].path)
                self.assertEqual(childrow.path, tree_store[child_path].path)

                self.assertEqual(childrow[0], tree_store[child][0])
                self.assertEqual(childrow[0], j)
                self.assertEqual(childrow[1], tree_store[child][1])
                self.assertEqual(childrow[1], 'this is child #%d of node #%d' % (j, i))

                self.assertRaises(IndexError, get_by_index, child, 2)

                tree_store[child][1] = 'this was child #%d of node #%d' % (j, i)
                self.assertEqual(childrow[1], 'this was child #%d of node #%d' % (j, i))

                nth_child = tree_store.iter_nth_child(row.iter, j)
                self.assertEqual(childrow.path, tree_store.get_path(nth_child))

                childrow2 = tree_store["%d:%d" % (i, j)]
                self.assertEqual(childrow.path, childrow2.path)

                childrow2 = tree_store[(i, j,)]
                self.assertEqual(childrow.path, childrow2.path)

                child = tree_store.iter_next(child)
                if j < 19:
                    self.assertEqual(childrow.next.path, tree_store.get_path(child))
                else:
                    self.assertEqual(child, childrow.next)
                    self.assertEqual(child, None)

            self.assertEqual(j, 19)

        self.assertEqual(i, 99)

        # negative indices
        for i in range(-1, -100, -1):
            i_real = i + 100
            self.assertEqual(tree_store[i][0], i_real)

            row = tree_store[i]
            for j in range(-1, -20, -1):
                j_real = j + 20
                path = (i_real, j_real,)

                self.assertEqual(tree_store[path][-2], j_real)

                label = 'this was child #%d of node #%d' % (j_real, i_real)
                self.assertEqual(tree_store[path][-1], label)

                new_label = 'this still is child #%d of node #%d' % (j_real, i_real)
                tree_store[path][-1] = new_label
                self.assertEqual(tree_store[path][-1], new_label)

                self.assertRaises(IndexError, get_by_index, path, -3)

        self.assertRaises(IndexError, get_by_index, -101)

        last_row = tree_store[99]
        self.assertNotEqual(last_row, None)

        for i, childrow in enumerate(last_row.iterchildren()):
            if i < 19:
                self.assertTrue(tree_store.remove(childrow.iter))
            else:
                self.assertFalse(tree_store.remove(childrow.iter))

        self.assertEqual(i, 19)

        self.assertEqual(tree_store.iter_n_children(last_row.iter), 0)
        for childrow in last_row.iterchildren():
            self.fail("Should not be reached")

        aiter = tree_store.get_iter(10)
        self.assertRaises(TypeError, tree_store.get, aiter, 1, 'a')
        self.assertRaises(ValueError, tree_store.get, aiter, 1, -1)
        self.assertRaises(ValueError, tree_store.get, aiter, 1, 100)
        self.assertEqual(tree_store.get(aiter, 0, 1), (10, 'this is row #10'))

        # check __delitem__
        self.assertEqual(len(tree_store), 100)
        aiter = tree_store.get_iter(10)
        del tree_store[aiter]
        self.assertEqual(len(tree_store), 99)
        self.assertRaises(TypeError, tree_store.__delitem__, None)
        self.assertRaises(IndexError, tree_store.__delitem__, -101)
        self.assertRaises(IndexError, tree_store.__delitem__, 101)

    def test_tree_model_get_iter_fail(self):
        # TreeModel class with a failing get_iter()
        class MyTreeModel(GObject.GObject, Gtk.TreeModel):
            def do_get_iter(self, iter):
                return (False, None)

        tm = MyTreeModel()
        self.assertEqual(tm.get_iter_first(), None)

    def test_tree_model_edit(self):
        model = Gtk.ListStore(int, str, float)
        model.append([1, "one", -0.1])
        model.append([2, "two", -0.2])

        def set_row(value):
            model[1] = value

        self.assertRaises(TypeError, set_row, 3)
        self.assertRaises(TypeError, set_row, "three")
        self.assertRaises(ValueError, set_row, [])
        self.assertRaises(ValueError, set_row, [3, "three"])

        model[0] = (3, "three", -0.3)

    def test_tree_row_slice(self):
        model = Gtk.ListStore(int, str, float)
        model.append([1, "one", -0.1])

        self.assertEqual([1, "one", -0.1], model[0][:])
        self.assertEqual([1, "one"], model[0][:2])
        self.assertEqual(["one", -0.1], model[0][1:])
        self.assertEqual(["one"], model[0][1:-1])
        self.assertEqual([1], model[0][:-2])
        self.assertEqual([], model[0][5:])
        self.assertEqual([1, -0.1], model[0][0:3:2])

        model[0][:] = (2, "two", -0.2)
        self.assertEqual([2, "two", -0.2], model[0][:])

        model[0][:2] = (3, "three")
        self.assertEqual([3, "three", -0.2], model[0][:])

        model[0][1:] = ("four", -0.4)
        self.assertEqual([3, "four", -0.4], model[0][:])

        model[0][1:-1] = ("five",)
        self.assertEqual([3, "five", -0.4], model[0][:])

        model[0][0:3:2] = (6, -0.6)
        self.assertEqual([6, "five", -0.6], model[0][:])

        def set_row1():
            model[0][5:] = ("doesn't", "matter",)

        self.assertRaises(ValueError, set_row1)

        def set_row2():
            model[0][:1] = (0, "zero", 0)

        self.assertRaises(ValueError, set_row2)

        def set_row3():
            model[0][:2] = ("0", 0)

        self.assertRaises(TypeError, set_row3)

    def test_tree_row_sequence(self):
        model = Gtk.ListStore(int, str, float)
        model.append([1, "one", -0.1])

        self.assertEqual([1, "one", -0.1], model[0][0, 1, 2])
        self.assertEqual([1, "one"], model[0][0, 1])
        self.assertEqual(["one", -0.1], model[0][1, 2])
        self.assertEqual("one", model[0][1])
        self.assertEqual([1, -0.1], model[0][0, 2])
        self.assertEqual([-0.1, 1], model[0][2, 0])

        model[0][0, 1, 2] = (2, "two", -0.2)
        self.assertEqual([2, "two", -0.2], model[0][0, 1, 2])

        model[0][0, 1] = (3, "three")
        self.assertEqual([3, "three"], model[0][0, 1])

        model[0][1, 2] = ("four", -0.4)
        self.assertEqual(["four", -0.4], model[0][1, 2])

        model[0][0, 2] = (5, -0.5)
        self.assertEqual([5, -0.5], model[0][0, 2])

        model[0][0, 1, 2] = (6, "six", -0.6)
        self.assertEqual([-0.6, 6, "six"], model[0][2, 0, 1])

        def set_row1():
            model[0][4, 5] = ("shouldn't", "work",)

        self.assertRaises(IndexError, set_row1)

        def set_row2():
            model[0][0, 1] = (0, "zero", 0)

        self.assertRaises(ValueError, set_row2)

        def set_row3():
            model[0][0, 1] = ("shouldn't", 0)

        self.assertRaises(TypeError, set_row3)

        def set_row4():
            model[0][0, "two"] = (0, "zero")

        self.assertRaises(TypeError, set_row4)

    def test_tree_model_set_value_to_none(self):
        # Tests allowing the usage of None to set an empty value on a model.
        store = Gtk.ListStore(str)
        row = store.append(['test'])
        self.assertSequenceEqual(store[0][:], ['test'])
        store.set_value(row, 0, None)
        self.assertSequenceEqual(store[0][:], [None])

    def test_signal_emission_tree_path_coerce(self):
        class Model(GObject.Object, Gtk.TreeModel):
            pass

        model = Model()
        tree_paths = []

        def on_any_signal(model, path, *args):
            tree_paths.append(path.to_string())

        model.connect('row-changed', on_any_signal)
        model.connect('row-deleted', on_any_signal)
        model.connect('row-has-child-toggled', on_any_signal)
        model.connect('row-inserted', on_any_signal)

        model.row_changed('0', Gtk.TreeIter())
        self.assertEqual(tree_paths[-1], '0')

        model.row_deleted('1')
        self.assertEqual(tree_paths[-1], '1')

        model.row_has_child_toggled('2', Gtk.TreeIter())
        self.assertEqual(tree_paths[-1], '2')

        model.row_inserted('3', Gtk.TreeIter())
        self.assertEqual(tree_paths[-1], '3')

    def test_tree_model_filter(self):
        model = Gtk.ListStore(int, str, float)
        model.append([1, "one", -0.1])
        model.append([2, "two", -0.2])

        filtered = Gtk.TreeModelFilter(child_model=model)

        self.assertEqual(filtered[0][1], 'one')
        filtered[0][1] = 'ONE'
        self.assertEqual(filtered[0][1], 'ONE')

    def test_list_store_performance(self):
        model = Gtk.ListStore(int, str)

        iterations = 2000
        start = time.clock()
        i = iterations
        while i > 0:
            model.append([1, 'hello'])
            i -= 1
        end = time.clock()
        sys.stderr.write('[%.0f µs/append] ' % ((end - start) * 1000000 / iterations))

    def test_filter_new_default(self):
        # Test filter_new accepts implicit default of None
        model = Gtk.ListStore(int)
        filt = model.filter_new()
        self.assertTrue(filt is not None)


@unittest.skipIf(sys.platform == "darwin", "hangs")
@unittest.skipUnless(Gtk, 'Gtk not available')
class TestTreeView(unittest.TestCase):
    def test_tree_view(self):
        store = Gtk.ListStore(int, str)
        store.append((0, "foo"))
        store.append((1, "bar"))
        view = Gtk.TreeView()

        with realized(view):
            view.set_cursor(store[1].path)
            view.set_cursor(str(store[1].path))

            view.get_cell_area(store[1].path)
            view.get_cell_area(str(store[1].path))

    def test_tree_view_column(self):
        cell = Gtk.CellRendererText()
        col = Gtk.TreeViewColumn(title='This is just a test',
                                 cell_renderer=cell,
                                 text=0,
                                 style=2)

        # Regression test for: https://bugzilla.gnome.org/show_bug.cgi?id=711173
        col.set_cell_data_func(cell, None, None)

    def test_tree_view_add_column_with_attributes(self):
        model = Gtk.ListStore(str, str, str)
        # deliberately use out-of-order sorting here; we assign column 0 to
        # model index 2, etc.
        model.append(['cell13', 'cell11', 'cell12'])
        model.append(['cell23', 'cell21', 'cell22'])

        tree = Gtk.TreeView(model=model)
        cell1 = Gtk.CellRendererText()
        cell2 = Gtk.CellRendererText()
        cell3 = Gtk.CellRendererText()
        cell4 = Gtk.CellRendererText()

        tree.insert_column_with_attributes(0, 'Head2', cell2, text=2)
        tree.insert_column_with_attributes(0, 'Head1', cell1, text=1)
        tree.insert_column_with_attributes(-1, 'Head3', cell3, text=0)
        # unconnected
        tree.insert_column_with_attributes(-1, 'Head4', cell4)

        with realized(tree):
            tree.set_cursor(model[0].path)
            while Gtk.events_pending():
                Gtk.main_iteration()

            self.assertEqual(tree.get_column(0).get_title(), 'Head1')
            self.assertEqual(tree.get_column(1).get_title(), 'Head2')
            self.assertEqual(tree.get_column(2).get_title(), 'Head3')
            self.assertEqual(tree.get_column(3).get_title(), 'Head4')

            # cursor should be at the first row
            self.assertEqual(cell1.props.text, 'cell11')
            self.assertEqual(cell2.props.text, 'cell12')
            self.assertEqual(cell3.props.text, 'cell13')
            self.assertEqual(cell4.props.text, None)

    def test_tree_view_column_set_attributes(self):
        store = Gtk.ListStore(int, str)
        directors = ['Fellini', 'Tarantino', 'Tarkovskiy']
        for i, director in enumerate(directors):
            store.append([i, director])

        treeview = Gtk.TreeView()
        treeview.set_model(store)

        column = Gtk.TreeViewColumn()
        treeview.append_column(column)

        cell = Gtk.CellRendererText()
        column.pack_start(cell, expand=True)
        column.set_attributes(cell, text=1)

        with realized(treeview):
            self.assertTrue(cell.props.text in directors)

    def test_tree_selection(self):
        store = Gtk.ListStore(int, str)
        for i in range(10):
            store.append((i, "foo"))
        view = Gtk.TreeView()
        view.set_model(store)
        firstpath = store.get_path(store.get_iter_first())
        sel = view.get_selection()

        sel.select_path(firstpath)
        (m, s) = sel.get_selected()
        self.assertEqual(m, store)
        self.assertEqual(store.get_path(s), firstpath)

        sel.select_path(0)
        (m, s) = sel.get_selected()
        self.assertEqual(m, store)
        self.assertEqual(store.get_path(s), firstpath)

        sel.select_path("0:0")
        (m, s) = sel.get_selected()
        self.assertEqual(m, store)
        self.assertEqual(store.get_path(s), firstpath)

        sel.select_path((0, 0))
        (m, s) = sel.get_selected()
        self.assertEqual(m, store)
        self.assertEqual(store.get_path(s), firstpath)


@unittest.skipUnless(Gtk, 'Gtk not available')
class TestTextBuffer(unittest.TestCase):
    def test_text_buffer(self):
        self.assertEqual(Gtk.TextBuffer, gi.overrides.Gtk.TextBuffer)
        buffer = Gtk.TextBuffer()
        tag = buffer.create_tag('title', font='Sans 18')

        self.assertEqual(tag.props.name, 'title')
        self.assertEqual(tag.props.font, 'Sans 18')

        (start, end) = buffer.get_bounds()

        mark = buffer.create_mark(None, start)
        self.assertFalse(mark.get_left_gravity())

        buffer.set_text('Hello Jane Hello Bob')
        (start, end) = buffer.get_bounds()
        text = buffer.get_text(start, end, False)
        self.assertEqual(text, 'Hello Jane Hello Bob')

        buffer.set_text('')
        (start, end) = buffer.get_bounds()
        text = buffer.get_text(start, end, False)
        self.assertEqual(text, '')

        buffer.insert(end, 'HelloHello')
        buffer.insert(end, ' Bob')

        cursor_iter = end.copy()
        cursor_iter.backward_chars(9)
        buffer.place_cursor(cursor_iter)
        buffer.insert_at_cursor(' Jane ')

        (start, end) = buffer.get_bounds()
        text = buffer.get_text(start, end, False)
        self.assertEqual(text, 'Hello Jane Hello Bob')

        sel = buffer.get_selection_bounds()
        self.assertEqual(sel, ())
        buffer.select_range(start, end)
        sel = buffer.get_selection_bounds()
        self.assertTrue(sel[0].equal(start))
        self.assertTrue(sel[1].equal(end))

        buffer.set_text('')
        buffer.insert_with_tags(buffer.get_start_iter(), 'HelloHello')
        start, end = buffer.get_bounds()
        text = buffer.get_text(start, end, False)
        self.assertEqual(text, 'HelloHello')

        buffer.set_text('')
        buffer.insert_with_tags_by_name(buffer.get_start_iter(), 'HelloHello')
        start, end = buffer.get_bounds()
        text = buffer.get_text(start, end, False)
        self.assertEqual(text, 'HelloHello')

        try:
            starts_tag = Gtk.TextIter.starts_tag
        except AttributeError:
            starts_tag = Gtk.TextIter.begins_tag

        buffer.set_text('')
        buffer.insert_with_tags(buffer.get_start_iter(), 'HelloHello', tag)
        (start, end) = buffer.get_bounds()
        self.assertTrue(starts_tag(start, tag))
        self.assertTrue(start.has_tag(tag))

        buffer.set_text('')
        buffer.insert_with_tags_by_name(buffer.get_start_iter(), 'HelloHello', 'title')
        (start, end) = buffer.get_bounds()
        self.assertTrue(starts_tag(start, tag))
        self.assertTrue(start.has_tag(tag))

        self.assertRaises(ValueError, buffer.insert_with_tags_by_name,
                          buffer.get_start_iter(), 'HelloHello', 'unknowntag')

    def test_text_iter(self):
        try:
            starts_tag = Gtk.TextIter.starts_tag
        except AttributeError:
            starts_tag = Gtk.TextIter.begins_tag

        self.assertEqual(Gtk.TextIter, gi.overrides.Gtk.TextIter)
        buffer = Gtk.TextBuffer()
        buffer.set_text('Hello Jane Hello Bob')
        tag = buffer.create_tag('title', font='Sans 18')
        (start, end) = buffer.get_bounds()
        start.forward_chars(10)
        buffer.apply_tag(tag, start, end)
        self.assertTrue(starts_tag(start))
        self.assertTrue(end.ends_tag())
        self.assertTrue(start.toggles_tag())
        self.assertTrue(end.toggles_tag())
        start.backward_chars(1)
        self.assertFalse(starts_tag(start))
        self.assertFalse(start.ends_tag())
        self.assertFalse(start.toggles_tag())

    def test_text_buffer_search(self):
        buffer = Gtk.TextBuffer()
        buffer.set_text('Hello World Hello GNOME')

        i = buffer.get_iter_at_offset(0)
        self.assertTrue(isinstance(i, Gtk.TextIter))

        self.assertEqual(i.forward_search('world', 0, None), None)

        (start, end) = i.forward_search('World', 0, None)
        self.assertEqual(start.get_offset(), 6)
        self.assertEqual(end.get_offset(), 11)

        (start, end) = i.forward_search('world',
                                        Gtk.TextSearchFlags.CASE_INSENSITIVE,
                                        None)
        self.assertEqual(start.get_offset(), 6)
        self.assertEqual(end.get_offset(), 11)

    def test_insert_text_signal_location_modification(self):
        # Regression test for: https://bugzilla.gnome.org/show_bug.cgi?id=736175

        def callback(buffer, location, text, length):
            location.assign(buffer.get_end_iter())

        buffer = Gtk.TextBuffer()
        buffer.set_text('first line\n')
        buffer.connect('insert-text', callback)

        # attempt insertion at the beginning of the buffer, the callback will
        # modify the insert location to the end.
        buffer.place_cursor(buffer.get_start_iter())
        buffer.insert_at_cursor('second line\n')

        self.assertEqual(buffer.get_property('text'),
                         'first line\nsecond line\n')

    @unittest.skipIf(gtkver() < (3, 20, 0), "broken with older gtk")
    def test_backward_find_char(self):
        buffer = Gtk.TextBuffer()
        buffer.set_text('abc')
        end = buffer.get_iter_at_line(99)

        values = []

        def pred_func(ch, user_data):
            values.append(ch)
            return ch == u"a"

        self.assertTrue(end.backward_find_char(pred_func))
        self.assertEqual(values, [u"c", u"b", u"a"])


@unittest.skipUnless(Gtk, 'Gtk not available')
class TestContainer(unittest.TestCase):

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_child_set_property(self):
        box = Gtk.Box()
        child = Gtk.Button()
        box.pack_start(child, expand=False, fill=True, padding=0)

        box.child_set_property(child, 'padding', 42)

        value = GObject.Value(int)
        box.child_get_property(child, 'padding', value)
        self.assertEqual(value.get_int(), 42)

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_child_get_property_gvalue(self):
        box = Gtk.Box()
        child = Gtk.Button()
        box.pack_start(child, expand=False, fill=True, padding=42)

        value = GObject.Value(int)
        box.child_get_property(child, 'padding', value)
        self.assertEqual(value.get_int(), 42)

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_child_get_property_return_with_explicit_gvalue(self):
        box = Gtk.Box()
        child = Gtk.Button()
        box.pack_start(child, expand=False, fill=True, padding=42)

        value = GObject.Value(int)
        result = box.child_get_property(child, 'padding', value)
        self.assertEqual(result, 42)

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_child_get_property_return_with_implicit_gvalue(self):
        box = Gtk.Box()
        child = Gtk.Button()
        box.pack_start(child, expand=False, fill=True, padding=42)

        result = box.child_get_property(child, 'padding')
        self.assertEqual(result, 42)

    def test_child_get_property_error(self):
        box = Gtk.Box()
        child = Gtk.Button()
        if Gtk_version == "4.0":
            box.pack_start(child)
        else:
            box.pack_start(child, expand=False, fill=True, padding=42)
        with self.assertRaises(ValueError):
            box.child_get_property(child, 'not-a-valid-child-property')

    @unittest.skipIf(Gtk_version == "4.0", "not in gtk4")
    def test_child_get_and_set(self):
        box = Gtk.Box()
        child = Gtk.Button()
        box.pack_start(child, expand=True, fill=True, padding=42)

        expand, fill, padding = box.child_get(child, 'expand', 'fill', 'padding')
        self.assertEqual(expand, True)
        self.assertEqual(fill, True)
        self.assertEqual(padding, 42)

        box.child_set(child, expand=False, fill=False, padding=21, pack_type=1)
        expand, fill, padding, pack_type = box.child_get(child, 'expand', 'fill', 'padding', 'pack-type')
        self.assertEqual(expand, False)
        self.assertEqual(fill, False)
        self.assertEqual(padding, 21)

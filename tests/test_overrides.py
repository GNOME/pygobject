# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest

import sys
sys.path.insert(0, "../")

from gi.repository import GLib
from gi.repository import GObject
from gi.repository import Gdk
from gi.repository import Gtk
from gi.repository import Pango
import gi.overrides as overrides
import gi.types

class TestGLib(unittest.TestCase):

    def test_gvariant(self):
        variant = GLib.Variant('i', 42)
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertEquals(variant.get_int32(), 42)

        variant = GLib.Variant('(ss)', 'mec', 'mac')
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(0), GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(1), GLib.Variant))
        self.assertEquals(variant.get_child_value(0).get_string(), 'mec')
        self.assertEquals(variant.get_child_value(1).get_string(), 'mac')

        variant = GLib.Variant('a{si}', {'key1': 1, 'key2': 2})
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(0), GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(1), GLib.Variant))
        # Looks like order is not preserved
        self.assertEquals(variant.get_child_value(1).get_child_value(0).get_string(), 'key1')
        self.assertEquals(variant.get_child_value(1).get_child_value(1).get_int32(), 1)
        self.assertEquals(variant.get_child_value(0).get_child_value(0).get_string(), 'key2')
        self.assertEquals(variant.get_child_value(0).get_child_value(1).get_int32(), 2)

class TestPango(unittest.TestCase):

    def test_font_description(self):
        desc = Pango.FontDescription('monospace')
        self.assertEquals(desc.get_family(), 'monospace')

class TestGdk(unittest.TestCase):

    def test_color(self):
        color = Gdk.Color(100, 200, 300)
        self.assertEquals(color.red, 100)
        self.assertEquals(color.green, 200)
        self.assertEquals(color.blue, 300)

    def test_event(self):
        event = Gdk.Event.new(Gdk.EventType.CONFIGURE)
        self.assertEquals(event.type, Gdk.EventType.CONFIGURE)
        self.assertEquals(event.send_event, 0)

        event = Gdk.Event.new(Gdk.EventType.DRAG_MOTION)
        event.x_root, event.y_root = 0, 5
        self.assertEquals(event.x_root, 0)
        self.assertEquals(event.y_root, 5)

class TestGtk(unittest.TestCase):

    def test_container(self):
        box = Gtk.Box()
        self.assertTrue(box)
        label = Gtk.Label()
        label2 = Gtk.Label()
        box.add(label)
        box.add(label2)
        self.assertTrue(label in box)
        self.assertTrue(label2 in box)
        self.assertEqual(len(box), 2)
        self.assertTrue(box)
        l = [x for x in box]
        self.assertEqual(l, [label, label2])

    def test_actiongroup(self):
        self.assertEquals(Gtk.ActionGroup, overrides.Gtk.ActionGroup)
        self.assertRaises(TypeError, Gtk.ActionGroup)

        action_group = Gtk.ActionGroup (name = 'TestActionGroup')
        callback_data = "callback data"

        def test_action_callback_data(action, user_data):
            self.assertEquals(user_data, callback_data);

        def test_radio_action_callback_data(action, current, user_data):
            self.assertEquals(user_data, callback_data);

        action_group.add_actions ([
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

    def test_uimanager(self):
        self.assertEquals(Gtk.UIManager, overrides.Gtk.UIManager)
        ui = Gtk.UIManager()
        ui.add_ui_from_string(
"""
<ui>
    <menubar name="menubar1"></menubar>
</ui>
"""
)
        menubar = ui.get_widget("/menubar1")
        self.assertEquals(type(menubar), Gtk.MenuBar)

        ag = Gtk.ActionGroup (name="ag1")
        ui.insert_action_group(ag)
        ag2 = Gtk.ActionGroup (name="ag2")
        ui.insert_action_group(ag2)
        groups = ui.get_action_groups()
        self.assertEquals(ag, groups[-2])
        self.assertEquals(ag2, groups[-1])

    def test_builder(self):
        self.assertEquals(Gtk.Builder, overrides.Gtk.Builder)

        class SignalTest(GObject.GObject):
            __gtype_name__ = "GIOverrideSignalTest"
            __gsignals__ = {
                "test-signal": (GObject.SIGNAL_RUN_FIRST,
                                GObject.TYPE_NONE,
                                []),
            }


        class SignalCheck:
            def __init__(self):
                self.sentinel = 0

            def on_signal_1(self, *args):
                self.sentinel += 1

            def on_signal_3(self, *args):
                self.sentinel += 3

        signal_checker = SignalCheck()
        builder = Gtk.Builder()

        # add object1 to the builder
        builder.add_from_string(
"""
<interface>
  <object class="GIOverrideSignalTest" id="object1">
      <signal name="test-signal" handler="on_signal_1" />
  </object>
</interface>
""")

        # only add object3 to the builder
        builder.add_objects_from_string(
"""
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

""",
            ['object3'])

        # hook up signals
        builder.connect_signals(signal_checker)

        # call their notify signals and check sentinel
        objects = builder.get_objects()
        self.assertEquals(len(objects), 2)
        for obj in objects:
            obj.emit('test-signal')

        self.assertEquals(signal_checker.sentinel, 4)

    def test_dialogs(self):
        self.assertEquals(Gtk.Dialog, overrides.Gtk.Dialog)
        self.assertEquals(Gtk.AboutDialog, overrides.Gtk.AboutDialog)
        self.assertEquals(Gtk.MessageDialog, overrides.Gtk.MessageDialog)
        self.assertEquals(Gtk.ColorSelectionDialog, overrides.Gtk.ColorSelectionDialog)
        self.assertEquals(Gtk.FileChooserDialog, overrides.Gtk.FileChooserDialog)
        self.assertEquals(Gtk.FontSelectionDialog, overrides.Gtk.FontSelectionDialog)
        self.assertEquals(Gtk.RecentChooserDialog, overrides.Gtk.RecentChooserDialog)

        # Gtk.Dialog
        dialog = Gtk.Dialog (title='Foo',
                             flags=Gtk.DialogFlags.MODAL,
                             buttons=('test-button1', 1))

        dialog.add_buttons ('test-button2', 2, Gtk.STOCK_CLOSE, Gtk.ResponseType.CLOSE)

        self.assertEquals('Foo', dialog.get_title())
        self.assertTrue(dialog.get_modal())
        button = dialog.get_widget_for_response (1)
        self.assertEquals('test-button1', button.get_label())
        button = dialog.get_widget_for_response (2)
        self.assertEquals('test-button2', button.get_label())
        button = dialog.get_widget_for_response (Gtk.ResponseType.CLOSE)
        self.assertEquals(Gtk.STOCK_CLOSE, button.get_label())

        # Gtk.AboutDialog
        dialog = Gtk.AboutDialog()

        # Gtk.MessageDialog
        dialog = Gtk.MessageDialog (title='message dialog test',
                                    flags=Gtk.DialogFlags.MODAL,
                                    buttons=Gtk.ButtonsType.OK,
                                    message_format='dude!')

        self.assertEquals('message dialog test', dialog.get_title())
        self.assertTrue(dialog.get_modal())
        text = dialog.get_property('text')
        self.assertEquals('dude!', text)

        dialog.format_secondary_text('2nd text')
        self.assertEqual(dialog.get_property('secondary-text'), '2nd text')
        self.assertFalse(dialog.get_property('secondary-use-markup'))

        dialog.format_secondary_markup('2nd markup')
        self.assertEqual(dialog.get_property('secondary-text'), '2nd markup')
        self.assertTrue(dialog.get_property('secondary-use-markup'))

        # Gtk.ColorSelectionDialog
        dialog = Gtk.ColorSelectionDialog("color selection dialog test")
        self.assertEquals('color selection dialog test', dialog.get_title())

        # Gtk.FileChooserDialog
        dialog = Gtk.FileChooserDialog (title='file chooser dialog test',
                                        buttons=('test-button1', 1),
                                        action=Gtk.FileChooserAction.SAVE)

        dialog.add_buttons ('test-button2', 2, Gtk.STOCK_CLOSE, Gtk.ResponseType.CLOSE)
        self.assertEquals('file chooser dialog test', dialog.get_title())
        button = dialog.get_widget_for_response (1)
        self.assertEquals('test-button1', button.get_label())
        button = dialog.get_widget_for_response (2)
        self.assertEquals('test-button2', button.get_label())
        button = dialog.get_widget_for_response (Gtk.ResponseType.CLOSE)
        self.assertEquals(Gtk.STOCK_CLOSE, button.get_label())
        action = dialog.get_property('action')
        self.assertEquals(Gtk.FileChooserAction.SAVE, action)


        # Gtk.FontSelectionDialog
        dialog = Gtk.ColorSelectionDialog("font selection dialog test")
        self.assertEquals('font selection dialog test', dialog.get_title())

        # Gtk.RecentChooserDialog
        test_manager = Gtk.RecentManager()
        dialog = Gtk.RecentChooserDialog (title='recent chooser dialog test',
                                          buttons=('test-button1', 1),
                                          manager=test_manager)

        dialog.add_buttons ('test-button2', 2, Gtk.STOCK_CLOSE, Gtk.ResponseType.CLOSE)
        self.assertEquals('recent chooser dialog test', dialog.get_title())
        button = dialog.get_widget_for_response (1)
        self.assertEquals('test-button1', button.get_label())
        button = dialog.get_widget_for_response (2)
        self.assertEquals('test-button2', button.get_label())
        button = dialog.get_widget_for_response (Gtk.ResponseType.CLOSE)
        self.assertEquals(Gtk.STOCK_CLOSE, button.get_label())

    class TestClass(GObject.GObject):
        __gtype_name__ = "GIOverrideTreeAPITest"

        def __init__(self, tester, int_value, string_value):
            super(TestGtk.TestClass, self).__init__()
            self.tester = tester
            self.int_value = int_value
            self.string_value = string_value

        def check(self, int_value, string_value):
            self.tester.assertEquals(int_value, self.int_value)
            self.tester.assertEquals(string_value, self.string_value)

    def test_tree_store(self):
        self.assertEquals(Gtk.TreeStore, overrides.Gtk.TreeStore)
        self.assertEquals(Gtk.ListStore, overrides.Gtk.ListStore)
        self.assertEquals(Gtk.TreeModel, overrides.Gtk.TreeModel)
        self.assertEquals(Gtk.TreeViewColumn, overrides.Gtk.TreeViewColumn)

        class TestPyObject(object):
            pass

        test_pyobj = TestPyObject()
        test_pydict = {1:1, "2":2, "3":"3"}
        test_pylist = [1,"2", "3"]
        tree_store = Gtk.TreeStore(int, 'gchararray', TestGtk.TestClass, object, object, object)

        parent = None
        for i in range(100):
            label = 'this is child #%d' % i
            testobj = TestGtk.TestClass(self, i, label)
            parent = tree_store.append(parent, (i,
                                                label,
                                                testobj,
                                                test_pyobj,
                                                test_pydict,
                                                test_pylist))

        # len gets the number of children in the root node
        # since we kept appending to the previous node
        # there should only be one child of the root
        self.assertEquals(len(tree_store), 1)

        # walk the tree to see if the values were stored correctly
        parent = None
        i = 0

        treeiter = tree_store.iter_children(parent)
        while treeiter:
           i = tree_store.get_value(treeiter, 0)
           s = tree_store.get_value(treeiter, 1)
           obj = tree_store.get_value(treeiter, 2)
           i = tree_store.get_value(treeiter, 0)
           s = tree_store.get_value(treeiter, 1)
           obj = tree_store.get_value(treeiter, 2)
           obj.check(i, s)

           pyobj = tree_store.get_value(treeiter, 3)
           self.assertEquals(pyobj, test_pyobj)
           pydict = tree_store.get_value(treeiter, 4)
           self.assertEquals(pydict, test_pydict)
           pylist = tree_store.get_value(treeiter, 5)
           self.assertEquals(pylist, test_pylist)

           parent = treeiter
           treeiter = tree_store.iter_children(parent)

        self.assertEquals(i, 99)

    def test_list_store(self):
        class TestPyObject(object):
            pass

        test_pyobj = TestPyObject()
        test_pydict = {1:1, "2":2, "3":"3"}
        test_pylist = [1,"2", "3"]

        list_store = Gtk.ListStore(int, str, 'GIOverrideTreeAPITest', object, object, object)
        for i in range(95):
            label = 'this is row #%d' % i
            testobj = TestGtk.TestClass(self, i, label)
            parent = list_store.append((i,
                                        label,
                                        testobj,
                                        test_pyobj,
                                        test_pydict,
                                        test_pylist))

        # add sorted items out of order to test insert* apis
        i = 97
        label = 'this is row #97'
        treeiter = list_store.append((i,
                                      label,
                                      TestGtk.TestClass(self, i, label),
                                      test_pyobj,
                                      test_pydict,
                                      test_pylist))
        # this should append
        i = 99
        label = 'this is row #99'
        list_store.insert(9999, (i,
                                 label,
                                 TestGtk.TestClass(self, i, label),
                                 test_pyobj,
                                 test_pydict,
                                 test_pylist))

        i = 96
        label = 'this is row #96'
        list_store.insert_before(treeiter, (i,
                                            label,
                                            TestGtk.TestClass(self, i, label),
                                            test_pyobj,
                                            test_pydict,
                                            test_pylist))

        i = 98
        label = 'this is row #98'
        list_store.insert_after(treeiter, (i,
                                           label,
                                           TestGtk.TestClass(self, i, label),
                                           test_pyobj,
                                           test_pydict,
                                           test_pylist))


        i = 95
        label = 'this is row #95'
        list_store.insert(95, (i,
                               label,
                               TestGtk.TestClass(self, i, label),
                               test_pyobj,
                               test_pydict,
                               test_pylist))

        self.assertEquals(len(list_store), 100)

        # walk the list to see if the values were stored correctly
        i = 0
        treeiter = list_store.get_iter_first()

        counter = 0
        while treeiter:
            i = list_store.get_value(treeiter, 0)
            self.assertEquals(i, counter)
            s = list_store.get_value(treeiter, 1)
            obj = list_store.get_value(treeiter, 2)
            obj.check(i, s)

            pyobj = list_store.get_value(treeiter, 3)
            self.assertEquals(pyobj, test_pyobj)
            pydict = list_store.get_value(treeiter, 4)
            self.assertEquals(pydict, test_pydict)
            pylist = list_store.get_value(treeiter, 5)
            self.assertEquals(pylist, test_pylist)
            treeiter = list_store.iter_next(treeiter)

            counter += 1

        self.assertEquals(i, 99)

    def test_tree_path(self):
        p1 = Gtk.TreePath()
        p2 = Gtk.TreePath.new_first()
        self.assertEqual(p1, p2)
        self.assertEqual(str(p1), '0')
        p1 = Gtk.TreePath(2)
        p2 = Gtk.TreePath.new_from_string('2')
        self.assertEqual(p1, p2)
        self.assertEqual(str(p1), '2')
        p1 = Gtk.TreePath('1:2:3')
        p2 = Gtk.TreePath.new_from_string('1:2:3')
        self.assertEqual(p1, p2)
        self.assertEqual(str(p1), '1:2:3')
        p1 = Gtk.TreePath((1,2,3))
        p2 = Gtk.TreePath.new_from_string('1:2:3')
        self.assertEqual(p1, p2)
        self.assertEqual(str(p1), '1:2:3')

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
            self.assertNotEquals(parent, None)
            for j in range(20):
                label = 'this is child #%d of node #%d' % (j, i)
                child = tree_store.append(parent, (j, label,))
                self.assertNotEqual(child, None)

        self.assertTrue(tree_store)
        self.assertEqual(len(tree_store), 100)

        for i,row in enumerate(tree_store):
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
            else:
                self.assertEqual(next, None)

            self.assertEqual(tree_store.iter_n_children(row.iter), 20)

            child = tree_store.iter_children(row.iter)
            for j,childrow in enumerate(row.iterchildren()):
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
        for i in range(-1,-100,-1):
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

        for i,childrow in enumerate(last_row.iterchildren()):
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

    def test_tree_view_column(self):
        cell = Gtk.CellRendererText()
        column = Gtk.TreeViewColumn(title='This is just a test',
                                    cell_renderer=cell,
                                    text=0,
                                    style=2)

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

        sel.select_path((0,0))
        (m, s) = sel.get_selected()
        self.assertEqual(m, store)
        self.assertEqual(store.get_path(s), firstpath)

    def test_text_buffer(self):
        self.assertEquals(Gtk.TextBuffer, overrides.Gtk.TextBuffer)
        buffer = Gtk.TextBuffer()
        tag = buffer.create_tag ('title', font = 'Sans 18')

        self.assertEquals(tag.props.name, 'title')
        self.assertEquals(tag.props.font, 'Sans 18')

        (start, end) = buffer.get_bounds()

        mark = buffer.create_mark(None, start)
        self.assertFalse(mark.get_left_gravity())

        buffer.set_text('Hello Jane Hello Bob')
        (start, end) = buffer.get_bounds()
        text = buffer.get_text(start, end, False)
        self.assertEquals(text, 'Hello Jane Hello Bob')

        buffer.set_text('')
        (start, end) = buffer.get_bounds()
        text = buffer.get_text(start, end, False)
        self.assertEquals(text, '')

        buffer.insert(end, 'HelloHello')
        buffer.insert(end, ' Bob')

        cursor_iter = end.copy()
        cursor_iter.backward_chars(9)
        buffer.place_cursor(cursor_iter)
        buffer.insert_at_cursor(' Jane ')

        (start, end) = buffer.get_bounds()
        text = buffer.get_text(start, end, False)
        self.assertEquals(text, 'Hello Jane Hello Bob')

        sel = buffer.get_selection_bounds()
        self.assertEquals(sel, ())
        buffer.select_range(start, end)
        sel = buffer.get_selection_bounds()
        self.assertTrue(sel[0].equal(start))
        self.assertTrue(sel[1].equal(end))

    def test_text_iter(self):
        self.assertEquals(Gtk.TextIter, overrides.Gtk.TextIter)
        buffer = Gtk.TextBuffer()
        buffer.set_text('Hello Jane Hello Bob')
        tag = buffer.create_tag ('title', font = 'Sans 18')
        (start, end) = buffer.get_bounds()
        start.forward_chars(10)
        buffer.apply_tag(tag, start, end)
        self.assertTrue(start.begins_tag())
        self.assertTrue(end.ends_tag())
        self.assertTrue(start.toggles_tag())
        self.assertTrue(end.toggles_tag())
        start.backward_chars(1)
        self.assertFalse(start.begins_tag())
        self.assertFalse(start.ends_tag())
        self.assertFalse(start.toggles_tag())

    def test_buttons(self):
        self.assertEquals(Gtk.Button, overrides.Gtk.Button)

        # test Gtk.Button
        button = Gtk.Button()
        button = Gtk.Button(stock=Gtk.STOCK_CLOSE)
        self.assertEquals(Gtk.STOCK_CLOSE, button.get_label())
        self.assertTrue(button.get_use_stock())
        self.assertTrue(button.get_use_underline())

        # test Gtk.LinkButton
        self.assertRaises(TypeError, Gtk.LinkButton)
        button = Gtk.LinkButton('http://www.gtk.org', 'Gtk')
        self.assertEquals('http://www.gtk.org', button.get_uri())
        self.assertEquals('Gtk', button.get_label())

    def test_inheritance(self):
        for name in overrides.Gtk.__all__:
            over = getattr(overrides.Gtk, name)
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
        self.assertEquals(Gtk.Editable, overrides.Gtk.Editable)

        # need to use Gtk.Entry because Editable is an interface
        entry=Gtk.Entry()
        pos = entry.insert_text('HeWorld', 0)
        self.assertEquals(pos, 7)
        pos = entry.insert_text('llo ', 2)
        self.assertEquals(pos, 6)
        text = entry.get_chars(0, 11)
        self.assertEquals('Hello World', text)

    def test_label(self):
        label = Gtk.Label('Hello')
        self.assertEquals(label.get_text(), 'Hello')

    def test_table(self):
        table = Gtk.Table()
        self.assertEquals(table.get_size(), (1,1))
        self.assertEquals(table.get_homogeneous(), False)
        table = Gtk.Table(2, 3)
        self.assertEquals(table.get_size(), (2,3))
        self.assertEquals(table.get_homogeneous(), False)
        table = Gtk.Table(2, 3, True)
        self.assertEquals(table.get_size(), (2,3))
        self.assertEquals(table.get_homogeneous(), True)


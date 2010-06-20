# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import pygtk
pygtk.require("2.0")

import unittest

import sys
sys.path.insert(0, "../")

from gi.repository import GObject
from gi.repository import Gdk
from gi.repository import Gtk
import gi.overrides as overrides

class TestGdk(unittest.TestCase):

    def test_color(self):
        color = Gdk.Color(100, 200, 300)
        self.assertEquals(color.red, 100)
        self.assertEquals(color.green, 200)
        self.assertEquals(color.blue, 300)

class TestGtk(unittest.TestCase):
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

    def test_actiongroup(self):
        self.assertEquals(Gtk.ActionGroup, overrides.Gtk.ActionGroup)
        action_group = Gtk.ActionGroup (name = 'TestActionGroup')

        action_group.add_actions ([
            ('test-action1', None, 'Test Action 1',
             None, None, None),
            ('test-action2', Gtk.STOCK_COPY, 'Test Action 2',
              None, None, None)])
        action_group.add_toggle_actions([
            ('test-toggle-action1', None, 'Test Toggle Action 1',
             None, None, None, False),
            ('test-toggle-action2', Gtk.STOCK_COPY, 'Test Toggle Action 2',
              None, None, None, True)])
        action_group.add_radio_actions([
            ('test-radio-action1', None, 'Test Radio Action 1'),
            ('test-radio-action2', Gtk.STOCK_COPY, 'Test Radio Action 2')], 1, None)

        expected_results = (('test-action1', Gtk.Action),
                            ('test-action2', Gtk.Action),
                            ('test-toggle-action1', Gtk.ToggleAction),
                            ('test-toggle-action2', Gtk.ToggleAction),
                            ('test-radio-action1', Gtk.RadioAction),
                            ('test-radio-action2', Gtk.RadioAction))

        for action, cmp in zip(action_group.list_actions(), expected_results):
            a = (action.get_name(), type(action))
            self.assertEquals(a,cmp)

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



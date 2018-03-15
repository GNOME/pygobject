# -*- Mode: Python -*-

from __future__ import absolute_import

import unittest

from gi.repository import GLib

import testhelper


class TestThread(unittest.TestCase):
    def setUp(self):
        self.main = GLib.MainLoop()
        self.called = False

    def from_thread_cb(self, test, enum):
        assert test == self.obj
        assert int(enum) == 0
        assert type(enum) != int
        self.called = True
        GLib.idle_add(self.timeout_cb)

    def idle_cb(self):
        self.obj = testhelper.get_test_thread()
        self.obj.connect('from-thread', self.from_thread_cb)
        self.obj.emit('emit-signal')

    def test_extension_module(self):
        GLib.idle_add(self.idle_cb)
        GLib.timeout_add(2000, self.timeout_cb)
        self.main.run()
        self.assertTrue(self.called)

    def timeout_cb(self):
        self.main.quit()

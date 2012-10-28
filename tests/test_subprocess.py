# -*- Mode: Python -*-

import sys
import unittest

from gi.repository import GLib


class TestProcess(unittest.TestCase):

    def test_child_watch_no_data(self):
        def cb(pid, condition):
            self.loop.quit()

        self.loop = GLib.MainLoop()
        argv = [sys.executable, '-c', 'import sys']
        pid, stdin, stdout, stderr = GLib.spawn_async(
            argv, flags=GLib.SpawnFlags.DO_NOT_REAP_CHILD)
        pid.close()
        GLib.child_watch_add(pid, cb)
        self.loop.run()

    def test_child_watch_data_priority(self):
        def cb(pid, condition, data):
            self.data = data
            self.loop.quit()

        self.data = None
        self.loop = GLib.MainLoop()
        argv = [sys.executable, '-c', 'import sys']
        pid, stdin, stdout, stderr = GLib.spawn_async(
            argv, flags=GLib.SpawnFlags.DO_NOT_REAP_CHILD)
        pid.close()
        id = GLib.child_watch_add(pid, cb, 12345, GLib.PRIORITY_HIGH)
        self.assertEqual(self.loop.get_context().find_source_by_id(id).priority,
                         GLib.PRIORITY_HIGH)
        self.loop.run()
        self.assertEqual(self.data, 12345)

    def test_backwards_compat_flags(self):
        self.assertEqual(GLib.SpawnFlags.DO_NOT_REAP_CHILD,
                         GLib.SPAWN_DO_NOT_REAP_CHILD)

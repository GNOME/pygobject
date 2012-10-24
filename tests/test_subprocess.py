# -*- Mode: Python -*-

import sys
import unittest

from gi.repository import GLib


class TestProcess(unittest.TestCase):

    def _child_watch_cb(self, pid, condition, data):
        self.data = data
        self.loop.quit()

    def test_child_watch(self):
        self.data = None
        self.loop = GLib.MainLoop()
        argv = [sys.executable, '-c', 'import sys']
        pid, stdin, stdout, stderr = GLib.spawn_async(
            argv, flags=GLib.SpawnFlags.DO_NOT_REAP_CHILD)
        pid.close()
        GLib.child_watch_add(pid, self._child_watch_cb, 12345)
        self.loop.run()
        self.assertEqual(self.data, 12345)

    def test_backwards_compat_flags(self):
        self.assertEqual(GLib.SpawnFlags.DO_NOT_REAP_CHILD,
                         GLib.SPAWN_DO_NOT_REAP_CHILD)

# -*- Mode: Python -*-

import gc
import unittest
import sys

from common import gobject

class TestProcess(unittest.TestCase):

    def _child_watch_cb(self, pid, condition, data):
        self.data = data
        self.loop.quit()

    def testChildWatch(self):
        self.data = None
        self.loop = gobject.MainLoop()
        argv = [sys.executable, '-c', 'import sys']
        pid, stdin, stdout, stderr = gobject.spawn_async(
            argv, flags=gobject.SPAWN_DO_NOT_REAP_CHILD)
        gobject.child_watch_add(pid, self._child_watch_cb, 12345)
        self.loop.run()
        self.assertEqual(self.data, 12345)

if __name__ == '__main__':
    unittest.main()

import os
import unittest

from common import gobject, gtk, testhelper

class TestThread(unittest.TestCase):
    def from_thread_cb(self, test, enum):
        assert test == self.obj
        assert int(enum) == 0
        assert type(enum) != int
        
    def idle_cb(self):
        self.obj = testhelper.get_test_thread()
        self.obj.connect('from-thread', self.from_thread_cb)
        self.obj.emit('emit-signal')

    def testExtensionModule(self):
        gobject.idle_add(self.idle_cb)
        gobject.timeout_add(50, self.timeout_cb)
        gtk.main()

    def timeout_cb(self):
        gtk.main_quit()

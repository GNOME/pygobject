import os
import unittest

from common import gobject, gtk, testhelper

# Enable PyGILState API
os.environ['PYGTK_USE_GIL_STATE_API'] = ''

gobject.threads_init()

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
        gtk.idle_add(self.idle_cb)
        gtk.timeout_add(50, self.timeout_cb)
        gtk.main()

    def timeout_cb(self):
        gtk.main_quit()

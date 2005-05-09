# -*- Mode: Python -*-

import unittest

from common import gtk

class TestGdkEvent(unittest.TestCase):
    def testWindowSetter(self):
        event = gtk.gdk.Event(gtk.gdk.BUTTON_PRESS)

        win1 = gtk.Window()
        win1.realize()
        event.window = win1.window
        self.assertEqual(event.window, win1.window)
        
        win2 = gtk.Window()
        win2.realize()
        event.window = win2.window
        self.assertEqual(event.window, win2.window)

if __name__ == '__main__':
    unittest.main()

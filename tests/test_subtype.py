import unittest

from common import gobject, gtk

class TestSubType(unittest.TestCase):
    def testSubType(self):
        t = type('testtype', (gobject.GObject, gobject.GInterface), {})
        assert issubclass(t, gobject.GObject)
        assert issubclass(t, gobject.GInterface)
        t = type('testtype2', (gobject.GObject, gtk.TreeModel), {})
        assert issubclass(t, gobject.GObject)
        assert issubclass(t, gtk.TreeModel)
        

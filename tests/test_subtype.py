import unittest

from common import gobject, gtk, testhelper

class TestSubType(unittest.TestCase):
    def testSubType(self):
        t = type('testtype', (gobject.GObject, gobject.GInterface), {})
        self.assert_(issubclass(t, gobject.GObject))
        self.assert_(issubclass(t, gobject.GInterface))
        t = type('testtype2', (gobject.GObject, gtk.TreeModel), {})
        self.assert_(issubclass(t, gobject.GObject))
        self.assert_(issubclass(t, gtk.TreeModel))

    def testTpBasicSize(self):
        iface = testhelper.get_tp_basicsize(gobject.GInterface)
        gobj = testhelper.get_tp_basicsize(gobject.GObject)
        
        widget = testhelper.get_tp_basicsize(gtk.Widget)
        self.assert_(gobj == widget)

        treemodel = testhelper.get_tp_basicsize(gtk.TreeModel)
        self.assert_(iface == treemodel)

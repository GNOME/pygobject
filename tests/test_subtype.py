import unittest

from common import gobject, gtk, testhelper
import testmodule

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

    def testBuiltinContructorRefcount(self):
        foo = gtk.Label()
        self.assertEqual(foo.__grefcount__, 1)

    def testPyContructorRefcount(self):
        foo = testmodule.PyLabel()
        self.assertEqual(foo.__grefcount__, 1)

    def testBuiltinObjNewRefcount(self):
        foo = gobject.new(gtk.Label)
        self.assertEqual(foo.__grefcount__, 1)

    def testPyObjNewRefcount(self):
        foo = gobject.new(testmodule.PyLabel)
        self.assertEqual(foo.__grefcount__, 1)

    def testPyContructorPropertyChaining(self):
        foo = testmodule.PyLabel()
        self.assertEqual(foo.__grefcount__, 1)

    def testPyObjNewPropertyChaining(self):
        foo = gobject.new(testmodule.PyLabel)
        self.assertEqual(foo.props.label, "hello")

    def testCPyCSubclassing1(self):
        obj = testhelper.create_test_type()
        self.assertEqual(obj.__grefcount__, 1)

    def testCPyCSubclassing1(self):
        refcount = testhelper.test_g_object_new()
        self.assertEqual(refcount, 2)

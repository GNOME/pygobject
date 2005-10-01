import unittest

import testmodule
from common import gobject, gtk, testhelper
from gobject import GObject, GInterface

class TestSubType(unittest.TestCase):
    def testSubType(self):
        t = type('testtype', (GObject, GInterface), {})
        self.failUnless(issubclass(t, GObject))
        self.failUnless(issubclass(t, GInterface))
        t = type('testtype2', (GObject, gtk.TreeModel), {})
        self.failUnless(issubclass(t, GObject))
        self.failUnless(issubclass(t, gtk.TreeModel))
        
    def testTpBasicSize(self):
        self.assertEqual(GObject.__basicsize__,
                         gtk.Widget.__basicsize__)

        self.assertEqual(GInterface.__basicsize__,
                         gtk.TreeModel.__basicsize__)

    def testLabel(self):
        label = gtk.Label()
        self.assertEqual(label.__grefcount__, 1)
        label = gobject.new(gtk.Label)
        self.assertEqual(label.__grefcount__, 1)

    def testPythonSubclass(self):
        label = testmodule.PyLabel()
        self.assertEqual(label.__grefcount__, 1)
        self.assertEqual(label.props.label, "hello")
        label = gobject.new(testmodule.PyLabel)
        self.assertEqual(label.__grefcount__, 1)
        self.assertEqual(label.props.label, "hello")

    def testCPyCSubclassing(self):
        obj = testhelper.create_test_type()
        self.assertEqual(obj.__grefcount__, 1)
        refcount = testhelper.test_g_object_new()
        self.assertEqual(refcount, 2)
        
    def testMassiveGtkSubclassing(self):
        for name, cls in [(name, getattr(gtk, name)) for name in dir(gtk)]:
            ## Skip some deprecated types
            if name in ['CTree', '_gobject']:
                continue
            try:
                if not issubclass(cls, gobject.GObject):
                    continue
            except TypeError: # raised by issubclass if cls is not a class
                    continue
            subname = name + "PyGtkTestSubclass"
            sub = type(subname, (cls,), {'__gtype_name__': subname })
    
    def testGtkWindowObjNewRefcount(self):
        foo = gobject.new(gtk.Window)
        self.assertEqual(foo.__grefcount__, 2)
        
    def testGtkWindowFactoryRefcount(self):
        foo = gtk.Window()
        self.assertEqual(foo.__grefcount__, 2)
        
    def testPyWindowObjNewRefcount(self):
        PyWindow = type('PyWindow', (gtk.Window,), dict(__gtype_name__='PyWindow1'))
        foo = gobject.new(PyWindow)
        self.assertEqual(foo.__grefcount__, 2)
        
    def testGtkWindowFactoryRefcount(self):
        PyWindow = type('PyWindow', (gtk.Window,), dict(__gtype_name__='PyWindow2'))
        foo = PyWindow()
        self.assertEqual(foo.__grefcount__, 2)

    def testRegisterArgNotType(self):
        self.assertRaises(TypeError, gobject.type_register, 1)

    def testGObjectNewError(self):
        self.assertRaises(TypeError, gobject.new, gtk.Label, text='foo')

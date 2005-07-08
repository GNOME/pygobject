
import unittest

from common import testhelper
from gobject import GObject, GType, PARAM_READWRITE

class PropertyObject(GObject):
    __gproperties__ = {
        'property': (str, 'blurb', 'description',
                     'default',
                     PARAM_READWRITE),
    }
    def __init__(self):
        GObject.__init__(self)
        self._value = 'default'
        
    def do_get_property(self, pspec):
        if pspec.name != 'property':
            raise AssertionError
        return self._value

    def do_set_property(self, pspec, value):
        if pspec.name != 'property':
            raise AssertionError
        self._value = value
        
class TestProperties(unittest.TestCase):
    def testGetSet(self):
        obj = PropertyObject()
        obj.props.property = "value"
        self.assertEqual(obj.props.property, "value")

    def testListWithInstance(self):
        obj = PropertyObject()
        self.failUnless(hasattr(obj.props, "property"))

    def testListWithoutInstance(self):
        self.failUnless(hasattr(PropertyObject.props, "property"))

    def testSetNoInstance(self):
        def set(obj):
            obj.props.property = "foobar"
            
        self.assertRaises(TypeError, set, PropertyObject)

    def testIterator(self):
        for obj in (PropertyObject.props, PropertyObject().props):
            for pspec in obj:
                gtype = GType(pspec)
                self.assertEqual(gtype.parent.name, 'GParam')
                self.assertEqual(pspec.name, 'property')

            self.assertEqual(len(obj), 1)
            self.assertEqual(list(obj), [pspec])

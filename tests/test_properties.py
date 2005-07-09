
import unittest

from common import testhelper
from gobject import *

class PropertyObject(GObject):
    __gproperties__ = {
        'property': (str, 'blurb', 'description',
                     'default',
                     PARAM_READWRITE),
        'construct-property': (str, 'blurb', 'description',
                     'default',
                     PARAM_READWRITE|PARAM_CONSTRUCT_ONLY),
    }
    def __init__(self):
        GObject.__init__(self)
        self._value = 'default'
        self._construct_property = None
        
    def do_get_property(self, pspec):
        if pspec.name == 'property':
            return self._value
        elif pspec.name == 'construct-property':
            return self._construct_property
        raise AssertionError


    def do_set_property(self, pspec, value):
        if pspec.name == 'property':
            self._value = value
        elif pspec.name == 'construct-property':
            self._construct_property = value
        else:
            raise AssertionError
            
        
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
                self.assert_(pspec.name in ['property', 'construct-property'])

            self.assertEqual(len(obj), 2)

    def testConstructProperty(self):
        obj = new(PropertyObject, construct_property="123")
        self.assertEqual(obj.props.construct_property, "123")
        ## TODO: raise exception when setting construct-only properties
        #obj.set_property("construct-property", "456")
        #self.assertEqual(obj.props.construct_property, "456")


import unittest

from common import testhelper
from gobject import GObject, GType, new, PARAM_READWRITE, \
     PARAM_CONSTRUCT, PARAM_CONSTRUCT_ONLY

class PropertyObject(GObject):
    __gproperties__ = {
        'normal': (str, 'blurb', 'description',  'default',
                     PARAM_READWRITE),
        'construct': (str, 'blurb', 'description', 'default',
                     PARAM_READWRITE|PARAM_CONSTRUCT),
        'construct-only': (str, 'blurb', 'description', 'default',
                     PARAM_READWRITE|PARAM_CONSTRUCT_ONLY),
    }
    
    def __init__(self):
        GObject.__init__(self)
        self._value = 'default'
        self._construct_only = None
        self._construct = None
        
    def do_get_property(self, pspec):
        if pspec.name == 'normal':
            return self._value
        elif pspec.name == 'construct':
            return self._construct
        elif pspec.name == 'construct-only':
            return self._construct_only
        else:
            raise AssertionError

    def do_set_property(self, pspec, value):
        if pspec.name == 'normal':
            self._value = value
        elif pspec.name == 'construct':
            self._construct = value
        elif pspec.name == 'construct-only':
            self._construct_only = value
        else:
            raise AssertionError
        
class TestProperties(unittest.TestCase):
    def testGetSet(self):
        obj = PropertyObject()
        obj.props.normal = "value"
        self.assertEqual(obj.props.normal, "value")

    def testListWithInstance(self):
        obj = PropertyObject()
        self.failUnless(hasattr(obj.props, "normal"))

    def testListWithoutInstance(self):
        self.failUnless(hasattr(PropertyObject.props, "normal"))

    def testSetNoInstance(self):
        def set(obj):
            obj.props.normal = "foobar"
            
        self.assertRaises(TypeError, set, PropertyObject)

    def testIterator(self):
        for obj in (PropertyObject.props, PropertyObject().props):
            for pspec in obj:
                gtype = GType(pspec)
                self.assertEqual(gtype.parent.name, 'GParam')
                self.failUnless(pspec.name in ['normal',
                                               'construct',
                                               'construct-only'])
            self.assertEqual(len(obj), 3)

    def testNormal(self):
        obj = new(PropertyObject, normal="123")
        self.assertEqual(obj.props.normal, "123")
        obj.set_property('normal', '456')
        self.assertEqual(obj.props.normal, "456")
        obj.props.normal = '789'
        self.assertEqual(obj.props.normal, "789")
        
    def testConstruct(self):
        obj = new(PropertyObject, construct="123")
        self.assertEqual(obj.props.construct, "123")
        obj.set_property('construct', '456')
        self.assertEqual(obj.props.construct, "456")
        obj.props.construct = '789'
        self.assertEqual(obj.props.construct, "789")
                         
    def testConstructOnly(self):
        obj = new(PropertyObject, construct_only="123")
        self.assertEqual(obj.props.construct_only, "123")
        self.assertRaises(TypeError,
                          setattr, obj.props, 'construct_only', '456')
        self.assertRaises(TypeError,
                          obj.set_property, 'construct-only', '456')

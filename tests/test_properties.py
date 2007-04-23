
import struct
import unittest

from common import testhelper
from gobject import GObject, GType, new, PARAM_READWRITE, \
     PARAM_CONSTRUCT, PARAM_READABLE, PARAM_WRITABLE, PARAM_CONSTRUCT_ONLY, \
     TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, \
     TYPE_INT64, TYPE_UINT64

class PropertyObject(GObject):
    __gproperties__ = {
        'normal': (str, 'blurb', 'description',  'default',
                     PARAM_READWRITE),
        'construct': (str, 'blurb', 'description', 'default',
                     PARAM_READWRITE|PARAM_CONSTRUCT),
        'construct-only': (str, 'blurb', 'description', 'default',
                     PARAM_READWRITE|PARAM_CONSTRUCT_ONLY),
        'uint64': (TYPE_UINT64, 'blurb', 'description', 0, 10, 0,
                   PARAM_READWRITE),
    }

    def __init__(self):
        GObject.__init__(self)
        self._value = 'default'
        self._construct_only = None
        self._construct = None
        self._uint64 = 0L

    def do_get_property(self, pspec):
        if pspec.name == 'normal':
            return self._value
        elif pspec.name == 'construct':
            return self._construct
        elif pspec.name == 'construct-only':
            return self._construct_only
        elif pspec.name == 'uint64':
            return self._uint64
        else:
            raise AssertionError

    def do_set_property(self, pspec, value):
        if pspec.name == 'normal':
            self._value = value
        elif pspec.name == 'construct':
            self._construct = value
        elif pspec.name == 'construct-only':
            self._construct_only = value
        elif pspec.name == 'uint64':
            self._uint64 = value
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
                                               'construct-only',
                                               'uint64'])
            self.assertEqual(len(obj), 4)

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

    def testUint64(self):
        obj = new(PropertyObject)
        self.assertEqual(obj.props.uint64, 0)
        obj.props.uint64 = 1L
        self.assertEqual(obj.props.uint64, 1L)
        obj.props.uint64 = 1
        self.assertEqual(obj.props.uint64, 1L)

    def testUInt64DefaultValue(self):
        try:
            class TimeControl(GObject):
                __gproperties__ = {
                    'time': (TYPE_UINT64, 'Time', 'Time',
                             0L, (1<<64) - 1, 0L,
                             PARAM_READABLE)
                    }
        except OverflowError, ex:
            self.fail(str(ex))

    def testRange(self):
        # kiwi code
        def max(c):
            return 2 ** ((8 * struct.calcsize(c)) - 1) - 1
        def umax(c):
            return 2 ** (8 * struct.calcsize(c)) - 1

        maxint = max('i')
        minint = -maxint - 1
        maxuint = umax('I')
        maxlong = max('l')
        minlong = -maxlong - 1
        maxulong = umax('L')
        maxint64 = max('q')
        minint64 = -maxint64 - 1
        maxuint64 = umax('Q')

        types = dict(int=(TYPE_INT, minint, maxint),
                     uint=(TYPE_UINT, 0, maxuint),
                     long=(TYPE_LONG, minlong, maxlong),
                     ulong=(TYPE_ULONG, 0, maxulong),
                     int64=(TYPE_INT64, minint64, maxint64),
                     uint64=(TYPE_UINT64, 0, maxuint64))

        def build_gproperties(types):
            d = {}
            for key, (gtype, min, max) in types.items():
                d[key] = (gtype, 'blurb', 'desc', min, max, 0,
                          PARAM_READABLE | PARAM_WRITABLE)
            return d

        class RangeCheck(GObject):
            __gproperties__ = build_gproperties(types)

            def __init__(self):
                self.values = {}
                GObject.__init__(self)

            def do_set_property(self, pspec, value):
                self.values[pspec.name] = value

            def do_get_property(self, pspec):
                return self.values.get(pspec.name, pspec.default_value)

        self.assertEqual(RangeCheck.props.int.minimum, minint)
        self.assertEqual(RangeCheck.props.int.maximum, maxint)
        self.assertEqual(RangeCheck.props.uint.minimum, 0)
        self.assertEqual(RangeCheck.props.uint.maximum, maxuint)
        self.assertEqual(RangeCheck.props.long.minimum, minlong)
        self.assertEqual(RangeCheck.props.long.maximum, maxlong)
        self.assertEqual(RangeCheck.props.ulong.minimum, 0)
        self.assertEqual(RangeCheck.props.ulong.maximum, maxulong)
        self.assertEqual(RangeCheck.props.int64.minimum, minint64)
        self.assertEqual(RangeCheck.props.int64.maximum, maxint64)
        self.assertEqual(RangeCheck.props.uint64.minimum, 0)
        self.assertEqual(RangeCheck.props.uint64.maximum, maxuint64)

        obj = RangeCheck()
        for key, (gtype, min, max) in types.items():
            self.assertEqual(obj.get_property(key),
                             getattr(RangeCheck.props, key).default_value)

            obj.set_property(key, min)
            self.assertEqual(obj.get_property(key), min)

            obj.set_property(key, max)
            self.assertEqual(obj.get_property(key), max)


    def testMulti(self):
        obj = PropertyObject()
        obj.set_properties(normal="foo",
                           uint64=7)
        normal, uint64 = obj.get_properties("normal", "uint64")
        self.assertEqual(normal, "foo")
        self.assertEqual(uint64, 7)

# coding=utf-8

import sys
import struct
import types
import unittest

from gi.repository import GObject
from gi.repository.GObject import GType, new, PARAM_READWRITE, \
    PARAM_CONSTRUCT, PARAM_READABLE, PARAM_WRITABLE, PARAM_CONSTRUCT_ONLY
from gi.repository.GObject import \
    TYPE_INT, TYPE_UINT, TYPE_LONG, TYPE_ULONG, TYPE_INT64, \
    TYPE_UINT64, TYPE_GTYPE, TYPE_INVALID, TYPE_NONE, TYPE_STRV, \
    TYPE_INTERFACE, TYPE_CHAR, TYPE_UCHAR, TYPE_BOOLEAN, TYPE_FLOAT, \
    TYPE_DOUBLE, TYPE_POINTER, TYPE_BOXED, TYPE_PARAM, TYPE_OBJECT, \
    TYPE_STRING, TYPE_PYOBJECT, TYPE_VARIANT

from gi.repository.GObject import \
    G_MININT, G_MAXINT, G_MAXUINT, G_MINLONG, G_MAXLONG, G_MAXULONG, \
    G_MAXUINT64, G_MAXINT64, G_MININT64

from gi.repository import Gio
from gi.repository import GLib
from gi.repository import Regress
from gi.repository import GIMarshallingTests
from gi._gobject import propertyhelper

if sys.version_info < (3, 0):
    TEST_UTF8 = "\xe2\x99\xa5"
    UNICODE_UTF8 = unicode(TEST_UTF8, 'UTF-8')
else:
    TEST_UTF8 = "♥"
    UNICODE_UTF8 = TEST_UTF8

from compathelper import _long


class PropertyObject(GObject.GObject):
    normal = GObject.Property(type=str)
    construct = GObject.Property(
        type=str,
        flags=PARAM_READWRITE | PARAM_CONSTRUCT, default='default')
    construct_only = GObject.Property(
        type=str,
        flags=PARAM_READWRITE | PARAM_CONSTRUCT_ONLY)
    uint64 = GObject.Property(
        type=TYPE_UINT64, flags=PARAM_READWRITE | PARAM_CONSTRUCT)

    enum = GObject.Property(
        type=Gio.SocketType, default=Gio.SocketType.STREAM)

    boxed = GObject.Property(
        type=GLib.Regex, flags=PARAM_READWRITE | PARAM_CONSTRUCT)

    flags = GObject.Property(
        type=GIMarshallingTests.Flags, flags=PARAM_READWRITE | PARAM_CONSTRUCT,
        default=GIMarshallingTests.Flags.VALUE1)

    gtype = GObject.Property(
        type=TYPE_GTYPE, flags=PARAM_READWRITE | PARAM_CONSTRUCT)

    strings = GObject.Property(
        type=TYPE_STRV, flags=PARAM_READWRITE | PARAM_CONSTRUCT)

    variant = GObject.Property(
        type=TYPE_VARIANT, flags=PARAM_READWRITE | PARAM_CONSTRUCT)

    variant_def = GObject.Property(
        type=TYPE_VARIANT, flags=PARAM_READWRITE | PARAM_CONSTRUCT,
        default=GLib.Variant('i', 42))

    interface = GObject.Property(
        type=Gio.File, flags=PARAM_READWRITE | PARAM_CONSTRUCT)


class PropertyInheritanceObject(Regress.TestObj):
    # override property from the base class, with a different type
    string = GObject.Property(type=int)

    # a property entirely defined at the Python level
    python_prop = GObject.Property(type=str)


class PropertySubClassObject(PropertyInheritanceObject):
    # override property from the base class, with a different type
    python_prop = GObject.Property(type=int)


class TestPropertyInheritanceObject(unittest.TestCase):
    def test_override_gi_property(self):
        self.assertNotEqual(Regress.TestObj.props.string.value_type,
                            PropertyInheritanceObject.props.string.value_type)
        obj = PropertyInheritanceObject()
        self.assertEqual(type(obj.props.string), int)
        obj.props.string = 4
        self.assertEqual(obj.props.string, 4)

    def test_override_python_property(self):
        obj = PropertySubClassObject()
        self.assertEqual(type(obj.props.python_prop), int)
        obj.props.python_prop = 5
        self.assertEqual(obj.props.python_prop, 5)


class TestPropertyObject(unittest.TestCase):
    def test_get_set(self):
        obj = PropertyObject()
        obj.props.normal = "value"
        self.assertEqual(obj.props.normal, "value")

    def test_hasattr_on_object(self):
        obj = PropertyObject()
        self.assertTrue(hasattr(obj.props, "normal"))

    def test_hasattr_on_class(self):
        self.assertTrue(hasattr(PropertyObject.props, "normal"))

    def test_set_on_class(self):
        def set(obj):
            obj.props.normal = "foobar"

        self.assertRaises(TypeError, set, PropertyObject)

    def test_iteration(self):
        for obj in (PropertyObject.props, PropertyObject().props):
            names = []
            for pspec in obj:
                gtype = GType(pspec)
                self.assertEqual(gtype.parent.name, 'GParam')
                names.append(pspec.name)

            names.sort()
            self.assertEqual(names, ['boxed',
                                     'construct',
                                     'construct-only',
                                     'enum',
                                     'flags',
                                     'gtype',
                                     'interface',
                                     'normal',
                                     'strings',
                                     'uint64',
                                     'variant',
                                     'variant-def'])

    def test_normal(self):
        obj = new(PropertyObject, normal="123")
        self.assertEqual(obj.props.normal, "123")
        obj.set_property('normal', '456')
        self.assertEqual(obj.props.normal, "456")
        obj.props.normal = '789'
        self.assertEqual(obj.props.normal, "789")

    def test_construct(self):
        obj = new(PropertyObject, construct="123")
        self.assertEqual(obj.props.construct, "123")
        obj.set_property('construct', '456')
        self.assertEqual(obj.props.construct, "456")
        obj.props.construct = '789'
        self.assertEqual(obj.props.construct, "789")

    def test_utf8(self):
        obj = new(PropertyObject, construct_only=UNICODE_UTF8)
        self.assertEqual(obj.props.construct_only, TEST_UTF8)
        obj.set_property('construct', UNICODE_UTF8)
        self.assertEqual(obj.props.construct, TEST_UTF8)
        obj.props.normal = UNICODE_UTF8
        self.assertEqual(obj.props.normal, TEST_UTF8)

    def test_int_to_str(self):
        obj = new(PropertyObject, construct_only=1)
        self.assertEqual(obj.props.construct_only, '1')
        obj.set_property('construct', '2')
        self.assertEqual(obj.props.construct, '2')
        obj.props.normal = 3
        self.assertEqual(obj.props.normal, '3')

    def test_construct_only(self):
        obj = new(PropertyObject, construct_only="123")
        self.assertEqual(obj.props.construct_only, "123")
        self.assertRaises(TypeError,
                          setattr, obj.props, 'construct_only', '456')
        self.assertRaises(TypeError,
                          obj.set_property, 'construct-only', '456')

    def test_uint64(self):
        obj = new(PropertyObject)
        self.assertEqual(obj.props.uint64, 0)
        obj.props.uint64 = _long(1)
        self.assertEqual(obj.props.uint64, _long(1))
        obj.props.uint64 = 1
        self.assertEqual(obj.props.uint64, _long(1))

        self.assertRaises((TypeError, OverflowError), obj.set_property, "uint64", _long(-1))
        self.assertRaises((TypeError, OverflowError), obj.set_property, "uint64", -1)

    def test_uint64_default_value(self):
        try:
            class TimeControl(GObject.GObject):
                __gproperties__ = {
                    'time': (TYPE_UINT64, 'Time', 'Time',
                             _long(0), (1 << 64) - 1, _long(0),
                             PARAM_READABLE)
                    }
        except OverflowError:
            (etype, ex) = sys.exc_info()[2:]
            self.fail(str(ex))

    def test_enum(self):
        obj = new(PropertyObject)
        self.assertEqual(obj.props.enum, Gio.SocketType.STREAM)
        self.assertEqual(obj.enum, Gio.SocketType.STREAM)
        obj.enum = Gio.SocketType.DATAGRAM
        self.assertEqual(obj.props.enum, Gio.SocketType.DATAGRAM)
        self.assertEqual(obj.enum, Gio.SocketType.DATAGRAM)
        obj.props.enum = Gio.SocketType.STREAM
        self.assertEqual(obj.props.enum, Gio.SocketType.STREAM)
        self.assertEqual(obj.enum, Gio.SocketType.STREAM)
        obj.props.enum = 2
        self.assertEqual(obj.props.enum, Gio.SocketType.DATAGRAM)
        self.assertEqual(obj.enum, Gio.SocketType.DATAGRAM)
        obj.enum = 1
        self.assertEqual(obj.props.enum, Gio.SocketType.STREAM)
        self.assertEqual(obj.enum, Gio.SocketType.STREAM)

        self.assertRaises(TypeError, setattr, obj, 'enum', 'foo')
        self.assertRaises(TypeError, setattr, obj, 'enum', object())

        self.assertRaises(TypeError, GObject.Property, type=Gio.SocketType)
        self.assertRaises(TypeError, GObject.Property, type=Gio.SocketType,
                          default=Gio.SocketProtocol.TCP)
        self.assertRaises(TypeError, GObject.Property, type=Gio.SocketType,
                          default=object())
        self.assertRaises(TypeError, GObject.Property, type=Gio.SocketType,
                          default=1)

    def test_flags(self):
        obj = new(PropertyObject)
        self.assertEqual(obj.props.flags, GIMarshallingTests.Flags.VALUE1)
        self.assertEqual(obj.flags, GIMarshallingTests.Flags.VALUE1)

        obj.flags = GIMarshallingTests.Flags.VALUE2 | GIMarshallingTests.Flags.VALUE3
        self.assertEqual(obj.props.flags, GIMarshallingTests.Flags.VALUE2 | GIMarshallingTests.Flags.VALUE3)
        self.assertEqual(obj.flags, GIMarshallingTests.Flags.VALUE2 | GIMarshallingTests.Flags.VALUE3)

        self.assertRaises(TypeError, setattr, obj, 'flags', 'foo')
        self.assertRaises(TypeError, setattr, obj, 'flags', object())
        self.assertRaises(TypeError, setattr, obj, 'flags', None)

        self.assertRaises(TypeError, GObject.Property,
                          type=GIMarshallingTests.Flags, default='foo')
        self.assertRaises(TypeError, GObject.Property,
                          type=GIMarshallingTests.Flags, default=object())
        self.assertRaises(TypeError, GObject.Property,
                          type=GIMarshallingTests.Flags, default=None)

    def test_gtype(self):
        obj = new(PropertyObject)

        self.assertEqual(obj.props.gtype, TYPE_NONE)
        self.assertEqual(obj.gtype, TYPE_NONE)

        obj.gtype = TYPE_UINT64
        self.assertEqual(obj.props.gtype, TYPE_UINT64)
        self.assertEqual(obj.gtype, TYPE_UINT64)

        obj.gtype = TYPE_INVALID
        self.assertEqual(obj.props.gtype, TYPE_INVALID)
        self.assertEqual(obj.gtype, TYPE_INVALID)

        # GType parameters do not support defaults in GLib
        self.assertRaises(TypeError, GObject.Property, type=TYPE_GTYPE,
                          default=TYPE_INT)

        # incompatible type
        self.assertRaises(TypeError, setattr, obj, 'gtype', 'foo')
        self.assertRaises(TypeError, setattr, obj, 'gtype', object())

        self.assertRaises(TypeError, GObject.Property, type=TYPE_GTYPE,
                          default='foo')
        self.assertRaises(TypeError, GObject.Property, type=TYPE_GTYPE,
                          default=object())

        # set in constructor
        obj = new(PropertyObject, gtype=TYPE_UINT)
        self.assertEqual(obj.props.gtype, TYPE_UINT)
        self.assertEqual(obj.gtype, TYPE_UINT)

    def test_boxed(self):
        obj = new(PropertyObject)

        regex = GLib.Regex.new('[a-z]*', 0, 0)
        obj.props.boxed = regex
        self.assertEqual(obj.props.boxed.get_pattern(), '[a-z]*')
        self.assertEqual(obj.boxed.get_pattern(), '[a-z]*')

        self.assertRaises(TypeError, setattr, obj, 'boxed', 'foo')
        self.assertRaises(TypeError, setattr, obj, 'boxed', object())

    def test_strings(self):
        obj = new(PropertyObject)

        # Should work with actual GStrv objects as well as
        # Python string lists
        class GStrv(list):
            __gtype__ = GObject.TYPE_STRV

        self.assertEqual(obj.props.strings, GStrv([]))
        self.assertEqual(obj.strings, GStrv([]))
        self.assertEqual(obj.props.strings, [])
        self.assertEqual(obj.strings, [])

        obj.strings = ['hello', 'world']
        self.assertEqual(obj.props.strings, ['hello', 'world'])
        self.assertEqual(obj.strings, ['hello', 'world'])

        obj.strings = GStrv(['hello', 'world'])
        self.assertEqual(obj.props.strings, GStrv(['hello', 'world']))
        self.assertEqual(obj.strings, GStrv(['hello', 'world']))

        obj.strings = []
        self.assertEqual(obj.strings, [])
        obj.strings = GStrv([])
        self.assertEqual(obj.strings, GStrv([]))

        p = GObject.Property(type=TYPE_STRV, default=['hello', '1'])
        self.assertEqual(p.default, ['hello', '1'])
        self.assertEqual(p.type, TYPE_STRV)
        p = GObject.Property(type=TYPE_STRV, default=GStrv(['hello', '1']))
        self.assertEqual(p.default, ['hello', '1'])
        self.assertEqual(p.type, TYPE_STRV)

        # set in constructor
        obj = new(PropertyObject, strings=['hello', 'world'])
        self.assertEqual(obj.props.strings, ['hello', 'world'])
        self.assertEqual(obj.strings, ['hello', 'world'])

        # wrong types
        self.assertRaises(TypeError, setattr, obj, 'strings', 1)
        self.assertRaises(TypeError, setattr, obj, 'strings', 'foo')
        self.assertRaises(TypeError, setattr, obj, 'strings', ['foo', 1])

        self.assertRaises(TypeError, GObject.Property, type=TYPE_STRV,
                          default=1)
        self.assertRaises(TypeError, GObject.Property, type=TYPE_STRV,
                          default='foo')
        self.assertRaises(TypeError, GObject.Property, type=TYPE_STRV,
                          default=['hello', 1])

    def test_variant(self):
        obj = new(PropertyObject)

        self.assertEqual(obj.props.variant, None)
        self.assertEqual(obj.variant, None)

        obj.variant = GLib.Variant('s', 'hello')
        self.assertEqual(obj.variant.print_(True), "'hello'")

        obj.variant = GLib.Variant('b', True)
        self.assertEqual(obj.variant.print_(True), "true")

        obj.props.variant = GLib.Variant('y', 2)
        self.assertEqual(obj.variant.print_(True), "byte 0x02")

        obj.variant = None
        self.assertEqual(obj.variant, None)

        # set in constructor
        obj = new(PropertyObject, variant=GLib.Variant('u', 5))
        self.assertEqual(obj.props.variant.print_(True), 'uint32 5')

        GObject.Property(type=TYPE_VARIANT, default=GLib.Variant('i', 1))

        # incompatible types
        self.assertRaises(TypeError, setattr, obj, 'variant', 'foo')
        self.assertRaises(TypeError, setattr, obj, 'variant', 42)

        self.assertRaises(TypeError, GObject.Property, type=TYPE_VARIANT,
                          default='foo')
        self.assertRaises(TypeError, GObject.Property, type=TYPE_VARIANT,
                          default=object())

    def test_variant_default(self):
        obj = new(PropertyObject)

        self.assertEqual(obj.props.variant_def.print_(True), '42')
        self.assertEqual(obj.variant_def.print_(True), '42')

        obj.props.variant_def = GLib.Variant('y', 2)
        self.assertEqual(obj.variant_def.print_(True), "byte 0x02")

        # set in constructor
        obj = new(PropertyObject, variant_def=GLib.Variant('u', 5))
        self.assertEqual(obj.props.variant_def.print_(True), 'uint32 5')

    def test_interface(self):
        obj = new(PropertyObject)

        file = Gio.File.new_for_path('/some/path')
        obj.props.interface = file
        self.assertEqual(obj.props.interface.get_path(), '/some/path')
        self.assertEqual(obj.interface.get_path(), '/some/path')

        self.assertRaises(TypeError, setattr, obj, 'interface', 'foo')
        self.assertRaises(TypeError, setattr, obj, 'interface', object())

    def test_range(self):
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

        types_ = dict(int=(TYPE_INT, minint, maxint),
                      uint=(TYPE_UINT, 0, maxuint),
                      long=(TYPE_LONG, minlong, maxlong),
                      ulong=(TYPE_ULONG, 0, maxulong),
                      int64=(TYPE_INT64, minint64, maxint64),
                      uint64=(TYPE_UINT64, 0, maxuint64))

        def build_gproperties(types_):
            d = {}
            for key, (gtype, min, max) in types_.items():
                d[key] = (gtype, 'blurb', 'desc', min, max, 0,
                          PARAM_READABLE | PARAM_WRITABLE)
            return d

        class RangeCheck(GObject.GObject):
            __gproperties__ = build_gproperties(types_)

            def __init__(self):
                self.values = {}
                GObject.GObject.__init__(self)

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
        for key, (gtype, min, max) in types_.items():
            self.assertEqual(obj.get_property(key),
                             getattr(RangeCheck.props, key).default_value)

            obj.set_property(key, min)
            self.assertEqual(obj.get_property(key), min)

            obj.set_property(key, max)
            self.assertEqual(obj.get_property(key), max)

    def test_multi(self):
        obj = PropertyObject()
        obj.set_properties(normal="foo",
                           uint64=7)
        normal, uint64 = obj.get_properties("normal", "uint64")
        self.assertEqual(normal, "foo")
        self.assertEqual(uint64, 7)


class TestProperty(unittest.TestCase):
    def test_simple(self):
        class C(GObject.GObject):
            str = GObject.Property(type=str)
            int = GObject.Property(type=int)
            float = GObject.Property(type=float)
            long = GObject.Property(type=_long)

        self.assertTrue(hasattr(C.props, 'str'))
        self.assertTrue(hasattr(C.props, 'int'))
        self.assertTrue(hasattr(C.props, 'float'))
        self.assertTrue(hasattr(C.props, 'long'))

        o = C()
        self.assertEqual(o.str, '')
        o.str = 'str'
        self.assertEqual(o.str, 'str')

        self.assertEqual(o.int, 0)
        o.int = 1138
        self.assertEqual(o.int, 1138)

        self.assertEqual(o.float, 0.0)
        o.float = 3.14
        self.assertEqual(o.float, 3.14)

        self.assertEqual(o.long, _long(0))
        o.long = _long(100)
        self.assertEqual(o.long, _long(100))

    def test_custom_getter(self):
        class C(GObject.GObject):
            def get_prop(self):
                return 'value'
            prop = GObject.Property(getter=get_prop)

        o = C()
        self.assertEqual(o.prop, 'value')
        self.assertRaises(TypeError, setattr, o, 'prop', 'xxx')

    def test_custom_setter(self):
        class C(GObject.GObject):
            def set_prop(self, value):
                self._value = value
            prop = GObject.Property(setter=set_prop)

            def __init__(self):
                self._value = None
                GObject.GObject.__init__(self)

        o = C()
        self.assertEqual(o._value, None)
        o.prop = 'bar'
        self.assertEqual(o._value, 'bar')
        self.assertRaises(TypeError, getattr, o, 'prop')

    def test_decorator_default(self):
        class C(GObject.GObject):
            _value = 'value'

            @GObject.Property
            def value(self):
                return self._value

            @value.setter
            def value_setter(self, value):
                self._value = value

        o = C()
        self.assertEqual(o.value, 'value')
        o.value = 'blah'
        self.assertEqual(o.value, 'blah')
        self.assertEqual(o.props.value, 'blah')

    def test_decorator_private_setter(self):
        class C(GObject.GObject):
            _value = 'value'

            @GObject.Property
            def value(self):
                return self._value

            @value.setter
            def _set_value(self, value):
                self._value = value

        o = C()
        self.assertEqual(o.value, 'value')
        o.value = 'blah'
        self.assertEqual(o.value, 'blah')
        self.assertEqual(o.props.value, 'blah')

    def test_decorator_with_call(self):
        class C(GObject.GObject):
            _value = 1

            @GObject.Property(type=int, default=1, minimum=1, maximum=10)
            def typedValue(self):
                return self._value

            @typedValue.setter
            def typedValue_setter(self, value):
                self._value = value

        o = C()
        self.assertEqual(o.typedValue, 1)
        o.typedValue = 5
        self.assertEqual(o.typedValue, 5)
        self.assertEqual(o.props.typedValue, 5)

    def test_errors(self):
        self.assertRaises(TypeError, GObject.Property, type='str')
        self.assertRaises(TypeError, GObject.Property, nick=False)
        self.assertRaises(TypeError, GObject.Property, blurb=False)
        # this never fail while bool is a subclass of int
        # >>> bool.__bases__
        # (<type 'int'>,)
        # self.assertRaises(TypeError, GObject.Property, type=bool, default=0)
        self.assertRaises(TypeError, GObject.Property, type=bool, default='ciao mamma')
        self.assertRaises(TypeError, GObject.Property, type=bool)
        self.assertRaises(TypeError, GObject.Property, type=object, default=0)
        self.assertRaises(TypeError, GObject.Property, type=complex)

    def test_defaults(self):
        GObject.Property(type=bool, default=True)
        GObject.Property(type=bool, default=False)

    def test_name_with_underscore(self):
        class C(GObject.GObject):
            prop_name = GObject.Property(type=int)
        o = C()
        o.prop_name = 10
        self.assertEqual(o.prop_name, 10)

    def test_range(self):
        types_ = [
            (TYPE_INT, G_MININT, G_MAXINT),
            (TYPE_UINT, 0, G_MAXUINT),
            (TYPE_LONG, G_MINLONG, G_MAXLONG),
            (TYPE_ULONG, 0, G_MAXULONG),
            (TYPE_INT64, G_MININT64, G_MAXINT64),
            (TYPE_UINT64, 0, G_MAXUINT64),
            ]

        for gtype, min, max in types_:
            # Normal, everything is alright
            prop = GObject.Property(type=gtype, minimum=min, maximum=max)
            subtype = type('', (GObject.GObject,), dict(prop=prop))
            self.assertEqual(subtype.props.prop.minimum, min)
            self.assertEqual(subtype.props.prop.maximum, max)

            # Lower than minimum
            self.assertRaises(TypeError,
                              GObject.Property, type=gtype, minimum=min - 1,
                              maximum=max)

            # Higher than maximum
            self.assertRaises(TypeError,
                              GObject.Property, type=gtype, minimum=min,
                              maximum=max + 1)

    def test_min_max(self):
        class C(GObject.GObject):
            prop_int = GObject.Property(type=int, minimum=1, maximum=100, default=1)
            prop_float = GObject.Property(type=float, minimum=0.1, maximum=10.5, default=1.1)

            def __init__(self):
                GObject.GObject.__init__(self)

        # we test known-bad values here which cause Gtk-WARNING logs.
        # Explicitly allow these for this test.
        old_mask = GLib.log_set_always_fatal(GLib.LogLevelFlags.LEVEL_CRITICAL)
        try:
            o = C()
            self.assertEqual(o.prop_int, 1)

            o.prop_int = 5
            self.assertEqual(o.prop_int, 5)

            o.prop_int = 0
            self.assertEqual(o.prop_int, 5)

            o.prop_int = 101
            self.assertEqual(o.prop_int, 5)

            self.assertEqual(o.prop_float, 1.1)

            o.prop_float = 7.75
            self.assertEqual(o.prop_float, 7.75)

            o.prop_float = 0.09
            self.assertEqual(o.prop_float, 7.75)

            o.prop_float = 10.51
            self.assertEqual(o.prop_float, 7.75)
        finally:
            GLib.log_set_always_fatal(old_mask)

    def test_multiple_instances(self):
        class C(GObject.GObject):
            prop = GObject.Property(type=str, default='default')

        o1 = C()
        o2 = C()
        self.assertEqual(o1.prop, 'default')
        self.assertEqual(o2.prop, 'default')
        o1.prop = 'value'
        self.assertEqual(o1.prop, 'value')
        self.assertEqual(o2.prop, 'default')

    def test_object_property(self):
        class PropertyObject(GObject.GObject):
            obj = GObject.Property(type=GObject.GObject)

        pobj1 = PropertyObject()
        obj1_hash = hash(pobj1)
        pobj2 = PropertyObject()

        pobj2.obj = pobj1
        del pobj1
        pobj1 = pobj2.obj
        self.assertEqual(hash(pobj1), obj1_hash)

    def test_object_subclass_property(self):
        class ObjectSubclass(GObject.GObject):
            __gtype_name__ = 'ObjectSubclass'

        class PropertyObjectSubclass(GObject.GObject):
            obj = GObject.Property(type=ObjectSubclass)

        PropertyObjectSubclass(obj=ObjectSubclass())

    def test_property_subclass(self):
        # test for #470718
        class A(GObject.GObject):
            prop1 = GObject.Property(type=int)

        class B(A):
            prop2 = GObject.Property(type=int)

        b = B()
        b.prop2 = 10
        self.assertEqual(b.prop2, 10)
        b.prop1 = 20
        self.assertEqual(b.prop1, 20)

    def test_property_subclass_c(self):
        class A(Regress.TestSubObj):
            prop1 = GObject.Property(type=int)

        a = A()
        a.prop1 = 10
        self.assertEqual(a.prop1, 10)

        # also has parent properties
        a.props.int = 20
        self.assertEqual(a.props.int, 20)

        # Some of which are unusable without introspection
        a.props.list = ("str1", "str2")
        self.assertEqual(a.props.list, ["str1", "str2"])

        a.set_property("list", ("str3", "str4"))
        self.assertEqual(a.props.list, ["str3", "str4"])

    def test_property_subclass_custom_setter(self):
        # test for #523352
        class A(GObject.GObject):
            def get_first(self):
                return 'first'
            first = GObject.Property(type=str, getter=get_first)

        class B(A):
            def get_second(self):
                return 'second'
            second = GObject.Property(type=str, getter=get_second)

        a = A()
        self.assertEqual(a.first, 'first')
        self.assertRaises(TypeError, setattr, a, 'first', 'foo')

        b = B()
        self.assertEqual(b.first, 'first')
        self.assertRaises(TypeError, setattr, b, 'first', 'foo')
        self.assertEqual(b.second, 'second')
        self.assertRaises(TypeError, setattr, b, 'second', 'foo')

    def test_property_subclass_custom_setter_error(self):
        try:
            class A(GObject.GObject):
                def get_first(self):
                    return 'first'
                first = GObject.Property(type=str, getter=get_first)

                def do_get_property(self, pspec):
                    pass
        except TypeError:
            pass
        else:
            raise AssertionError

    # Bug 587637.

    def test_float_min(self):
        GObject.Property(type=float, minimum=-1)
        GObject.Property(type=GObject.TYPE_FLOAT, minimum=-1)
        GObject.Property(type=GObject.TYPE_DOUBLE, minimum=-1)

    # Bug 644039

    def test_reference_count(self):
        # We can check directly if an object gets finalized, so we will
        # observe it indirectly through the refcount of a member object.

        # We create our dummy object and store its current refcount
        o = object()
        rc = sys.getrefcount(o)

        # We add our object as a member to our newly created object we
        # want to observe. Its refcount is increased by one.
        t = PropertyObject(normal="test")
        t.o = o
        self.assertEqual(sys.getrefcount(o), rc + 1)

        # Now we want to ensure we do not leak any references to our
        # object with properties. If no ref is leaked, then when deleting
        # the local reference to this object, its reference count shoud
        # drop to zero, and our dummy object should loose one reference.
        del t
        self.assertEqual(sys.getrefcount(o), rc)

    def test_doc_strings(self):
        class C(GObject.GObject):
            foo_blurbed = GObject.Property(type=int, blurb='foo_blurbed doc string')

            @GObject.Property
            def foo_getter(self):
                """foo_getter doc string"""
                return 0

        self.assertEqual(C.foo_blurbed.blurb, 'foo_blurbed doc string')
        self.assertEqual(C.foo_blurbed.__doc__, 'foo_blurbed doc string')

        self.assertEqual(C.foo_getter.blurb, 'foo_getter doc string')
        self.assertEqual(C.foo_getter.__doc__, 'foo_getter doc string')

    def test_python_to_glib_type_mapping(self):
        tester = GObject.Property()
        self.assertEqual(tester._type_from_python(int), GObject.TYPE_INT)
        if sys.version_info < (3, 0):
            self.assertEqual(tester._type_from_python(long), GObject.TYPE_LONG)
        self.assertEqual(tester._type_from_python(bool), GObject.TYPE_BOOLEAN)
        self.assertEqual(tester._type_from_python(float), GObject.TYPE_DOUBLE)
        self.assertEqual(tester._type_from_python(str), GObject.TYPE_STRING)
        self.assertEqual(tester._type_from_python(object), GObject.TYPE_PYOBJECT)

        self.assertEqual(tester._type_from_python(GObject.GObject), GObject.GObject.__gtype__)
        self.assertEqual(tester._type_from_python(GObject.GEnum), GObject.GEnum.__gtype__)
        self.assertEqual(tester._type_from_python(GObject.GFlags), GObject.GFlags.__gtype__)
        self.assertEqual(tester._type_from_python(GObject.GBoxed), GObject.GBoxed.__gtype__)
        self.assertEqual(tester._type_from_python(GObject.GInterface), GObject.GInterface.__gtype__)

        for type_ in [TYPE_NONE, TYPE_INTERFACE, TYPE_CHAR, TYPE_UCHAR,
                      TYPE_INT, TYPE_UINT, TYPE_BOOLEAN, TYPE_LONG,
                      TYPE_ULONG, TYPE_INT64, TYPE_UINT64,
                      TYPE_FLOAT, TYPE_DOUBLE, TYPE_POINTER,
                      TYPE_BOXED, TYPE_PARAM, TYPE_OBJECT, TYPE_STRING,
                      TYPE_PYOBJECT, TYPE_GTYPE, TYPE_STRV]:
            self.assertEqual(tester._type_from_python(type_), type_)

        self.assertRaises(TypeError, tester._type_from_python, types.CodeType)


class TestInstallProperties(unittest.TestCase):
    # These tests only test how signalhelper.install_signals works
    # with the __gsignals__ dict and therefore does not need to use
    # GObject as a base class because that would automatically call
    # install_signals within the meta-class.
    class Base(object):
        __gproperties__ = {'test': (0, '', '', 0, 0, 0, 0)}

    class Sub1(Base):
        pass

    class Sub2(Base):
        @GObject.Property(type=int)
        def sub2test(self):
            return 123

    class ClassWithPropertyAndGetterVFunc(object):
        @GObject.Property(type=int)
        def sub2test(self):
            return 123

        def do_get_property(self, name):
            return 321

    class ClassWithPropertyRedefined(object):
        __gproperties__ = {'test': (0, '', '', 0, 0, 0, 0)}
        test = GObject.Property(type=int)

    def setUp(self):
        self.assertEqual(len(self.Base.__gproperties__), 1)
        propertyhelper.install_properties(self.Base)
        self.assertEqual(len(self.Base.__gproperties__), 1)

    def test_subclass_without_properties_is_not_modified(self):
        self.assertFalse('__gproperties__' in self.Sub1.__dict__)
        propertyhelper.install_properties(self.Sub1)
        self.assertFalse('__gproperties__' in self.Sub1.__dict__)

    def test_subclass_with_decorator_gets_gproperties_dict(self):
        # Sub2 has Property instances but will not have a __gproperties__
        # until install_properties is called
        self.assertFalse('__gproperties__' in self.Sub2.__dict__)
        self.assertFalse('do_get_property' in self.Sub2.__dict__)
        self.assertFalse('do_set_property' in self.Sub2.__dict__)

        propertyhelper.install_properties(self.Sub2)
        self.assertTrue('__gproperties__' in self.Sub2.__dict__)
        self.assertEqual(len(self.Base.__gproperties__), 1)
        self.assertEqual(len(self.Sub2.__gproperties__), 1)
        self.assertTrue('sub2test' in self.Sub2.__gproperties__)

        # get/set vfuncs should have been added
        self.assertTrue('do_get_property' in self.Sub2.__dict__)
        self.assertTrue('do_set_property' in self.Sub2.__dict__)

    def test_object_with_property_and_do_get_property_vfunc_raises(self):
        self.assertRaises(TypeError, propertyhelper.install_properties,
                          self.ClassWithPropertyAndGetterVFunc)

    def test_same_name_property_definitions_raises(self):
        self.assertRaises(ValueError, propertyhelper.install_properties,
                          self.ClassWithPropertyRedefined)

if __name__ == '__main__':
    unittest.main()

import os
import gc
import sys
import struct
import types
import unittest
import tempfile

import pytest

from gi.repository import GObject
from gi.repository.GObject import ParamFlags, GType, new
from gi.repository.GObject import (
    TYPE_INT,
    TYPE_UINT,
    TYPE_LONG,
    TYPE_ULONG,
    TYPE_INT64,
    TYPE_UINT64,
    TYPE_GTYPE,
    TYPE_INVALID,
    TYPE_NONE,
    TYPE_STRV,
    TYPE_INTERFACE,
    TYPE_CHAR,
    TYPE_UCHAR,
    TYPE_BOOLEAN,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_POINTER,
    TYPE_BOXED,
    TYPE_PARAM,
    TYPE_OBJECT,
    TYPE_STRING,
    TYPE_PYOBJECT,
    TYPE_VARIANT,
)

from gi.repository.GLib import (
    MININT,
    MAXINT,
    MAXUINT,
    MINLONG,
    MAXLONG,
    MAXULONG,
    MAXUINT64,
    MAXINT64,
    MININT64,
)

from gi.repository import Gio
from gi.repository import GLib
from gi.repository import GIMarshallingTests
from gi.repository import Regress
from gi import _propertyhelper as propertyhelper

from .helper import capture_glib_warnings


class PropertyObject(GObject.GObject):
    normal = GObject.Property(type=str)
    construct = GObject.Property(
        type=str,
        flags=ParamFlags.READABLE | ParamFlags.WRITABLE | ParamFlags.CONSTRUCT,
        default="default",
    )

    construct_only = GObject.Property(
        type=str,
        flags=ParamFlags.READABLE | ParamFlags.WRITABLE | ParamFlags.CONSTRUCT_ONLY,
    )

    uint64 = GObject.Property(
        type=TYPE_UINT64,
        flags=ParamFlags.READABLE | ParamFlags.WRITABLE | ParamFlags.CONSTRUCT,
    )

    enum = GObject.Property(type=Gio.SocketType, default=Gio.SocketType.STREAM)

    boxed = GObject.Property(
        type=GLib.Regex,
        flags=ParamFlags.READABLE | ParamFlags.WRITABLE | ParamFlags.CONSTRUCT,
    )

    flags = GObject.Property(
        type=GIMarshallingTests.Flags,
        flags=ParamFlags.READABLE | ParamFlags.WRITABLE | ParamFlags.CONSTRUCT,
        default=GIMarshallingTests.Flags.VALUE1,
    )

    gtype = GObject.Property(
        type=TYPE_GTYPE,
        flags=ParamFlags.READABLE | ParamFlags.WRITABLE | ParamFlags.CONSTRUCT,
    )

    strings = GObject.Property(
        type=TYPE_STRV,
        flags=ParamFlags.READABLE | ParamFlags.WRITABLE | ParamFlags.CONSTRUCT,
    )

    variant = GObject.Property(
        type=TYPE_VARIANT,
        flags=ParamFlags.READABLE | ParamFlags.WRITABLE | ParamFlags.CONSTRUCT,
    )

    variant_def = GObject.Property(
        type=TYPE_VARIANT,
        flags=ParamFlags.READABLE | ParamFlags.WRITABLE | ParamFlags.CONSTRUCT,
        default=GLib.Variant("i", 42),
    )

    interface = GObject.Property(
        type=Gio.File,
        flags=ParamFlags.READABLE | ParamFlags.WRITABLE | ParamFlags.CONSTRUCT,
    )


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
        self.assertNotEqual(
            Regress.TestObj.props.string.value_type,
            PropertyInheritanceObject.props.string.value_type,
        )
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
                self.assertEqual(gtype.parent.name, "GParam")
                names.append(pspec.name)

            names.sort()
            self.assertEqual(
                names,
                [
                    "boxed",
                    "construct",
                    "construct-only",
                    "enum",
                    "flags",
                    "gtype",
                    "interface",
                    "normal",
                    "strings",
                    "uint64",
                    "variant",
                    "variant-def",
                ],
            )

    def test_iterator_protocol_for_properties(self):
        action = Gio.SimpleAction()

        i = iter(action.props)
        assert i is iter(i)

    def test_normal(self):
        obj = new(PropertyObject, normal="123")
        self.assertEqual(obj.props.normal, "123")
        obj.set_property("normal", "456")
        self.assertEqual(obj.props.normal, "456")
        obj.props.normal = "789"
        self.assertEqual(obj.props.normal, "789")

    def test_construct(self):
        obj = new(PropertyObject, construct="123")
        self.assertEqual(obj.props.construct, "123")
        obj.set_property("construct", "456")
        self.assertEqual(obj.props.construct, "456")
        obj.props.construct = "789"
        self.assertEqual(obj.props.construct, "789")

    def test_utf8(self):
        test_utf8 = "♥"
        unicode_utf8 = "♥"
        obj = new(PropertyObject, construct_only=unicode_utf8)
        self.assertEqual(obj.props.construct_only, test_utf8)
        obj.set_property("construct", unicode_utf8)
        self.assertEqual(obj.props.construct, test_utf8)
        obj.props.normal = unicode_utf8
        self.assertEqual(obj.props.normal, test_utf8)

    def test_utf8_lone_surrogate(self):
        obj = PropertyObject()
        with pytest.raises(TypeError):
            obj.set_property("construct", "\ud83d")

    def test_int_to_str(self):
        obj = new(PropertyObject, construct_only=1)
        self.assertEqual(obj.props.construct_only, "1")
        obj.set_property("construct", "2")
        self.assertEqual(obj.props.construct, "2")
        obj.props.normal = 3
        self.assertEqual(obj.props.normal, "3")

    def test_construct_only(self):
        obj = new(PropertyObject, construct_only="123")
        self.assertEqual(obj.props.construct_only, "123")
        self.assertRaises(TypeError, setattr, obj.props, "construct_only", "456")
        self.assertRaises(TypeError, obj.set_property, "construct-only", "456")

    def test_uint64(self):
        obj = new(PropertyObject)
        self.assertEqual(obj.props.uint64, 0)
        obj.props.uint64 = 1
        self.assertEqual(obj.props.uint64, 1)
        obj.props.uint64 = 1
        self.assertEqual(obj.props.uint64, 1)

        self.assertRaises((TypeError, OverflowError), obj.set_property, "uint64", -1)
        self.assertRaises((TypeError, OverflowError), obj.set_property, "uint64", -1)

    def test_uint64_default_value(self):
        try:

            class TimeControl(GObject.GObject):
                __gproperties__ = {
                    "time": (
                        TYPE_UINT64,
                        "Time",
                        "Time",
                        0,
                        (1 << 64) - 1,
                        0,
                        ParamFlags.READABLE,
                    )
                }
        except OverflowError:
            (_etype, ex) = sys.exc_info()[2:]
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

        self.assertRaises(TypeError, setattr, obj, "enum", "foo")
        self.assertRaises(TypeError, setattr, obj, "enum", object())

        self.assertRaises(TypeError, GObject.Property, type=Gio.SocketType)
        self.assertRaises(
            TypeError,
            GObject.Property,
            type=Gio.SocketType,
            default=Gio.SocketProtocol.TCP,
        )
        self.assertRaises(
            TypeError, GObject.Property, type=Gio.SocketType, default=object()
        )
        self.assertRaises(TypeError, GObject.Property, type=Gio.SocketType, default=1)

    def test_repr(self):
        prop = GObject.Property(type=int)
        assert repr(prop) == "<GObject Property (uninitialized) (gint)>"

    def test_flags(self):
        obj = new(PropertyObject)
        self.assertEqual(obj.props.flags, GIMarshallingTests.Flags.VALUE1)
        self.assertEqual(obj.flags, GIMarshallingTests.Flags.VALUE1)

        obj.flags = GIMarshallingTests.Flags.VALUE2 | GIMarshallingTests.Flags.VALUE3
        self.assertEqual(
            obj.props.flags,
            GIMarshallingTests.Flags.VALUE2 | GIMarshallingTests.Flags.VALUE3,
        )
        self.assertEqual(
            obj.flags, GIMarshallingTests.Flags.VALUE2 | GIMarshallingTests.Flags.VALUE3
        )

        self.assertRaises(TypeError, setattr, obj, "flags", "foo")
        self.assertRaises(TypeError, setattr, obj, "flags", object())
        self.assertRaises(TypeError, setattr, obj, "flags", None)

        self.assertRaises(
            TypeError, GObject.Property, type=GIMarshallingTests.Flags, default="foo"
        )
        self.assertRaises(
            TypeError, GObject.Property, type=GIMarshallingTests.Flags, default=object()
        )
        self.assertRaises(
            TypeError, GObject.Property, type=GIMarshallingTests.Flags, default=None
        )

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
        self.assertRaises(
            TypeError, GObject.Property, type=TYPE_GTYPE, default=TYPE_INT
        )

        # incompatible type
        self.assertRaises(TypeError, setattr, obj, "gtype", "foo")
        self.assertRaises(TypeError, setattr, obj, "gtype", object())

        self.assertRaises(TypeError, GObject.Property, type=TYPE_GTYPE, default="foo")
        self.assertRaises(
            TypeError, GObject.Property, type=TYPE_GTYPE, default=object()
        )

        # set in constructor
        obj = new(PropertyObject, gtype=TYPE_UINT)
        self.assertEqual(obj.props.gtype, TYPE_UINT)
        self.assertEqual(obj.gtype, TYPE_UINT)

    def test_boxed(self):
        obj = new(PropertyObject)

        regex = GLib.Regex.new("[a-z]*", 0, 0)
        obj.props.boxed = regex
        self.assertEqual(obj.props.boxed.get_pattern(), "[a-z]*")
        self.assertEqual(obj.boxed.get_pattern(), "[a-z]*")

        self.assertRaises(TypeError, setattr, obj, "boxed", "foo")
        self.assertRaises(TypeError, setattr, obj, "boxed", object())

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

        obj.strings = ["hello", "world"]
        self.assertEqual(obj.props.strings, ["hello", "world"])
        self.assertEqual(obj.strings, ["hello", "world"])

        obj.strings = GStrv(["hello", "world"])
        self.assertEqual(obj.props.strings, GStrv(["hello", "world"]))
        self.assertEqual(obj.strings, GStrv(["hello", "world"]))

        obj.strings = []
        self.assertEqual(obj.strings, [])
        obj.strings = GStrv([])
        self.assertEqual(obj.strings, GStrv([]))

        p = GObject.Property(type=TYPE_STRV, default=["hello", "1"])
        self.assertEqual(p.default, ["hello", "1"])
        self.assertEqual(p.type, TYPE_STRV)
        p = GObject.Property(type=TYPE_STRV, default=GStrv(["hello", "1"]))
        self.assertEqual(p.default, ["hello", "1"])
        self.assertEqual(p.type, TYPE_STRV)

        # set in constructor
        obj = new(PropertyObject, strings=["hello", "world"])
        self.assertEqual(obj.props.strings, ["hello", "world"])
        self.assertEqual(obj.strings, ["hello", "world"])

        # wrong types
        self.assertRaises(TypeError, setattr, obj, "strings", 1)
        self.assertRaises(TypeError, setattr, obj, "strings", "foo")
        self.assertRaises(TypeError, setattr, obj, "strings", ["foo", 1])

        self.assertRaises(TypeError, GObject.Property, type=TYPE_STRV, default=1)
        self.assertRaises(TypeError, GObject.Property, type=TYPE_STRV, default="foo")
        self.assertRaises(
            TypeError, GObject.Property, type=TYPE_STRV, default=["hello", 1]
        )

    def test_variant(self):
        obj = new(PropertyObject)

        self.assertEqual(obj.props.variant, None)
        self.assertEqual(obj.variant, None)

        obj.variant = GLib.Variant("s", "hello")
        self.assertEqual(obj.variant.print_(True), "'hello'")

        obj.variant = GLib.Variant("b", True)
        self.assertEqual(obj.variant.print_(True), "true")

        obj.props.variant = GLib.Variant("y", 2)
        self.assertEqual(obj.variant.print_(True), "byte 0x02")

        obj.variant = None
        self.assertEqual(obj.variant, None)

        # set in constructor
        obj = new(PropertyObject, variant=GLib.Variant("u", 5))
        self.assertEqual(obj.props.variant.print_(True), "uint32 5")

        GObject.Property(type=TYPE_VARIANT, default=GLib.Variant("i", 1))

        # incompatible types
        self.assertRaises(TypeError, setattr, obj, "variant", "foo")
        self.assertRaises(TypeError, setattr, obj, "variant", 42)

        self.assertRaises(TypeError, GObject.Property, type=TYPE_VARIANT, default="foo")
        self.assertRaises(
            TypeError, GObject.Property, type=TYPE_VARIANT, default=object()
        )

    def test_variant_default(self):
        obj = new(PropertyObject)

        self.assertEqual(obj.props.variant_def.print_(True), "42")
        self.assertEqual(obj.variant_def.print_(True), "42")

        obj.props.variant_def = GLib.Variant("y", 2)
        self.assertEqual(obj.variant_def.print_(True), "byte 0x02")

        # set in constructor
        obj = new(PropertyObject, variant_def=GLib.Variant("u", 5))
        self.assertEqual(obj.props.variant_def.print_(True), "uint32 5")

    def test_interface(self):
        obj = new(PropertyObject)

        path = os.path.join(tempfile.gettempdir(), "some", "path")
        file = Gio.File.new_for_path(path)
        obj.props.interface = file
        self.assertEqual(obj.props.interface.get_path(), path)
        self.assertEqual(obj.interface.get_path(), path)

        self.assertRaises(TypeError, setattr, obj, "interface", "foo")
        self.assertRaises(TypeError, setattr, obj, "interface", object())

    def test_range(self):
        # kiwi code
        def max(c):
            return 2 ** ((8 * struct.calcsize(c)) - 1) - 1

        def umax(c):
            return 2 ** (8 * struct.calcsize(c)) - 1

        maxint = max("i")
        minint = -maxint - 1
        maxuint = umax("I")
        maxlong = max("l")
        minlong = -maxlong - 1
        maxulong = umax("L")
        maxint64 = max("q")
        minint64 = -maxint64 - 1
        maxuint64 = umax("Q")

        types_ = {
            "int": (TYPE_INT, minint, maxint),
            "uint": (TYPE_UINT, 0, maxuint),
            "long": (TYPE_LONG, minlong, maxlong),
            "ulong": (TYPE_ULONG, 0, maxulong),
            "int64": (TYPE_INT64, minint64, maxint64),
            "uint64": (TYPE_UINT64, 0, maxuint64),
        }

        def build_gproperties(types_):
            d = {}
            for key, (gtype, min, max) in types_.items():
                d[key] = (
                    gtype,
                    "blurb",
                    "desc",
                    min,
                    max,
                    0,
                    ParamFlags.READABLE | ParamFlags.WRITABLE,
                )
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
            self.assertEqual(
                obj.get_property(key), getattr(RangeCheck.props, key).default_value
            )

            obj.set_property(key, min)
            self.assertEqual(obj.get_property(key), min)

            obj.set_property(key, max)
            self.assertEqual(obj.get_property(key), max)

    def test_multi(self):
        obj = PropertyObject()
        obj.set_properties(normal="foo", uint64=7)
        normal, uint64 = obj.get_properties("normal", "uint64")
        self.assertEqual(normal, "foo")
        self.assertEqual(uint64, 7)


class TestProperty(unittest.TestCase):
    def test_simple(self):
        class C(GObject.GObject):
            str = GObject.Property(type=str)
            float = GObject.Property(type=float)
            long = GObject.Property(type=int)
            int = GObject.Property(type=int)

        self.assertTrue(hasattr(C.props, "str"))
        self.assertTrue(hasattr(C.props, "int"))
        self.assertTrue(hasattr(C.props, "float"))
        self.assertTrue(hasattr(C.props, "long"))

        o = C()
        self.assertEqual(o.str, "")
        o.str = "str"
        self.assertEqual(o.str, "str")

        self.assertEqual(o.int, 0)
        o.int = 1138
        self.assertEqual(o.int, 1138)

        self.assertEqual(o.float, 0.0)
        o.float = 3.14
        self.assertEqual(o.float, 3.14)

        self.assertEqual(o.long, 0)
        o.long = 100
        self.assertEqual(o.long, 100)

    def test_custom_getter(self):
        class C(GObject.GObject):
            def get_prop(self):
                return "value"

            prop = GObject.Property(getter=get_prop)

        o = C()
        self.assertEqual(o.prop, "value")
        self.assertRaises(TypeError, setattr, o, "prop", "xxx")

    def test_getter_exception(self):
        class C(GObject.Object):
            @GObject.Property(type=int)
            def prop(self):
                raise ValueError("something bad happend")

        o = C()

        with self.assertRaisesRegex(ValueError, "something bad happend"):
            o.prop

        with self.assertRaisesRegex(ValueError, "something bad happend"):
            o.get_property("prop")

        with self.assertRaisesRegex(ValueError, "something bad happend"):
            o.props.prop

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
        o.prop = "bar"
        self.assertEqual(o._value, "bar")
        self.assertRaises(TypeError, getattr, o, "prop")

    def test_decorator_default(self):
        class C(GObject.GObject):
            _value = "value"

            @GObject.Property
            def value(self):
                return self._value

            @value.setter
            def value_setter(self, value):
                self._value = value

        o = C()
        self.assertEqual(o.value, "value")
        o.value = "blah"
        self.assertEqual(o.value, "blah")
        self.assertEqual(o.props.value, "blah")

    def test_decorator_private_setter(self):
        class C(GObject.GObject):
            _value = "value"

            @GObject.Property
            def value(self):
                return self._value

            @value.setter
            def _set_value(self, value):
                self._value = value

        o = C()
        self.assertEqual(o.value, "value")
        o.value = "blah"
        self.assertEqual(o.value, "blah")
        self.assertEqual(o.props.value, "blah")

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
        self.assertRaises(TypeError, GObject.Property, type="str")
        self.assertRaises(TypeError, GObject.Property, nick=False)
        self.assertRaises(TypeError, GObject.Property, blurb=False)
        # this never fail while bool is a subclass of int
        # >>> bool.__bases__
        # (<type 'int'>,)
        # self.assertRaises(TypeError, GObject.Property, type=bool, default=0)
        self.assertRaises(TypeError, GObject.Property, type=bool, default="ciao mamma")
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
            (TYPE_INT, MININT, MAXINT),
            (TYPE_UINT, 0, MAXUINT),
            (TYPE_LONG, MINLONG, MAXLONG),
            (TYPE_ULONG, 0, MAXULONG),
            (TYPE_INT64, MININT64, MAXINT64),
            (TYPE_UINT64, 0, MAXUINT64),
        ]

        for gtype, min, max in types_:
            # Normal, everything is alright
            prop = GObject.Property(type=gtype, minimum=min, maximum=max)
            subtype = type("", (GObject.GObject,), {"prop": prop})
            self.assertEqual(subtype.props.prop.minimum, min)
            self.assertEqual(subtype.props.prop.maximum, max)

            # Lower than minimum
            self.assertRaises(
                TypeError, GObject.Property, type=gtype, minimum=min - 1, maximum=max
            )

            # Higher than maximum
            self.assertRaises(
                TypeError, GObject.Property, type=gtype, minimum=min, maximum=max + 1
            )

    def test_min_max(self):
        class C(GObject.GObject):
            prop_int = GObject.Property(type=int, minimum=1, maximum=100, default=1)
            prop_float = GObject.Property(
                type=float, minimum=0.1, maximum=10.5, default=1.1
            )

            def __init__(self):
                GObject.GObject.__init__(self)

        # we test known-bad values here which cause Gtk-WARNING logs.
        # Explicitly allow these for this test.
        with capture_glib_warnings(allow_warnings=True, allow_criticals=True):
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

    def test_multiple_instances(self):
        class C(GObject.GObject):
            prop = GObject.Property(type=str, default="default")

        o1 = C()
        o2 = C()
        self.assertEqual(o1.prop, "default")
        self.assertEqual(o2.prop, "default")
        o1.prop = "value"
        self.assertEqual(o1.prop, "value")
        self.assertEqual(o2.prop, "default")

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
            __gtype_name__ = "ObjectSubclass"

        class PropertyObjectSubclass(GObject.GObject):
            obj = GObject.Property(type=ObjectSubclass)

        PropertyObjectSubclass(obj=ObjectSubclass())

    def test_generic_instance_property(self):
        GObject.Property(type=Gio.ListStore[Gio.File])

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
                return "first"

            first = GObject.Property(type=str, getter=get_first)

        class B(A):
            def get_second(self):
                return "second"

            second = GObject.Property(type=str, getter=get_second)

        a = A()
        self.assertEqual(a.first, "first")
        self.assertRaises(TypeError, setattr, a, "first", "foo")

        b = B()
        self.assertEqual(b.first, "first")
        self.assertRaises(TypeError, setattr, b, "first", "foo")
        self.assertEqual(b.second, "second")
        self.assertRaises(TypeError, setattr, b, "second", "foo")

    def test_property_subclass_custom_setter_error(self):
        try:

            class A(GObject.GObject):
                def get_first(self):
                    return "first"

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
    @unittest.skipUnless(hasattr(sys, "getrefcount"), "no sys.getrefcount")
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
            foo_blurbed = GObject.Property(type=int, blurb="foo_blurbed doc string")

            @GObject.Property
            def foo_getter(self):
                """foo_getter doc string"""
                return 0

        self.assertEqual(C.foo_blurbed.blurb, "foo_blurbed doc string")
        self.assertEqual(C.foo_blurbed.__doc__, "foo_blurbed doc string")

        self.assertEqual(C.foo_getter.blurb, "foo_getter doc string")
        self.assertEqual(C.foo_getter.__doc__, "foo_getter doc string")

    def test_python_to_glib_type_mapping(self):
        tester = GObject.Property()
        self.assertEqual(tester._type_from_python(int), GObject.TYPE_INT)
        self.assertEqual(tester._type_from_python(bool), GObject.TYPE_BOOLEAN)
        self.assertEqual(tester._type_from_python(float), GObject.TYPE_DOUBLE)
        self.assertEqual(tester._type_from_python(str), GObject.TYPE_STRING)
        self.assertEqual(tester._type_from_python(object), GObject.TYPE_PYOBJECT)

        self.assertEqual(
            tester._type_from_python(GObject.GObject), GObject.GObject.__gtype__
        )
        self.assertEqual(
            tester._type_from_python(GObject.GEnum), GObject.GEnum.__gtype__
        )
        self.assertEqual(
            tester._type_from_python(GObject.GFlags), GObject.GFlags.__gtype__
        )
        self.assertEqual(
            tester._type_from_python(GObject.GBoxed), GObject.GBoxed.__gtype__
        )
        self.assertEqual(
            tester._type_from_python(GObject.GInterface), GObject.GInterface.__gtype__
        )

        for type_ in [
            TYPE_NONE,
            TYPE_INTERFACE,
            TYPE_CHAR,
            TYPE_UCHAR,
            TYPE_INT,
            TYPE_UINT,
            TYPE_BOOLEAN,
            TYPE_LONG,
            TYPE_ULONG,
            TYPE_INT64,
            TYPE_UINT64,
            TYPE_FLOAT,
            TYPE_DOUBLE,
            TYPE_POINTER,
            TYPE_BOXED,
            TYPE_PARAM,
            TYPE_OBJECT,
            TYPE_STRING,
            TYPE_PYOBJECT,
            TYPE_GTYPE,
            TYPE_STRV,
        ]:
            self.assertEqual(tester._type_from_python(type_), type_)

        self.assertRaises(TypeError, tester._type_from_python, types.CodeType)


class TestInstallProperties(unittest.TestCase):
    # These tests only test how signalhelper.install_signals works
    # with the __gsignals__ dict and therefore does not need to use
    # GObject as a base class because that would automatically call
    # install_signals within the meta-class.
    class Base:
        __gproperties__ = {"test": (0, "", "", 0, 0, 0, 0)}

    class Sub1(Base):
        pass

    class Sub2(Base):
        @GObject.Property(type=int)
        def sub2test(self):
            return 123

    class ClassWithPropertyAndGetterVFunc:
        @GObject.Property(type=int)
        def sub2test(self):
            return 123

        def do_get_property(self, name):
            return 321

    class ClassWithPropertyRedefined:
        __gproperties__ = {"test": (0, "", "", 0, 0, 0, 0)}
        test = GObject.Property(type=int)

    def setUp(self):
        self.assertEqual(len(self.Base.__gproperties__), 1)
        propertyhelper.install_properties(self.Base)
        self.assertEqual(len(self.Base.__gproperties__), 1)

    def test_subclass_without_properties_is_not_modified(self):
        self.assertFalse("__gproperties__" in self.Sub1.__dict__)
        propertyhelper.install_properties(self.Sub1)
        self.assertFalse("__gproperties__" in self.Sub1.__dict__)

    def test_subclass_with_decorator_gets_gproperties_dict(self):
        # Sub2 has Property instances but will not have a __gproperties__
        # until install_properties is called
        self.assertFalse("__gproperties__" in self.Sub2.__dict__)
        self.assertFalse("do_get_property" in self.Sub2.__dict__)
        self.assertFalse("do_set_property" in self.Sub2.__dict__)

        propertyhelper.install_properties(self.Sub2)
        self.assertTrue("__gproperties__" in self.Sub2.__dict__)
        self.assertEqual(len(self.Base.__gproperties__), 1)
        self.assertEqual(len(self.Sub2.__gproperties__), 1)
        self.assertTrue("sub2test" in self.Sub2.__gproperties__)

        # get/set vfuncs should have been added
        self.assertTrue("do_get_property" in self.Sub2.__dict__)
        self.assertTrue("do_set_property" in self.Sub2.__dict__)

    def test_object_with_property_and_do_get_property_vfunc_raises(self):
        self.assertRaises(
            TypeError,
            propertyhelper.install_properties,
            self.ClassWithPropertyAndGetterVFunc,
        )

    def test_same_name_property_definitions_raises(self):
        self.assertRaises(
            ValueError,
            propertyhelper.install_properties,
            self.ClassWithPropertyRedefined,
        )


class CPropertiesTestBase:
    # Tests for properties implemented in C not Python.

    def setUp(self):
        self.obj = GIMarshallingTests.PropertiesObject()

    def get_prop(self, obj, name):
        raise NotImplementedError

    def set_prop(self, obj, name, value):
        raise NotImplementedError

    def test_flags(self):
        self.assertEqual(
            self.get_prop(self.obj, "some-flags"), GIMarshallingTests.Flags.VALUE1
        )
        self.set_prop(self.obj, "some-flags", GIMarshallingTests.Flags.VALUE2)
        self.assertEqual(
            self.get_prop(self.obj, "some-flags"), GIMarshallingTests.Flags.VALUE2
        )

        obj = GIMarshallingTests.PropertiesObject(
            some_flags=GIMarshallingTests.Flags.VALUE3
        )
        self.assertEqual(
            self.get_prop(obj, "some-flags"), GIMarshallingTests.Flags.VALUE3
        )

    def test_flags_values(self):
        prop = self.obj.find_property("some-flags")
        flags_class = prop.flags_class

        assert GIMarshallingTests.Flags.VALUE1 in flags_class
        assert GIMarshallingTests.Flags.VALUE2 in flags_class
        assert GIMarshallingTests.Flags.VALUE3 in flags_class

    def test_enum(self):
        self.assertEqual(
            self.get_prop(self.obj, "some-enum"), GIMarshallingTests.GEnum.VALUE1
        )
        self.set_prop(self.obj, "some-enum", GIMarshallingTests.GEnum.VALUE2)
        self.assertEqual(
            self.get_prop(self.obj, "some-enum"), GIMarshallingTests.GEnum.VALUE2
        )

        obj = GIMarshallingTests.PropertiesObject(
            some_enum=GIMarshallingTests.GEnum.VALUE3
        )
        self.assertEqual(
            self.get_prop(obj, "some-enum"), GIMarshallingTests.GEnum.VALUE3
        )

    def test_enum_values(self):
        prop = self.obj.find_property("some-enum")
        enum_class = prop.enum_class

        assert GIMarshallingTests.GEnum.VALUE1 in enum_class
        assert GIMarshallingTests.GEnum.VALUE2 in enum_class
        assert GIMarshallingTests.GEnum.VALUE3 in enum_class

    def test_boolean(self):
        self.assertEqual(self.get_prop(self.obj, "some-boolean"), False)
        self.set_prop(self.obj, "some-boolean", True)
        self.assertEqual(self.get_prop(self.obj, "some-boolean"), True)

        obj = GIMarshallingTests.PropertiesObject(some_boolean=True)
        self.assertEqual(self.get_prop(obj, "some-boolean"), True)

    def test_char(self):
        self.assertEqual(self.get_prop(self.obj, "some-char"), 0)
        self.set_prop(self.obj, "some-char", GLib.MAXINT8)
        self.assertEqual(self.get_prop(self.obj, "some-char"), GLib.MAXINT8)

        obj = GIMarshallingTests.PropertiesObject(some_char=-42)
        self.assertEqual(self.get_prop(obj, "some-char"), -42)

        with pytest.raises(OverflowError):
            self.set_prop(obj, "some-char", GLib.MAXINT8 + 1)
        with pytest.raises(OverflowError):
            self.set_prop(obj, "some-char", GLib.MININT8 - 1)

        self.set_prop(obj, "some-char", b"\x44")
        assert self.get_prop(obj, "some-char") == 0x44

        self.set_prop(obj, "some-char", b"\xff")
        assert self.get_prop(obj, "some-char") == -1

        obj = GIMarshallingTests.PropertiesObject(some_char="\x7f")
        assert self.get_prop(obj, "some-char") == 0x7F

        with pytest.raises(TypeError):
            GIMarshallingTests.PropertiesObject(some_char="€")

        with pytest.raises(TypeError):
            GIMarshallingTests.PropertiesObject(some_char="\ud83d")

    def test_uchar(self):
        self.assertEqual(self.get_prop(self.obj, "some-uchar"), 0)
        self.set_prop(self.obj, "some-uchar", GLib.MAXUINT8)
        self.assertEqual(self.get_prop(self.obj, "some-uchar"), GLib.MAXUINT8)

        obj = GIMarshallingTests.PropertiesObject(some_uchar=42)
        self.assertEqual(self.get_prop(obj, "some-uchar"), 42)

        with pytest.raises(OverflowError):
            self.set_prop(obj, "some-uchar", GLib.MAXUINT8 + 1)
        with pytest.raises(OverflowError):
            self.set_prop(obj, "some-uchar", -1)

        self.set_prop(obj, "some-uchar", b"\x57")
        assert self.get_prop(obj, "some-uchar") == 0x57

        self.set_prop(obj, "some-uchar", b"\xff")
        assert self.get_prop(obj, "some-uchar") == 255

        obj = GIMarshallingTests.PropertiesObject(some_uchar="\x7f")
        assert self.get_prop(obj, "some-uchar") == 127

        with pytest.raises(TypeError):
            GIMarshallingTests.PropertiesObject(some_uchar="\x80")

        with pytest.raises(TypeError):
            GIMarshallingTests.PropertiesObject(some_uchar="\ud83d")

    def test_int(self):
        self.assertEqual(self.get_prop(self.obj, "some_int"), 0)
        self.set_prop(self.obj, "some-int", GLib.MAXINT)
        self.assertEqual(self.get_prop(self.obj, "some_int"), GLib.MAXINT)

        obj = GIMarshallingTests.PropertiesObject(some_int=-42)
        self.assertEqual(self.get_prop(obj, "some-int"), -42)

        self.assertRaises(TypeError, self.set_prop, self.obj, "some-int", "foo")
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-int", None)

        self.assertEqual(self.get_prop(obj, "some-int"), -42)

    def test_uint(self):
        self.assertEqual(self.get_prop(self.obj, "some_uint"), 0)
        self.set_prop(self.obj, "some-uint", GLib.MAXUINT)
        self.assertEqual(self.get_prop(self.obj, "some_uint"), GLib.MAXUINT)

        obj = GIMarshallingTests.PropertiesObject(some_uint=42)
        self.assertEqual(self.get_prop(obj, "some-uint"), 42)

        self.assertRaises(TypeError, self.set_prop, self.obj, "some-uint", "foo")
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-uint", None)

        self.assertEqual(self.get_prop(obj, "some-uint"), 42)

    def test_long(self):
        self.assertEqual(self.get_prop(self.obj, "some_long"), 0)
        self.set_prop(self.obj, "some-long", GLib.MAXLONG)
        self.assertEqual(self.get_prop(self.obj, "some_long"), GLib.MAXLONG)

        obj = GIMarshallingTests.PropertiesObject(some_long=-42)
        self.assertEqual(self.get_prop(obj, "some-long"), -42)

        self.assertRaises(TypeError, self.set_prop, self.obj, "some-long", "foo")
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-long", None)

        self.assertEqual(self.get_prop(obj, "some-long"), -42)

    def test_ulong(self):
        self.assertEqual(self.get_prop(self.obj, "some_ulong"), 0)
        self.set_prop(self.obj, "some-ulong", GLib.MAXULONG)
        self.assertEqual(self.get_prop(self.obj, "some_ulong"), GLib.MAXULONG)

        obj = GIMarshallingTests.PropertiesObject(some_ulong=42)
        self.assertEqual(self.get_prop(obj, "some-ulong"), 42)

        self.assertRaises(TypeError, self.set_prop, self.obj, "some-ulong", "foo")
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-ulong", None)

        self.assertEqual(self.get_prop(obj, "some-ulong"), 42)

    def test_int64(self):
        self.assertEqual(self.get_prop(self.obj, "some-int64"), 0)
        self.set_prop(self.obj, "some-int64", GLib.MAXINT64)
        self.assertEqual(self.get_prop(self.obj, "some-int64"), GLib.MAXINT64)

        obj = GIMarshallingTests.PropertiesObject(some_int64=-4200000000000000)
        self.assertEqual(self.get_prop(obj, "some-int64"), -4200000000000000)

    def test_uint64(self):
        self.assertEqual(self.get_prop(self.obj, "some-uint64"), 0)
        self.set_prop(self.obj, "some-uint64", GLib.MAXUINT64)
        self.assertEqual(self.get_prop(self.obj, "some-uint64"), GLib.MAXUINT64)

        obj = GIMarshallingTests.PropertiesObject(some_uint64=4200000000000000)
        self.assertEqual(self.get_prop(obj, "some-uint64"), 4200000000000000)

    def test_float(self):
        self.assertEqual(self.get_prop(self.obj, "some-float"), 0)
        self.set_prop(self.obj, "some-float", GLib.MAXFLOAT)
        self.assertEqual(self.get_prop(self.obj, "some-float"), GLib.MAXFLOAT)

        obj = GIMarshallingTests.PropertiesObject(some_float=42.42)
        self.assertAlmostEqual(self.get_prop(obj, "some-float"), 42.42, places=4)

        obj = GIMarshallingTests.PropertiesObject(some_float=42)
        self.assertAlmostEqual(self.get_prop(obj, "some-float"), 42.0, places=4)

        self.assertRaises(TypeError, self.set_prop, self.obj, "some-float", "foo")
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-float", None)

        self.assertAlmostEqual(self.get_prop(obj, "some-float"), 42.0, places=4)

    def test_double(self):
        self.assertEqual(self.get_prop(self.obj, "some-double"), 0)
        self.set_prop(self.obj, "some-double", GLib.MAXDOUBLE)
        self.assertEqual(self.get_prop(self.obj, "some-double"), GLib.MAXDOUBLE)

        obj = GIMarshallingTests.PropertiesObject(some_double=42.42)
        self.assertAlmostEqual(self.get_prop(obj, "some-double"), 42.42)

        obj = GIMarshallingTests.PropertiesObject(some_double=42)
        self.assertAlmostEqual(self.get_prop(obj, "some-double"), 42.0)

        self.assertRaises(TypeError, self.set_prop, self.obj, "some-double", "foo")
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-double", None)

        self.assertAlmostEqual(self.get_prop(obj, "some-double"), 42.0)

    def test_strv(self):
        self.assertEqual(self.get_prop(self.obj, "some-strv"), [])
        self.set_prop(self.obj, "some-strv", ["hello", "world"])
        self.assertEqual(self.get_prop(self.obj, "some-strv"), ["hello", "world"])

        self.assertRaises(TypeError, self.set_prop, self.obj, "some-strv", 1)
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-strv", "foo")
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-strv", [1, 2])
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-strv", ["foo", 1])

        self.assertEqual(self.get_prop(self.obj, "some-strv"), ["hello", "world"])

        obj = GIMarshallingTests.PropertiesObject(some_strv=["hello", "world"])
        self.assertEqual(self.get_prop(obj, "some-strv"), ["hello", "world"])

        # unicode on py2
        obj = GIMarshallingTests.PropertiesObject(some_strv=["foo"])
        self.assertEqual(self.get_prop(obj, "some-strv"), ["foo"])
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-strv", ["foo", 1])

    def test_boxed_struct(self):
        self.assertEqual(self.get_prop(self.obj, "some-boxed-struct"), None)

        class GStrv(list):
            __gtype__ = GObject.TYPE_STRV

        struct1 = GIMarshallingTests.BoxedStruct()
        struct1.long_ = 1

        self.set_prop(self.obj, "some-boxed-struct", struct1)
        self.assertEqual(self.get_prop(self.obj, "some-boxed-struct").long_, 1)
        self.assertEqual(self.obj.some_boxed_struct.long_, 1)

        self.assertRaises(TypeError, self.set_prop, self.obj, "some-boxed-struct", 1)
        self.assertRaises(
            TypeError, self.set_prop, self.obj, "some-boxed-struct", "foo"
        )

        obj = GIMarshallingTests.PropertiesObject(some_boxed_struct=struct1)
        self.assertEqual(self.get_prop(obj, "some-boxed-struct").long_, 1)

    def test_boxed_glist(self):
        self.assertEqual(self.get_prop(self.obj, "some-boxed-glist"), [])

        list_ = [GLib.MININT, 42, GLib.MAXINT]
        self.set_prop(self.obj, "some-boxed-glist", list_)
        self.assertEqual(self.get_prop(self.obj, "some-boxed-glist"), list_)
        self.set_prop(self.obj, "some-boxed-glist", [])
        self.assertEqual(self.get_prop(self.obj, "some-boxed-glist"), [])

        self.assertRaises(TypeError, self.set_prop, self.obj, "some-boxed-glist", 1)
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-boxed-glist", "foo")
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-boxed-glist", ["a"])

    def test_annotated_glist(self):
        obj = Regress.TestObj()
        self.assertEqual(self.get_prop(obj, "list"), [])

        self.set_prop(obj, "list", ["1", "2", "3"])
        self.assertTrue(isinstance(self.get_prop(obj, "list"), list))
        self.assertEqual(self.get_prop(obj, "list"), ["1", "2", "3"])

    @unittest.expectedFailure
    def test_boxed_glist_ctor(self):
        list_ = [GLib.MININT, 42, GLib.MAXINT]
        obj = GIMarshallingTests.PropertiesObject(some_boxed_glist=list_)
        self.assertEqual(self.get_prop(obj, "some-boxed-glist"), list_)

    def test_variant(self):
        self.assertEqual(self.get_prop(self.obj, "some-variant"), None)

        self.set_prop(self.obj, "some-variant", GLib.Variant("o", "/myobj"))
        self.assertEqual(self.get_prop(self.obj, "some-variant").get_type_string(), "o")
        self.assertEqual(
            self.get_prop(self.obj, "some-variant").print_(False), "'/myobj'"
        )

        self.set_prop(self.obj, "some-variant", None)
        self.assertEqual(self.get_prop(self.obj, "some-variant"), None)

        obj = GIMarshallingTests.PropertiesObject(some_variant=GLib.Variant("b", True))
        self.assertEqual(self.get_prop(obj, "some-variant").get_type_string(), "b")
        self.assertEqual(self.get_prop(obj, "some-variant").get_boolean(), True)

        self.assertRaises(TypeError, self.set_prop, self.obj, "some-variant", "foo")
        self.assertRaises(TypeError, self.set_prop, self.obj, "some-variant", 23)

        self.assertEqual(self.get_prop(obj, "some-variant").get_type_string(), "b")
        self.assertEqual(self.get_prop(obj, "some-variant").get_boolean(), True)

    def test_setting_several_properties(self):
        obj = GIMarshallingTests.PropertiesObject()
        obj.set_properties(some_uchar=54, some_int=42)
        self.assertEqual(42, self.get_prop(obj, "some-int"))
        self.assertEqual(54, self.get_prop(obj, "some-uchar"))

    def test_gtype(self):
        obj = Regress.TestObj()
        self.assertEqual(self.get_prop(obj, "gtype"), GObject.TYPE_INVALID)
        self.set_prop(obj, "gtype", int)
        self.assertEqual(self.get_prop(obj, "gtype"), GObject.TYPE_INT)

        obj = Regress.TestObj(gtype=int)
        self.assertEqual(self.get_prop(obj, "gtype"), GObject.TYPE_INT)
        self.set_prop(obj, "gtype", str)
        self.assertEqual(self.get_prop(obj, "gtype"), GObject.TYPE_STRING)

    def test_hash_table(self):
        obj = Regress.TestObj()
        self.assertEqual(self.get_prop(obj, "hash-table"), None)

        self.set_prop(obj, "hash-table", {"mec": 56})
        self.assertTrue(isinstance(self.get_prop(obj, "hash-table"), dict))
        self.assertEqual(
            next(iter(self.get_prop(obj, "hash-table").items())), ("mec", 56)
        )

    def test_parent_class(self):
        class A(Regress.TestObj):
            prop1 = GObject.Property(type=int)

        a = A()
        self.set_prop(a, "int", 20)
        self.assertEqual(self.get_prop(a, "int"), 20)

        # test parent property which needs introspection
        self.set_prop(a, "list", ("str1", "str2"))
        self.assertEqual(self.get_prop(a, "list"), ["str1", "str2"])

    def test_held_object_ref_count_getter(self):
        holder = GIMarshallingTests.PropertiesObject()
        held = GObject.Object()

        self.assertEqual(holder.__grefcount__, 1)
        self.assertEqual(held.__grefcount__, 1)

        self.set_prop(holder, "some-object", held)
        self.assertEqual(holder.__grefcount__, 1)

        initial_ref_count = held.__grefcount__
        self.get_prop(holder, "some-object")
        gc.collect()
        self.assertEqual(held.__grefcount__, initial_ref_count)

    def test_held_object_ref_count_setter(self):
        holder = GIMarshallingTests.PropertiesObject()
        held = GObject.Object()

        self.assertEqual(holder.__grefcount__, 1)
        self.assertEqual(held.__grefcount__, 1)

        # Setting property should only increase ref count by 1
        self.set_prop(holder, "some-object", held)
        self.assertEqual(holder.__grefcount__, 1)
        self.assertEqual(held.__grefcount__, 2)

        # Clearing should pull it back down
        self.set_prop(holder, "some-object", None)
        self.assertEqual(held.__grefcount__, 1)

    def test_set_object_property_to_invalid_type(self):
        obj = GIMarshallingTests.PropertiesObject()
        self.assertRaises(TypeError, self.set_prop, obj, "some-object", "not_an_object")


class TestCPropsAccessor(CPropertiesTestBase, unittest.TestCase):
    # C property tests using the "props" accessor.
    def get_prop(self, obj, name):
        return getattr(obj.props, name.replace("-", "_"))

    def set_prop(self, obj, name, value):
        setattr(obj.props, name.replace("-", "_"), value)

    def test_props_accessor_dir(self):
        # Test class
        props = dir(GIMarshallingTests.PropertiesObject.props)
        self.assertTrue("some_float" in props)
        self.assertTrue("some_double" in props)
        self.assertTrue("some_variant" in props)

        # Test instance
        obj = GIMarshallingTests.PropertiesObject()
        props = dir(obj.props)
        self.assertTrue("some_float" in props)
        self.assertTrue("some_double" in props)
        self.assertTrue("some_variant" in props)

    def test_param_spec_dir(self):
        attrs = dir(GIMarshallingTests.PropertiesObject.props.some_float)
        self.assertTrue("name" in attrs)
        self.assertTrue("nick" in attrs)
        self.assertTrue("blurb" in attrs)
        self.assertTrue("flags" in attrs)
        self.assertTrue("default_value" in attrs)
        self.assertTrue("minimum" in attrs)
        self.assertTrue("maximum" in attrs)


class TestCGetPropertyMethod(CPropertiesTestBase, unittest.TestCase):
    # C property tests using the "props" accessor.
    def get_prop(self, obj, name):
        return obj.get_property(name)

    def set_prop(self, obj, name, value):
        obj.set_property(name, value)


def test_gobject_inheritance_with_incomplete_initialization():
    class Test:
        def __init__(self):
            # super().__init__() should have been called here
            pass

    class Bomb(Test, GObject.Object):
        def __init__(self):
            super().__init__()

    bomb = Bomb()

    with pytest.raises(RuntimeError, match="is not initialized"):
        bomb.qdata


@pytest.mark.skipif(
    not hasattr(Regress, "AnnotationObject"), reason="no Regress.AnnotationObject"
)
def test_get_function_property():
    obj = Regress.AnnotationObject()

    with pytest.raises(TypeError):
        assert obj.props.function_property


@pytest.mark.skipif(
    not hasattr(Regress, "AnnotationObject"), reason="no Regress.AnnotationObject"
)
def test_set_function_property():
    obj = Regress.AnnotationObject()

    with pytest.raises(TypeError):
        obj.props.function_property = lambda *x: x

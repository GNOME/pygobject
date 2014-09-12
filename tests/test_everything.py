# -*- Mode: Python; py-indent-offset: 4 -*-
# coding=utf-8
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest
import traceback
import ctypes
import warnings
import sys

try:
    import cairo
    cairo  # Pyflakes
    has_cairo = True
    from gi.repository import Regress as Everything
except ImportError:
    has_cairo = False

from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio

try:
    from gi.repository import Gtk
    Gtk  # pyflakes
except:
    Gtk = None

if sys.version_info < (3, 0):
    UNICHAR = "\xe2\x99\xa5"
    PY2_UNICODE_UNICHAR = unicode(UNICHAR, 'UTF-8')
else:
    UNICHAR = "♥"


const_str = b'const \xe2\x99\xa5 utf8'
if sys.version_info >= (3, 0):
    const_str = const_str.decode('UTF-8')
noconst_str = 'non' + const_str


class RawGList(ctypes.Structure):
    _fields_ = [('data', ctypes.c_void_p),
                ('next', ctypes.c_void_p),
                ('prev', ctypes.c_void_p)]

    @classmethod
    def from_wrapped(cls, obj):
        offset = sys.getsizeof(object())  # size of PyObject_HEAD
        return ctypes.POINTER(cls).from_address(id(obj) + offset)


@unittest.skipUnless(has_cairo, 'built without cairo support')
class TestEverything(unittest.TestCase):

    def test_bool(self):
        self.assertEqual(Everything.test_boolean(False), False)
        self.assertEqual(Everything.test_boolean(True), True)
        self.assertEqual(Everything.test_boolean('hello'), True)
        self.assertEqual(Everything.test_boolean(''), False)

        self.assertEqual(Everything.test_boolean_true(True), True)
        self.assertEqual(Everything.test_boolean_false(False), False)

    def test_int8(self):
        self.assertEqual(Everything.test_int8(GObject.G_MAXINT8),
                         GObject.G_MAXINT8)
        self.assertEqual(Everything.test_int8(GObject.G_MININT8),
                         GObject.G_MININT8)
        self.assertRaises(OverflowError, Everything.test_int8, GObject.G_MAXINT8 + 1)

        self.assertEqual(Everything.test_uint8(GObject.G_MAXUINT8),
                         GObject.G_MAXUINT8)
        self.assertEqual(Everything.test_uint8(0), 0)
        self.assertRaises(OverflowError, Everything.test_uint8, -1)
        self.assertRaises(OverflowError, Everything.test_uint8, GObject.G_MAXUINT8 + 1)

    def test_int16(self):
        self.assertEqual(Everything.test_int16(GObject.G_MAXINT16),
                         GObject.G_MAXINT16)
        self.assertEqual(Everything.test_int16(GObject.G_MININT16),
                         GObject.G_MININT16)
        self.assertRaises(OverflowError, Everything.test_int16, GObject.G_MAXINT16 + 1)

        self.assertEqual(Everything.test_uint16(GObject.G_MAXUINT16),
                         GObject.G_MAXUINT16)
        self.assertEqual(Everything.test_uint16(0), 0)
        self.assertRaises(OverflowError, Everything.test_uint16, -1)
        self.assertRaises(OverflowError, Everything.test_uint16, GObject.G_MAXUINT16 + 1)

    def test_int32(self):
        self.assertEqual(Everything.test_int32(GObject.G_MAXINT32),
                         GObject.G_MAXINT32)
        self.assertEqual(Everything.test_int32(GObject.G_MININT32),
                         GObject.G_MININT32)
        self.assertRaises(OverflowError, Everything.test_int32, GObject.G_MAXINT32 + 1)

        self.assertEqual(Everything.test_uint32(GObject.G_MAXUINT32),
                         GObject.G_MAXUINT32)
        self.assertEqual(Everything.test_uint32(0), 0)
        self.assertRaises(OverflowError, Everything.test_uint32, -1)
        self.assertRaises(OverflowError, Everything.test_uint32, GObject.G_MAXUINT32 + 1)

    def test_int64(self):
        self.assertEqual(Everything.test_int64(GObject.G_MAXINT64),
                         GObject.G_MAXINT64)
        self.assertEqual(Everything.test_int64(GObject.G_MININT64),
                         GObject.G_MININT64)
        self.assertRaises(OverflowError, Everything.test_int64, GObject.G_MAXINT64 + 1)

        self.assertEqual(Everything.test_uint64(GObject.G_MAXUINT64),
                         GObject.G_MAXUINT64)
        self.assertEqual(Everything.test_uint64(0), 0)
        self.assertRaises(OverflowError, Everything.test_uint64, -1)
        self.assertRaises(OverflowError, Everything.test_uint64, GObject.G_MAXUINT64 + 1)

    def test_int(self):
        self.assertEqual(Everything.test_int(GObject.G_MAXINT),
                         GObject.G_MAXINT)
        self.assertEqual(Everything.test_int(GObject.G_MININT),
                         GObject.G_MININT)
        self.assertRaises(OverflowError, Everything.test_int, GObject.G_MAXINT + 1)

        self.assertEqual(Everything.test_uint(GObject.G_MAXUINT),
                         GObject.G_MAXUINT)
        self.assertEqual(Everything.test_uint(0), 0)
        self.assertRaises(OverflowError, Everything.test_uint, -1)
        self.assertRaises(OverflowError, Everything.test_uint, GObject.G_MAXUINT + 1)

    def test_short(self):
        self.assertEqual(Everything.test_short(GObject.G_MAXSHORT),
                         GObject.G_MAXSHORT)
        self.assertEqual(Everything.test_short(GObject.G_MINSHORT),
                         GObject.G_MINSHORT)
        self.assertRaises(OverflowError, Everything.test_short, GObject.G_MAXSHORT + 1)

        self.assertEqual(Everything.test_ushort(GObject.G_MAXUSHORT),
                         GObject.G_MAXUSHORT)
        self.assertEqual(Everything.test_ushort(0), 0)
        self.assertRaises(OverflowError, Everything.test_ushort, -1)
        self.assertRaises(OverflowError, Everything.test_ushort, GObject.G_MAXUSHORT + 1)

    def test_long(self):
        self.assertEqual(Everything.test_long(GObject.G_MAXLONG),
                         GObject.G_MAXLONG)
        self.assertEqual(Everything.test_long(GObject.G_MINLONG),
                         GObject.G_MINLONG)
        self.assertRaises(OverflowError, Everything.test_long, GObject.G_MAXLONG + 1)

        self.assertEqual(Everything.test_ulong(GObject.G_MAXULONG),
                         GObject.G_MAXULONG)
        self.assertEqual(Everything.test_ulong(0), 0)
        self.assertRaises(OverflowError, Everything.test_ulong, -1)
        self.assertRaises(OverflowError, Everything.test_ulong, GObject.G_MAXULONG + 1)

    def test_size(self):
        self.assertEqual(Everything.test_ssize(GObject.G_MAXSSIZE),
                         GObject.G_MAXSSIZE)
        self.assertEqual(Everything.test_ssize(GObject.G_MINSSIZE),
                         GObject.G_MINSSIZE)
        self.assertRaises(OverflowError, Everything.test_ssize, GObject.G_MAXSSIZE + 1)

        self.assertEqual(Everything.test_size(GObject.G_MAXSIZE),
                         GObject.G_MAXSIZE)
        self.assertEqual(Everything.test_size(0), 0)
        self.assertRaises(OverflowError, Everything.test_size, -1)
        self.assertRaises(OverflowError, Everything.test_size, GObject.G_MAXSIZE + 1)

    def test_timet(self):
        self.assertEqual(Everything.test_timet(42), 42)
        self.assertRaises(OverflowError, Everything.test_timet, GObject.G_MAXUINT64 + 1)

    def test_unichar(self):
        self.assertEqual("c", Everything.test_unichar("c"))

        if sys.version_info < (3, 0):
            self.assertEqual(UNICHAR, Everything.test_unichar(PY2_UNICODE_UNICHAR))
        self.assertEqual(UNICHAR, Everything.test_unichar(UNICHAR))
        self.assertRaises(TypeError, Everything.test_unichar, "")
        self.assertRaises(TypeError, Everything.test_unichar, "morethanonechar")

    def test_float(self):
        self.assertEqual(Everything.test_float(GObject.G_MAXFLOAT),
                         GObject.G_MAXFLOAT)
        self.assertEqual(Everything.test_float(GObject.G_MINFLOAT),
                         GObject.G_MINFLOAT)
        self.assertRaises(OverflowError, Everything.test_float, GObject.G_MAXFLOAT * 2)

    def test_double(self):
        self.assertEqual(Everything.test_double(GObject.G_MAXDOUBLE),
                         GObject.G_MAXDOUBLE)
        self.assertEqual(Everything.test_double(GObject.G_MINDOUBLE),
                         GObject.G_MINDOUBLE)

        (two, three) = Everything.test_multi_double_args(2.5)
        self.assertAlmostEqual(two, 5.0)
        self.assertAlmostEqual(three, 7.5)

    def test_value(self):
        self.assertEqual(Everything.test_int_value_arg(GObject.G_MAXINT), GObject.G_MAXINT)
        self.assertEqual(Everything.test_value_return(GObject.G_MAXINT), GObject.G_MAXINT)

    def test_variant(self):
        v = Everything.test_gvariant_i()
        self.assertEqual(v.get_type_string(), 'i')
        self.assertEqual(v.get_int32(), 1)

        v = Everything.test_gvariant_s()
        self.assertEqual(v.get_type_string(), 's')
        self.assertEqual(v.get_string(), 'one')

        v = Everything.test_gvariant_v()
        self.assertEqual(v.get_type_string(), 'v')
        vi = v.get_variant()
        self.assertEqual(vi.get_type_string(), 's')
        self.assertEqual(vi.get_string(), 'contents')

        v = Everything.test_gvariant_as()
        self.assertEqual(v.get_type_string(), 'as')
        self.assertEqual(v.get_strv(), ['one', 'two', 'three'])

        v = Everything.test_gvariant_asv()
        self.assertEqual(v.get_type_string(), 'a{sv}')
        self.assertEqual(v.lookup_value('nosuchkey', None), None)
        name = v.lookup_value('name', None)
        self.assertEqual(name.get_string(), 'foo')
        timeout = v.lookup_value('timeout', None)
        self.assertEqual(timeout.get_int32(), 10)

    def test_utf8_const_return(self):
        self.assertEqual(Everything.test_utf8_const_return(), const_str)

    def test_utf8_nonconst_return(self):
        self.assertEqual(Everything.test_utf8_nonconst_return(), noconst_str)

    def test_utf8_out(self):
        self.assertEqual(Everything.test_utf8_out(), noconst_str)

    def test_utf8_const_in(self):
        Everything.test_utf8_const_in(const_str)

    def test_utf8_inout(self):
        self.assertEqual(Everything.test_utf8_inout(const_str), noconst_str)

    def test_filename_return(self):
        self.assertEqual(Everything.test_filename_return(), ['åäö', '/etc/fstab'])

    def test_int_out_utf8(self):
        # returns g_utf8_strlen() in out argument
        self.assertEqual(Everything.test_int_out_utf8(''), 0)
        self.assertEqual(Everything.test_int_out_utf8('hello world'), 11)
        self.assertEqual(Everything.test_int_out_utf8('åäö'), 3)

    def test_utf8_out_out(self):
        self.assertEqual(Everything.test_utf8_out_out(), ('first', 'second'))

    def test_utf8_out_nonconst_return(self):
        self.assertEqual(Everything.test_utf8_out_nonconst_return(), ('first', 'second'))

    def test_enum(self):
        self.assertEqual(Everything.test_enum_param(Everything.TestEnum.VALUE1), 'value1')
        self.assertEqual(Everything.test_enum_param(Everything.TestEnum.VALUE3), 'value3')
        self.assertRaises(TypeError, Everything.test_enum_param, 'hello')

    # FIXME: ValueError: invalid enum value: 2147483648
    @unittest.expectedFailure
    def test_enum_unsigned(self):
        self.assertEqual(Everything.test_unsigned_enum_param(Everything.TestEnumUnsigned.VALUE1), 'value1')
        self.assertEqual(Everything.test_unsigned_enum_param(Everything.TestEnumUnsigned.VALUE3), 'value3')
        self.assertRaises(TypeError, Everything.test_unsigned_enum_param, 'hello')

    def test_flags(self):
        result = Everything.global_get_flags_out()
        # assert that it's not an int
        self.assertEqual(type(result), Everything.TestFlags)
        self.assertEqual(result, Everything.TestFlags.FLAG1 | Everything.TestFlags.FLAG3)

    def test_floating(self):
        e = Everything.TestFloating()
        self.assertEqual(e.__grefcount__, 1)

        e = GObject.new(Everything.TestFloating)
        self.assertEqual(e.__grefcount__, 1)

        e = Everything.TestFloating.new()
        self.assertEqual(e.__grefcount__, 1)

    def test_caller_allocates(self):
        struct_a = Everything.TestStructA()
        struct_a.some_int = 10
        struct_a.some_int8 = 21
        struct_a.some_double = 3.14
        struct_a.some_enum = Everything.TestEnum.VALUE3

        struct_a_clone = struct_a.clone()
        self.assertTrue(struct_a != struct_a_clone)
        self.assertEqual(struct_a.some_int, struct_a_clone.some_int)
        self.assertEqual(struct_a.some_int8, struct_a_clone.some_int8)
        self.assertEqual(struct_a.some_double, struct_a_clone.some_double)
        self.assertEqual(struct_a.some_enum, struct_a_clone.some_enum)

        struct_b = Everything.TestStructB()
        struct_b.some_int8 = 8
        struct_b.nested_a.some_int = 20
        struct_b.nested_a.some_int8 = 12
        struct_b.nested_a.some_double = 333.3333
        struct_b.nested_a.some_enum = Everything.TestEnum.VALUE2

        struct_b_clone = struct_b.clone()
        self.assertTrue(struct_b != struct_b_clone)
        self.assertEqual(struct_b.some_int8, struct_b_clone.some_int8)
        self.assertEqual(struct_b.nested_a.some_int, struct_b_clone.nested_a.some_int)
        self.assertEqual(struct_b.nested_a.some_int8, struct_b_clone.nested_a.some_int8)
        self.assertEqual(struct_b.nested_a.some_double, struct_b_clone.nested_a.some_double)
        self.assertEqual(struct_b.nested_a.some_enum, struct_b_clone.nested_a.some_enum)

        struct_a = Everything.test_struct_a_parse('ignored')
        self.assertEqual(struct_a.some_int, 23)

    def test_wrong_type_of_arguments(self):
        try:
            Everything.test_int8()
        except TypeError:
            (e_type, e) = sys.exc_info()[:2]
            self.assertEqual(e.args, ("test_int8() takes exactly 1 argument (0 given)",))

    def test_gtypes(self):
        gchararray_gtype = GObject.type_from_name('gchararray')
        gtype = Everything.test_gtype(str)
        self.assertEqual(gchararray_gtype, gtype)
        gtype = Everything.test_gtype('gchararray')
        self.assertEqual(gchararray_gtype, gtype)
        gobject_gtype = GObject.GObject.__gtype__
        gtype = Everything.test_gtype(GObject.GObject)
        self.assertEqual(gobject_gtype, gtype)
        gtype = Everything.test_gtype('GObject')
        self.assertEqual(gobject_gtype, gtype)
        self.assertRaises(TypeError, Everything.test_gtype, 'invalidgtype')

        class NotARegisteredClass(object):
            pass

        self.assertRaises(TypeError, Everything.test_gtype, NotARegisteredClass)

        class ARegisteredClass(GObject.GObject):
            __gtype_name__ = 'EverythingTestsARegisteredClass'

        gtype = Everything.test_gtype('EverythingTestsARegisteredClass')
        self.assertEqual(ARegisteredClass.__gtype__, gtype)
        gtype = Everything.test_gtype(ARegisteredClass)
        self.assertEqual(ARegisteredClass.__gtype__, gtype)
        self.assertRaises(TypeError, Everything.test_gtype, 'ARegisteredClass')

    def test_dir(self):
        attr_list = dir(Everything)

        # test that typelib attributes are listed
        self.assertTrue('TestStructA' in attr_list)

        # test that class attributes and methods are listed
        self.assertTrue('__class__' in attr_list)
        self.assertTrue('__dir__' in attr_list)
        self.assertTrue('__repr__' in attr_list)

        # test that instance members are listed
        self.assertTrue('_namespace' in attr_list)
        self.assertTrue('_version' in attr_list)

        # test that there are no duplicates returned
        self.assertEqual(len(attr_list), len(set(attr_list)))

    def test_array_int_in_empty(self):
        self.assertEqual(Everything.test_array_int_in([]), 0)

    def test_array_int_in(self):
        self.assertEqual(Everything.test_array_int_in([1, 5, -2]), 4)

    def test_array_int_out(self):
        self.assertEqual(Everything.test_array_int_out(), [0, 1, 2, 3, 4])

    def test_array_int_full_out(self):
        self.assertEqual(Everything.test_array_int_full_out(), [0, 1, 2, 3, 4])

    def test_array_int_none_out(self):
        self.assertEqual(Everything.test_array_int_none_out(), [1, 2, 3, 4, 5])

    def test_array_int_inout(self):
        self.assertEqual(Everything.test_array_int_inout([1, 5, 42, -8]), [6, 43, -7])

    def test_array_int_inout_empty(self):
        self.assertEqual(Everything.test_array_int_inout([]), [])

    def test_array_gint8_in(self):
        if sys.version_info >= (3, 0):
            self.assertEqual(Everything.test_array_gint8_in(b'\x01\x03\x05'), 9)
        self.assertEqual(Everything.test_array_gint8_in([1, 3, 5, -50]), -41)

    def test_array_gint16_in(self):
        self.assertEqual(Everything.test_array_gint16_in([256, 257, -1000, 10000]), 9513)

    def test_array_gint32_in(self):
        self.assertEqual(Everything.test_array_gint32_in([30000, 1, -2]), 29999)

    def test_array_gint64_in(self):
        self.assertEqual(Everything.test_array_gint64_in([2 ** 33, 2 ** 34]), 2 ** 33 + 2 ** 34)

    def test_array_gtype_in(self):
        self.assertEqual(Everything.test_array_gtype_in(
            [GObject.TYPE_STRING, GObject.TYPE_UINT64, GObject.TYPE_VARIANT]),
            '[gchararray,guint64,GVariant,]')

    def test_array_fixed_size_int_in(self):
        # fixed length of 5
        self.assertEqual(Everything.test_array_fixed_size_int_in([1, 2, -10, 5, 3]), 1)

    def test_array_fixed_size_int_in_error(self):
        self.assertRaises(ValueError, Everything.test_array_fixed_size_int_in, [1, 2, 3, 4])
        self.assertRaises(ValueError, Everything.test_array_fixed_size_int_in, [1, 2, 3, 4, 5, 6])

    def test_array_fixed_size_int_out(self):
        self.assertEqual(Everything.test_array_fixed_size_int_out(), [0, 1, 2, 3, 4])

    def test_array_fixed_size_int_return(self):
        self.assertEqual(Everything.test_array_fixed_size_int_return(), [0, 1, 2, 3, 4])

    def test_garray_container_return(self):
        # GPtrArray transfer container
        result = Everything.test_garray_container_return()
        self.assertEqual(result, ['regress'])
        result = None

    def test_garray_full_return(self):
        # GPtrArray transfer full
        result = Everything.test_garray_full_return()
        self.assertEqual(result, ['regress'])
        result = None

    def test_strv_out(self):
        self.assertEqual(Everything.test_strv_out(), ['thanks', 'for', 'all', 'the', 'fish'])

    def test_strv_out_c(self):
        self.assertEqual(Everything.test_strv_out_c(), ['thanks', 'for', 'all', 'the', 'fish'])

    def test_strv_out_container(self):
        self.assertEqual(Everything.test_strv_out_container(), ['1', '2', '3'])

    def test_strv_outarg(self):
        self.assertEqual(Everything.test_strv_outarg(), ['1', '2', '3'])

    def test_strv_in_gvalue(self):
        self.assertEqual(Everything.test_strv_in_gvalue(), ['one', 'two', 'three'])

    def test_strv_in(self):
        Everything.test_strv_in(['1', '2', '3'])

    def test_glist(self):
        self.assertEqual(Everything.test_glist_nothing_return(), ['1', '2', '3'])
        self.assertEqual(Everything.test_glist_nothing_return2(), ['1', '2', '3'])
        self.assertEqual(Everything.test_glist_container_return(), ['1', '2', '3'])
        self.assertEqual(Everything.test_glist_everything_return(), ['1', '2', '3'])

        Everything.test_glist_nothing_in(['1', '2', '3'])
        Everything.test_glist_nothing_in2(['1', '2', '3'])

    def test_gslist(self):
        self.assertEqual(Everything.test_gslist_nothing_return(), ['1', '2', '3'])
        self.assertEqual(Everything.test_gslist_nothing_return2(), ['1', '2', '3'])
        self.assertEqual(Everything.test_gslist_container_return(), ['1', '2', '3'])
        self.assertEqual(Everything.test_gslist_everything_return(), ['1', '2', '3'])

        Everything.test_gslist_nothing_in(['1', '2', '3'])
        Everything.test_gslist_nothing_in2(['1', '2', '3'])

    def test_hash_return(self):
        expected = {'foo': 'bar', 'baz': 'bat', 'qux': 'quux'}

        self.assertEqual(Everything.test_ghash_null_return(), None)
        self.assertEqual(Everything.test_ghash_nothing_return(), expected)
        self.assertEqual(Everything.test_ghash_nothing_return(), expected)
        self.assertEqual(Everything.test_ghash_container_return(), expected)
        self.assertEqual(Everything.test_ghash_everything_return(), expected)

        result = Everything.test_ghash_gvalue_return()
        self.assertEqual(result['integer'], 12)
        self.assertEqual(result['boolean'], True)
        self.assertEqual(result['string'], 'some text')
        self.assertEqual(result['strings'], ['first', 'second', 'third'])
        self.assertEqual(result['flags'], Everything.TestFlags.FLAG1 | Everything.TestFlags.FLAG3)
        self.assertEqual(result['enum'], Everything.TestEnum.VALUE2)
        result = None

    # FIXME: CRITICAL **: Unsupported type ghash
    def disabled_test_hash_return_nested(self):
        self.assertEqual(Everything.test_ghash_nested_everything_return(), {})
        self.assertEqual(Everything.test_ghash_nested_everything_return2(), {})

    def test_hash_in(self):
        expected = {'foo': 'bar', 'baz': 'bat', 'qux': 'quux'}

        Everything.test_ghash_nothing_in(expected)
        Everything.test_ghash_nothing_in2(expected)

    def test_hash_in_with_typed_strv(self):
        class GStrv(list):
            __gtype__ = GObject.TYPE_STRV

        data = {'integer': 12,
                'boolean': True,
                'string': 'some text',
                'strings': GStrv(['first', 'second', 'third']),
                'flags': Everything.TestFlags.FLAG1 | Everything.TestFlags.FLAG3,
                'enum': Everything.TestEnum.VALUE2,
               }
        Everything.test_ghash_gvalue_in(data)
        data = None

    def test_hash_in_with_gvalue_strv(self):
        data = {'integer': 12,
                'boolean': True,
                'string': 'some text',
                'strings': GObject.Value(GObject.TYPE_STRV, ['first', 'second', 'third']),
                'flags': Everything.TestFlags.FLAG1 | Everything.TestFlags.FLAG3,
                'enum': Everything.TestEnum.VALUE2,
               }
        Everything.test_ghash_gvalue_in(data)
        data = None

    def test_struct_gpointer(self):
        glist = GLib.List()
        raw = RawGList.from_wrapped(glist)

        # Note that pointer fields use 0 for NULL in PyGObject and None in ctypes
        self.assertEqual(glist.data, 0)
        self.assertEqual(raw.contents.data, None)

        glist.data = 123
        self.assertEqual(glist.data, 123)
        self.assertEqual(raw.contents.data, 123)

        glist.data = None
        self.assertEqual(glist.data, 0)
        self.assertEqual(raw.contents.data, None)

        # Setting to anything other than an int should raise
        self.assertRaises(TypeError, setattr, glist.data, 'nan')
        self.assertRaises(TypeError, setattr, glist.data, object())
        self.assertRaises(TypeError, setattr, glist.data, 123.321)

    def test_struct_opaque(self):
        # we should get a sensible error message
        try:
            Everything.TestBoxedPrivate()
            self.fail('allocating disguised struct without default constructor unexpectedly succeeded')
        except TypeError:
            (e_type, e_value, e_tb) = sys.exc_info()
            self.assertEqual(e_type, TypeError)
            self.assertTrue('TestBoxedPrivate' in str(e_value), str(e_value))
            self.assertTrue('constructor' in str(e_value), str(e_value))
            tb = ''.join(traceback.format_exception(e_type, e_value, e_tb))
            self.assertTrue('tests/test_everything.py", line' in tb, tb)


@unittest.skipUnless(has_cairo, 'built without cairo support')
class TestNullableArgs(unittest.TestCase):
    def test_in_nullable_hash(self):
        Everything.test_ghash_null_in(None)

    def test_in_nullable_list(self):
        Everything.test_gslist_null_in(None)
        Everything.test_glist_null_in(None)
        Everything.test_gslist_null_in([])
        Everything.test_glist_null_in([])

    def test_in_nullable_array(self):
        Everything.test_array_int_null_in(None)
        Everything.test_array_int_null_in([])

    def test_in_nullable_string(self):
        Everything.test_utf8_null_in(None)

    def test_in_nullable_object(self):
        Everything.func_obj_null_in(None)

    def test_out_nullable_hash(self):
        self.assertEqual(None, Everything.test_ghash_null_out())

    def test_out_nullable_list(self):
        self.assertEqual([], Everything.test_gslist_null_out())
        self.assertEqual([], Everything.test_glist_null_out())

    def test_out_nullable_array(self):
        self.assertEqual([], Everything.test_array_int_null_out())

    def test_out_nullable_string(self):
        self.assertEqual(None, Everything.test_utf8_null_out())

    def test_out_nullable_object(self):
        self.assertEqual(None, Everything.TestObj.null_out())


@unittest.skipUnless(has_cairo, 'built without cairo support')
class TestCallbacks(unittest.TestCase):
    called = False
    main_loop = GLib.MainLoop()

    def test_callback(self):
        TestCallbacks.called = False

        def callback():
            TestCallbacks.called = True

        Everything.test_simple_callback(callback)
        self.assertTrue(TestCallbacks.called)

    def test_callback_exception(self):
        """
        This test ensures that we get errors from callbacks correctly
        and in particular that we do not segv when callbacks fail
        """
        def callback():
            x = 1 / 0
            self.fail('unexpected surviving zero divsion:' + str(x))

        # note that we do NOT expect the ZeroDivisionError to be propagated
        # through from the callback, as it crosses the Python<->C boundary
        # twice. (See GNOME #616279)
        Everything.test_simple_callback(callback)

    def test_double_callback_exception(self):
        """
        This test ensures that we get errors from callbacks correctly
        and in particular that we do not segv when callbacks fail
        """
        def badcallback():
            x = 1 / 0
            self.fail('unexpected surviving zero divsion:' + str(x))

        def callback():
            Everything.test_boolean(True)
            Everything.test_boolean(False)
            Everything.test_simple_callback(badcallback())

        # note that we do NOT expect the ZeroDivisionError to be propagated
        # through from the callback, as it crosses the Python<->C boundary
        # twice. (See GNOME #616279)
        Everything.test_simple_callback(callback)

    def test_return_value_callback(self):
        TestCallbacks.called = False

        def callback():
            TestCallbacks.called = True
            return 44

        self.assertEqual(Everything.test_callback(callback), 44)
        self.assertTrue(TestCallbacks.called)

    def test_callback_scope_async(self):
        TestCallbacks.called = False
        ud = 'Test Value 44'

        def callback(user_data):
            self.assertEqual(user_data, ud)
            TestCallbacks.called = True
            return 44

        ud_refcount = sys.getrefcount(ud)
        callback_refcount = sys.getrefcount(callback)

        self.assertEqual(Everything.test_callback_async(callback, ud), None)
        # Callback should not have run and the ref count is increased by 1
        self.assertEqual(TestCallbacks.called, False)
        self.assertEqual(sys.getrefcount(callback), callback_refcount + 1)
        self.assertEqual(sys.getrefcount(ud), ud_refcount + 1)

        # test_callback_thaw_async will run the callback previously supplied.
        # references should be auto decremented after this call.
        self.assertEqual(Everything.test_callback_thaw_async(), 44)
        self.assertTrue(TestCallbacks.called)

        # Make sure refcounts are returned to normal
        self.assertEqual(sys.getrefcount(callback), callback_refcount)
        self.assertEqual(sys.getrefcount(ud), ud_refcount)

    def test_callback_scope_call_multi(self):
        # This tests a callback that gets called multiple times from a
        # single scope call in python.
        TestCallbacks.called = 0

        def callback():
            TestCallbacks.called += 1
            return TestCallbacks.called

        refcount = sys.getrefcount(callback)
        result = Everything.test_multi_callback(callback)
        # first callback should give 1, second 2, and the function sums them up
        self.assertEqual(result, 3)
        self.assertEqual(TestCallbacks.called, 2)
        self.assertEqual(sys.getrefcount(callback), refcount)

    def test_callback_scope_call_array(self):
        # This tests a callback that gets called multiple times from a
        # single scope call in python with array arguments
        TestCallbacks.callargs = []

        # FIXME: would be cleaner without the explicit length args:
        # def callback(one, two):
        def callback(one, one_length, two, two_length):
            TestCallbacks.callargs.append((one, two))
            return len(TestCallbacks.callargs)

        refcount = sys.getrefcount(callback)
        result = Everything.test_array_callback(callback)
        # first callback should give 1, second 2, and the function sums them up
        self.assertEqual(result, 3)
        self.assertEqual(TestCallbacks.callargs,
                         [([-1, 0, 1, 2], ['one', 'two', 'three'])] * 2)
        self.assertEqual(sys.getrefcount(callback), refcount)

    @unittest.skipUnless(hasattr(Everything, 'test_array_inout_callback'),
                         'Requires newer version of GI')
    def test_callback_scope_call_array_inout(self):
        # This tests a callback that gets called multiple times from a
        # single scope call in python with inout array arguments
        TestCallbacks.callargs = []

        def callback(ints, ints_length):
            TestCallbacks.callargs.append(ints)
            return ints[1:], len(ints[1:])

        refcount = sys.getrefcount(callback)
        result = Everything.test_array_inout_callback(callback)
        self.assertEqual(TestCallbacks.callargs,
                         [[-2, -1, 0, 1, 2], [-1, 0, 1, 2]])
        # first callback should give 4, second 3
        self.assertEqual(result, 3)
        self.assertEqual(sys.getrefcount(callback), refcount)

    def test_callback_userdata(self):
        TestCallbacks.called = 0

        def callback(userdata):
            self.assertEqual(userdata, "Test%d" % TestCallbacks.called)
            TestCallbacks.called += 1
            return TestCallbacks.called

        for i in range(100):
            val = Everything.test_callback_user_data(callback, "Test%d" % i)
            self.assertEqual(val, i + 1)

        self.assertEqual(TestCallbacks.called, 100)

    def test_callback_userdata_no_user_data(self):
        TestCallbacks.called = 0

        def callback():
            TestCallbacks.called += 1
            return TestCallbacks.called

        for i in range(100):
            val = Everything.test_callback_user_data(callback)
            self.assertEqual(val, i + 1)

        self.assertEqual(TestCallbacks.called, 100)

    def test_callback_userdata_varargs(self):
        TestCallbacks.called = 0
        collected_user_data = []

        def callback(a, b):
            collected_user_data.extend([a, b])
            TestCallbacks.called += 1
            return TestCallbacks.called

        for i in range(10):
            val = Everything.test_callback_user_data(callback, 1, 2)
            self.assertEqual(val, i + 1)

        self.assertEqual(TestCallbacks.called, 10)
        self.assertSequenceEqual(collected_user_data, [1, 2] * 10)

    def test_callback_userdata_as_kwarg_tuple(self):
        TestCallbacks.called = 0
        collected_user_data = []

        def callback(user_data):
            collected_user_data.extend(user_data)
            TestCallbacks.called += 1
            return TestCallbacks.called

        for i in range(10):
            val = Everything.test_callback_user_data(callback, user_data=(1, 2))
            self.assertEqual(val, i + 1)

        self.assertEqual(TestCallbacks.called, 10)
        self.assertSequenceEqual(collected_user_data, [1, 2] * 10)

    def test_callback_user_data_middle_none(self):
        cb_info = {}

        def callback(userdata):
            cb_info['called'] = True
            cb_info['userdata'] = userdata
            return 1

        (y, z, q) = Everything.test_torture_signature_2(
            42, callback, None, 'some string', 3)
        self.assertEqual(y, 42)
        self.assertEqual(z, 84)
        self.assertEqual(q, 14)
        self.assertTrue(cb_info['called'])
        self.assertEqual(cb_info['userdata'], None)

    def test_callback_user_data_middle_single(self):
        cb_info = {}

        def callback(userdata):
            cb_info['called'] = True
            cb_info['userdata'] = userdata
            return 1

        (y, z, q) = Everything.test_torture_signature_2(
            42, callback, 'User Data', 'some string', 3)
        self.assertEqual(y, 42)
        self.assertEqual(z, 84)
        self.assertEqual(q, 14)
        self.assertTrue(cb_info['called'])
        self.assertEqual(cb_info['userdata'], 'User Data')

    def test_callback_user_data_middle_tuple(self):
        cb_info = {}

        def callback(userdata):
            cb_info['called'] = True
            cb_info['userdata'] = userdata
            return 1

        (y, z, q) = Everything.test_torture_signature_2(
            42, callback, (-5, 'User Data'), 'some string', 3)
        self.assertEqual(y, 42)
        self.assertEqual(z, 84)
        self.assertEqual(q, 14)
        self.assertTrue(cb_info['called'])
        self.assertEqual(cb_info['userdata'], (-5, 'User Data'))

    def test_async_ready_callback(self):
        TestCallbacks.called = False
        TestCallbacks.main_loop = GLib.MainLoop()

        def callback(obj, result, user_data):
            TestCallbacks.main_loop.quit()
            TestCallbacks.called = True

        Everything.test_async_ready_callback(callback)

        TestCallbacks.main_loop.run()

        self.assertTrue(TestCallbacks.called)

    def test_callback_scope_notified_with_destroy(self):
        TestCallbacks.called = 0
        ud = 'Test scope notified data 33'

        def callback(user_data):
            self.assertEqual(user_data, ud)
            TestCallbacks.called += 1
            return 33

        value_refcount = sys.getrefcount(ud)
        callback_refcount = sys.getrefcount(callback)

        # Callback is immediately called.
        for i in range(100):
            res = Everything.test_callback_destroy_notify(callback, ud)
            self.assertEqual(res, 33)

        self.assertEqual(TestCallbacks.called, 100)
        self.assertEqual(sys.getrefcount(callback), callback_refcount + 100)
        self.assertEqual(sys.getrefcount(ud), value_refcount + 100)

        # thaw will call the callback again, this time resources should be freed
        self.assertEqual(Everything.test_callback_thaw_notifications(), 33 * 100)
        self.assertEqual(TestCallbacks.called, 200)
        self.assertEqual(sys.getrefcount(callback), callback_refcount)
        self.assertEqual(sys.getrefcount(ud), value_refcount)

    def test_callback_scope_notified_with_destroy_no_user_data(self):
        TestCallbacks.called = 0

        def callback(user_data):
            self.assertEqual(user_data, None)
            TestCallbacks.called += 1
            return 34

        callback_refcount = sys.getrefcount(callback)

        # Run with warning as exception
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("error")
            self.assertRaises(RuntimeWarning,
                              Everything.test_callback_destroy_notify_no_user_data,
                              callback)

        self.assertEqual(TestCallbacks.called, 0)
        self.assertEqual(sys.getrefcount(callback), callback_refcount)

        # Run with warning as warning
        with warnings.catch_warnings(record=True) as w:
            # Cause all warnings to always be triggered.
            warnings.simplefilter("default")
            # Trigger a warning.
            res = Everything.test_callback_destroy_notify_no_user_data(callback)
            # Verify some things
            self.assertEqual(len(w), 1)
            self.assertTrue(issubclass(w[-1].category, RuntimeWarning))
            self.assertTrue('Callables passed to' in str(w[-1].message))

        self.assertEqual(res, 34)
        self.assertEqual(TestCallbacks.called, 1)
        self.assertEqual(sys.getrefcount(callback), callback_refcount + 1)

        # thaw will call the callback again,
        # refcount will not go down without user_data parameter
        self.assertEqual(Everything.test_callback_thaw_notifications(), 34)
        self.assertEqual(TestCallbacks.called, 2)
        self.assertEqual(sys.getrefcount(callback), callback_refcount + 1)

    def test_callback_in_methods(self):
        object_ = Everything.TestObj()

        def callback():
            TestCallbacks.called = True
            return 42

        TestCallbacks.called = False
        object_.instance_method_callback(callback)
        self.assertTrue(TestCallbacks.called)

        TestCallbacks.called = False
        Everything.TestObj.static_method_callback(callback)
        self.assertTrue(TestCallbacks.called)

        def callbackWithUserData(user_data):
            TestCallbacks.called += 1
            return 42

        TestCallbacks.called = 0
        Everything.TestObj.new_callback(callbackWithUserData, None)
        self.assertEqual(TestCallbacks.called, 1)
        # Note: using "new_callback" adds the notification to the same global
        # list as Everything.test_callback_destroy_notify, so thaw the list
        # so we don't get confusion between tests.
        self.assertEqual(Everything.test_callback_thaw_notifications(), 42)
        self.assertEqual(TestCallbacks.called, 2)

    def test_callback_none(self):
        # make sure this doesn't assert or crash
        Everything.test_simple_callback(None)

    def test_callback_gerror(self):
        def callback(error):
            self.assertEqual(error.message, 'regression test error')
            self.assertTrue('g-io' in error.domain)
            self.assertEqual(error.code, Gio.IOErrorEnum.NOT_SUPPORTED)
            TestCallbacks.called = True

        TestCallbacks.called = False
        Everything.test_gerror_callback(callback)
        self.assertTrue(TestCallbacks.called)

    def test_callback_null_gerror(self):
        def callback(error):
            self.assertEqual(error, None)
            TestCallbacks.called = True

        TestCallbacks.called = False
        Everything.test_null_gerror_callback(callback)
        self.assertTrue(TestCallbacks.called)

    def test_callback_owned_gerror(self):
        def callback(error):
            self.assertEqual(error.message, 'regression test owned error')
            self.assertTrue('g-io' in error.domain)
            self.assertEqual(error.code, Gio.IOErrorEnum.PERMISSION_DENIED)
            TestCallbacks.called = True

        TestCallbacks.called = False
        Everything.test_owned_gerror_callback(callback)
        self.assertTrue(TestCallbacks.called)

    def test_callback_hashtable(self):
        def callback(data):
            self.assertEqual(data, mydict)
            mydict['new'] = 42
            TestCallbacks.called = True

        mydict = {'foo': 1, 'bar': 2}
        TestCallbacks.called = False
        Everything.test_hash_table_callback(mydict, callback)
        self.assertTrue(TestCallbacks.called)
        self.assertEqual(mydict, {'foo': 1, 'bar': 2, 'new': 42})


@unittest.skipUnless(has_cairo, 'built without cairo support')
class TestClosures(unittest.TestCase):
    def test_no_arg(self):
        def callback():
            self.called = True
            return 42

        self.called = False
        result = Everything.test_closure(callback)
        self.assertTrue(self.called)
        self.assertEqual(result, 42)

    def test_int_arg(self):
        def callback(num):
            self.called = True
            return num + 1

        self.called = False
        result = Everything.test_closure_one_arg(callback, 42)
        self.assertTrue(self.called)
        self.assertEqual(result, 43)

    def test_variant(self):
        def callback(variant):
            self.called = True
            if variant is None:
                return None
            self.assertEqual(variant.get_type_string(), 'i')
            return GLib.Variant('i', variant.get_int32() + 1)

        self.called = False
        result = Everything.test_closure_variant(callback, GLib.Variant('i', 42))
        self.assertTrue(self.called)
        self.assertEqual(result.get_type_string(), 'i')
        self.assertEqual(result.get_int32(), 43)

        self.called = False
        result = Everything.test_closure_variant(callback, None)
        self.assertTrue(self.called)
        self.assertEqual(result, None)

        self.called = False
        self.assertRaises(TypeError, Everything.test_closure_variant, callback, 'foo')
        self.assertFalse(self.called)

    def test_variant_wrong_return_type(self):
        def callback(variant):
            return 'no_variant'

        # reset last error
        sys.last_type = None

        # this does not directly raise an exception (see
        # https://bugzilla.gnome.org/show_bug.cgi?id=616279)
        result = Everything.test_closure_variant(callback, GLib.Variant('i', 42))
        # ... but the result shouldn't be a string
        self.assertEqual(result, None)
        # and the error should be shown
        self.assertEqual(sys.last_type, TypeError)
        self.assertTrue('return value' in str(sys.last_value), sys.last_value)


@unittest.skipUnless(has_cairo, 'built without cairo support')
class TestBoxed(unittest.TestCase):
    def test_boxed(self):
        object_ = Everything.TestObj()
        self.assertEqual(object_.props.boxed, None)

        boxed = Everything.TestBoxed()
        boxed.some_int8 = 42
        object_.props.boxed = boxed

        self.assertTrue(isinstance(object_.props.boxed, Everything.TestBoxed))
        self.assertEqual(object_.props.boxed.some_int8, 42)

    def test_boxed_alternative_constructor(self):
        boxed = Everything.TestBoxed.new_alternative_constructor1(5)
        self.assertEqual(boxed.some_int8, 5)

        boxed = Everything.TestBoxed.new_alternative_constructor2(5, 3)
        self.assertEqual(boxed.some_int8, 8)

        boxed = Everything.TestBoxed.new_alternative_constructor3("-3")
        self.assertEqual(boxed.some_int8, -3)

    def test_boxed_equality(self):
        boxed42 = Everything.TestBoxed.new_alternative_constructor1(42)
        boxed5 = Everything.TestBoxed.new_alternative_constructor1(5)
        boxed42_2 = Everything.TestBoxed.new_alternative_constructor2(41, 1)

        self.assertFalse(boxed42.equals(boxed5))
        self.assertTrue(boxed42.equals(boxed42_2))
        self.assertTrue(boxed42_2.equals(boxed42))
        self.assertTrue(boxed42.equals(boxed42))

    def test_boxed_c_equality(self):
        boxed = Everything.TestBoxedC()
        # TestBoxedC uses refcounting, so we know that
        # the pointer is the same when copied
        copy = boxed.copy()
        self.assertEqual(boxed, copy)
        self.assertNotEqual(id(boxed), id(copy))


@unittest.skipUnless(has_cairo, 'built without cairo support')
class TestTortureProfile(unittest.TestCase):
    def test_torture_profile(self):
        import time
        total_time = 0
        print("")
        object_ = Everything.TestObj()
        sys.stdout.write("\ttorture test 1 (10000 iterations): ")

        start_time = time.clock()
        for i in range(10000):
            (y, z, q) = object_.torture_signature_0(5000,
                                                    "Torture Test 1",
                                                    12345)

        end_time = time.clock()
        delta_time = end_time - start_time
        total_time += delta_time
        print("%f secs" % delta_time)

        sys.stdout.write("\ttorture test 2 (10000 iterations): ")

        start_time = time.clock()
        for i in range(10000):
            (y, z, q) = Everything.TestObj().torture_signature_0(
                5000, "Torture Test 2", 12345)

        end_time = time.clock()
        delta_time = end_time - start_time
        total_time += delta_time
        print("%f secs" % delta_time)

        sys.stdout.write("\ttorture test 3 (10000 iterations): ")
        start_time = time.clock()
        for i in range(10000):
            try:
                (y, z, q) = object_.torture_signature_1(
                    5000, "Torture Test 3", 12345)
            except:
                pass
        end_time = time.clock()
        delta_time = end_time - start_time
        total_time += delta_time
        print("%f secs" % delta_time)

        sys.stdout.write("\ttorture test 4 (10000 iterations): ")

        def callback(userdata):
            return 0

        userdata = [1, 2, 3, 4]
        start_time = time.clock()
        for i in range(10000):
            (y, z, q) = Everything.test_torture_signature_2(
                5000, callback, userdata, "Torture Test 4", 12345)
        end_time = time.clock()
        delta_time = end_time - start_time
        total_time += delta_time
        print("%f secs" % delta_time)
        print("\t====")
        print("\tTotal: %f sec" % total_time)


@unittest.skipUnless(has_cairo, 'built without cairo support')
class TestAdvancedInterfaces(unittest.TestCase):
    def test_array_objs(self):
        obj1, obj2 = Everything.test_array_fixed_out_objects()
        self.assertTrue(isinstance(obj1, Everything.TestObj))
        self.assertTrue(isinstance(obj2, Everything.TestObj))
        self.assertNotEqual(obj1, obj2)

    def test_obj_skip_return_val(self):
        obj = Everything.TestObj()
        ret = obj.skip_return_val(50, 42.0, 60, 2, 3)
        self.assertEqual(len(ret), 3)
        self.assertEqual(ret[0], 51)
        self.assertEqual(ret[1], 61)
        self.assertEqual(ret[2], 32)

    def test_obj_skip_return_val_no_out(self):
        obj = Everything.TestObj()
        # raises an error for 0, succeeds for any other value
        self.assertRaises(GLib.GError, obj.skip_return_val_no_out, 0)

        ret = obj.skip_return_val_no_out(1)
        self.assertEqual(ret, None)

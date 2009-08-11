# -*- Mode: Python -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest
from datetime import datetime

import gobject
from gobject import constants

from gi.repository import Everything

INT8_MIN = -128
INT16_MIN = -32768
INT32_MIN = -2147483648
INT64_MIN = -9223372036854775808L

INT8_MAX = 127
INT16_MAX = 32767
INT32_MAX = 2147483647
INT64_MAX = 9223372036854775807L

UINT8_MAX = 255
UINT16_MAX = 65535
UINT32_MAX = 4294967295
UINT64_MAX = 18446744073709551615L

utf8_const = 'const \xe2\x99\xa5 utf8'
utf8_nonconst = 'nonconst \xe2\x99\xa5 utf8'

test_sequence = ('1', '2', '3')
test_dict = {'foo': 'bar', 'baz': 'bat', 'qux': 'quux'}


class SignalHandler:
    def __init__(self):
        self.counter = 0
        self.obj = Everything.TestObj('foo')
        self.some_object = None

    def callback(self, signal_obj):
        self.counter += 1
        self.some_object = signal_obj
        self.obj.disconnect(self.h_id)

class TestGIEverything(unittest.TestCase):

    def testBoolean(self):
        self.assertTrue(Everything.test_boolean(True))
        self.assertFalse(Everything.test_boolean(False))
        self.assertFalse(Everything.test_boolean(0))
        self.assertTrue(Everything.test_boolean(2))

    def testShort(self):
        self.assertEqual(3, Everything.test_short(3))
        self.assertEqual(3, Everything.test_short(3L))
        self.assertEqual(-3, Everything.test_short(-3))
        self.assertEqual(-3, Everything.test_short(-3L))
        self.assertEqual(constants.G_MINSHORT, Everything.test_short(constants.G_MINSHORT))
        self.assertEqual(constants.G_MAXSHORT, Everything.test_short(constants.G_MAXSHORT))
        self.assertEqual(constants.G_MINSHORT, Everything.test_short(long(constants.G_MINSHORT)))
        self.assertEqual(constants.G_MAXSHORT, Everything.test_short(long(constants.G_MAXSHORT)))
        self.assertRaises(TypeError, Everything.test_short, 'a')
        self.assertRaises(ValueError, Everything.test_short, constants.G_MINSHORT-1)
        self.assertRaises(ValueError, Everything.test_short, constants.G_MAXSHORT+1)
        self.assertRaises(ValueError, Everything.test_short, long(constants.G_MINSHORT-1))
        self.assertRaises(ValueError, Everything.test_short, long(constants.G_MAXSHORT+1))

    def testInt(self):
        self.assertEqual(3, Everything.test_int(3))
        self.assertEqual(-3, Everything.test_int(-3))
        self.assertRaises(TypeError, Everything.test_int, 'a')

    def testInt8(self):
        self.assertEqual(3, Everything.test_int8(3))
        self.assertEqual(-3, Everything.test_int8(-3))
        self.assertEqual(INT8_MIN, Everything.test_int8(INT8_MIN))
        self.assertEqual(INT8_MAX, Everything.test_int8(INT8_MAX))
        self.assertEqual(INT8_MIN, Everything.test_int8(long(INT8_MIN)))
        self.assertEqual(INT8_MAX, Everything.test_int8(long(INT8_MAX)))
        self.assertRaises(TypeError, Everything.test_int, 'a')
        self.assertRaises(ValueError, Everything.test_int8, INT8_MIN-1)
        self.assertRaises(ValueError, Everything.test_int8, INT8_MAX+1)
        self.assertRaises(ValueError, Everything.test_int8, long(INT8_MIN-1))
        self.assertRaises(ValueError, Everything.test_int8, long(INT8_MAX+1))

    def testInt16(self):
        self.assertEqual(3, Everything.test_int16(3))
        self.assertEqual(-3, Everything.test_int16(-3))
        self.assertEqual(INT16_MIN, Everything.test_int16(INT16_MIN))
        self.assertEqual(INT16_MAX, Everything.test_int16(INT16_MAX))
        self.assertEqual(INT16_MIN, Everything.test_int16(long(INT16_MIN)))
        self.assertEqual(INT16_MAX, Everything.test_int16(long(INT16_MAX)))
        self.assertRaises(TypeError, Everything.test_int, 'a')
        self.assertRaises(ValueError, Everything.test_int16, INT16_MIN-1)
        self.assertRaises(ValueError, Everything.test_int16, INT16_MAX+1)
        self.assertRaises(ValueError, Everything.test_int16, long(INT16_MIN-1))
        self.assertRaises(ValueError, Everything.test_int16, long(INT16_MAX+1))

    def testInt32(self):
        self.assertEqual(3, Everything.test_int32(3))
        self.assertEqual(-3, Everything.test_int32(-3))
        self.assertEqual(INT32_MIN, Everything.test_int32(INT32_MIN))
        self.assertEqual(INT32_MAX, Everything.test_int32(INT32_MAX))
        self.assertEqual(INT32_MIN, Everything.test_int32(long(INT32_MIN)))
        self.assertEqual(INT32_MAX, Everything.test_int32(long(INT32_MAX)))
        self.assertRaises(TypeError, Everything.test_int, 'a')
        self.assertRaises(ValueError, Everything.test_int32, INT32_MIN-1)
        self.assertRaises(ValueError, Everything.test_int32, INT32_MAX+1)
        self.assertRaises(ValueError, Everything.test_int32, long(INT32_MIN-1))
        self.assertRaises(ValueError, Everything.test_int32, long(INT32_MAX+1))

    def testInt64(self):
        self.assertEqual(3, Everything.test_int64(3))
        self.assertEqual(-3, Everything.test_int64(-3))
        self.assertEqual(INT64_MIN, Everything.test_int64(INT64_MIN))
        self.assertEqual(INT64_MAX, Everything.test_int64(INT64_MAX))
        self.assertEqual(INT64_MIN, Everything.test_int64(long(INT64_MIN)))
        self.assertEqual(INT64_MAX, Everything.test_int64(long(INT64_MAX)))
        self.assertRaises(TypeError, Everything.test_int, 'a')
        self.assertRaises(ValueError, Everything.test_int64, INT64_MIN-1)
        self.assertRaises(ValueError, Everything.test_int64, INT64_MAX+1)

    def testUShort(self):
        self.assertEqual(3, Everything.test_ushort(3))
        self.assertEqual(3, Everything.test_ushort(3L))
        self.assertEqual(constants.G_MAXUSHORT, Everything.test_ushort(constants.G_MAXUSHORT))
        self.assertEqual(constants.G_MAXUSHORT, Everything.test_ushort(long(constants.G_MAXUSHORT)))
        self.assertRaises(TypeError, Everything.test_ushort, 'a')
        self.assertRaises(ValueError, Everything.test_ushort, -3)
        self.assertRaises(ValueError, Everything.test_ushort, -3L)
        self.assertRaises(ValueError, Everything.test_ushort, constants.G_MAXUSHORT+1)
        self.assertRaises(ValueError, Everything.test_ushort, long(constants.G_MAXUSHORT+1))


    def testUInt(self):
        self.assertEqual(3, Everything.test_uint(3))
        self.assertEqual(3, Everything.test_uint(3L))
        self.assertRaises(TypeError, Everything.test_uint, 'a')
        self.assertRaises(ValueError, Everything.test_uint, -3)
        self.assertRaises(ValueError, Everything.test_uint, -3L)

    def testUInt8(self):
        self.assertEqual(3, Everything.test_uint8(3))
        self.assertEqual(3, Everything.test_uint8(3L))
        self.assertEqual(UINT8_MAX, Everything.test_uint8(UINT8_MAX))
        self.assertEqual(UINT8_MAX, Everything.test_uint8(long(UINT8_MAX)))
        self.assertRaises(TypeError, Everything.test_uint8, 'a')
        self.assertRaises(ValueError, Everything.test_uint8, -3)
        self.assertRaises(ValueError, Everything.test_uint8, -3L)
        self.assertRaises(ValueError, Everything.test_uint8, UINT8_MAX+1)
        self.assertRaises(ValueError, Everything.test_uint8, long(UINT8_MAX+1))

    def testUInt16(self):
        self.assertEqual(3, Everything.test_uint16(3))
        self.assertEqual(3, Everything.test_uint16(3L))
        self.assertEqual(UINT16_MAX, Everything.test_uint16(UINT16_MAX))
        self.assertEqual(UINT16_MAX, Everything.test_uint16(long(UINT16_MAX)))
        self.assertRaises(TypeError, Everything.test_uint16, 'a')
        self.assertRaises(ValueError, Everything.test_uint16, -3)
        self.assertRaises(ValueError, Everything.test_uint16, -3L)
        self.assertRaises(ValueError, Everything.test_uint16, UINT16_MAX+1)
        self.assertRaises(ValueError, Everything.test_uint16, long(UINT16_MAX+1))

    def testUInt32(self):
        self.assertEqual(3, Everything.test_uint32(3))
        self.assertEqual(3, Everything.test_uint32(3L))
        self.assertEqual(UINT32_MAX, Everything.test_uint32(UINT32_MAX))
        self.assertEqual(UINT32_MAX, Everything.test_uint32(long(UINT32_MAX)))
        self.assertRaises(TypeError, Everything.test_uint32, 'a')
        self.assertRaises(ValueError, Everything.test_uint32, -3)
        self.assertRaises(ValueError, Everything.test_uint32, -3L)
        self.assertRaises(ValueError, Everything.test_uint32, UINT32_MAX+1)
        self.assertRaises(ValueError, Everything.test_uint32, long(UINT32_MAX+1))

    def testUInt64(self):
        self.assertEqual(3, Everything.test_uint64(3))
        self.assertEqual(3, Everything.test_uint64(3L))
        self.assertEqual(UINT64_MAX, Everything.test_uint64(UINT64_MAX))
        self.assertEqual(UINT64_MAX, Everything.test_uint64(long(UINT64_MAX)))
        self.assertRaises(TypeError, Everything.test_uint64, 'a')
        self.assertRaises(ValueError, Everything.test_uint64, -3)
        self.assertRaises(ValueError, Everything.test_uint64, -3L)
        self.assertRaises(ValueError, Everything.test_uint64, UINT64_MAX+1)

    def testLong(self):
        self.assertEqual(3, Everything.test_long(3))
        self.assertEqual(-3, Everything.test_long(-3))
        self.assertRaises(TypeError, Everything.test_long, 'a')

    def testULong(self):
        self.assertEqual(3, Everything.test_ulong(3))
        self.assertEqual(3, Everything.test_ulong(3L))
        self.assertRaises(TypeError, Everything.test_ulong, 'a')
        self.assertRaises(ValueError, Everything.test_ulong, -3)
        self.assertRaises(ValueError, Everything.test_ulong, -3L)

    def testSSize(self):
        self.assertEqual(3, Everything.test_ssize(3))
        self.assertEqual(3, Everything.test_ssize(3L))
        self.assertEqual(-3, Everything.test_ssize(-3))
        self.assertRaises(TypeError, Everything.test_ssize, 'a')

    def testSize(self):
        self.assertEqual(3, Everything.test_size(3))
        self.assertEqual(3, Everything.test_size(3L))
        self.assertRaises(TypeError, Everything.test_ssize, 'a')
        self.assertRaises(ValueError, Everything.test_size, -3)

    def testFloat(self):
        self.assertAlmostEqual(3.14, Everything.test_float(3.14), 6)
        self.assertAlmostEqual(-3.14, Everything.test_float(-3.14), 6)
        self.assertRaises(TypeError, Everything.test_float, 'a')

    def testDouble(self):
        self.assertAlmostEqual(3.14, Everything.test_double(3.14))
        self.assertAlmostEqual(-3.14, Everything.test_double(-3.14))
        self.assertRaises(TypeError, Everything.test_double, 'a')

    def testTimeT(self):
        now = datetime.now()
        retval = Everything.test_timet(now)

        # We can't do a direct comparison because we lost the microseconds precision.
        self.assertEquals(retval.year, now.year)
        self.assertEquals(retval.month, now.month)
        self.assertEquals(retval.day, now.day)
        self.assertEquals(retval.hour, now.hour)
        self.assertEquals(retval.minute, now.minute)
        self.assertEquals(retval.second, now.second)
        self.assertEquals(retval.tzinfo, now.tzinfo)

    def testGType(self):
        self.assertEqual(gobject.TYPE_INT, Everything.test_gtype(gobject.TYPE_INT))
        self.assertEqual(Everything.TestObj.__gtype__, Everything.test_gtype(Everything.TestObj))
        self.assertRaises(TypeError, Everything.test_gtype, 'a')


# UTF-8

    def testUtf8ConstReturn(self):
        self.assertEquals(utf8_const, Everything.test_utf8_const_return())

    def testUtf8NonconstReturn(self):
        self.assertEquals(utf8_nonconst, Everything.test_utf8_nonconst_return())

    def testUtf8NullReturn(self):
        self.assertEquals(None, Everything.test_utf8_null_return())

    def testUtf8NonconstIn(self):
        Everything.test_utf8_nonconst_in(utf8_nonconst)

    def testUtf8ConstIn(self):
        Everything.test_utf8_const_in(utf8_const)

    def testUtf8NullIn(self):
        Everything.test_utf8_null_in(None)

    def testUtf8Out(self):
        self.assertEquals(utf8_nonconst, Everything.test_utf8_out())

    def testUtf8Inout(self):
        self.assertEquals(utf8_nonconst, Everything.test_utf8_inout(utf8_const))

    def testFilenameReturn(self):
        filenames = Everything.test_filename_return()
        self.assertEquals(2, len(filenames))
        self.assertEquals('\xc3\xa5\xc3\xa4\xc3\xb6', filenames[0])
        self.assertEquals('/etc/fstab', filenames[1])


# Multiple output arguments

    def testUtf8OutOut(self):
        self.assertEquals(("first", "second"), Everything.test_utf8_out_out())

    def testUtf8OutNonconstReturn(self):
        self.assertEquals(("first", "second"), Everything.test_utf8_out_nonconst_return())


# Arrays

    def testArrayIntIn(self):
        self.assertEquals(5, Everything.test_array_int_in((1, 2, 3, -1)))
        self.assertEquals(0, Everything.test_array_int_in(()))

        self.assertRaises(TypeError, Everything.test_array_int_in, 0)
        self.assertRaises(TypeError, Everything.test_array_int_in, (2, 'a'))

    def testArrayIntInout(self):
        self.assertEquals((2, 3, 4, 5), Everything.test_array_int_inout((0, 1, 2, 3, 4)))
        self.assertEquals((), Everything.test_array_int_inout(()))

    def testArrayIntOut(self):
        self.assertEquals((0, 1, 2, 3, 4), Everything.test_array_int_out())

    def testArrayInt8In(self):
        self.assertEquals(5, Everything.test_array_gint8_in((1, 2, 3, -1)))
        self.assertEquals(-1, Everything.test_array_gint8_in((INT8_MAX, INT8_MIN)))

        self.assertRaises(TypeError, Everything.test_array_gint8_in, 0)
        self.assertRaises(TypeError, Everything.test_array_gint8_in, (2, 'a'))
        self.assertRaises(ValueError, Everything.test_array_gint8_in, (INT8_MAX+1,))
        self.assertRaises(ValueError, Everything.test_array_gint8_in, (INT8_MIN-1,))

    def testArrayInt16In(self):
        self.assertEquals(5, Everything.test_array_gint16_in((1, 2, 3, -1)))
        self.assertEquals(-1, Everything.test_array_gint16_in((INT16_MAX, INT16_MIN)))

        self.assertRaises(TypeError, Everything.test_array_gint16_in, 0)
        self.assertRaises(TypeError, Everything.test_array_gint16_in, (2, 'a'))
        self.assertRaises(ValueError, Everything.test_array_gint16_in, (INT16_MAX+1,))
        self.assertRaises(ValueError, Everything.test_array_gint16_in, (INT16_MIN-1,))

    def testArrayInt32In(self):
        self.assertEquals(5, Everything.test_array_gint32_in((1, 2, 3, -1)))
        self.assertEquals(-1, Everything.test_array_gint32_in((INT32_MAX, INT32_MIN)))

        self.assertRaises(TypeError, Everything.test_array_gint32_in, 0)
        self.assertRaises(TypeError, Everything.test_array_gint32_in, (2, 'a'))
        self.assertRaises(ValueError, Everything.test_array_gint32_in, (INT32_MAX+1,))
        self.assertRaises(ValueError, Everything.test_array_gint32_in, (INT32_MIN-1,))

    def testArrayInt32In(self):
        self.assertEquals(5, Everything.test_array_gint32_in((1, 2, 3, -1)))
        self.assertEquals(-1, Everything.test_array_gint32_in((INT32_MAX, INT32_MIN)))

        self.assertRaises(TypeError, Everything.test_array_gint32_in, 0)
        self.assertRaises(TypeError, Everything.test_array_gint32_in, (2, 'a'))
        self.assertRaises(ValueError, Everything.test_array_gint32_in, (INT32_MAX+1,))
        self.assertRaises(ValueError, Everything.test_array_gint32_in, (INT32_MIN-1,))

    def testArrayInt64In(self):
        self.assertEquals(5, Everything.test_array_gint64_in((1, 2, 3, -1)))
        self.assertEquals(-1, Everything.test_array_gint64_in((INT64_MAX, INT64_MIN)))

        self.assertRaises(TypeError, Everything.test_array_gint64_in, 0)
        self.assertRaises(TypeError, Everything.test_array_gint64_in, (2, 'a'))
        self.assertRaises(ValueError, Everything.test_array_gint64_in, (INT64_MAX+1,))
        self.assertRaises(ValueError, Everything.test_array_gint64_in, (INT64_MIN-1,))

    def testStrvIn(self):
        self.assertTrue(Everything.test_strv_in(test_sequence))

        # Test with equivalent instances which offer the sequence protocol too.
        self.assertTrue(Everything.test_strv_in(list(test_sequence)))
        self.assertTrue(Everything.test_strv_in(''.join(test_sequence)))

        # Test an empty one.
        self.assertFalse(Everything.test_strv_in(()))

        # Test type checking.
        self.assertRaises(TypeError, Everything.test_strv_in, 1)
        self.assertRaises(TypeError, Everything.test_strv_in, ('1', 2, 3))

    def testArrayGTypeIn(self):
        self.assertEqual('[gint,TestObj,]', Everything.test_array_gtype_in((gobject.TYPE_INT, Everything.TestObj)))

        # Test type checking.
        self.assertRaises(TypeError, Everything.test_array_gtype_in, ('gint', Everything.TestObj))

    def testStrvOut(self):
        self.assertEquals(("thanks", "for", "all", "the", "fish"), Everything.test_strv_out())
        self.assertEquals(("thanks", "for", "all", "the", "fish"), Everything.test_strv_out_c())

    def testStrvOutarg(self):
        self.assertEquals(("1", "2", "3"), Everything.test_strv_outarg())

    def testArrayFixedSizeIntIn(self):
        self.assertEquals(9, Everything.test_array_fixed_size_int_in((1, 2, 3, 4, -1)))

    def testArrayFixedSizeIntOut(self):
        self.assertEquals((0, 1, 2, 3, 4), Everything.test_array_fixed_size_int_out())

    def testArrayFixedSizeIntReturn(self):
        self.assertEquals((0, 1, 2, 3, 4), Everything.test_array_fixed_size_int_return())


# Transfer tests

    def testArrayIntInTake(self):
        self.assertEquals(5, Everything.test_array_int_in_take((1, 2, 3, -1)))

    def testStrvInContainer(self):
        self.assertTrue(Everything.test_strv_in_container(test_sequence))

    def testArrayIntFullOut(self):
        self.assertEquals((0, 1, 2, 3, 4), Everything.test_array_int_full_out())

    def testArrayIntNoneOut(self):
        self.assertEquals((1, 2, 3, 4, 5), Everything.test_array_int_none_out())


# Interface
# GList

    def testGListNothingReturn(self):
        self.assertEqual(list(test_sequence), Everything.test_glist_nothing_return())

    def testGListContainerReturn(self):
        self.assertEqual(list(test_sequence), Everything.test_glist_container_return())

    def testGListEverythingReturn(self):
        self.assertEqual(list(test_sequence), Everything.test_glist_everything_return())

    def testGListNothingIn(self):
        Everything.test_glist_nothing_in(test_sequence)

        # Test as string, which implements the sequence protocol too.
        Everything.test_glist_nothing_in('123')

        # Test type checking.
        self.assertRaises(TypeError, Everything.test_glist_nothing_in, 1)
        self.assertRaises(TypeError, Everything.test_glist_nothing_in, (1, 2, 3))

    def testGListContainerIn(self):
        Everything.test_glist_container_in(test_sequence)

    def testGListEverythingIn(self):
        Everything.test_glist_everything_in(test_sequence)


# GSList

    def testGSListNothingReturn(self):
        self.assertEqual(list(test_sequence), Everything.test_gslist_nothing_return())

    def testGSListContainerReturn(self):
        self.assertEqual(list(test_sequence), Everything.test_gslist_container_return())

    def testGSListEverythingReturn(self):
        self.assertEqual(list(test_sequence), Everything.test_gslist_everything_return())

    def testGSListNothingIn(self):
        Everything.test_gslist_nothing_in(test_sequence)

        # Test as string, which implements the sequence protocol too.
        Everything.test_gslist_nothing_in('123')

        # Test type checking.
        self.assertRaises(TypeError, Everything.test_gslist_nothing_in, 1)
        self.assertRaises(TypeError, Everything.test_gslist_nothing_in, (1, 2, 3))

    def testGSListContainerIn(self):
        Everything.test_gslist_container_in(test_sequence)

    def testGSListEverythingIn(self):
        Everything.test_gslist_everything_in(test_sequence)


# GHashTable

    def testGHashTableNothingReturn(self):
        self.assertEqual(test_dict, Everything.test_ghash_nothing_return())

    def testGHashTableContainerReturn(self):
        self.assertEqual(test_dict, Everything.test_ghash_container_return())

    def testGHashTableEverythingReturn(self):
        self.assertEqual(test_dict, Everything.test_ghash_everything_return())

    def testGHashTableNothingIn(self):
        Everything.test_ghash_nothing_in(test_dict)

        # Test type checking.
        self.assertRaises(TypeError, Everything.test_ghash_nothing_in, 'foo')
        self.assertRaises(TypeError, Everything.test_ghash_nothing_in, {'foo': 42})
        self.assertRaises(TypeError, Everything.test_ghash_nothing_in, {42: 'foo'})

    def testGHashTableContainerIn(self):
        Everything.test_ghash_container_in(test_dict)

    def testGHashTableEverythingIn(self):
        Everything.test_ghash_everything_in(test_dict)


# closure

    def testClosure(self):
        self.assertEquals(3, Everything.test_closure(lambda: 3))

    def testClosureOneArg(self):
        self.assertEquals(3, Everything.test_closure_one_arg(lambda x: x, 3))

    def testIntValueArg(self):
        self.assertEquals(42, Everything.test_int_value_arg(42))

    def testValueReturn(self):
        self.assertEquals(42, Everything.test_value_return(42))


# Enum

    def testEnum(self):
        self.assertTrue(issubclass(Everything.TestEnum, gobject.GEnum))

        # Test instantiation.
        self.assertTrue(isinstance(Everything.TestEnum(42), Everything.TestEnum))

        # Test attributes.
        self.assertTrue(isinstance(Everything.TestEnum.VALUE3, Everything.TestEnum))

        # Test numeric equivalence.
        self.assertEquals(Everything.TestEnum.VALUE3, 42)


# Flags

    def testFlags(self):
        self.assertTrue(issubclass(Everything.TestFlags, gobject.GFlags))

        # Test instantiation.
        self.assertTrue(isinstance(Everything.TestFlags(1 << 2), Everything.TestFlags))

        # Test attributes.
        self.assertTrue(isinstance(Everything.TestFlags.FLAG3, Everything.TestFlags))

        # Test numeric equivalence.
        self.assertEquals(Everything.TestFlags.FLAG3, 1 << 2)


    def testSignal(self):
        h = SignalHandler()
        h.h_id = h.obj.connect('test', h.callback)
        h.obj.emit('test')
        self.assertEquals(1, h.counter)
        self.assertEquals(h.obj, h.some_object)
        h.obj.emit('test')
        self.assertEquals(1, h.counter)

    def testInvalidSignal(self):
        def signal_handler(o):
            pass
        o = Everything.TestObj('foo')
        self.assertRaises(TypeError, o.connect, 'invalid-signal', signal_handler)
        self.assertRaises(TypeError, o.emit, 'invalid-signal')


# Structures

    def testStructA(self):
        # Test inheritance.
        self.assertTrue(issubclass(Everything.TestStructA, gobject.GBoxed))

        # Test instanciation.
        a = Everything.TestStructA()
        self.assertTrue(isinstance(a, Everything.TestStructA))

        # Test fields.
        a.some_int = 3
        a.some_int8 = 1
        a.some_double = 4.15
        a.some_enum= Everything.TestEnum.VALUE3

        self.assertEquals(a.some_int, 3)
        self.assertEquals(a.some_int8, 1)
        self.assertEquals(a.some_double, 4.15)
        self.assertEquals(a.some_enum, Everything.TestEnum.VALUE3)

        self.assertRaises(TypeError, setattr, a, 'some_int', 'a')
        self.assertRaises(ValueError, setattr, a, 'some_int8', INT8_MIN-1)
        self.assertRaises(ValueError, setattr, a, 'some_int8', INT8_MAX+1)

        # Test method call and marshalling.
        a_out = Everything.TestStructA()
        a.clone(a_out)

        self.assertEquals(a.some_int, a_out.some_int)
        self.assertEquals(a.some_int8, a_out.some_int8)
        self.assertEquals(a.some_double, a_out.some_double)
        self.assertEquals(a.some_enum, a_out.some_enum)

        # Test instance checking by passing a wrong instance.
        self.assertRaises(TypeError, Everything.TestStructA.clone, 'a', a_out)

    def testStructB(self):
        b = Everything.TestStructB()
        a = Everything.TestStructA()

        # Test nested structures.
        b.nested_a = a
        a_out = b.nested_a

        self.assertTrue(isinstance(b.nested_a, Everything.TestStructA))
        self.assertEquals(a.some_int, a_out.some_int)
        self.assertEquals(a.some_int8, a_out.some_int8)
        self.assertEquals(a.some_double, a_out.some_double)
        self.assertEquals(a.some_enum, a_out.some_enum)

        # Test assignment checking.
        self.assertRaises(TypeError, setattr, b, 'nested_a', 'a')
        self.assertRaises(TypeError, setattr, b, 'nested_a', Everything.TestStructB())

    def testStructC(self):
        self.assertRaises(TypeError, Everything.TestStructC)


# GObject

    def testObj(self):
        # Test inheritance.
        self.assertTrue(issubclass(Everything.TestObj, gobject.GObject))
        self.assertEquals(Everything.TestObj.__gtype__, Everything.TestObj.__info__.get_g_type())

    def testDefaultConstructor(self):
        # Test instanciation.
        obj = Everything.TestObj('foo')
        self.assertTrue(isinstance(obj, Everything.TestObj))

        # Test the argument count.
        self.assertRaises(TypeError, Everything.TestObj)
        self.assertRaises(TypeError, Everything.TestObj, 'foo', 'bar')

    def testAlternateConstructor(self):
        # Test instanciation.
        obj = Everything.TestObj.new_from_file('foo')
        self.assertTrue(isinstance(obj, Everything.TestObj))

        obj = obj.new_from_file('foo')
        self.assertTrue(isinstance(obj, Everything.TestObj))

        # Test the argument count.
        self.assertRaises(TypeError, Everything.TestObj.new_from_file)
        self.assertRaises(TypeError, Everything.TestObj.new_from_file, 'foo', 'bar')

    def testInstanceMethod(self):
        obj = Everything.TestObj('foo')

        # Test call.
        self.assertEquals(-1, obj.instance_method())

        # Test argument count.
        self.assertRaises(TypeError, obj.instance_method, 'foo')

        # Test instance checking by passing a wrong instance.
        obj = gobject.GObject()
        self.assertRaises(TypeError, Everything.TestObj.instance_method)

    def testVirtualMethod(self):
        obj = Everything.TestObj('foo')

        # Test call.
        self.assertEquals(42, obj.do_matrix('matrix'))

        # Test argument count.
        self.assertRaises(TypeError, obj.do_matrix)
        self.assertRaises(TypeError, obj.do_matrix, 'matrix', 'foo')
        self.assertRaises(TypeError, Everything.TestObj.do_matrix, 'matrix')

        # Test instance checking by passing a wrong instance.
        obj = gobject.GObject()
        self.assertRaises(TypeError, Everything.TestObj.do_matrix, obj, 'matrix')

    def testStaticMethod(self):
        obj = Everything.TestObj('foo')

        # Test calls.
        self.assertEquals(42, Everything.TestObj.static_method(42))
        self.assertEquals(42, obj.static_method(42))

        # Test argument count.
        self.assertRaises(TypeError, Everything.TestObj.static_method)
        self.assertRaises(TypeError, Everything.TestObj.static_method, 'foo', 'bar')

    def testObjIn(self):
        obj = Everything.TestObj('foo')
        bare = Everything.TestObj('bar')
        obj.set_bare(bare)

    def testObjOut(self):
        obj = Everything.TestObj('foo')
        bare = Everything.TestObj('bar')
        obj.set_bare(bare)
        self.assertTrue(bare is obj.get_bare())


# Inheritance

    def testSubObj(self):
        # Test subclassing.
        self.assertTrue(issubclass(Everything.TestSubObj, Everything.TestObj))

        # Test static method.
        self.assertEquals(42, Everything.TestSubObj.static_method(42))

        # Test constructor.
        subobj = Everything.TestSubObj()
        self.assertTrue(isinstance(subobj, Everything.TestSubObj))
        self.assertTrue(isinstance(subobj, Everything.TestObj))

        # Test alternate constructor.
        subobj = Everything.TestSubObj.new_from_file('foo')
        self.assertTrue(isinstance(subobj, Everything.TestSubObj))

        # Test method inheritance.
        self.assertTrue(hasattr(subobj, 'set_bare'))
        self.assertEquals(42, subobj.do_matrix('foo'))

        # Test method overriding.
        self.assertEquals(0, subobj.instance_method())

        # Test instance checking by passing a wrong instance.
        obj = Everything.TestObj('foo')
        self.assertRaises(TypeError, Everything.TestSubObj.instance_method, obj)

    def testPythonSubObj(self):
        class PythonSubObj(Everything.TestObj):
            def __new__(cls):
                return super(PythonSubObj, cls).__new__(cls, 'foo')
        gobject.type_register(PythonSubObj)

        # Test subclassing.
        self.assertTrue(issubclass(PythonSubObj, Everything.TestObj))
        self.assertTrue(PythonSubObj.__gtype__ != Everything.TestObj.__gtype__)
        self.assertTrue(PythonSubObj.__gtype__.is_a(Everything.TestObj.__gtype__))

        # Test static method.
        self.assertEquals(42, PythonSubObj.static_method(42))

        # Test instanciation.
        subobj = PythonSubObj()
        self.assertTrue(isinstance(subobj, PythonSubObj))

        subobj = PythonSubObj.new_from_file('foo')
        self.assertTrue(isinstance(subobj, PythonSubObj))

        # Test method inheritance.
        self.assertTrue(hasattr(subobj, 'set_bare'))
        self.assertEquals(42, subobj.do_matrix('foo'))


# Interface

    def testInterface(self):
        self.assertTrue(issubclass(Everything.TestInterface, gobject.GInterface))
        self.assertEquals(Everything.TestInterface.__gtype__.name, 'EverythingTestInterface')

        # Test instanciation.
        self.assertRaises(NotImplementedError, Everything.TestInterface)


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(TestGIEverything)
    unittest.TextTestRunner(verbosity=2).run(suite)

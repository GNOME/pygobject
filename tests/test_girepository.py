'''
-*- Mode: Python; py-indent-offset: 4 -*-

PyBank unit tests.

Copyright (C) 2009  Mark Lee <gnome@lazymalevolence.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

----

Partially based on gjs's unit tests.
'''

import unittest

import time
import gobject

import girepository
import GLib
import Everything

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

    def testInt(self):
        self.assertEqual(3, Everything.test_int(3))
        self.assertEqual(-3, Everything.test_int(-3))
        self.assertRaises(TypeError, Everything.test_int, 'a')

    def testInt8(self):
        self.assertEqual(3, Everything.test_int8(3))
        self.assertEqual(-3, Everything.test_int8(-3))
        self.assertEqual(INT8_MIN, Everything.test_int8(INT8_MIN))
        self.assertEqual(INT8_MAX, Everything.test_int8(INT8_MAX))
        self.assertRaises(TypeError, Everything.test_int, 'a')

    def testInt16(self):
        self.assertEqual(3, Everything.test_int16(3))
        self.assertEqual(-3, Everything.test_int16(-3))
        self.assertEqual(INT16_MIN, Everything.test_int16(INT16_MIN))
        self.assertEqual(INT16_MAX, Everything.test_int16(INT16_MAX))
        self.assertRaises(TypeError, Everything.test_int, 'a')

    def testInt32(self):
        self.assertEqual(3, Everything.test_int32(3))
        self.assertEqual(-3, Everything.test_int32(-3))
        self.assertEqual(INT32_MIN, Everything.test_int32(INT32_MIN))
        self.assertEqual(INT32_MAX, Everything.test_int32(INT32_MAX))
        self.assertRaises(TypeError, Everything.test_int, 'a')

    def testInt64(self):
        self.assertEqual(3, Everything.test_int64(3))
        self.assertEqual(-3, Everything.test_int64(-3))
        self.assertEqual(INT64_MIN, Everything.test_int64(INT64_MIN))
        self.assertEqual(INT64_MAX, Everything.test_int64(INT64_MAX))
        self.assertRaises(TypeError, Everything.test_int, 'a')

    def testUInt(self):
        self.assertEqual(3, Everything.test_uint(3))
        self.assertRaises(TypeError, Everything.test_uint, -3)

    def testUInt8(self):
        self.assertEqual(3, Everything.test_uint8(3))
        self.assertEqual(UINT8_MAX, Everything.test_uint8(UINT8_MAX))
        self.assertRaises(TypeError, Everything.test_uint8, -3)

    def testUInt16(self):
        self.assertEqual(3, Everything.test_uint16(3))
        self.assertEqual(UINT16_MAX, Everything.test_uint16(UINT16_MAX))
        self.assertRaises(TypeError, Everything.test_uint16, -3)

    def testUInt32(self):
        self.assertEqual(3, Everything.test_uint32(3))
        self.assertEqual(UINT32_MAX, Everything.test_uint32(UINT32_MAX))
        self.assertRaises(TypeError, Everything.test_uint32, -3)

    def testUInt64(self):
        self.assertEqual(3, Everything.test_uint64(3))
        self.assertEqual(UINT64_MAX, Everything.test_uint64(UINT64_MAX))
        self.assertRaises(TypeError, Everything.test_uint64, -3)

    def testLong(self):
        self.assertEqual(3, Everything.test_long(3))
        self.assertEqual(-3, Everything.test_long(-3))
        self.assertRaises(TypeError, Everything.test_long, 'a')

    def testULong(self):
        self.assertEqual(3, Everything.test_ulong(3))
        self.assertRaises(TypeError, Everything.test_ulong, -3)

    def testSSize(self):
        self.assertEqual(3, Everything.test_ssize(3))
        self.assertEqual(-3, Everything.test_ssize(-3))
        self.assertRaises(TypeError, Everything.test_ssize, 'a')

    def testSize(self):
        self.assertEqual(3, Everything.test_size(3))
        self.assertRaises(TypeError, Everything.test_size, -3)

    def testFloat(self):
        self.assertAlmostEqual(3.14, Everything.test_float(3.14), 6)
        self.assertAlmostEqual(-3.14, Everything.test_float(-3.14), 6)
        self.assertRaises(TypeError, Everything.test_float, 'a')

    def testDouble(self):
        self.assertAlmostEqual(3.14, Everything.test_double(3.14))
        self.assertAlmostEqual(-3.14, Everything.test_double(-3.14))
        self.assertRaises(TypeError, Everything.test_double, 'a')

    def testTimeT(self):
        now = time.time()
        bounced = Everything.test_timet(now)
        self.assertEquals(now.tm_year, bounced.tm_year)
        self.assertEquals(now.tm_year, bounced.tm_mon)
        self.assertEquals(now.tm_year, bounced.tm_mday)
        self.assertEquals(now.tm_year, bounced.tm_hour)
        self.assertEquals(now.tm_year, bounced.tm_min)
        self.assertEquals(now.tm_year, bounced.tm_sec)
        self.assertEquals(now.tm_year, bounced.tm_wday)
        self.assertEquals(now.tm_year, bounced.tm_yday)
        self.assertEquals(now.tm_year, bounced.tm_isdst)

    def testGType(self):
        self.assertEqual(gobject.TYPE_INT, Everything.test_gtype(gobject.TYPE_INT))
        self.assertRaises(TypeError, Everything.test_gtype, 'a')

    def testFilenameReturn(self):
        filenames = Everything.test_filename_return()
        self.assertEquals(2, len(filenames))
        self.assertEquals('\u00e5\u00e4\u00f6', filenames[0])
        self.assertEquals('/etc/fstab', filenames[1])

    def testStrv(self):
        self.assertTrue(Everything.test_strv_in(('1', '2', '3')))
        self.assertTrue(Everything.test_strv_in(['1', '2', '3'])) # XXX valid?
        self.assertRaises(TypeError, Everything.test_strv_in(('1', 2, 3)))
        self.assertEquals((1, 2, 3), Everything.test_strv_out())

    def testGList(self):
        retval = Everything.test_glist_nothing_return()
        self.assertTrue(isinstance(retval, list))
        self.assertEquals(retval[0], '1')
        self.assertEquals(retval[1], '2')
        self.assertEquals(retval[2], '3')

    def testGSList(self):
        retval = Everything.test_gslist_nothing_return()
        self.assertTrue(isinstance(retval, list))
        self.assertEquals(retval[0], '1')
        self.assertEquals(retval[1], '2')
        self.assertEquals(retval[2], '3')

# XXX Currently causes a segfault.
#    def testClosure(self):
#        def someCallback():
#            return 3
#        self.assertEquals(3, Everything.test_closure(someCallback))
#        someLambda = lambda: 3
#        self.assertEquals(3, Everything.test_closure(someLambda))

#    def testClosureOneArg(self):
#        def someCallback(arg):
#            return arg
#        self.assertEquals(3, Everything.test_closure_one_arg(someCallback, 3))
#        someLambda = lambda x: x
#        self.assertEquals(3, Everything.test_closure_one_arg(someLambda, 3))

#    def testIntValueArg(self):
#        i = Everything.test_int_value_arg(42)
#        self.assertEquals(42, i)

    def testValueReturn(self):
        i = Everything.test_value_return(42)
        self.assertEquals(42, i)

    def testEnum(self):
        self.assertEqual('value1', Everything.test_enum_param(Everything.TestEnum.VALUE1))
        self.assertEqual('value2', Everything.test_enum_param(Everything.TestEnum.VALUE2))
        self.assertEqual('value3', Everything.test_enum_param(Everything.TestEnum.VALUE3))

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

    def createStructA(self):
        a = Everything.TestStructA()
        a.some_int = 3
        a.some_int8 = 1
        a.some_double = 4.15
        a.some_enum= Everything.TestEnum.VALUE3

        self.assertEquals(a.some_int, 3)
        self.assertEquals(a.some_int8, 1)
        self.assertEquals(a.some_double, 4.15)
        self.assertEquals(a.some_enum, Everything.TestEnum.VALUE3)

        return a

    def testStructA(self):
        a = self.createStructA()
        a_out = a.clone()
        self.assertEquals(a, a_out)

    def testStructB(self):
        b = Everything.TestStructB()
        b.some_int8 = 3
        b.nested_a = self.createStructA()
        b_out = b.clone()
        self.assertEquals(b, b_out)
        self.assertEquals(b.nested_a, b_out.nested_a)

    def testInterface(self):
        self.assertTrue(issubclass(Everything.TestInterface, gobject.GInterface))
        self.assertRaises(NotImplementedError, Everything.TestInterface)
        self.assertEquals(Everything.TestInterface.__gtype__.name, 'EverythingTestInterface')

    def testSubclass(self):
        class TestSubclass(Everything.TestObj):
            def __init__(self):
                Everything.TestObj.__init__(self, 'foo')
        s = TestSubclass()
        self.assertEquals(s.do_matrix('matrix'), 42)

    def testArrayOut(self):
        b, n_ints, ints = Everything.test_array_int_full_out2()
        self.assertEquals(b, True)
        self.assertEquals(n_ints, 5)
        self.assertEquals(ints, [1, 2, 3, 4, 5])

if __name__ == '__main__':
    unittest.main()

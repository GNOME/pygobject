# -*- Mode: Python -*-

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

utf8_const = 'const \xe2\x99\xa5 utf8'
utf8_nonconst = 'nonconst \xe2\x99\xa5 utf8'

def createStructA():
    a = Everything.TestStructA()
    a.some_int = 3
    a.some_int8 = 1
    a.some_double = 4.15
    a.some_enum= Everything.TestEnum.VALUE3
    return a

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

# FIXME
#======================================================================
#ERROR: testTimeT (__main__.TestGIEverything)
#----------------------------------------------------------------------
#Traceback (most recent call last):
#  File "test_girepository.py", line 193, in testTimeT
#    bounced = Everything.test_timet(now)
#  File "/opt/gnome-introspection/lib64/python2.5/site-packages/gtk-2.0/girepository/btypes.py", line 124, in __call__
#    self.type_check(name, value, argType)
#  File "/opt/gnome-introspection/lib64/python2.5/site-packages/gtk-2.0/girepository/btypes.py", line 97, in type_check
#    raise NotImplementedError('type checking for tag %d' % tag)
#NotImplementedError: type checking for tag 18
#    def testTimeT(self):
#        now = time.time()
#        bounced = Everything.test_timet(now)
#        self.assertEquals(now.tm_year, bounced.tm_year)
#        self.assertEquals(now.tm_year, bounced.tm_mon)
#        self.assertEquals(now.tm_year, bounced.tm_mday)
#        self.assertEquals(now.tm_year, bounced.tm_hour)
#        self.assertEquals(now.tm_year, bounced.tm_min)
#        self.assertEquals(now.tm_year, bounced.tm_sec)
#        self.assertEquals(now.tm_year, bounced.tm_wday)
#        self.assertEquals(now.tm_year, bounced.tm_yday)
#        self.assertEquals(now.tm_year, bounced.tm_isdst)

    def testGType(self):
        self.assertEqual(gobject.TYPE_INT, Everything.test_gtype(gobject.TYPE_INT))
        self.assertEqual(Everything.TestObj.__gtype__, Everything.test_gtype(Everything.TestObj))
        self.assertRaises(TypeError, Everything.test_gtype, 'a')


# UTF-8

    def testUtf8ConstReturn(self):
        self.assertEquals(utf8_const, Everything.test_utf8_const_return())

    def testUtf8NonconstReturn(self):
        self.assertEquals(utf8_nonconst, Everything.test_utf8_nonconst_return())

    def testUtf8NonconstIn(self):
        Everything.test_utf8_nonconst_in(utf8_nonconst)

    def testUtf8ConstIn(self):
        Everything.test_utf8_const_in(utf8_const)

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


# FIXME
# ======================================================================
# ERROR: testStrv (__main__.TestGIEverything)
# ----------------------------------------------------------------------
# Traceback (most recent call last):
#   File "test_girepository.py", line 179, in testStrv
#     self.assertTrue(Everything.test_strv_in(('1', '2', '3')))
#   File "/opt/gnome-introspection/lib64/python2.5/site-packages/gtk-2.0/girepository/btypes.py", line 124, in __call__
#     self.type_check(name, value, argType)
#   File "/opt/gnome-introspection/lib64/python2.5/site-packages/gtk-2.0/girepository/btypes.py", line 89, in type_check
#     raise TypeError("Must pass None for arrays currently")
# TypeError: Must pass None for arrays currently
#    def testStrv(self):
#        self.assertTrue(Everything.test_strv_in(('1', '2', '3')))
#        self.assertTrue(Everything.test_strv_in(['1', '2', '3'])) # XXX valid?
#        self.assertRaises(TypeError, Everything.test_strv_in(('1', 2, 3)))
#        self.assertEquals((1, 2, 3), Everything.test_strv_out())

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

# FIXME
# ======================================================================
# ERROR: testValueReturn (__main__.TestGIEverything)
# ----------------------------------------------------------------------
# Traceback (most recent call last):
#   File "test_girepository.py", line 219, in testValueReturn
#     self.assertEquals(42, i)
#   File "/opt/gnome-introspection/lib64/python2.5/unittest.py", line 332, in failUnlessEqual
#     if not first == second:
#   File "/opt/gnome-introspection/lib64/python2.5/site-packages/gtk-2.0/girepository/btypes.py", line 297, in __eq__
#     if getattr(self, field.getName()) != getattr(other, field.getName()):
# AttributeError: 'int' object has no attribute 'g_type'
# 	def testValueReturn(self):
#        i = Everything.test_value_return(42)
#        self.assertEquals(42, i)

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

    def testStructA(self):
        a = createStructA()
        self.assertEquals(a.some_int, 3)
        self.assertEquals(a.some_int8, 1)
        self.assertEquals(a.some_double, 4.15)
        self.assertEquals(a.some_enum, Everything.TestEnum.VALUE3)

        a_out = Everything.TestStructA()
        a.clone(a_out)
        self.assertEquals(a, a_out)

    def testStructB(self):
        b = Everything.TestStructB()
        b.some_int8 = 3
        a = createStructA()
        b.nested_a = a
        self.assertEquals(a, b.nested_a)

        b_out = Everything.TestStructB()
        b.clone(b_out)
        self.assertEquals(b, b_out)


# Plain-old-data boxed types

    def testSimpleBoxedA(self):
        a = Everything.TestSimpleBoxedA()
        a.some_int = 42
        a.some_int8 = 7
        a.some_double = 3.14
        a.some_enum = Everything.TestEnum.VALUE3

        self.assertEquals(42, a.some_int)
        self.assertEquals(7, a.some_int8)
        self.assertAlmostEquals(3.14, a.some_double)
        self.assertEquals(Everything.TestEnum.VALUE3, a.some_enum)

        self.assertRaises(TypeError, setattr, a, 'some_int', 'a')
        self.assertRaises(ValueError, setattr, a, 'some_int8', INT8_MIN-1)
        self.assertRaises(ValueError, setattr, a, 'some_int8', INT8_MAX+1)

        a_out = a.copy()
        self.assertTrue(a.equals(a_out))
        self.assertEquals(a, a_out)

        a_const = Everything.test_simple_boxed_a_const_return()
        self.assertEquals(5, a_const.some_int)
        self.assertEquals(6, a_const.some_int8)
        self.assertAlmostEquals(7.0, a_const.some_double)

    def testSimpleBoxedB(self):
        a_const = Everything.test_simple_boxed_a_const_return()

        b = Everything.TestSimpleBoxedB()
        b.some_int = 42
        b.nested_a = a_const.copy()

        self.assertEquals(a_const, b.nested_a)

        self.assertRaises(TypeError, setattr, b, 'nested_a', 'a')

        b_out = b.copy()
        self.assertEquals(b, b_out)


    def testInterface(self):
        self.assertTrue(issubclass(Everything.TestInterface, gobject.GInterface))
        self.assertRaises(NotImplementedError, Everything.TestInterface)
        self.assertEquals(Everything.TestInterface.__gtype__.name, 'EverythingTestInterface')

    def testConstructor(self):
        self.assertTrue(isinstance(Everything.TestObj("foo"), Everything.TestObj))
        self.assertRaises(TypeError, Everything.TestObj)
        self.assertRaises(TypeError, Everything.TestObj.__init__, None, 'foo')
        self.assertRaises(TypeError, Everything.TestObj, 'foo', 'bar')
        self.assertRaises(TypeError, Everything.TestObj, 42)

    def testlInstanceMethod(self):
        o = Everything.TestObj('foo')
        self.assertEquals(42, o.do_matrix('matrix'))
        self.assertRaises(TypeError, o.do_matrix)
        self.assertRaises(TypeError, o.do_matrix, 'matrix', 'foo')
        self.assertRaises(TypeError, Everything.TestObj.do_matrix, 'matrix')
        self.assertRaises(TypeError, Everything.TestObj.do_matrix, None, 'matrix')

    def testStaticMethod(self):
        self.assertEquals(42, Everything.TestObj.static_method(42))
        self.assertRaises(TypeError, Everything.TestObj.static_method)
        self.assertRaises(TypeError, Everything.TestObj.static_method, 'foo')
        self.assertRaises(TypeError, Everything.TestObj.static_method, 'foo', 'bar')
        o = Everything.TestObj('foo')
        self.assertEquals(42, o.static_method(42))

    def testSubclass(self):
        class TestSubclass(Everything.TestObj):
            def __init__(self):
                Everything.TestObj.__init__(self, 'foo')
        s = TestSubclass()
        self.assertEquals(s.do_matrix('matrix'), 42)

# FIXME
# ======================================================================
# ERROR: testArrayOut (__main__.TestGIEverything)
# ----------------------------------------------------------------------
# Traceback (most recent call last):
#   File "test_girepository.py", line 282, in testArrayOut
# 	b, n_ints, ints = Everything.test_array_int_full_out2()
#   File "/opt/gnome-introspection/lib64/python2.5/site-packages/gtk-2.0/girepository/module.py", line 56, in __getattr__
# 	self.__class__.__name__, name))
# AttributeError: 'DynamicModule' object has no attribute 'test_array_int_full_out2'
#	def testArrayOut(self):
#	    b, n_ints, ints = Everything.test_array_int_full_out2()
#	    self.assertEquals(b, True)
#	    self.assertEquals(n_ints, 5)
#	    self.assertEquals(ints, [1, 2, 3, 4, 5])

if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(TestGIEverything)
    unittest.TextTestRunner(verbosity=2).run(suite)

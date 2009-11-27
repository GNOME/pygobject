# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import pygtk
pygtk.require("2.0")

import unittest
import gobject

from datetime import datetime

from gi.repository import TestGI


CONSTANT_UTF8 = "const \xe2\x99\xa5 utf8"
CONSTANT_NUMBER = 42


class Number(object):

    def __init__(self, value):
        self.value = value

    def __int__(self):
        return int(self.value)

    def __float__(self):
        return float(self.value)


class Sequence(object):

    def __init__(self, sequence):
        self.sequence = sequence

    def __len__(self):
        return len(self.sequence)

    def __getitem__(self, key):
        return self.sequence[key]


class TestConstant(unittest.TestCase):

# Blocked by https://bugzilla.gnome.org/show_bug.cgi?id=595773
#    def test_constant_utf8(self):
#        self.assertEquals(CONSTANT_UTF8, TestGI.CONSTANT_UTF8)

    def test_constant_number(self):
        self.assertEquals(CONSTANT_NUMBER, TestGI.CONSTANT_NUMBER)


class TestBoolean(unittest.TestCase):

    def test_boolean_return(self):
        self.assertEquals(True, TestGI.boolean_return_true())
        self.assertEquals(False, TestGI.boolean_return_false())

    def test_boolean_return_ptr(self):
        self.assertEquals(True, TestGI.boolean_return_ptr_true())
        self.assertEquals(False, TestGI.boolean_return_ptr_false())

    def test_boolean_in(self):
        TestGI.boolean_in_true(True)
        TestGI.boolean_in_false(False)

        TestGI.boolean_in_true(1)
        TestGI.boolean_in_false(0)

    def test_boolean_in_ptr(self):
        TestGI.boolean_in_ptr_true(True)
        TestGI.boolean_in_ptr_false(False)
        TestGI.boolean_in_ptr_false(None)

    def test_boolean_out(self):
        self.assertEquals(True, TestGI.boolean_out_true())
        self.assertEquals(False, TestGI.boolean_out_false())

    def test_boolean_inout(self):
        self.assertEquals(False, TestGI.boolean_inout_true_false(True))
        self.assertEquals(True, TestGI.boolean_inout_false_true(False))


class TestInt8(unittest.TestCase):

    MAX = 2 ** 7 - 1
    MIN = - (2 ** 7)

    def test_int8_return(self):
        self.assertEquals(self.MAX, TestGI.int8_return_max())
        self.assertEquals(self.MIN, TestGI.int8_return_min())

    def test_int8_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.int8_return_ptr_max())
        self.assertEquals(self.MIN, TestGI.int8_return_ptr_min())

    def test_int8_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        TestGI.int8_in_max(max)
        TestGI.int8_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(ValueError, TestGI.int8_in_max, max)
        self.assertRaises(ValueError, TestGI.int8_in_min, min)

        self.assertRaises(TypeError, TestGI.int8_in_max, "self.MAX")

    def test_int8_in_ptr(self):
        TestGI.int8_in_ptr_max(Number(self.MAX))
        TestGI.int8_in_ptr_min(Number(self.MIN))
        self.assertRaises(TypeError, TestGI.int8_in_ptr_max, None)

    def test_int8_out(self):
        self.assertEquals(self.MAX, TestGI.int8_out_max())
        self.assertEquals(self.MIN, TestGI.int8_out_min())

    def test_int8_inout(self):
        self.assertEquals(self.MIN, TestGI.int8_inout_max_min(Number(self.MAX)))
        self.assertEquals(self.MAX, TestGI.int8_inout_min_max(Number(self.MIN)))


class TestUInt8(unittest.TestCase):

    MAX = 2 ** 8 - 1

    def test_uint8_return(self):
        self.assertEquals(self.MAX, TestGI.uint8_return())

# Blocked by https://bugzilla.gnome.org/show_bug.cgi?id=596420
#    def test_uint8_return_ptr(self):
#        self.assertEquals(self.MAX, TestGI.uint8_return_ptr())

    def test_uint8_in(self):
        number = Number(self.MAX)

        TestGI.uint8_in(number)

        number.value += 1

        self.assertRaises(ValueError, TestGI.uint8_in, number)
        self.assertRaises(ValueError, TestGI.uint8_in, Number(-1))

        self.assertRaises(TypeError, TestGI.uint8_in, "self.MAX")

    def test_uint8_out(self):
        self.assertEquals(self.MAX, TestGI.uint8_out())

    def test_uint8_inout(self):
        self.assertEquals(0, TestGI.uint8_inout(Number(self.MAX)))


class TestInt16(unittest.TestCase):

    MAX = 2 ** 15 - 1
    MIN = - (2 ** 15)

    def test_int16_return(self):
        self.assertEquals(self.MAX, TestGI.int16_return_max())
        self.assertEquals(self.MIN, TestGI.int16_return_min())

    def test_int16_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.int16_return_ptr_max())
        self.assertEquals(self.MIN, TestGI.int16_return_ptr_min())

    def test_int16_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        TestGI.int16_in_max(max)
        TestGI.int16_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(ValueError, TestGI.int16_in_max, max)
        self.assertRaises(ValueError, TestGI.int16_in_min, min)

        self.assertRaises(TypeError, TestGI.int16_in_max, "self.MAX")

    def test_int16_in_ptr(self):
        TestGI.int16_in_ptr_max(Number(self.MAX))
        TestGI.int16_in_ptr_min(Number(self.MIN))
        self.assertRaises(TypeError, TestGI.int16_in_ptr_max, None)

    def test_int16_out(self):
        self.assertEquals(self.MAX, TestGI.int16_out_max())
        self.assertEquals(self.MIN, TestGI.int16_out_min())

    def test_int16_inout(self):
        self.assertEquals(self.MIN, TestGI.int16_inout_max_min(Number(self.MAX)))
        self.assertEquals(self.MAX, TestGI.int16_inout_min_max(Number(self.MIN)))


class TestUInt16(unittest.TestCase):

    MAX = 2 ** 16 - 1

    def test_uint16_return(self):
        self.assertEquals(self.MAX, TestGI.uint16_return())

    def test_uint16_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.uint16_return_ptr())

    def test_uint16_in(self):
        number = Number(self.MAX)

        TestGI.uint16_in(number)

        number.value += 1

        self.assertRaises(ValueError, TestGI.uint16_in, number)
        self.assertRaises(ValueError, TestGI.uint16_in, Number(-1))

        self.assertRaises(TypeError, TestGI.uint16_in, "self.MAX")

    def test_uint16_out(self):
        self.assertEquals(self.MAX, TestGI.uint16_out())

    def test_uint16_inout(self):
        self.assertEquals(0, TestGI.uint16_inout(Number(self.MAX)))


class TestInt32(unittest.TestCase):

    MAX = 2 ** 31 - 1
    MIN = - (2 ** 31)

    def test_int32_return(self):
        self.assertEquals(self.MAX, TestGI.int32_return_max())
        self.assertEquals(self.MIN, TestGI.int32_return_min())

    def test_int32_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.int32_return_ptr_max())
        self.assertEquals(self.MIN, TestGI.int32_return_ptr_min())

    def test_int32_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        TestGI.int32_in_max(max)
        TestGI.int32_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(ValueError, TestGI.int32_in_max, max)
        self.assertRaises(ValueError, TestGI.int32_in_min, min)

        self.assertRaises(TypeError, TestGI.int32_in_max, "self.MAX")

    def test_int32_in_ptr(self):
        TestGI.int32_in_ptr_max(Number(self.MAX))
        TestGI.int32_in_ptr_min(Number(self.MIN))
        self.assertRaises(TypeError, TestGI.int32_in_ptr_max, None)

    def test_int32_out(self):
        self.assertEquals(self.MAX, TestGI.int32_out_max())
        self.assertEquals(self.MIN, TestGI.int32_out_min())

    def test_int32_inout(self):
        self.assertEquals(self.MIN, TestGI.int32_inout_max_min(Number(self.MAX)))
        self.assertEquals(self.MAX, TestGI.int32_inout_min_max(Number(self.MIN)))


class TestUInt32(unittest.TestCase):

    MAX = 2 ** 32 - 1

    def test_uint32_return(self):
        self.assertEquals(self.MAX, TestGI.uint32_return())

    def test_uint32_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.uint32_return_ptr())

    def test_uint32_in(self):
        number = Number(self.MAX)

        TestGI.uint32_in(number)

        number.value += 1

        self.assertRaises(ValueError, TestGI.uint32_in, number)
        self.assertRaises(ValueError, TestGI.uint32_in, Number(-1))

        self.assertRaises(TypeError, TestGI.uint32_in, "self.MAX")

    def test_uint32_out(self):
        self.assertEquals(self.MAX, TestGI.uint32_out())

    def test_uint32_inout(self):
        self.assertEquals(0, TestGI.uint32_inout(Number(self.MAX)))


class TestInt64(unittest.TestCase):

    MAX = 2 ** 63 - 1
    MIN = - (2 ** 63)

    def test_int64_return(self):
        self.assertEquals(self.MAX, TestGI.int64_return_max())
        self.assertEquals(self.MIN, TestGI.int64_return_min())

    def test_int64_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.int64_return_ptr_max())
        self.assertEquals(self.MIN, TestGI.int64_return_ptr_min())

    def test_int64_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        TestGI.int64_in_max(max)
        TestGI.int64_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(ValueError, TestGI.int64_in_max, max)
        self.assertRaises(ValueError, TestGI.int64_in_min, min)

        self.assertRaises(TypeError, TestGI.int64_in_max, "self.MAX")

    def test_int64_in_ptr(self):
        TestGI.int64_in_ptr_max(Number(self.MAX))
        TestGI.int64_in_ptr_min(Number(self.MIN))
        self.assertRaises(TypeError, TestGI.int64_in_ptr_max, None)

    def test_int64_out(self):
        self.assertEquals(self.MAX, TestGI.int64_out_max())
        self.assertEquals(self.MIN, TestGI.int64_out_min())

    def test_int64_inout(self):
        self.assertEquals(self.MIN, TestGI.int64_inout_max_min(Number(self.MAX)))
        self.assertEquals(self.MAX, TestGI.int64_inout_min_max(Number(self.MIN)))


class TestUInt64(unittest.TestCase):

    MAX = 2 ** 64 - 1

    def test_uint64_return(self):
        self.assertEquals(self.MAX, TestGI.uint64_return())

    def test_uint64_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.uint64_return_ptr())

    def test_uint64_in(self):
        number = Number(self.MAX)

        TestGI.uint64_in(number)

        number.value += 1

        self.assertRaises(ValueError, TestGI.uint64_in, number)
        self.assertRaises(ValueError, TestGI.uint64_in, Number(-1))

        self.assertRaises(TypeError, TestGI.uint64_in, "self.MAX")

    def test_uint64_out(self):
        self.assertEquals(self.MAX, TestGI.uint64_out())

    def test_uint64_inout(self):
        self.assertEquals(0, TestGI.uint64_inout(Number(self.MAX)))


class TestShort(unittest.TestCase):

    MAX = gobject.constants.G_MAXSHORT
    MIN = gobject.constants.G_MINSHORT

    def test_short_return(self):
        self.assertEquals(self.MAX, TestGI.short_return_max())
        self.assertEquals(self.MIN, TestGI.short_return_min())

    def test_short_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.short_return_ptr_max())
        self.assertEquals(self.MIN, TestGI.short_return_ptr_min())

    def test_short_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        TestGI.short_in_max(max)
        TestGI.short_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(ValueError, TestGI.short_in_max, max)
        self.assertRaises(ValueError, TestGI.short_in_min, min)

        self.assertRaises(TypeError, TestGI.short_in_max, "self.MAX")

    def test_short_in_ptr(self):
        TestGI.short_in_ptr_max(Number(self.MAX))
        TestGI.short_in_ptr_min(Number(self.MIN))
        self.assertRaises(TypeError, TestGI.short_in_ptr_max, None)

    def test_short_out(self):
        self.assertEquals(self.MAX, TestGI.short_out_max())
        self.assertEquals(self.MIN, TestGI.short_out_min())

    def test_short_inout(self):
        self.assertEquals(self.MIN, TestGI.short_inout_max_min(Number(self.MAX)))
        self.assertEquals(self.MAX, TestGI.short_inout_min_max(Number(self.MIN)))


class TestUShort(unittest.TestCase):

    MAX = gobject.constants.G_MAXUSHORT

    def test_ushort_return(self):
        self.assertEquals(self.MAX, TestGI.ushort_return())

    def test_ushort_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.ushort_return_ptr())

    def test_ushort_in(self):
        number = Number(self.MAX)

        TestGI.ushort_in(number)

        number.value += 1

        self.assertRaises(ValueError, TestGI.ushort_in, number)
        self.assertRaises(ValueError, TestGI.ushort_in, Number(-1))

        self.assertRaises(TypeError, TestGI.ushort_in, "self.MAX")

    def test_ushort_out(self):
        self.assertEquals(self.MAX, TestGI.ushort_out())

    def test_ushort_inout(self):
        self.assertEquals(0, TestGI.ushort_inout(Number(self.MAX)))


class TestInt(unittest.TestCase):

    MAX = gobject.constants.G_MAXINT
    MIN = gobject.constants.G_MININT

    def test_int_return(self):
        self.assertEquals(self.MAX, TestGI.int_return_max())
        self.assertEquals(self.MIN, TestGI.int_return_min())

    def test_int_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.int_return_ptr_max())
        self.assertEquals(self.MIN, TestGI.int_return_ptr_min())

    def test_int_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        TestGI.int_in_max(max)
        TestGI.int_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(ValueError, TestGI.int_in_max, max)
        self.assertRaises(ValueError, TestGI.int_in_min, min)

        self.assertRaises(TypeError, TestGI.int_in_max, "self.MAX")

    def test_int_in_ptr(self):
        TestGI.int_in_ptr_max(Number(self.MAX))
        TestGI.int_in_ptr_min(Number(self.MIN))
        self.assertRaises(TypeError, TestGI.int_in_ptr_max, None)

    def test_int_out(self):
        self.assertEquals(self.MAX, TestGI.int_out_max())
        self.assertEquals(self.MIN, TestGI.int_out_min())

    def test_int_inout(self):
        self.assertEquals(self.MIN, TestGI.int_inout_max_min(Number(self.MAX)))
        self.assertEquals(self.MAX, TestGI.int_inout_min_max(Number(self.MIN)))


class TestUInt(unittest.TestCase):

    MAX = gobject.constants.G_MAXUINT

    def test_uint_return(self):
        self.assertEquals(self.MAX, TestGI.uint_return())

    def test_uint_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.uint_return_ptr())

    def test_uint_in(self):
        number = Number(self.MAX)

        TestGI.uint_in(number)

        number.value += 1

        self.assertRaises(ValueError, TestGI.uint_in, number)
        self.assertRaises(ValueError, TestGI.uint_in, Number(-1))

        self.assertRaises(TypeError, TestGI.uint_in, "self.MAX")

    def test_uint_out(self):
        self.assertEquals(self.MAX, TestGI.uint_out())

    def test_uint_inout(self):
        self.assertEquals(0, TestGI.uint_inout(Number(self.MAX)))


class TestLong(unittest.TestCase):

    MAX = gobject.constants.G_MAXLONG
    MIN = gobject.constants.G_MINLONG

    def test_long_return(self):
        self.assertEquals(self.MAX, TestGI.long_return_max())
        self.assertEquals(self.MIN, TestGI.long_return_min())

    def test_long_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.long_return_ptr_max())
        self.assertEquals(self.MIN, TestGI.long_return_ptr_min())

    def test_long_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        TestGI.long_in_max(max)
        TestGI.long_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(ValueError, TestGI.long_in_max, max)
        self.assertRaises(ValueError, TestGI.long_in_min, min)

        self.assertRaises(TypeError, TestGI.long_in_max, "self.MAX")

    def test_long_in_ptr(self):
        TestGI.long_in_ptr_max(Number(self.MAX))
        TestGI.long_in_ptr_min(Number(self.MIN))
        self.assertRaises(TypeError, TestGI.long_in_ptr_max, None)

    def test_long_out(self):
        self.assertEquals(self.MAX, TestGI.long_out_max())
        self.assertEquals(self.MIN, TestGI.long_out_min())

    def test_long_inout(self):
        self.assertEquals(self.MIN, TestGI.long_inout_max_min(Number(self.MAX)))
        self.assertEquals(self.MAX, TestGI.long_inout_min_max(Number(self.MIN)))


class TestULong(unittest.TestCase):

    MAX = gobject.constants.G_MAXULONG

    def test_ulong_return(self):
        self.assertEquals(self.MAX, TestGI.ulong_return())

    def test_ulong_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.ulong_return_ptr())

    def test_ulong_in(self):
        number = Number(self.MAX)

        TestGI.ulong_in(number)

        number.value += 1

        self.assertRaises(ValueError, TestGI.ulong_in, number)
        self.assertRaises(ValueError, TestGI.ulong_in, Number(-1))

        self.assertRaises(TypeError, TestGI.ulong_in, "self.MAX")

    def test_ulong_out(self):
        self.assertEquals(self.MAX, TestGI.ulong_out())

    def test_ulong_inout(self):
        self.assertEquals(0, TestGI.ulong_inout(Number(self.MAX)))


class TestSSize(unittest.TestCase):

    MAX = gobject.constants.G_MAXLONG
    MIN = gobject.constants.G_MINLONG

    def test_ssize_return(self):
        self.assertEquals(self.MAX, TestGI.ssize_return_max())
        self.assertEquals(self.MIN, TestGI.ssize_return_min())

    def test_ssize_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.ssize_return_ptr_max())
        self.assertEquals(self.MIN, TestGI.ssize_return_ptr_min())

    def test_ssize_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        TestGI.ssize_in_max(max)
        TestGI.ssize_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(ValueError, TestGI.ssize_in_max, max)
        self.assertRaises(ValueError, TestGI.ssize_in_min, min)

        self.assertRaises(TypeError, TestGI.ssize_in_max, "self.MAX")

    def test_ssize_in_ptr(self):
        TestGI.ssize_in_ptr_max(Number(self.MAX))
        TestGI.ssize_in_ptr_min(Number(self.MIN))
        self.assertRaises(TypeError, TestGI.ssize_in_ptr_max, None)

    def test_ssize_out(self):
        self.assertEquals(self.MAX, TestGI.ssize_out_max())
        self.assertEquals(self.MIN, TestGI.ssize_out_min())

    def test_ssize_inout(self):
        self.assertEquals(self.MIN, TestGI.ssize_inout_max_min(Number(self.MAX)))
        self.assertEquals(self.MAX, TestGI.ssize_inout_min_max(Number(self.MIN)))


class TestSize(unittest.TestCase):

    MAX = gobject.constants.G_MAXULONG

    def test_size_return(self):
        self.assertEquals(self.MAX, TestGI.size_return())

    def test_size_return_ptr(self):
        self.assertEquals(self.MAX, TestGI.size_return_ptr())

    def test_size_in(self):
        number = Number(self.MAX)

        TestGI.size_in(number)

        number.value += 1

        self.assertRaises(ValueError, TestGI.size_in, number)
        self.assertRaises(ValueError, TestGI.size_in, Number(-1))

        self.assertRaises(TypeError, TestGI.size_in, "self.MAX")

    def test_size_out(self):
        self.assertEquals(self.MAX, TestGI.size_out())

    def test_size_inout(self):
        self.assertEquals(0, TestGI.size_inout(Number(self.MAX)))


class TestFloat(unittest.TestCase):

    MAX = gobject.constants.G_MAXFLOAT
    MIN = gobject.constants.G_MINFLOAT

    def test_float_return(self):
        self.assertAlmostEquals(self.MAX, TestGI.float_return())

    def test_float_return_ptr(self):
        self.assertAlmostEquals(self.MAX, TestGI.float_return_ptr())

    def test_float_in(self):
        TestGI.float_in(Number(self.MAX))

        self.assertRaises(TypeError, TestGI.float_in, "self.MAX")

    def test_float_in_ptr(self):
        TestGI.float_in_ptr(Number(self.MAX))
        self.assertRaises(TypeError, TestGI.float_in_ptr, None)

    def test_float_out(self):
        self.assertAlmostEquals(self.MAX, TestGI.float_out())

    def test_float_inout(self):
        self.assertAlmostEquals(self.MIN, TestGI.float_inout(Number(self.MAX)))


class TestDouble(unittest.TestCase):

    MAX = gobject.constants.G_MAXDOUBLE
    MIN = gobject.constants.G_MINDOUBLE

    def test_double_return(self):
        self.assertAlmostEquals(self.MAX, TestGI.double_return())

    def test_double_return_ptr(self):
        self.assertAlmostEquals(self.MAX, TestGI.double_return_ptr())

    def test_double_in(self):
        TestGI.double_in(Number(self.MAX))

        self.assertRaises(TypeError, TestGI.double_in, "self.MAX")

    def test_double_in_ptr(self):
        TestGI.double_in_ptr(Number(self.MAX))
        self.assertRaises(TypeError, TestGI.double_in_ptr, None)

    def test_double_out(self):
        self.assertAlmostEquals(self.MAX, TestGI.double_out())

    def test_double_inout(self):
        self.assertAlmostEquals(self.MIN, TestGI.double_inout(Number(self.MAX)))


class TestTimeT(unittest.TestCase):

    DATETIME = datetime.fromtimestamp(1234567890)

    def test_time_t_return(self):
        self.assertEquals(self.DATETIME, TestGI.time_t_return())

    def test_time_t_return_ptr(self):
        self.assertEquals(self.DATETIME, TestGI.time_t_return_ptr())

    def test_time_t_in(self):
        TestGI.time_t_in(self.DATETIME)

        self.assertRaises(TypeError, TestGI.time_t_in, "self.DATETIME")

    def test_time_t_in_ptr(self):
        TestGI.time_t_in_ptr(self.DATETIME)
        self.assertRaises(TypeError, TestGI.time_t_in_ptr, None)

    def test_time_t_out(self):
        self.assertEquals(self.DATETIME, TestGI.time_t_out())

    def test_time_t_inout(self):
        self.assertEquals(datetime.fromtimestamp(0), TestGI.time_t_inout(self.DATETIME))


class TestGType(unittest.TestCase):

    def test_gtype_return(self):
        self.assertEquals(gobject.TYPE_NONE, TestGI.gtype_return())

    def test_gtype_return_ptr(self):
        self.assertEquals(gobject.TYPE_NONE, TestGI.gtype_return_ptr())

    def test_gtype_in(self):
        TestGI.gtype_in(gobject.TYPE_NONE)

        self.assertRaises(TypeError, TestGI.gtype_in, "gobject.TYPE_NONE")

    def test_gtype_in_ptr(self):
        TestGI.gtype_in_ptr(gobject.TYPE_NONE)
        self.assertRaises(TypeError, TestGI.gtype_in_ptr, None)

    def test_gtype_out(self):
        self.assertEquals(gobject.TYPE_NONE, TestGI.gtype_out())

    def test_gtype_inout(self):
        self.assertEquals(gobject.TYPE_INT, TestGI.gtype_inout(gobject.TYPE_NONE))


class TestUtf8(unittest.TestCase):

    def test_utf8_none_return(self):
        self.assertEquals(CONSTANT_UTF8, TestGI.utf8_none_return())

    def test_utf8_full_return(self):
        self.assertEquals(CONSTANT_UTF8, TestGI.utf8_full_return())

    def test_utf8_none_in(self):
        TestGI.utf8_none_in(CONSTANT_UTF8)

        self.assertRaises(TypeError, TestGI.utf8_none_in, CONSTANT_NUMBER)
        self.assertRaises(TypeError, TestGI.utf8_none_in, None)

    def test_utf8_full_in(self):
        TestGI.utf8_full_in(CONSTANT_UTF8)

    def test_utf8_none_out(self):
        self.assertEquals(CONSTANT_UTF8, TestGI.utf8_none_out())

    def test_utf8_full_out(self):
        self.assertEquals(CONSTANT_UTF8, TestGI.utf8_full_out())

    def test_utf8_none_inout(self):
        self.assertEquals("", TestGI.utf8_none_inout(CONSTANT_UTF8))

    def test_utf8_full_inout(self):
        self.assertEquals("", TestGI.utf8_full_inout(CONSTANT_UTF8))


class TestArray(unittest.TestCase):

    def test_array_fixed_int_return(self):
        self.assertEquals((-1, 0, 1, 2), TestGI.array_fixed_int_return())

    def test_array_fixed_short_return(self):
        self.assertEquals((-1, 0, 1, 2), TestGI.array_fixed_short_return())

    def test_array_fixed_int_in(self):
        TestGI.array_fixed_int_in(Sequence((-1, 0, 1, 2)))

        self.assertRaises(TypeError, TestGI.array_fixed_int_in, Sequence((-1, '0', 1, 2)))

        self.assertRaises(TypeError, TestGI.array_fixed_int_in, 42)
        self.assertRaises(TypeError, TestGI.array_fixed_int_in, None)

    def test_array_fixed_short_in(self):
        TestGI.array_fixed_short_in(Sequence((-1, 0, 1, 2)))

    def test_array_fixed_out(self):
        self.assertEquals((-1, 0, 1, 2), TestGI.array_fixed_out())

    def test_array_fixed_inout(self):
        self.assertEquals((2, 1, 0, -1), TestGI.array_fixed_inout((-1, 0, 1, 2)))


    def test_array_return(self):
        self.assertEquals((-1, 0, 1, 2), TestGI.array_return())

    def test_array_in(self):
        TestGI.array_in(Sequence((-1, 0, 1, 2)))

    def test_array_out(self):
        self.assertEquals((-1, 0, 1, 2), TestGI.array_out())

    def test_array_inout(self):
        self.assertEquals((-2, -1, 0, 1, 2), TestGI.array_inout(Sequence((-1, 0, 1, 2))))


    def test_array_fixed_out_struct(self):
        struct1, struct2 = TestGI.array_fixed_out_struct()

        self.assertEquals(7, struct1.long_)
        self.assertEquals(6, struct1.int8)
        self.assertEquals(6, struct2.long_)
        self.assertEquals(7, struct2.int8)

    def test_array_zero_terminated_return(self):
        self.assertEquals(('0', '1', '2'), TestGI.array_zero_terminated_return())

    def test_array_zero_terminated_in(self):
        TestGI.array_zero_terminated_in(Sequence(('0', '1', '2')))

    def test_array_zero_terminated_out(self):
        self.assertEquals(('0', '1', '2'), TestGI.array_zero_terminated_out())

    def test_array_zero_terminated_out(self):
        self.assertEquals(('0', '1', '2'), TestGI.array_zero_terminated_out())

    def test_array_zero_terminated_inout(self):
        self.assertEquals(('-1', '0', '1', '2'), TestGI.array_zero_terminated_inout(('0', '1', '2')))


class TestGList(unittest.TestCase):

    def test_glist_int_none_return(self):
        self.assertEquals([-1, 0, 1, 2], TestGI.glist_int_none_return())

    def test_glist_utf8_none_return(self):
        self.assertEquals(['0', '1', '2'], TestGI.glist_utf8_none_return())

    def test_glist_utf8_container_return(self):
        self.assertEquals(['0', '1', '2'], TestGI.glist_utf8_container_return())

    def test_glist_utf8_full_return(self):
        self.assertEquals(['0', '1', '2'], TestGI.glist_utf8_full_return())

    def test_glist_int_none_in(self):
        TestGI.glist_int_none_in(Sequence((-1, 0, 1, 2)))

        self.assertRaises(TypeError, TestGI.glist_int_none_in, Sequence((-1, '0', 1, 2)))

        self.assertRaises(TypeError, TestGI.glist_int_none_in, 42)
        self.assertRaises(TypeError, TestGI.glist_int_none_in, None)

    def test_glist_utf8_none_in(self):
        TestGI.glist_utf8_none_in(Sequence(('0', '1', '2')))

    def test_glist_utf8_container_in(self):
        TestGI.glist_utf8_container_in(Sequence(('0', '1', '2')))

    def test_glist_utf8_full_in(self):
        TestGI.glist_utf8_full_in(Sequence(('0', '1', '2')))

    def test_glist_utf8_none_out(self):
        self.assertEquals(['0', '1', '2'], TestGI.glist_utf8_none_out())

    def test_glist_utf8_container_out(self):
        self.assertEquals(['0', '1', '2'], TestGI.glist_utf8_container_out())

    def test_glist_utf8_full_out(self):
        self.assertEquals(['0', '1', '2'], TestGI.glist_utf8_full_out())

    def test_glist_utf8_none_inout(self):
        self.assertEquals(['-2', '-1', '0', '1'], TestGI.glist_utf8_none_inout(Sequence(('0', '1', '2'))))

    def test_glist_utf8_container_inout(self):
        self.assertEquals(['-2', '-1','0', '1'], TestGI.glist_utf8_container_inout(('0', '1', '2')))

    def test_glist_utf8_full_inout(self):
        self.assertEquals(['-2', '-1','0', '1'], TestGI.glist_utf8_full_inout(('0', '1', '2')))


class TestGSList(unittest.TestCase):

    def test_gslist_int_none_return(self):
        self.assertEquals([-1, 0, 1, 2], TestGI.gslist_int_none_return())

    def test_gslist_utf8_none_return(self):
        self.assertEquals(['0', '1', '2'], TestGI.gslist_utf8_none_return())

    def test_gslist_utf8_container_return(self):
        self.assertEquals(['0', '1', '2'], TestGI.gslist_utf8_container_return())

    def test_gslist_utf8_full_return(self):
        self.assertEquals(['0', '1', '2'], TestGI.gslist_utf8_full_return())

    def test_gslist_int_none_in(self):
        TestGI.gslist_int_none_in(Sequence((-1, 0, 1, 2)))

        self.assertRaises(TypeError, TestGI.gslist_int_none_in, Sequence((-1, '0', 1, 2)))

        self.assertRaises(TypeError, TestGI.gslist_int_none_in, 42)
        self.assertRaises(TypeError, TestGI.gslist_int_none_in, None)

    def test_gslist_utf8_none_in(self):
        TestGI.gslist_utf8_none_in(Sequence(('0', '1', '2')))

    def test_gslist_utf8_container_in(self):
        TestGI.gslist_utf8_container_in(Sequence(('0', '1', '2')))

    def test_gslist_utf8_full_in(self):
        TestGI.gslist_utf8_full_in(Sequence(('0', '1', '2')))

    def test_gslist_utf8_none_out(self):
        self.assertEquals(['0', '1', '2'], TestGI.gslist_utf8_none_out())

    def test_gslist_utf8_container_out(self):
        self.assertEquals(['0', '1', '2'], TestGI.gslist_utf8_container_out())

    def test_gslist_utf8_full_out(self):
        self.assertEquals(['0', '1', '2'], TestGI.gslist_utf8_full_out())

    def test_gslist_utf8_none_inout(self):
        self.assertEquals(['-2', '-1', '0', '1'], TestGI.gslist_utf8_none_inout(Sequence(('0', '1', '2'))))

    def test_gslist_utf8_container_inout(self):
        self.assertEquals(['-2', '-1','0', '1'], TestGI.gslist_utf8_container_inout(('0', '1', '2')))

    def test_gslist_utf8_full_inout(self):
        self.assertEquals(['-2', '-1','0', '1'], TestGI.gslist_utf8_full_inout(('0', '1', '2')))


class TestGHashTable(unittest.TestCase):

    def test_ghashtable_int_none_return(self):
        self.assertEquals({-1: 1, 0: 0, 1: -1, 2: -2}, TestGI.ghashtable_int_none_return())

    def test_ghashtable_int_none_return(self):
        self.assertEquals({'-1': '1', '0': '0', '1': '-1', '2': '-2'}, TestGI.ghashtable_utf8_none_return())

    def test_ghashtable_int_container_return(self):
        self.assertEquals({'-1': '1', '0': '0', '1': '-1', '2': '-2'}, TestGI.ghashtable_utf8_container_return())

    def test_ghashtable_int_full_return(self):
        self.assertEquals({'-1': '1', '0': '0', '1': '-1', '2': '-2'}, TestGI.ghashtable_utf8_full_return())

    def test_ghashtable_int_none_in(self):
        TestGI.ghashtable_int_none_in({-1: 1, 0: 0, 1: -1, 2: -2})

        self.assertRaises(TypeError, TestGI.ghashtable_int_none_in, {-1: 1, '0': 0, 1: -1, 2: -2})
        self.assertRaises(TypeError, TestGI.ghashtable_int_none_in, {-1: 1, 0: '0', 1: -1, 2: -2})

        self.assertRaises(TypeError, TestGI.ghashtable_int_none_in, '{-1: 1, 0: 0, 1: -1, 2: -2}')
        self.assertRaises(TypeError, TestGI.ghashtable_int_none_in, None)

    def test_ghashtable_utf8_none_in(self):
        TestGI.ghashtable_utf8_none_in({'-1': '1', '0': '0', '1': '-1', '2': '-2'})

    def test_ghashtable_utf8_container_in(self):
        TestGI.ghashtable_utf8_container_in({'-1': '1', '0': '0', '1': '-1', '2': '-2'})

    def test_ghashtable_utf8_full_in(self):
        TestGI.ghashtable_utf8_full_in({'-1': '1', '0': '0', '1': '-1', '2': '-2'})

    def test_ghashtable_utf8_none_out(self):
        self.assertEquals({'-1': '1', '0': '0', '1': '-1', '2': '-2'}, TestGI.ghashtable_utf8_none_out())

    def test_ghashtable_utf8_container_out(self):
        self.assertEquals({'-1': '1', '0': '0', '1': '-1', '2': '-2'}, TestGI.ghashtable_utf8_container_out())

    def test_ghashtable_utf8_full_out(self):
        self.assertEquals({'-1': '1', '0': '0', '1': '-1', '2': '-2'}, TestGI.ghashtable_utf8_full_out())

    def test_ghashtable_utf8_none_inout(self):
        self.assertEquals({'-1': '1', '0': '0', '1': '1'},
            TestGI.ghashtable_utf8_none_inout({'-1': '1', '0': '0', '1': '-1', '2': '-2'}))

    def test_ghashtable_utf8_container_inout(self):
        self.assertEquals({'-1': '1', '0': '0', '1': '1'},
            TestGI.ghashtable_utf8_container_inout({'-1': '1', '0': '0', '1': '-1', '2': '-2'}))

    def test_ghashtable_utf8_full_inout(self):
        self.assertEquals({'-1': '1', '0': '0', '1': '1'},
            TestGI.ghashtable_utf8_full_inout({'-1': '1', '0': '0', '1': '-1', '2': '-2'}))


class TestGValue(unittest.TestCase):

    def test_gvalue_return(self):
        self.assertEquals(42, TestGI.gvalue_return())

    def test_gvalue_in(self):
        TestGI.gvalue_in(42)
        self.assertRaises(TypeError, TestGI.gvalue_in, None)

    def test_gvalue_out(self):
        self.assertEquals(42, TestGI.gvalue_out())

    def test_gvalue_inout(self):
        self.assertEquals('42', TestGI.gvalue_inout(42))


class TestGClosure(unittest.TestCase):

    def test_gclosure_in(self):
        TestGI.gclosure_in(lambda: 42)

        self.assertRaises(TypeError, TestGI.gclosure_in, 42)
        self.assertRaises(TypeError, TestGI.gclosure_in, None)


class TestPointer(unittest.TestCase):
    def test_pointer_in_return(self):
        self.assertEquals(TestGI.pointer_in_return(42), 42)


class TestGEnum(unittest.TestCase):

    def test_enum(self):
        self.assertTrue(issubclass(TestGI.Enum, gobject.GEnum))
        self.assertTrue(isinstance(TestGI.Enum.VALUE1, TestGI.Enum))
        self.assertTrue(isinstance(TestGI.Enum.VALUE2, TestGI.Enum))
        self.assertTrue(isinstance(TestGI.Enum.VALUE3, TestGI.Enum))
        self.assertEquals(42, TestGI.Enum.VALUE3)

    def test_enum_in(self):
        TestGI.enum_in(TestGI.Enum.VALUE3)

        self.assertRaises(TypeError, TestGI.enum_in, 42)
        self.assertRaises(TypeError, TestGI.enum_in, 'TestGI.Enum.VALUE3')

    def test_enum_in_ptr(self):
        TestGI.enum_in_ptr(TestGI.Enum.VALUE3)

        self.assertRaises(TypeError, TestGI.enum_in_ptr, None)

    def test_enum_out(self):
        enum = TestGI.enum_out()
        self.assertTrue(isinstance(enum, TestGI.Enum))
        self.assertEquals(enum, TestGI.Enum.VALUE3)

    def test_enum_inout(self):
        enum = TestGI.enum_inout(TestGI.Enum.VALUE3)
        self.assertTrue(isinstance(enum, TestGI.Enum))
        self.assertEquals(enum, TestGI.Enum.VALUE1)


class TestGFlags(unittest.TestCase):

    def test_flags(self):
        self.assertTrue(issubclass(TestGI.Flags, gobject.GFlags))
        self.assertTrue(isinstance(TestGI.Flags.VALUE1, TestGI.Flags))
        self.assertTrue(isinstance(TestGI.Flags.VALUE2, TestGI.Flags))
        self.assertTrue(isinstance(TestGI.Flags.VALUE3, TestGI.Flags))
        self.assertEquals(1 << 1, TestGI.Flags.VALUE2)

    def test_flags_in(self):
        TestGI.flags_in(TestGI.Flags.VALUE2)
        TestGI.flags_in_zero(Number(0))

        self.assertRaises(TypeError, TestGI.flags_in, 1 << 1)
        self.assertRaises(TypeError, TestGI.flags_in, 'TestGI.Flags.VALUE2')

    def test_flags_in_ptr(self):
        TestGI.flags_in_ptr(TestGI.Flags.VALUE2)

        self.assertRaises(TypeError, TestGI.flags_in_ptr, None)

    def test_flags_out(self):
        flags = TestGI.flags_out()
        self.assertTrue(isinstance(flags, TestGI.Flags))
        self.assertEquals(flags, TestGI.Flags.VALUE2)

    def test_flags_inout(self):
        flags = TestGI.flags_inout(TestGI.Flags.VALUE2)
        self.assertTrue(isinstance(flags, TestGI.Flags))
        self.assertEquals(flags, TestGI.Flags.VALUE1)


class TestStructure(unittest.TestCase):

    def test_simple_struct(self):
        self.assertTrue(issubclass(TestGI.SimpleStruct, gobject.GPointer))

        struct = TestGI.SimpleStruct()
        self.assertTrue(isinstance(struct, TestGI.SimpleStruct))

        struct.long_ = 6
        struct.int8 = 7

        self.assertEquals(6, struct.long_)
        self.assertEquals(7, struct.int8)

        del struct

    def test_nested_struct(self):
        struct = TestGI.NestedStruct()

        self.assertTrue(isinstance(struct.simple_struct, TestGI.SimpleStruct))

        struct.simple_struct.long_ = 42
        self.assertEquals(42, struct.simple_struct.long_)

        del struct

    def test_not_simple_struct(self):
        self.assertRaises(TypeError, TestGI.NotSimpleStruct)

    def test_simple_struct_return(self):
        struct = TestGI.simple_struct_return()

        self.assertTrue(isinstance(struct, TestGI.SimpleStruct))
        self.assertEquals(6, struct.long_)
        self.assertEquals(7, struct.int8)

        del struct

    def test_simple_struct_in(self):
        struct = TestGI.SimpleStruct()
        struct.long_ = 6
        struct.int8 = 7

        TestGI.simple_struct_in(struct)

        del struct

        struct = TestGI.NestedStruct()

        self.assertRaises(TypeError, TestGI.simple_struct_in, struct)

        del struct

        self.assertRaises(TypeError, TestGI.simple_struct_in, None)

    def test_simple_struct_out(self):
        struct = TestGI.simple_struct_out()

        self.assertTrue(isinstance(struct, TestGI.SimpleStruct))
        self.assertEquals(6, struct.long_)
        self.assertEquals(7, struct.int8)

        del struct

    def test_simple_struct_inout(self):
        in_struct = TestGI.SimpleStruct()
        in_struct.long_ = 6
        in_struct.int8 = 7

        out_struct = TestGI.simple_struct_inout(in_struct)

        self.assertTrue(isinstance(out_struct, TestGI.SimpleStruct))
        self.assertEquals(7, out_struct.long_)
        self.assertEquals(6, out_struct.int8)

        del in_struct
        del out_struct

    def test_simple_struct_method(self):
        struct = TestGI.SimpleStruct()
        struct.long_ = 6
        struct.int8 = 7

        struct.method()

        del struct

        self.assertRaises(TypeError, TestGI.SimpleStruct.method)


    def test_pointer_struct(self):
        self.assertTrue(issubclass(TestGI.PointerStruct, gobject.GPointer))

        struct = TestGI.PointerStruct()
        self.assertTrue(isinstance(struct, TestGI.PointerStruct))

        del struct

    def test_pointer_struct_return(self):
        struct = TestGI.pointer_struct_return()

        self.assertTrue(isinstance(struct, TestGI.PointerStruct))
        self.assertEquals(42, struct.long_)

        del struct

    def test_pointer_struct_in(self):
        struct = TestGI.PointerStruct()
        struct.long_ = 42

        TestGI.pointer_struct_in(struct)

        del struct

    def test_pointer_struct_out(self):
        struct = TestGI.pointer_struct_out()

        self.assertTrue(isinstance(struct, TestGI.PointerStruct))
        self.assertEquals(42, struct.long_)

        del struct

    def test_pointer_struct_inout(self):
        in_struct = TestGI.PointerStruct()
        in_struct.long_ = 42

        out_struct = TestGI.pointer_struct_inout(in_struct)

        self.assertTrue(isinstance(out_struct, TestGI.PointerStruct))
        self.assertEquals(0, out_struct.long_)

        del in_struct
        del out_struct


    def test_boxed_struct(self):
        self.assertTrue(issubclass(TestGI.BoxedStruct, gobject.GBoxed))

        self.assertRaises(TypeError, TestGI.BoxedStruct)

    def test_boxed_instantiable_struct(self):
        struct = TestGI.BoxedInstantiableStruct()

        self.assertTrue(isinstance(struct, TestGI.BoxedInstantiableStruct))

        new_struct = struct.copy()
        self.assertTrue(isinstance(new_struct, TestGI.BoxedInstantiableStruct))

        del struct
        del new_struct

    def test_boxed_instantiable_struct_return(self):
        struct = TestGI.boxed_instantiable_struct_return()

        self.assertTrue(isinstance(struct, TestGI.BoxedInstantiableStruct))
        self.assertEquals(42, struct.long_)

        del struct

    def test_boxed_instantiable_struct_in(self):
        struct = TestGI.BoxedInstantiableStruct()
        struct.long_ = 42

        TestGI.boxed_instantiable_struct_in(struct)

        del struct

    def test_boxed_instantiable_struct_out(self):
        struct = TestGI.boxed_instantiable_struct_out()

        self.assertTrue(isinstance(struct, TestGI.BoxedInstantiableStruct))
        self.assertEquals(42, struct.long_)

        del struct

    def test_boxed_instantiable_struct_inout(self):
        in_struct = TestGI.BoxedInstantiableStruct()
        in_struct.long_ = 42

        out_struct = TestGI.boxed_instantiable_struct_inout(in_struct)

        self.assertTrue(isinstance(out_struct, TestGI.BoxedInstantiableStruct))
        self.assertEquals(0, out_struct.long_)

        del in_struct
        del out_struct


class TestGObject(unittest.TestCase):

    def test_object(self):
        self.assertTrue(issubclass(TestGI.Object, gobject.GObject))

        object_ = TestGI.Object()
        self.assertTrue(isinstance(object_, TestGI.Object))
        self.assertEquals(object_.__grefcount__, 1)

    def test_object_new(self):
        object_ = TestGI.Object.new(42)
        self.assertTrue(isinstance(object_, TestGI.Object))
        self.assertEquals(object_.__grefcount__, 1)

    def test_object_int(self):
        object_ = TestGI.Object(int = 42)
        self.assertEquals(object_.int_, 42)
# FIXME: Don't work yet.
#        object_.int_ = 0
#        self.assertEquals(object_.int_, 0)

    def test_object_static_method(self):
        TestGI.Object.static_method()

    def test_object_method(self):
        TestGI.Object(int = 42).method()
        self.assertRaises(TypeError, TestGI.Object.method, gobject.GObject())
        self.assertRaises(TypeError, TestGI.Object.method)


    def test_sub_object(self):
        self.assertTrue(issubclass(TestGI.SubObject, TestGI.Object))

        object_ = TestGI.SubObject()
        self.assertTrue(isinstance(object_, TestGI.SubObject))

    def test_sub_object_new(self):
        self.assertRaises(TypeError, TestGI.SubObject.new, 42)

    def test_sub_object_static_method(self):
        object_ = TestGI.SubObject()
        object_.static_method()

    def test_sub_object_method(self):
        object_ = TestGI.SubObject(int = 42)
        object_.method()

    def test_sub_object_sub_method(self):
        object_ = TestGI.SubObject()
        object_.sub_method()

    def test_sub_object_overwritten_method(self):
        object_ = TestGI.SubObject()
        object_.overwritten_method()

        self.assertRaises(TypeError, TestGI.SubObject.overwritten_method, TestGI.Object())

    def test_sub_object_int(self):
        object_ = TestGI.SubObject()
        self.assertEquals(object_.int_, 0)
# FIXME: Don't work yet.
#        object_.int_ = 42
#        self.assertEquals(object_.int_, 42)

    def test_object_none_return(self):
        object_ = TestGI.object_none_return()
        self.assertTrue(isinstance(object_, TestGI.Object))
        self.assertEquals(object_.__grefcount__, 2)

    def test_object_full_return(self):
        object_ = TestGI.object_full_return()
        self.assertTrue(isinstance(object_, TestGI.Object))
        self.assertEquals(object_.__grefcount__, 1)

    def test_object_none_in(self):
        object_ = TestGI.Object(int = 42)
        TestGI.object_none_in(object_)
        self.assertEquals(object_.__grefcount__, 1)

        object_ = TestGI.SubObject(int = 42)
        TestGI.object_none_in(object_)

        object_ = gobject.GObject()
        self.assertRaises(TypeError, TestGI.object_none_in, object_)

        self.assertRaises(TypeError, TestGI.object_none_in, None)

    def test_object_full_in(self):
        object_ = TestGI.Object(int = 42)
        TestGI.object_full_in(object_)
        self.assertEquals(object_.__grefcount__, 1)

    def test_object_none_out(self):
        object_ = TestGI.object_none_out()
        self.assertTrue(isinstance(object_, TestGI.Object))
        self.assertEquals(object_.__grefcount__, 2)

        new_object = TestGI.object_none_out()
        self.assertTrue(new_object is object_)

    def test_object_full_out(self):
        object_ = TestGI.object_full_out()
        self.assertTrue(isinstance(object_, TestGI.Object))
        self.assertEquals(object_.__grefcount__, 1)

    def test_object_none_inout(self):
        object_ = TestGI.Object(int = 42)
        new_object = TestGI.object_none_inout(object_)

        self.assertTrue(isinstance(new_object, TestGI.Object))

        self.assertFalse(object_ is new_object)

        self.assertEquals(object_.__grefcount__, 1)
        self.assertEquals(new_object.__grefcount__, 2)

        new_new_object = TestGI.object_none_inout(object_)
        self.assertTrue(new_new_object is new_object)

        TestGI.object_none_inout(TestGI.SubObject(int = 42))

    def test_object_full_inout(self):
        object_ = TestGI.Object(int = 42)
        new_object = TestGI.object_full_inout(object_)

        self.assertTrue(isinstance(new_object, TestGI.Object))

        self.assertFalse(object_ is new_object)

        self.assertEquals(object_.__grefcount__, 1)
        self.assertEquals(new_object.__grefcount__, 1)

# FIXME: Doesn't actually return the same object.
#    def test_object_inout_same(self):
#        object_ = TestGI.Object()
#        new_object = TestGI.object_full_inout(object_)
#        self.assertTrue(object_ is new_object)
#        self.assertEquals(object_.__grefcount__, 1)


class TestMultiOutputArgs(unittest.TestCase):

    def test_int_out_out(self):
        self.assertEquals((6, 7), TestGI.int_out_out())

    def test_int_return_out(self):
        self.assertEquals((6, 7), TestGI.int_return_out())



import sys

import unittest
import tempfile
import types
import shutil
import os
import gc
import weakref
import warnings
import pickle
import platform
import enum

import gi
import gi.overrides
from gi import PyGIWarning
from gi import PyGIDeprecationWarning
from gi.repository import GObject, GLib, Gio
from gi.repository import GIMarshallingTests
import pytest

from .helper import capture_exceptions, capture_output
import contextlib


CONSTANT_UTF8 = "const ♥ utf8"
CONSTANT_UCS4 = "const ♥ utf8"


class Number:
    def __init__(self, value):
        self.value = value

    def __int__(self):
        return int(self.value)

    def __float__(self):
        return float(self.value)


class Sequence:
    def __init__(self, sequence):
        self.sequence = sequence

    def __len__(self):
        return len(self.sequence)

    def __getitem__(self, key):
        return self.sequence[key]


class TestConstant(unittest.TestCase):
    def test_constant_utf8(self):
        self.assertEqual(CONSTANT_UTF8, GIMarshallingTests.CONSTANT_UTF8)

    def test_constant_number(self):
        self.assertEqual(42, GIMarshallingTests.CONSTANT_NUMBER)

    def test_min_max_int(self):
        self.assertEqual(GLib.MAXINT32, 2**31 - 1)
        self.assertEqual(GLib.MININT32, -(2**31))
        self.assertEqual(GLib.MAXUINT32, 2**32 - 1)

        self.assertEqual(GLib.MAXINT64, 2**63 - 1)
        self.assertEqual(GLib.MININT64, -(2**63))
        self.assertEqual(GLib.MAXUINT64, 2**64 - 1)


class TestBoolean(unittest.TestCase):
    def test_boolean_return(self):
        self.assertEqual(True, GIMarshallingTests.boolean_return_true())
        self.assertEqual(False, GIMarshallingTests.boolean_return_false())

    def test_boolean_in(self):
        GIMarshallingTests.boolean_in_true(True)
        GIMarshallingTests.boolean_in_false(False)

        GIMarshallingTests.boolean_in_true(1)
        GIMarshallingTests.boolean_in_false(0)

    def test_boolean_in_other_types(self):
        GIMarshallingTests.boolean_in_true([""])
        GIMarshallingTests.boolean_in_false([])
        GIMarshallingTests.boolean_in_false(None)

    def test_boolean_out(self):
        self.assertEqual(True, GIMarshallingTests.boolean_out_true())
        self.assertEqual(False, GIMarshallingTests.boolean_out_false())

    def test_boolean_inout(self):
        self.assertEqual(False, GIMarshallingTests.boolean_inout_true_false(True))
        self.assertEqual(True, GIMarshallingTests.boolean_inout_false_true(False))


class TestInt8(unittest.TestCase):
    MAX = GLib.MAXINT8
    MIN = GLib.MININT8

    def test_int8_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.int8_return_max())
        self.assertEqual(self.MIN, GIMarshallingTests.int8_return_min())

    def test_int8_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        GIMarshallingTests.int8_in_max(max)
        GIMarshallingTests.int8_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(OverflowError, GIMarshallingTests.int8_in_max, max)
        self.assertRaises(OverflowError, GIMarshallingTests.int8_in_min, min)

        self.assertRaises(TypeError, GIMarshallingTests.int8_in_max, "self.MAX")

    def test_int8_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.int8_out_max())
        self.assertEqual(self.MIN, GIMarshallingTests.int8_out_min())

    def test_int8_inout(self):
        self.assertEqual(
            self.MIN, GIMarshallingTests.int8_inout_max_min(Number(self.MAX))
        )
        self.assertEqual(
            self.MAX, GIMarshallingTests.int8_inout_min_max(Number(self.MIN))
        )


class TestUInt8(unittest.TestCase):
    MAX = GLib.MAXUINT8

    def test_uint8_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.uint8_return())

    def test_uint8_in(self):
        number = Number(self.MAX)

        GIMarshallingTests.uint8_in(number)
        GIMarshallingTests.uint8_in(b"\xff")

        number.value += 1
        self.assertRaises(OverflowError, GIMarshallingTests.uint8_in, number)
        self.assertRaises(OverflowError, GIMarshallingTests.uint8_in, Number(-1))

        self.assertRaises(TypeError, GIMarshallingTests.uint8_in, "self.MAX")

    def test_uint8_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.uint8_out())

    def test_uint8_inout(self):
        self.assertEqual(0, GIMarshallingTests.uint8_inout(Number(self.MAX)))


class TestInt16(unittest.TestCase):
    MAX = GLib.MAXINT16
    MIN = GLib.MININT16

    def test_int16_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.int16_return_max())
        self.assertEqual(self.MIN, GIMarshallingTests.int16_return_min())

    def test_int16_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        GIMarshallingTests.int16_in_max(max)
        GIMarshallingTests.int16_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(OverflowError, GIMarshallingTests.int16_in_max, max)
        self.assertRaises(OverflowError, GIMarshallingTests.int16_in_min, min)

        self.assertRaises(TypeError, GIMarshallingTests.int16_in_max, "self.MAX")

    def test_int16_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.int16_out_max())
        self.assertEqual(self.MIN, GIMarshallingTests.int16_out_min())

    def test_int16_inout(self):
        self.assertEqual(
            self.MIN, GIMarshallingTests.int16_inout_max_min(Number(self.MAX))
        )
        self.assertEqual(
            self.MAX, GIMarshallingTests.int16_inout_min_max(Number(self.MIN))
        )


class TestUInt16(unittest.TestCase):
    MAX = GLib.MAXUINT16

    def test_uint16_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.uint16_return())

    def test_uint16_in(self):
        number = Number(self.MAX)

        GIMarshallingTests.uint16_in(number)

        number.value += 1

        self.assertRaises(OverflowError, GIMarshallingTests.uint16_in, number)
        self.assertRaises(OverflowError, GIMarshallingTests.uint16_in, Number(-1))

        self.assertRaises(TypeError, GIMarshallingTests.uint16_in, "self.MAX")

    def test_uint16_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.uint16_out())

    def test_uint16_inout(self):
        self.assertEqual(0, GIMarshallingTests.uint16_inout(Number(self.MAX)))


class TestInt32(unittest.TestCase):
    MAX = GLib.MAXINT32
    MIN = GLib.MININT32

    def test_int32_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.int32_return_max())
        self.assertEqual(self.MIN, GIMarshallingTests.int32_return_min())

    def test_int32_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        GIMarshallingTests.int32_in_max(max)
        GIMarshallingTests.int32_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(OverflowError, GIMarshallingTests.int32_in_max, max)
        self.assertRaises(OverflowError, GIMarshallingTests.int32_in_min, min)

        self.assertRaises(TypeError, GIMarshallingTests.int32_in_max, "self.MAX")

    def test_int32_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.int32_out_max())
        self.assertEqual(self.MIN, GIMarshallingTests.int32_out_min())

    def test_int32_inout(self):
        self.assertEqual(
            self.MIN, GIMarshallingTests.int32_inout_max_min(Number(self.MAX))
        )
        self.assertEqual(
            self.MAX, GIMarshallingTests.int32_inout_min_max(Number(self.MIN))
        )


class TestUInt32(unittest.TestCase):
    MAX = GLib.MAXUINT32

    def test_uint32_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.uint32_return())

    def test_uint32_in(self):
        number = Number(self.MAX)

        GIMarshallingTests.uint32_in(number)

        number.value += 1

        self.assertRaises(OverflowError, GIMarshallingTests.uint32_in, number)
        self.assertRaises(OverflowError, GIMarshallingTests.uint32_in, Number(-1))

        self.assertRaises(TypeError, GIMarshallingTests.uint32_in, "self.MAX")

    def test_uint32_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.uint32_out())

    def test_uint32_inout(self):
        self.assertEqual(0, GIMarshallingTests.uint32_inout(Number(self.MAX)))


class TestInt64(unittest.TestCase):
    MAX = 2**63 - 1
    MIN = -(2**63)

    def test_int64_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.int64_return_max())
        self.assertEqual(self.MIN, GIMarshallingTests.int64_return_min())

    def test_int64_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        GIMarshallingTests.int64_in_max(max)
        GIMarshallingTests.int64_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(OverflowError, GIMarshallingTests.int64_in_max, max)
        self.assertRaises(OverflowError, GIMarshallingTests.int64_in_min, min)

        self.assertRaises(TypeError, GIMarshallingTests.int64_in_max, "self.MAX")

    def test_int64_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.int64_out_max())
        self.assertEqual(self.MIN, GIMarshallingTests.int64_out_min())

    def test_int64_inout(self):
        self.assertEqual(
            self.MIN, GIMarshallingTests.int64_inout_max_min(Number(self.MAX))
        )
        self.assertEqual(
            self.MAX, GIMarshallingTests.int64_inout_min_max(Number(self.MIN))
        )


class TestUInt64(unittest.TestCase):
    MAX = 2**64 - 1

    def test_uint64_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.uint64_return())

    def test_uint64_in(self):
        number = Number(self.MAX)

        GIMarshallingTests.uint64_in(number)

        number.value += 1

        self.assertRaises(OverflowError, GIMarshallingTests.uint64_in, number)
        self.assertRaises(OverflowError, GIMarshallingTests.uint64_in, Number(-1))

        self.assertRaises(TypeError, GIMarshallingTests.uint64_in, "self.MAX")

    def test_uint64_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.uint64_out())

    def test_uint64_inout(self):
        self.assertEqual(0, GIMarshallingTests.uint64_inout(Number(self.MAX)))


class TestShort(unittest.TestCase):
    MAX = GLib.MAXSHORT
    MIN = GLib.MINSHORT

    def test_short_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.short_return_max())
        self.assertEqual(self.MIN, GIMarshallingTests.short_return_min())

    def test_short_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        GIMarshallingTests.short_in_max(max)
        GIMarshallingTests.short_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(OverflowError, GIMarshallingTests.short_in_max, max)
        self.assertRaises(OverflowError, GIMarshallingTests.short_in_min, min)

        self.assertRaises(TypeError, GIMarshallingTests.short_in_max, "self.MAX")

    def test_short_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.short_out_max())
        self.assertEqual(self.MIN, GIMarshallingTests.short_out_min())

    def test_short_inout(self):
        self.assertEqual(
            self.MIN, GIMarshallingTests.short_inout_max_min(Number(self.MAX))
        )
        self.assertEqual(
            self.MAX, GIMarshallingTests.short_inout_min_max(Number(self.MIN))
        )


class TestUShort(unittest.TestCase):
    MAX = GLib.MAXUSHORT

    def test_ushort_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.ushort_return())

    def test_ushort_in(self):
        number = Number(self.MAX)

        GIMarshallingTests.ushort_in(number)

        number.value += 1

        self.assertRaises(OverflowError, GIMarshallingTests.ushort_in, number)
        self.assertRaises(OverflowError, GIMarshallingTests.ushort_in, Number(-1))

        self.assertRaises(TypeError, GIMarshallingTests.ushort_in, "self.MAX")

    def test_ushort_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.ushort_out())

    def test_ushort_inout(self):
        self.assertEqual(0, GIMarshallingTests.ushort_inout(Number(self.MAX)))


class TestInt(unittest.TestCase):
    MAX = GLib.MAXINT
    MIN = GLib.MININT

    def test_int_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.int_return_max())
        self.assertEqual(self.MIN, GIMarshallingTests.int_return_min())

    def test_int_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        GIMarshallingTests.int_in_max(max)
        GIMarshallingTests.int_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(OverflowError, GIMarshallingTests.int_in_max, max)
        self.assertRaises(OverflowError, GIMarshallingTests.int_in_min, min)

        self.assertRaises(TypeError, GIMarshallingTests.int_in_max, "self.MAX")

    def test_int_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.int_out_max())
        self.assertEqual(self.MIN, GIMarshallingTests.int_out_min())

    def test_int_inout(self):
        self.assertEqual(
            self.MIN, GIMarshallingTests.int_inout_max_min(Number(self.MAX))
        )
        self.assertEqual(
            self.MAX, GIMarshallingTests.int_inout_min_max(Number(self.MIN))
        )
        self.assertRaises(
            TypeError, GIMarshallingTests.int_inout_min_max, Number(self.MIN), 42
        )


class TestUInt(unittest.TestCase):
    MAX = GLib.MAXUINT

    def test_uint_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.uint_return())

    def test_uint_in(self):
        number = Number(self.MAX)

        GIMarshallingTests.uint_in(number)

        number.value += 1

        self.assertRaises(OverflowError, GIMarshallingTests.uint_in, number)
        self.assertRaises(OverflowError, GIMarshallingTests.uint_in, Number(-1))

        self.assertRaises(TypeError, GIMarshallingTests.uint_in, "self.MAX")

    def test_uint_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.uint_out())

    def test_uint_inout(self):
        self.assertEqual(0, GIMarshallingTests.uint_inout(Number(self.MAX)))


class TestLong(unittest.TestCase):
    MAX = GLib.MAXLONG
    MIN = GLib.MINLONG

    def test_long_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.long_return_max())
        self.assertEqual(self.MIN, GIMarshallingTests.long_return_min())

    def test_long_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        GIMarshallingTests.long_in_max(max)
        GIMarshallingTests.long_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(OverflowError, GIMarshallingTests.long_in_max, max)
        self.assertRaises(OverflowError, GIMarshallingTests.long_in_min, min)

        self.assertRaises(TypeError, GIMarshallingTests.long_in_max, "self.MAX")

    def test_long_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.long_out_max())
        self.assertEqual(self.MIN, GIMarshallingTests.long_out_min())

    def test_long_inout(self):
        self.assertEqual(
            self.MIN, GIMarshallingTests.long_inout_max_min(Number(self.MAX))
        )
        self.assertEqual(
            self.MAX, GIMarshallingTests.long_inout_min_max(Number(self.MIN))
        )


class TestULong(unittest.TestCase):
    MAX = GLib.MAXULONG

    def test_ulong_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.ulong_return())

    def test_ulong_in(self):
        number = Number(self.MAX)

        GIMarshallingTests.ulong_in(number)

        number.value += 1

        self.assertRaises(OverflowError, GIMarshallingTests.ulong_in, number)
        self.assertRaises(OverflowError, GIMarshallingTests.ulong_in, Number(-1))

        self.assertRaises(TypeError, GIMarshallingTests.ulong_in, "self.MAX")

    def test_ulong_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.ulong_out())

    def test_ulong_inout(self):
        self.assertEqual(0, GIMarshallingTests.ulong_inout(Number(self.MAX)))


class TestSSize(unittest.TestCase):
    MAX = GLib.MAXSSIZE
    MIN = GLib.MINSSIZE

    def test_ssize_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.ssize_return_max())
        self.assertEqual(self.MIN, GIMarshallingTests.ssize_return_min())

    def test_ssize_in(self):
        max = Number(self.MAX)
        min = Number(self.MIN)

        GIMarshallingTests.ssize_in_max(max)
        GIMarshallingTests.ssize_in_min(min)

        max.value += 1
        min.value -= 1

        self.assertRaises(OverflowError, GIMarshallingTests.ssize_in_max, max)
        self.assertRaises(OverflowError, GIMarshallingTests.ssize_in_min, min)

        self.assertRaises(TypeError, GIMarshallingTests.ssize_in_max, "self.MAX")

    def test_ssize_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.ssize_out_max())
        self.assertEqual(self.MIN, GIMarshallingTests.ssize_out_min())

    def test_ssize_inout(self):
        self.assertEqual(
            self.MIN, GIMarshallingTests.ssize_inout_max_min(Number(self.MAX))
        )
        self.assertEqual(
            self.MAX, GIMarshallingTests.ssize_inout_min_max(Number(self.MIN))
        )


class TestSize(unittest.TestCase):
    MAX = GLib.MAXSIZE

    def test_size_return(self):
        self.assertEqual(self.MAX, GIMarshallingTests.size_return())

    def test_size_in(self):
        number = Number(self.MAX)

        GIMarshallingTests.size_in(number)

        number.value += 1

        self.assertRaises(OverflowError, GIMarshallingTests.size_in, number)
        self.assertRaises(OverflowError, GIMarshallingTests.size_in, Number(-1))

        self.assertRaises(TypeError, GIMarshallingTests.size_in, "self.MAX")

    def test_size_out(self):
        self.assertEqual(self.MAX, GIMarshallingTests.size_out())

    def test_size_inout(self):
        self.assertEqual(0, GIMarshallingTests.size_inout(Number(self.MAX)))


class TestTimet(unittest.TestCase):
    def test_time_t_return(self):
        self.assertEqual(1234567890, GIMarshallingTests.time_t_return())

    def test_time_t_in(self):
        GIMarshallingTests.time_t_in(1234567890)
        self.assertRaises(TypeError, GIMarshallingTests.time_t_in, "hello")

    def test_time_t_out(self):
        self.assertEqual(1234567890, GIMarshallingTests.time_t_out())

    def test_time_t_inout(self):
        self.assertEqual(0, GIMarshallingTests.time_t_inout(1234567890))


class TestFloat(unittest.TestCase):
    MAX = GLib.MAXFLOAT
    MIN = GLib.MINFLOAT

    def test_float_return(self):
        self.assertAlmostEqual(self.MAX, GIMarshallingTests.float_return())

    def test_float_in(self):
        GIMarshallingTests.float_in(Number(self.MAX))

        self.assertRaises(TypeError, GIMarshallingTests.float_in, "self.MAX")

    def test_float_out(self):
        self.assertAlmostEqual(self.MAX, GIMarshallingTests.float_out())

    def test_float_inout(self):
        self.assertAlmostEqual(
            self.MIN, GIMarshallingTests.float_inout(Number(self.MAX))
        )


class TestDouble(unittest.TestCase):
    MAX = GLib.MAXDOUBLE
    MIN = GLib.MINDOUBLE

    def test_double_return(self):
        self.assertAlmostEqual(self.MAX, GIMarshallingTests.double_return())

    def test_double_in(self):
        GIMarshallingTests.double_in(Number(self.MAX))

        self.assertRaises(TypeError, GIMarshallingTests.double_in, "self.MAX")

    def test_double_out(self):
        self.assertAlmostEqual(self.MAX, GIMarshallingTests.double_out())

    def test_double_inout(self):
        self.assertAlmostEqual(
            self.MIN, GIMarshallingTests.double_inout(Number(self.MAX))
        )


class TestGType(unittest.TestCase):
    def test_gtype_name(self):
        self.assertEqual("void", GObject.TYPE_NONE.name)
        self.assertEqual("gchararray", GObject.TYPE_STRING.name)

        def check_readonly(gtype):
            gtype.name = "foo"

        errors = (AttributeError,)
        if platform.python_implementation() == "PyPy":
            # https://foss.heptapod.net/pypy/pypy/-/issues/2788
            errors = (AttributeError, TypeError)

        self.assertRaises(errors, check_readonly, GObject.TYPE_NONE)
        self.assertRaises(errors, check_readonly, GObject.TYPE_STRING)

    def test_gtype_return(self):
        self.assertEqual(GObject.TYPE_NONE, GIMarshallingTests.gtype_return())
        self.assertEqual(GObject.TYPE_STRING, GIMarshallingTests.gtype_string_return())

    def test_gtype_in(self):
        GIMarshallingTests.gtype_in(GObject.TYPE_NONE)
        GIMarshallingTests.gtype_string_in(GObject.TYPE_STRING)
        self.assertRaises(TypeError, GIMarshallingTests.gtype_in, "foo")
        self.assertRaises(TypeError, GIMarshallingTests.gtype_string_in, "foo")

    def test_gtype_out(self):
        self.assertEqual(GObject.TYPE_NONE, GIMarshallingTests.gtype_out())
        self.assertEqual(GObject.TYPE_STRING, GIMarshallingTests.gtype_string_out())

    def test_gtype_inout(self):
        self.assertEqual(
            GObject.TYPE_INT, GIMarshallingTests.gtype_inout(GObject.TYPE_NONE)
        )


class TestUtf8(unittest.TestCase):
    def test_utf8_as_uint8array_in(self):
        data = CONSTANT_UTF8
        if not isinstance(data, bytes):
            data = data.encode("utf-8")
        GIMarshallingTests.utf8_as_uint8array_in(data)

    def test_utf8_none_return(self):
        self.assertEqual(CONSTANT_UTF8, GIMarshallingTests.utf8_none_return())

    def test_utf8_full_return(self):
        self.assertEqual(CONSTANT_UTF8, GIMarshallingTests.utf8_full_return())

    def test_extra_utf8_full_return_invalid(self):
        with pytest.raises(UnicodeDecodeError):
            GIMarshallingTests.extra_utf8_full_return_invalid()

    def test_extra_utf8_full_out_invalid(self):
        with pytest.raises(UnicodeDecodeError):
            GIMarshallingTests.extra_utf8_full_out_invalid()

    def test_utf8_none_in(self):
        GIMarshallingTests.utf8_none_in(CONSTANT_UTF8)
        self.assertRaises(TypeError, GIMarshallingTests.utf8_none_in, 42)
        self.assertRaises(TypeError, GIMarshallingTests.utf8_none_in, None)

    def test_utf8_none_out(self):
        self.assertEqual(CONSTANT_UTF8, GIMarshallingTests.utf8_none_out())

    def test_utf8_full_out(self):
        self.assertEqual(CONSTANT_UTF8, GIMarshallingTests.utf8_full_out())

    def test_utf8_dangling_out(self):
        GIMarshallingTests.utf8_dangling_out()

    def test_utf8_none_inout(self):
        self.assertEqual("", GIMarshallingTests.utf8_none_inout(CONSTANT_UTF8))

    def test_utf8_full_inout(self):
        self.assertEqual("", GIMarshallingTests.utf8_full_inout(CONSTANT_UTF8))


class TestFilename(unittest.TestCase):
    def setUp(self):
        self.workdir = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.workdir)

    def tests_filename_list_return(self):
        assert GIMarshallingTests.filename_list_return() == []

    @unittest.skipIf(os.name == "nt", "fixme")
    def test_filename_in(self):
        fname = os.path.join(self.workdir, "testäø.txt")

        try:
            os.path.exists(fname)
        except ValueError:
            # non-unicode fs encoding
            return

        self.assertRaises(GLib.GError, GLib.file_get_contents, fname)

        with open(fname.encode("UTF-8"), "wb") as f:
            f.write(b"hello world!\n\x01\x02")

        (result, contents) = GLib.file_get_contents(fname)
        self.assertEqual(result, True)
        self.assertEqual(contents, b"hello world!\n\x01\x02")

    def test_filename_in_nullable(self):
        self.assertTrue(GIMarshallingTests.filename_copy(None) is None)
        self.assertRaises(TypeError, GIMarshallingTests.filename_exists, None)

    @unittest.skipIf(os.name == "nt", "fixme")
    def test_filename_out(self):
        self.assertRaises(GLib.GError, GLib.Dir.make_tmp, "test")
        name = "testäø.XXXXXX"

        try:
            os.path.exists(name)
        except ValueError:
            # non-unicode fs encoding
            return

        dirname = GLib.Dir.make_tmp(name)
        self.assertTrue(os.path.sep + "testäø." in dirname, dirname)
        self.assertTrue(os.path.isdir(dirname))
        os.rmdir(dirname)

    def test_wrong_types(self):
        self.assertRaises(TypeError, GIMarshallingTests.filename_copy, 23)
        self.assertRaises(TypeError, GIMarshallingTests.filename_copy, [])

    def test_null(self):
        self.assertTrue(GIMarshallingTests.filename_copy(None) is None)
        self.assertRaises(TypeError, GIMarshallingTests.filename_exists, None)

    def test_round_trip(self):
        self.assertEqual(GIMarshallingTests.filename_copy("foo"), "foo")
        self.assertEqual(GIMarshallingTests.filename_copy(b"foo"), "foo")

    def test_contains_null(self):
        self.assertRaises(
            (ValueError, TypeError), GIMarshallingTests.filename_copy, b"foo\x00"
        )
        self.assertRaises(
            (ValueError, TypeError), GIMarshallingTests.filename_copy, "foo\x00"
        )

    def test_win32_surrogates(self):
        if os.name != "nt":
            return

        copy = GIMarshallingTests.filename_copy
        glib_repr = GIMarshallingTests.filename_to_glib_repr

        self.assertEqual(copy("\ud83d"), "\ud83d")
        self.assertEqual(copy("\x61\udc00"), "\x61\udc00")
        self.assertEqual(copy("\ud800\udc01"), "\U00010001")
        self.assertEqual(copy("\ud83d\x20\udca9"), "\ud83d\x20\udca9")

        self.assertEqual(glib_repr("\ud83d"), b"\xed\xa0\xbd")
        self.assertEqual(glib_repr("\ud800\udc01"), b"\xf0\x90\x80\x81")

        self.assertEqual(glib_repr("\ud800\udbff"), b"\xed\xa0\x80\xed\xaf\xbf")
        self.assertEqual(glib_repr("\ud800\ue000"), b"\xed\xa0\x80\xee\x80\x80")
        self.assertEqual(glib_repr("\ud7ff\udc00"), b"\xed\x9f\xbf\xed\xb0\x80")
        self.assertEqual(glib_repr("\x61\udc00"), b"\x61\xed\xb0\x80")
        self.assertEqual(glib_repr("\udc00"), b"\xed\xb0\x80")

    def test_win32_bytes_py3(self):
        if os.name != "nt":
            return

        values = [
            b"foo",
            b"\xff\xff",
            b"\xc3\xb6\xc3\xa4\xc3\xbc",
            b"\xed\xa0\xbd",
            b"\xf0\x90\x80\x81",
        ]

        for v in values:
            try:
                uni = v.decode(sys.getfilesystemencoding(), "surrogatepass")
            except UnicodeDecodeError:
                continue
            self.assertEqual(GIMarshallingTests.filename_copy(v), uni)

    def test_unix_various(self):
        if os.name == "nt":
            return

        copy = GIMarshallingTests.filename_copy
        glib_repr = GIMarshallingTests.filename_to_glib_repr

        try:
            os.fsdecode(b"\xff\xfe")
        except UnicodeDecodeError:
            self.assertRaises(UnicodeDecodeError, copy, b"\xff\xfe")
        else:
            str_path = copy(b"\xff\xfe")
            self.assertTrue(isinstance(str_path, str))
            self.assertEqual(str_path, os.fsdecode(b"\xff\xfe"))
            self.assertEqual(copy(str_path), str_path)
            self.assertEqual(glib_repr(b"\xff\xfe"), b"\xff\xfe")
            self.assertEqual(glib_repr(str_path), b"\xff\xfe")

        # if getfilesystemencoding is ASCII, then we should fail like
        # os.fsencode
        try:
            byte_path = os.fsencode("ä")
        except UnicodeEncodeError:
            self.assertRaises(UnicodeEncodeError, copy, "ä")
        else:
            self.assertEqual(copy("ä"), "ä")
            self.assertEqual(glib_repr("ä"), byte_path)

    @unittest.skip("glib can't handle non-unicode paths")
    def test_win32_surrogates_exists(self):
        if os.name != "nt":
            return

        path = os.path.join(self.workdir, "\ud83d")
        with open(path, "wb"):
            self.assertTrue(os.path.exists(path))
            self.assertTrue(GIMarshallingTests.filename_exists(path))
        os.unlink(path)

    def test_path_exists_various_types(self):
        wd = self.workdir
        wdb = os.fsencode(wd)

        paths = [(wdb, b"foo-1"), (wd, "foo-2"), (wd, "öäü-3")]

        if sys.platform != "darwin":
            with contextlib.suppress(UnicodeDecodeError):
                # depends on the code page
                paths.append((wd, os.fsdecode(b"\xff\xfe-4")))

            if os.name != "nt":
                paths.append((wdb, b"\xff\xfe-5"))

        def valid_path(p):
            try:
                os.path.exists(p)
            except ValueError:
                return False
            return True

        for d, path in paths:
            if not valid_path(path):
                continue
            path = os.path.join(d, path)
            with open(path, "wb"):
                self.assertTrue(GIMarshallingTests.filename_exists(path))


class TestArray(unittest.TestCase):
    @unittest.skipUnless(hasattr(GIMarshallingTests, "array_bool_in"), "too old gi")
    def test_array_bool_in(self):
        GIMarshallingTests.array_bool_in([True, False, True, True])

    @unittest.skipUnless(hasattr(GIMarshallingTests, "array_bool_out"), "too old gi")
    def test_array_bool_out(self):
        assert GIMarshallingTests.array_bool_out() == [True, False, True, True]

    @unittest.skipUnless(hasattr(GIMarshallingTests, "array_int64_in"), "too old gi")
    def test_array_int64_in(self):
        GIMarshallingTests.array_int64_in([-1, 0, 1, 2])

    @unittest.skipUnless(hasattr(GIMarshallingTests, "array_uint64_in"), "too old gi")
    def test_array_uint64_in(self):
        GIMarshallingTests.array_uint64_in([GLib.MAXUINT64, 0, 1, 2])

    @unittest.skipUnless(hasattr(GIMarshallingTests, "array_unichar_in"), "too old gi")
    def test_array_unichar_in(self):
        GIMarshallingTests.array_unichar_in(list(CONSTANT_UCS4))
        GIMarshallingTests.array_unichar_in(CONSTANT_UCS4)

    @unittest.skipUnless(hasattr(GIMarshallingTests, "array_unichar_out"), "too old gi")
    def test_array_unichar_out(self):
        result = list(CONSTANT_UCS4)
        assert GIMarshallingTests.array_unichar_out() == result

    def test_array_zero_terminated_return_unichar(self):
        assert GIMarshallingTests.array_zero_terminated_return_unichar() == list(
            CONSTANT_UCS4
        )

    def test_array_fixed_int_return(self):
        self.assertEqual([-1, 0, 1, 2], GIMarshallingTests.array_fixed_int_return())

    def test_array_fixed_short_return(self):
        self.assertEqual([-1, 0, 1, 2], GIMarshallingTests.array_fixed_short_return())

    def test_array_fixed_int_in(self):
        GIMarshallingTests.array_fixed_int_in(Sequence([-1, 0, 1, 2]))

        self.assertRaises(
            TypeError, GIMarshallingTests.array_fixed_int_in, Sequence([-1, "0", 1, 2])
        )

        self.assertRaises(TypeError, GIMarshallingTests.array_fixed_int_in, 42)
        self.assertRaises(TypeError, GIMarshallingTests.array_fixed_int_in, None)

    def test_array_fixed_short_in(self):
        GIMarshallingTests.array_fixed_short_in(Sequence([-1, 0, 1, 2]))

    def test_array_fixed_out(self):
        self.assertEqual([-1, 0, 1, 2], GIMarshallingTests.array_fixed_out())

    def test_array_fixed_inout(self):
        self.assertEqual(
            [2, 1, 0, -1], GIMarshallingTests.array_fixed_inout([-1, 0, 1, 2])
        )

    def test_array_return(self):
        self.assertEqual([-1, 0, 1, 2], GIMarshallingTests.array_return())

    def test_array_return_etc(self):
        self.assertEqual(([5, 0, 1, 9], 14), GIMarshallingTests.array_return_etc(5, 9))

    def test_array_in(self):
        GIMarshallingTests.array_in(Sequence([-1, 0, 1, 2]))
        GIMarshallingTests.array_in_guint64_len(Sequence([-1, 0, 1, 2]))
        GIMarshallingTests.array_in_guint8_len(Sequence([-1, 0, 1, 2]))

    def test_array_in_len_before(self):
        GIMarshallingTests.array_in_len_before(Sequence([-1, 0, 1, 2]))

    def test_array_in_len_zero_terminated(self):
        GIMarshallingTests.array_in_len_zero_terminated(Sequence([-1, 0, 1, 2]))

    def test_array_uint8_in(self):
        GIMarshallingTests.array_uint8_in(Sequence([97, 98, 99, 100]))
        GIMarshallingTests.array_uint8_in(b"abcd")

    def test_array_string_in(self):
        GIMarshallingTests.array_string_in(["foo", "bar"])

    def test_array_out(self):
        self.assertEqual([-1, 0, 1, 2], GIMarshallingTests.array_out())

    def test_array_out_etc(self):
        self.assertEqual(([-5, 0, 1, 9], 4), GIMarshallingTests.array_out_etc(-5, 9))

    def test_array_inout(self):
        self.assertEqual(
            [-2, -1, 0, 1, 2], GIMarshallingTests.array_inout(Sequence([-1, 0, 1, 2]))
        )

    def test_array_inout_etc(self):
        self.assertEqual(
            ([-5, -1, 0, 1, 9], 4),
            GIMarshallingTests.array_inout_etc(-5, Sequence([-1, 0, 1, 2]), 9),
        )

    def test_method_array_in(self):
        object_ = GIMarshallingTests.Object()
        object_.method_array_in(Sequence([-1, 0, 1, 2]))

    def test_method_array_out(self):
        object_ = GIMarshallingTests.Object()
        self.assertEqual([-1, 0, 1, 2], object_.method_array_out())

    def test_method_array_inout(self):
        object_ = GIMarshallingTests.Object()
        self.assertEqual(
            [-2, -1, 0, 1, 2], object_.method_array_inout(Sequence([-1, 0, 1, 2]))
        )

    def test_method_array_return(self):
        object_ = GIMarshallingTests.Object()
        self.assertEqual([-1, 0, 1, 2], object_.method_array_return())

    def test_array_enum_in(self):
        GIMarshallingTests.array_enum_in(
            [
                GIMarshallingTests.Enum.VALUE1,
                GIMarshallingTests.Enum.VALUE2,
                GIMarshallingTests.Enum.VALUE3,
            ]
        )

    def test_array_boxed_struct_in(self):
        struct1 = GIMarshallingTests.BoxedStruct()
        struct1.long_ = 1
        struct2 = GIMarshallingTests.BoxedStruct()
        struct2.long_ = 2
        struct3 = GIMarshallingTests.BoxedStruct()
        struct3.long_ = 3

        GIMarshallingTests.array_struct_in([struct1, struct2, struct3])

    def test_array_boxed_struct_in_item_marshal_failure(self):
        struct1 = GIMarshallingTests.BoxedStruct()
        struct1.long_ = 1
        struct2 = GIMarshallingTests.BoxedStruct()
        struct2.long_ = 2

        self.assertRaises(
            TypeError,
            GIMarshallingTests.array_struct_in,
            [struct1, struct2, "not_a_struct"],
        )

    def test_array_boxed_struct_value_in(self):
        struct1 = GIMarshallingTests.BoxedStruct()
        struct1.long_ = 1
        struct2 = GIMarshallingTests.BoxedStruct()
        struct2.long_ = 2
        struct3 = GIMarshallingTests.BoxedStruct()
        struct3.long_ = 3

        GIMarshallingTests.array_struct_value_in([struct1, struct2, struct3])

    def test_array_boxed_struct_value_in_item_marshal_failure(self):
        struct1 = GIMarshallingTests.BoxedStruct()
        struct1.long_ = 1
        struct2 = GIMarshallingTests.BoxedStruct()
        struct2.long_ = 2

        self.assertRaises(
            TypeError,
            GIMarshallingTests.array_struct_value_in,
            [struct1, struct2, "not_a_struct"],
        )

    def test_array_boxed_struct_take_in(self):
        struct1 = GIMarshallingTests.BoxedStruct()
        struct1.long_ = 1
        struct2 = GIMarshallingTests.BoxedStruct()
        struct2.long_ = 2
        struct3 = GIMarshallingTests.BoxedStruct()
        struct3.long_ = 3

        GIMarshallingTests.array_struct_take_in([struct1, struct2, struct3])

        self.assertEqual(1, struct1.long_)

    def test_array_boxed_struct_return(self):
        (struct1, struct2, struct3) = (
            GIMarshallingTests.array_zero_terminated_return_struct()
        )
        self.assertEqual(GIMarshallingTests.BoxedStruct, type(struct1))
        self.assertEqual(GIMarshallingTests.BoxedStruct, type(struct2))
        self.assertEqual(GIMarshallingTests.BoxedStruct, type(struct3))
        self.assertEqual(42, struct1.long_)
        self.assertEqual(43, struct2.long_)
        self.assertEqual(44, struct3.long_)

    def test_array_simple_struct_in(self):
        struct1 = GIMarshallingTests.SimpleStruct()
        struct1.long_ = 1
        struct2 = GIMarshallingTests.SimpleStruct()
        struct2.long_ = 2
        struct3 = GIMarshallingTests.SimpleStruct()
        struct3.long_ = 3

        GIMarshallingTests.array_simple_struct_in([struct1, struct2, struct3])

    def test_array_simple_struct_in_item_marshal_failure(self):
        struct1 = GIMarshallingTests.SimpleStruct()
        struct1.long_ = 1
        struct2 = GIMarshallingTests.SimpleStruct()
        struct2.long_ = 2

        self.assertRaises(
            TypeError,
            GIMarshallingTests.array_simple_struct_in,
            [struct1, struct2, "not_a_struct"],
        )

    def test_array_multi_array_key_value_in(self):
        GIMarshallingTests.multi_array_key_value_in(["one", "two", "three"], [1, 2, 3])

    def test_array_in_nonzero_nonlen(self):
        GIMarshallingTests.array_in_nonzero_nonlen(1, b"abcd")

    def test_array_fixed_out_struct(self):
        struct1, struct2 = GIMarshallingTests.array_fixed_out_struct()

        self.assertEqual(7, struct1.long_)
        self.assertEqual(6, struct1.int8)
        self.assertEqual(6, struct2.long_)
        self.assertEqual(7, struct2.int8)

    def test_array_zero_terminated_return(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.array_zero_terminated_return()
        )

    def test_array_zero_terminated_return_null(self):
        self.assertEqual([], GIMarshallingTests.array_zero_terminated_return_null())

    def test_array_zero_terminated_in(self):
        GIMarshallingTests.array_zero_terminated_in(Sequence(["0", "1", "2"]))

    def test_array_zero_terminated_out(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.array_zero_terminated_out()
        )

    def test_array_zero_terminated_inout(self):
        self.assertEqual(
            ["-1", "0", "1", "2"],
            GIMarshallingTests.array_zero_terminated_inout(["0", "1", "2"]),
        )

    def test_init_function(self):
        self.assertEqual((True, []), GIMarshallingTests.init_function([]))
        self.assertEqual((True, []), GIMarshallingTests.init_function(["hello"]))
        self.assertEqual(
            (True, ["hello"]), GIMarshallingTests.init_function(["hello", "world"])
        )

    def test_enum_array_return_type(self):
        self.assertEqual(
            GIMarshallingTests.enum_array_return_type(),
            [
                GIMarshallingTests.ExtraEnum.VALUE1,
                GIMarshallingTests.ExtraEnum.VALUE2,
                GIMarshallingTests.ExtraEnum.VALUE3,
            ],
        )


class TestLengthArray(unittest.TestCase):
    def test_length_array_utf8_none_inout(self):
        assert GIMarshallingTests.length_array_utf8_none_inout(
            ["🅰", "β", "c", "d"]
        ) == ["a", "b", "¢", "🔠"]

    def test_length_array_utf8_full_inout(self):
        assert GIMarshallingTests.length_array_utf8_full_inout(
            ["🅰", "β", "c", "d"]
        ) == ["a", "b", "¢", "🔠"]

    def test_length_array_utf8_optional_inout(self):
        assert GIMarshallingTests.length_array_utf8_optional_inout(
            ["🅰", "β", "c", "d"]
        ) == ["a", "b", "¢", "🔠"]

    def test_length_array_utf8_optional_inout_none_arg(self):
        assert GIMarshallingTests.length_array_utf8_optional_inout(None) == ["a", "b"]

    def test_length_array_utf8_optional_inout_no_arg(self):
        assert GIMarshallingTests.length_array_utf8_optional_inout() == ["a", "b"]


class TestGStrv(unittest.TestCase):
    def test_gstrv_return(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.gstrv_return())

    def test_gstrv_in(self):
        GIMarshallingTests.gstrv_in(Sequence(["0", "1", "2"]))

    def test_gstrv_out(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.gstrv_out())

    def test_gstrv_inout(self):
        self.assertEqual(
            ["-1", "0", "1", "2"], GIMarshallingTests.gstrv_inout(["0", "1", "2"])
        )


class TestArrayGVariant(unittest.TestCase):
    def test_array_gvariant_none_in(self):
        v = [GLib.Variant("i", 27), GLib.Variant("s", "Hello")]
        returned = [
            GLib.Variant.unpack(r) for r in GIMarshallingTests.array_gvariant_none_in(v)
        ]
        self.assertEqual([27, "Hello"], returned)

    def test_array_gvariant_container_in(self):
        v = [GLib.Variant("i", 27), GLib.Variant("s", "Hello")]
        returned = [
            GLib.Variant.unpack(r)
            for r in GIMarshallingTests.array_gvariant_container_in(v)
        ]
        self.assertEqual([27, "Hello"], returned)

    def test_array_gvariant_full_in(self):
        v = [GLib.Variant("i", 27), GLib.Variant("s", "Hello")]
        returned = [
            GLib.Variant.unpack(r) for r in GIMarshallingTests.array_gvariant_full_in(v)
        ]
        self.assertEqual([27, "Hello"], returned)

    def test_bytearray_gvariant(self):
        v = GLib.Variant.new_bytestring(b"foo")
        self.assertEqual(v.get_bytestring(), b"foo")


class TestGArray(unittest.TestCase):
    @unittest.skipUnless(
        hasattr(GIMarshallingTests, "garray_bool_none_in"), "too old gi"
    )
    def test_garray_bool_none_in(self):
        GIMarshallingTests.garray_bool_none_in([True, False, True, True])

    @unittest.skipUnless(
        hasattr(GIMarshallingTests, "garray_unichar_none_in"), "too old gi"
    )
    def test_garray_unichar_none_in(self):
        GIMarshallingTests.garray_unichar_none_in(CONSTANT_UCS4)
        GIMarshallingTests.garray_unichar_none_in(list(CONSTANT_UCS4))

    def test_garray_int_none_return(self):
        self.assertEqual([-1, 0, 1, 2], GIMarshallingTests.garray_int_none_return())

    def test_garray_uint64_none_return(self):
        self.assertEqual(
            [0, GLib.MAXUINT64], GIMarshallingTests.garray_uint64_none_return()
        )

    def test_garray_utf8_none_return(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.garray_utf8_none_return())

    def test_garray_utf8_container_return(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.garray_utf8_container_return()
        )

    def test_garray_utf8_full_return(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.garray_utf8_full_return())

    def test_garray_int_none_in(self):
        GIMarshallingTests.garray_int_none_in(Sequence([-1, 0, 1, 2]))

        self.assertRaises(
            TypeError, GIMarshallingTests.garray_int_none_in, Sequence([-1, "0", 1, 2])
        )

        self.assertRaises(TypeError, GIMarshallingTests.garray_int_none_in, 42)
        self.assertRaises(TypeError, GIMarshallingTests.garray_int_none_in, None)

    def test_garray_uint64_none_in(self):
        GIMarshallingTests.garray_uint64_none_in(Sequence([0, GLib.MAXUINT64]))

    def test_garray_utf8_none_in(self):
        GIMarshallingTests.garray_utf8_none_in(Sequence(["0", "1", "2"]))

    def test_garray_utf8_none_out(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.garray_utf8_none_out())

    def test_garray_utf8_container_out(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.garray_utf8_container_out()
        )

    def test_garray_utf8_full_out(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.garray_utf8_full_out())

    def test_garray_utf8_full_out_caller_allocated(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.garray_utf8_full_out_caller_allocated()
        )

    def test_garray_utf8_none_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.garray_utf8_none_inout(Sequence(("0", "1", "2"))),
        )

    def test_garray_utf8_container_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.garray_utf8_container_inout(["0", "1", "2"]),
        )

    def test_garray_utf8_full_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.garray_utf8_full_inout(["0", "1", "2"]),
        )


class TestGPtrArray(unittest.TestCase):
    def test_gptrarray_utf8_none_return(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.gptrarray_utf8_none_return()
        )

    def test_gptrarray_utf8_container_return(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.gptrarray_utf8_container_return()
        )

    def test_gptrarray_utf8_full_return(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.gptrarray_utf8_full_return()
        )

    def test_gptrarray_utf8_none_in(self):
        GIMarshallingTests.gptrarray_utf8_none_in(Sequence(["0", "1", "2"]))

    def test_gptrarray_utf8_none_out(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.gptrarray_utf8_none_out())

    def test_gptrarray_utf8_container_out(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.gptrarray_utf8_container_out()
        )

    def test_gptrarray_utf8_full_out(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.gptrarray_utf8_full_out())

    def test_gptrarray_utf8_none_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.gptrarray_utf8_none_inout(Sequence(("0", "1", "2"))),
        )

    def test_gptrarray_utf8_container_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.gptrarray_utf8_container_inout(["0", "1", "2"]),
        )

    def test_gptrarray_utf8_full_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.gptrarray_utf8_full_inout(["0", "1", "2"]),
        )


class TestGBytes(unittest.TestCase):
    def test_gbytes_create(self):
        b = GLib.Bytes.new(b"\x00\x01\xff")
        self.assertEqual(3, b.get_size())
        self.assertEqual(b"\x00\x01\xff", b.get_data())

    def test_gbytes_create_take(self):
        b = GLib.Bytes.new_take(b"\x00\x01\xff")
        self.assertEqual(3, b.get_size())
        self.assertEqual(b"\x00\x01\xff", b.get_data())

    def test_gbytes_full_return(self):
        b = GIMarshallingTests.gbytes_full_return()
        self.assertEqual(4, b.get_size())
        self.assertEqual(b"\x00\x31\xff\x33", b.get_data())

    def test_gbytes_none_in(self):
        b = GIMarshallingTests.gbytes_full_return()
        GIMarshallingTests.gbytes_none_in(b)

    def test_compare(self):
        a1 = GLib.Bytes.new(b"\x00\x01\xff")
        a2 = GLib.Bytes.new(b"\x00\x01\xff")
        b = GLib.Bytes.new(b"\x00\x01\xfe")

        self.assertTrue(a1.equal(a2))
        self.assertTrue(a2.equal(a1))
        self.assertFalse(a1.equal(b))
        self.assertFalse(b.equal(a2))

        self.assertEqual(0, a1.compare(a2))
        self.assertLess(0, a1.compare(b))
        self.assertGreater(0, b.compare(a1))


class TestGByteArray(unittest.TestCase):
    def test_new(self):
        ba = GLib.ByteArray.new()
        self.assertEqual(b"", ba)

        ba = GLib.ByteArray.new_take(b"\x01\x02\xff")
        self.assertEqual(b"\x01\x02\xff", ba)

    def test_bytearray_full_return(self):
        self.assertEqual(b"\x001\xff3", GIMarshallingTests.bytearray_full_return())

    def test_bytearray_none_in(self):
        b = b"\x00\x31\xff\x33"
        ba = GLib.ByteArray.new_take(b)

        # b should always have the same value even
        # though the generated GByteArray is being modified
        GIMarshallingTests.bytearray_none_in(b)
        GIMarshallingTests.bytearray_none_in(b)

        # The GByteArray is just a bytes
        # thus it will not reflect any changes
        GIMarshallingTests.bytearray_none_in(ba)
        GIMarshallingTests.bytearray_none_in(ba)


class TestGList(unittest.TestCase):
    def test_glist_int_none_return(self):
        self.assertEqual([-1, 0, 1, 2], GIMarshallingTests.glist_int_none_return())

    def test_glist_uint32_none_return(self):
        self.assertEqual(
            [0, GLib.MAXUINT32], GIMarshallingTests.glist_uint32_none_return()
        )

    def test_glist_utf8_none_return(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.glist_utf8_none_return())

    def test_glist_utf8_container_return(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.glist_utf8_container_return()
        )

    def test_glist_utf8_full_return(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.glist_utf8_full_return())

    def test_glist_int_none_in(self):
        GIMarshallingTests.glist_int_none_in(Sequence((-1, 0, 1, 2)))

        self.assertRaises(
            TypeError, GIMarshallingTests.glist_int_none_in, Sequence((-1, "0", 1, 2))
        )

        self.assertRaises(TypeError, GIMarshallingTests.glist_int_none_in, 42)
        self.assertRaises(TypeError, GIMarshallingTests.glist_int_none_in, None)

    def test_glist_int_none_in_error_getitem(self):
        class FailingSequence(Sequence):
            def __getitem__(self, key):
                raise Exception

        self.assertRaises(
            Exception,
            GIMarshallingTests.glist_int_none_in,
            FailingSequence((-1, 0, 1, 2)),
        )

    def test_glist_uint32_none_in(self):
        GIMarshallingTests.glist_uint32_none_in(Sequence((0, GLib.MAXUINT32)))

    def test_glist_utf8_none_in(self):
        GIMarshallingTests.glist_utf8_none_in(Sequence(("0", "1", "2")))

    def test_glist_utf8_none_out(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.glist_utf8_none_out())

    def test_glist_utf8_container_out(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.glist_utf8_container_out())

    def test_glist_utf8_full_out(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.glist_utf8_full_out())

    def test_glist_utf8_none_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.glist_utf8_none_inout(Sequence(("0", "1", "2"))),
        )

    def test_glist_utf8_container_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.glist_utf8_container_inout(("0", "1", "2")),
        )

    def test_glist_utf8_full_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.glist_utf8_full_inout(("0", "1", "2")),
        )


class TestGSList(unittest.TestCase):
    def test_gslist_int_none_return(self):
        self.assertEqual([-1, 0, 1, 2], GIMarshallingTests.gslist_int_none_return())

    def test_gslist_utf8_none_return(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.gslist_utf8_none_return())

    def test_gslist_utf8_container_return(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.gslist_utf8_container_return()
        )

    def test_gslist_utf8_full_return(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.gslist_utf8_full_return())

    def test_gslist_int_none_in(self):
        GIMarshallingTests.gslist_int_none_in(Sequence((-1, 0, 1, 2)))

        self.assertRaises(
            TypeError, GIMarshallingTests.gslist_int_none_in, Sequence((-1, "0", 1, 2))
        )

        self.assertRaises(TypeError, GIMarshallingTests.gslist_int_none_in, 42)
        self.assertRaises(TypeError, GIMarshallingTests.gslist_int_none_in, None)

    def test_gslist_int_none_in_error_getitem(self):
        class FailingSequence(Sequence):
            def __getitem__(self, key):
                raise Exception

        self.assertRaises(
            Exception,
            GIMarshallingTests.gslist_int_none_in,
            FailingSequence((-1, 0, 1, 2)),
        )

    def test_gslist_utf8_none_in(self):
        GIMarshallingTests.gslist_utf8_none_in(Sequence(("0", "1", "2")))

    def test_gslist_utf8_none_out(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.gslist_utf8_none_out())

    def test_gslist_utf8_container_out(self):
        self.assertEqual(
            ["0", "1", "2"], GIMarshallingTests.gslist_utf8_container_out()
        )

    def test_gslist_utf8_full_out(self):
        self.assertEqual(["0", "1", "2"], GIMarshallingTests.gslist_utf8_full_out())

    def test_gslist_utf8_none_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.gslist_utf8_none_inout(Sequence(("0", "1", "2"))),
        )

    def test_gslist_utf8_container_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.gslist_utf8_container_inout(("0", "1", "2")),
        )

    def test_gslist_utf8_full_inout(self):
        self.assertEqual(
            ["-2", "-1", "0", "1"],
            GIMarshallingTests.gslist_utf8_full_inout(("0", "1", "2")),
        )


class TestGHashTable(unittest.TestCase):
    @unittest.skip("broken")
    def test_ghashtable_double_in(self):
        GIMarshallingTests.ghashtable_double_in(
            {"-1": -0.1, "0": 0.0, "1": 0.1, "2": 0.2}
        )

    @unittest.skip("broken")
    def test_ghashtable_float_in(self):
        GIMarshallingTests.ghashtable_float_in(
            {"-1": -0.1, "0": 0.0, "1": 0.1, "2": 0.2}
        )

    @unittest.skip("broken")
    def test_ghashtable_int64_in(self):
        GIMarshallingTests.ghashtable_int64_in(
            {"-1": GLib.MAXUINT32 + 1, "0": 0, "1": 1, "2": 2}
        )

    @unittest.skip("broken")
    def test_ghashtable_uint64_in(self):
        GIMarshallingTests.ghashtable_uint64_in(
            {"-1": GLib.MAXUINT32 + 1, "0": 0, "1": 1, "2": 2}
        )

    def test_ghashtable_int_none_return(self):
        self.assertEqual(
            {-1: 1, 0: 0, 1: -1, 2: -2}, GIMarshallingTests.ghashtable_int_none_return()
        )

    def test_ghashtable_int_none_return2(self):
        self.assertEqual(
            {"-1": "1", "0": "0", "1": "-1", "2": "-2"},
            GIMarshallingTests.ghashtable_utf8_none_return(),
        )

    def test_ghashtable_int_container_return(self):
        self.assertEqual(
            {"-1": "1", "0": "0", "1": "-1", "2": "-2"},
            GIMarshallingTests.ghashtable_utf8_container_return(),
        )

    def test_ghashtable_int_full_return(self):
        self.assertEqual(
            {"-1": "1", "0": "0", "1": "-1", "2": "-2"},
            GIMarshallingTests.ghashtable_utf8_full_return(),
        )

    def test_ghashtable_int_none_in(self):
        GIMarshallingTests.ghashtable_int_none_in({-1: 1, 0: 0, 1: -1, 2: -2})

        self.assertRaises(
            TypeError,
            GIMarshallingTests.ghashtable_int_none_in,
            {-1: 1, "0": 0, 1: -1, 2: -2},
        )
        self.assertRaises(
            TypeError,
            GIMarshallingTests.ghashtable_int_none_in,
            {-1: 1, 0: "0", 1: -1, 2: -2},
        )

        self.assertRaises(
            TypeError,
            GIMarshallingTests.ghashtable_int_none_in,
            "{-1: 1, 0: 0, 1: -1, 2: -2}",
        )
        self.assertRaises(TypeError, GIMarshallingTests.ghashtable_int_none_in, None)

    def test_ghashtable_utf8_none_in(self):
        GIMarshallingTests.ghashtable_utf8_none_in(
            {"-1": "1", "0": "0", "1": "-1", "2": "-2"}
        )

    def test_ghashtable_utf8_none_out(self):
        self.assertEqual(
            {"-1": "1", "0": "0", "1": "-1", "2": "-2"},
            GIMarshallingTests.ghashtable_utf8_none_out(),
        )

    def test_ghashtable_utf8_container_out(self):
        self.assertEqual(
            {"-1": "1", "0": "0", "1": "-1", "2": "-2"},
            GIMarshallingTests.ghashtable_utf8_container_out(),
        )

    def test_ghashtable_utf8_full_out(self):
        self.assertEqual(
            {"-1": "1", "0": "0", "1": "-1", "2": "-2"},
            GIMarshallingTests.ghashtable_utf8_full_out(),
        )

    def test_ghashtable_utf8_none_inout(self):
        i = {"-1": "1", "0": "0", "1": "-1", "2": "-2"}
        self.assertEqual(
            {"-1": "1", "0": "0", "1": "1"},
            GIMarshallingTests.ghashtable_utf8_none_inout(i),
        )

    def test_ghashtable_utf8_container_inout(self):
        i = {"-1": "1", "0": "0", "1": "-1", "2": "-2"}
        self.assertEqual(
            {"-1": "1", "0": "0", "1": "1"},
            GIMarshallingTests.ghashtable_utf8_container_inout(i),
        )

    def test_ghashtable_utf8_full_inout(self):
        i = {"-1": "1", "0": "0", "1": "-1", "2": "-2"}
        self.assertEqual(
            {"-1": "1", "0": "0", "1": "1"},
            GIMarshallingTests.ghashtable_utf8_full_inout(i),
        )

    def test_ghashtable_enum_none_in(self):
        GIMarshallingTests.ghashtable_enum_none_in(
            {
                1: GIMarshallingTests.ExtraEnum.VALUE1,
                2: GIMarshallingTests.ExtraEnum.VALUE2,
                3: GIMarshallingTests.ExtraEnum.VALUE3,
            }
        )

    def test_ghashtable_enum_none_return(self):
        self.assertEqual(
            {
                1: GIMarshallingTests.ExtraEnum.VALUE1,
                2: GIMarshallingTests.ExtraEnum.VALUE2,
                3: GIMarshallingTests.ExtraEnum.VALUE3,
            },
            GIMarshallingTests.ghashtable_enum_none_return(),
        )


class TestGValue(unittest.TestCase):
    def test_gvalue_return(self):
        self.assertEqual(42, GIMarshallingTests.gvalue_return())

    def test_gvalue_in(self):
        GIMarshallingTests.gvalue_in(42)
        value = GObject.Value(GObject.TYPE_INT, 42)
        GIMarshallingTests.gvalue_in(value)

    def test_gvalue_in_with_modification(self):
        value = GObject.Value(GObject.TYPE_INT, 42)
        GIMarshallingTests.gvalue_in_with_modification(value)
        self.assertEqual(value.get_int(), 24)

    def test_gvalue_int64_in(self):
        value = GObject.Value(GObject.TYPE_INT64, GLib.MAXINT64)
        GIMarshallingTests.gvalue_int64_in(value)

    def test_gvalue_in_with_type(self):
        value = GObject.Value(GObject.TYPE_STRING, "foo")
        GIMarshallingTests.gvalue_in_with_type(value, GObject.TYPE_STRING)

        value = GObject.Value(
            GIMarshallingTests.Flags.__gtype__, GIMarshallingTests.Flags.VALUE1
        )
        GIMarshallingTests.gvalue_in_with_type(value, GObject.TYPE_FLAGS)

    def test_gvalue_in_enum(self):
        # It is unclear what GIMarshallingTests expects here, since
        # GIMarshallingTests.Enum is an unregistered type.
        # https://gitlab.gnome.org/GNOME/gobject-introspection-tests/-/issues/8
        value = GObject.Value(
            GIMarshallingTests.GEnum.__gtype__, GIMarshallingTests.GEnum.VALUE3
        )
        GIMarshallingTests.gvalue_in_enum(value)

    def test_gvalue_out(self):
        self.assertEqual(42, GIMarshallingTests.gvalue_out())

    def test_gvalue_int64_out(self):
        self.assertEqual(GLib.MAXINT64, GIMarshallingTests.gvalue_int64_out())

    def test_gvalue_out_caller_allocates(self):
        self.assertEqual(42, GIMarshallingTests.gvalue_out_caller_allocates())

    def test_gvalue_inout(self):
        self.assertEqual("42", GIMarshallingTests.gvalue_inout(42))
        value = GObject.Value(int, 42)
        self.assertEqual("42", GIMarshallingTests.gvalue_inout(value))

    def test_gvalue_flat_array_in(self):
        # the function already asserts the correct values
        GIMarshallingTests.gvalue_flat_array([42, "42", True])

    def test_gvalue_flat_array_in_item_marshal_failure(self):
        # Tests the failure to marshal 2^256 to a GValue mid-way through the array marshaling.
        self.assertRaises(
            OverflowError, GIMarshallingTests.gvalue_flat_array, [42, 2**256, True]
        )

        self.assertRaises(
            OverflowError,
            GIMarshallingTests.gvalue_flat_array,
            [GLib.MAXINT + 1, "42", True],
        )
        self.assertRaises(
            OverflowError,
            GIMarshallingTests.gvalue_flat_array,
            [GLib.MININT - 1, "42", True],
        )

        # FIXME: https://gitlab.gnome.org/GNOME/pygobject/-/issues/582#note_1764164
        exc_prefix = "Item 0: " if sys.version_info[:2] < (3, 12) else ""

        with pytest.raises(
            OverflowError,
            match=exc_prefix
            + f"{GLib.MAXINT + 1:d} not in range {GLib.MININT:d} to {GLib.MAXINT:d}",
        ):
            GIMarshallingTests.gvalue_flat_array([GLib.MAXINT + 1, "42", True])

        min_, max_ = GLib.MININT, GLib.MAXINT

        with pytest.raises(
            OverflowError,
            match=exc_prefix
            + f"{GLib.MAXUINT64 * 2:d} not in range {min_:d} to {max_:d}",
        ):
            GIMarshallingTests.gvalue_flat_array([GLib.MAXUINT64 * 2, "42", True])

    def test_gvalue_flat_array_out(self):
        values = GIMarshallingTests.return_gvalue_flat_array()
        self.assertEqual(values, [42, "42", True])

    def test_gvalue_gobject_ref_counts_simple(self):
        obj = GObject.Object()
        grefcount = obj.__grefcount__
        value = GObject.Value(GObject.TYPE_OBJECT, obj)
        del value
        gc.collect()
        gc.collect()
        assert obj.__grefcount__ == grefcount

    def test_gvalue_gobject_ref_counts(self):
        # Tests a GObject held by a GValue
        obj = GObject.Object()
        ref = weakref.ref(obj)
        grefcount = obj.__grefcount__

        value = GObject.Value()
        value.init(GObject.TYPE_OBJECT)

        # TYPE_OBJECT will inc ref count as it should
        value.set_object(obj)
        self.assertEqual(obj.__grefcount__, grefcount + 1)

        # multiple set_object should not inc ref count
        value.set_object(obj)
        self.assertEqual(obj.__grefcount__, grefcount + 1)

        # get_object will re-use the same wrapper as obj
        res = value.get_object()
        self.assertEqual(obj, res)
        self.assertEqual(obj.__grefcount__, grefcount + 1)

        # multiple get_object should not inc ref count
        res = value.get_object()
        self.assertEqual(obj.__grefcount__, grefcount + 1)

        # deletion of the result and value holder should bring the
        # refcount back to where we started
        del res
        del value
        gc.collect()
        gc.collect()
        self.assertEqual(obj.__grefcount__, grefcount)

        del obj
        gc.collect()
        self.assertEqual(ref(), None)

    @unittest.skipUnless(hasattr(sys, "getrefcount"), "no sys.getrefcount")
    def test_gvalue_boxed_ref_counts(self):
        # Tests a boxed type wrapping a python object pointer (TYPE_PYOBJECT)
        # held by a GValue
        class Obj:
            pass

        obj = Obj()
        ref = weakref.ref(obj)
        refcount = sys.getrefcount(obj)

        value = GObject.Value()
        value.init(GObject.TYPE_PYOBJECT)

        # boxed TYPE_PYOBJECT will inc ref count as it should
        value.set_boxed(obj)
        self.assertEqual(sys.getrefcount(obj), refcount + 1)

        # multiple set_boxed should not inc ref count
        value.set_boxed(obj)
        self.assertEqual(sys.getrefcount(obj), refcount + 1)

        res = value.get_boxed()
        self.assertEqual(obj, res)
        self.assertEqual(sys.getrefcount(obj), refcount + 2)

        # multiple get_boxed should not inc ref count
        res = value.get_boxed()
        self.assertEqual(sys.getrefcount(obj), refcount + 2)

        # deletion of the result and value holder should bring the
        # refcount back to where we started
        del res
        del value
        gc.collect()
        self.assertEqual(sys.getrefcount(obj), refcount)

        del obj
        gc.collect()
        self.assertEqual(ref(), None)

    @unittest.skip("broken")
    def test_gvalue_flat_array_round_trip(self):
        self.assertEqual(
            [42, "42", True],
            GIMarshallingTests.gvalue_flat_array_round_trip(42, "42", True),
        )

    def test_gvalue_float(self):
        GIMarshallingTests.gvalue_float(GObject.Float(3.14), 3.14)


class TestGClosure(unittest.TestCase):
    def test_in(self):
        GIMarshallingTests.gclosure_in(lambda: 42)

    def test_in_partial(self):
        from functools import partial

        called_args = []
        called_kwargs = {}

        def callback(*args, **kwargs):
            called_args.extend(args)
            called_kwargs.update(kwargs)
            return 42

        func = partial(callback, 1, 2, 3, foo=42)
        GIMarshallingTests.gclosure_in(func)
        assert called_args == [1, 2, 3]
        assert called_kwargs["foo"] == 42

    def test_pass(self):
        # test passing a closure between two C calls
        closure = GIMarshallingTests.gclosure_return()
        GIMarshallingTests.gclosure_in(closure)

    def test_type_error(self):
        self.assertRaises(TypeError, GIMarshallingTests.gclosure_in, 42)
        self.assertRaises(TypeError, GIMarshallingTests.gclosure_in, None)


class TestCallbacks(unittest.TestCase):
    def test_return_value_only(self):
        def cb():
            return 5

        self.assertEqual(GIMarshallingTests.callback_return_value_only(cb), 5)

    def test_one_out_arg(self):
        def cb():
            return 5.5

        self.assertAlmostEqual(GIMarshallingTests.callback_one_out_parameter(cb), 5.5)

    def test_multiple_out_args(self):
        def cb():
            return (5.5, 42.0)

        res = GIMarshallingTests.callback_multiple_out_parameters(cb)
        self.assertAlmostEqual(res[0], 5.5)
        self.assertAlmostEqual(res[1], 42.0)

    def test_return_and_one_out_arg(self):
        def cb():
            return (5, 42.0)

        res = GIMarshallingTests.callback_return_value_and_one_out_parameter(cb)
        self.assertEqual(res[0], 5)
        self.assertAlmostEqual(res[1], 42.0)

    def test_return_and_multiple_out_arg(self):
        def cb():
            return (5, 42, -1000)

        self.assertEqual(
            GIMarshallingTests.callback_return_value_and_multiple_out_parameters(cb),
            (5, 42, -1000),
        )


class TestPointer(unittest.TestCase):
    def test_pointer_in_return(self):
        self.assertEqual(GIMarshallingTests.pointer_in_return(42), 42)


class TestEnum(unittest.TestCase):
    def test_enum(self):
        self.assertTrue(issubclass(GIMarshallingTests.Enum, int))
        self.assertTrue(
            isinstance(GIMarshallingTests.Enum.VALUE1, GIMarshallingTests.Enum)
        )
        self.assertTrue(
            isinstance(GIMarshallingTests.Enum.VALUE2, GIMarshallingTests.Enum)
        )
        self.assertTrue(
            isinstance(GIMarshallingTests.Enum.VALUE3, GIMarshallingTests.Enum)
        )
        self.assertEqual(42, GIMarshallingTests.Enum.VALUE3)

    def test_enum_in(self):
        GIMarshallingTests.enum_in(GIMarshallingTests.Enum.VALUE3)
        GIMarshallingTests.enum_in(42)

        self.assertRaises(TypeError, GIMarshallingTests.enum_in, 43)
        self.assertRaises(
            TypeError, GIMarshallingTests.enum_in, "GIMarshallingTests.Enum.VALUE3"
        )

    def test_enum_return(self):
        enum = GIMarshallingTests.enum_returnv()
        self.assertTrue(isinstance(enum, GIMarshallingTests.Enum))
        self.assertEqual(enum, GIMarshallingTests.Enum.VALUE3)

    def test_enum_out(self):
        enum = GIMarshallingTests.enum_out()
        self.assertTrue(isinstance(enum, GIMarshallingTests.Enum))
        self.assertEqual(enum, GIMarshallingTests.Enum.VALUE3)

    def test_enum_inout(self):
        enum = GIMarshallingTests.enum_inout(GIMarshallingTests.Enum.VALUE3)
        self.assertTrue(isinstance(enum, GIMarshallingTests.Enum))
        self.assertEqual(enum, GIMarshallingTests.Enum.VALUE1)

    def test_enum_second(self):
        # check for the bug where different non-gtype enums share the same class
        self.assertNotEqual(GIMarshallingTests.Enum, GIMarshallingTests.SecondEnum)

        # check that values are not being shared between different enums
        self.assertTrue(hasattr(GIMarshallingTests.SecondEnum, "SECONDVALUE1"))
        self.assertRaises(
            AttributeError, getattr, GIMarshallingTests.Enum, "SECONDVALUE1"
        )
        self.assertTrue(hasattr(GIMarshallingTests.Enum, "VALUE1"))
        self.assertRaises(
            AttributeError, getattr, GIMarshallingTests.SecondEnum, "VALUE1"
        )

    def test_enum_has_no_gtype(self):
        # GIMarshallingTests.Enum is not registered with the GType system
        self.assertFalse(hasattr(GIMarshallingTests.Enum, "__gtype__"))
        self.assertFalse(isinstance(GIMarshallingTests.Enum, GObject.GEnum))

    def test_enum_add_type_error(self):
        with self.assertRaises(TypeError):
            gi._gi.enum_add(
                self,
                "Flags",
                GIMarshallingTests.Flags.__gtype__,
                GIMarshallingTests.Flags.__info__,
            )

    def test_enum_add_module_type_error(self):
        with self.assertRaises(TypeError):
            gi._gi.flags_add(
                "self",
                "GEnum",
                GIMarshallingTests.GEnum.__gtype__,
                GIMarshallingTests.GEnum.__info__,
            )

    def test_type_module_name(self):
        self.assertEqual(GIMarshallingTests.Enum.__name__, "Enum")
        self.assertEqual(
            GIMarshallingTests.Enum.__module__, "gi.repository.GIMarshallingTests"
        )

    def test_hash(self):
        assert hash(GIMarshallingTests.Enum.VALUE1) == hash(
            GIMarshallingTests.Enum(GIMarshallingTests.Enum.VALUE1)
        )

    def test_repr(self):
        self.assertEqual(repr(GIMarshallingTests.Enum.VALUE3), "<Enum.VALUE3: 42>")

    def test_enum_field_set(self):
        option = GLib.OptionEntry()
        # GLib.OptionEntry.arg is of type GLib.OptionArg, which is not
        # registered with the GObject type system by libglib.
        option.arg = GLib.OptionArg.NONE


class TestEnumVFuncResults(unittest.TestCase):
    class EnumTester(GIMarshallingTests.Object):
        def do_vfunc_return_enum(self):
            return GIMarshallingTests.Enum.VALUE2

        def do_vfunc_out_enum(self):
            return GIMarshallingTests.Enum.VALUE3

    def test_vfunc_return_enum(self):
        tester = self.EnumTester()
        self.assertEqual(tester.vfunc_return_enum(), GIMarshallingTests.Enum.VALUE2)

    def test_vfunc_out_enum(self):
        tester = self.EnumTester()
        self.assertEqual(tester.vfunc_out_enum(), GIMarshallingTests.Enum.VALUE3)


class TestGEnum(unittest.TestCase):
    def test_genum(self):
        self.assertTrue(issubclass(GIMarshallingTests.GEnum, GObject.GEnum))
        self.assertTrue(
            isinstance(GIMarshallingTests.GEnum.VALUE1, GIMarshallingTests.GEnum)
        )
        self.assertTrue(
            isinstance(GIMarshallingTests.GEnum.VALUE2, GIMarshallingTests.GEnum)
        )
        self.assertTrue(
            isinstance(GIMarshallingTests.GEnum.VALUE3, GIMarshallingTests.GEnum)
        )
        self.assertEqual(42, GIMarshallingTests.GEnum.VALUE3)

    def test_pickle(self):
        v = GIMarshallingTests.GEnum.VALUE3
        new_v = pickle.loads(pickle.dumps(v))
        assert new_v == v
        assert isinstance(new_v, GIMarshallingTests.GEnum)

    def test_value_nick_and_name(self):
        self.assertEqual(GIMarshallingTests.GEnum.VALUE1.value_nick, "value1")
        self.assertEqual(GIMarshallingTests.GEnum.VALUE2.value_nick, "value2")
        self.assertEqual(GIMarshallingTests.GEnum.VALUE3.value_nick, "value3")

        self.assertEqual(
            GIMarshallingTests.GEnum.VALUE1.value_name,
            "GI_MARSHALLING_TESTS_GENUM_VALUE1",
        )
        self.assertEqual(
            GIMarshallingTests.GEnum.VALUE2.value_name,
            "GI_MARSHALLING_TESTS_GENUM_VALUE2",
        )
        self.assertEqual(
            GIMarshallingTests.GEnum.VALUE3.value_name,
            "GI_MARSHALLING_TESTS_GENUM_VALUE3",
        )

    def test_genum_in(self):
        GIMarshallingTests.genum_in(GIMarshallingTests.GEnum.VALUE3)
        GIMarshallingTests.genum_in(42)
        GIMarshallingTests.GEnum.in_(42)

        self.assertRaises(TypeError, GIMarshallingTests.genum_in, 43)
        self.assertRaises(
            TypeError, GIMarshallingTests.genum_in, "GIMarshallingTests.GEnum.VALUE3"
        )

    def test_genum_return(self):
        genum = GIMarshallingTests.genum_returnv()
        self.assertTrue(isinstance(genum, GIMarshallingTests.GEnum))
        self.assertEqual(genum, GIMarshallingTests.GEnum.VALUE3)

    def test_genum_out(self):
        genum = GIMarshallingTests.genum_out()
        genum = GIMarshallingTests.GEnum.out()
        self.assertTrue(isinstance(genum, GIMarshallingTests.GEnum))
        self.assertEqual(genum, GIMarshallingTests.GEnum.VALUE3)

    def test_genum_inout(self):
        genum = GIMarshallingTests.genum_inout(GIMarshallingTests.GEnum.VALUE3)
        self.assertTrue(isinstance(genum, GIMarshallingTests.GEnum))
        self.assertEqual(genum, GIMarshallingTests.GEnum.VALUE1)

    def test_type_module_name(self):
        self.assertEqual(GIMarshallingTests.GEnum.__name__, "GEnum")
        self.assertEqual(
            GIMarshallingTests.GEnum.__module__, "gi.repository.GIMarshallingTests"
        )

    def test_enum_values_property(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", PyGIDeprecationWarning)

            assert GIMarshallingTests.GEnum.__enum_values__ == {
                0: GIMarshallingTests.GEnum.VALUE1,
                1: GIMarshallingTests.GEnum.VALUE2,
                42: GIMarshallingTests.GEnum.VALUE3,
            }

    def test_hash(self):
        assert hash(GIMarshallingTests.GEnum.VALUE3) == hash(
            GIMarshallingTests.GEnum(GIMarshallingTests.GEnum.VALUE3)
        )

    def test_repr(self):
        self.assertEqual(repr(GIMarshallingTests.GEnum.VALUE3), "<GEnum.VALUE3: 42>")


class TestGFlags(unittest.TestCase):
    def test_flags(self):
        self.assertTrue(issubclass(GIMarshallingTests.Flags, GObject.GFlags))
        self.assertTrue(
            isinstance(GIMarshallingTests.Flags.VALUE1, GIMarshallingTests.Flags)
        )
        self.assertTrue(
            isinstance(GIMarshallingTests.Flags.VALUE2, GIMarshallingTests.Flags)
        )
        self.assertTrue(
            isinstance(GIMarshallingTests.Flags.VALUE3, GIMarshallingTests.Flags)
        )
        # __or__() operation should still return an instance, not an int.
        self.assertTrue(
            isinstance(
                GIMarshallingTests.Flags.VALUE1 | GIMarshallingTests.Flags.VALUE2,
                GIMarshallingTests.Flags,
            )
        )
        self.assertEqual(1 << 1, GIMarshallingTests.Flags.VALUE2)

    def test_value_nick_and_name(self):
        self.assertEqual(GIMarshallingTests.Flags.VALUE1.first_value_nick, "value1")
        self.assertEqual(GIMarshallingTests.Flags.VALUE2.first_value_nick, "value2")
        self.assertEqual(GIMarshallingTests.Flags.VALUE3.first_value_nick, "value3")

        self.assertEqual(
            GIMarshallingTests.Flags.VALUE1.first_value_name,
            "GI_MARSHALLING_TESTS_FLAGS_VALUE1",
        )
        self.assertEqual(
            GIMarshallingTests.Flags.VALUE2.first_value_name,
            "GI_MARSHALLING_TESTS_FLAGS_VALUE2",
        )
        self.assertEqual(
            GIMarshallingTests.Flags.VALUE3.first_value_name,
            "GI_MARSHALLING_TESTS_FLAGS_VALUE3",
        )

    def test_flags_in(self):
        GIMarshallingTests.flags_in(GIMarshallingTests.Flags.VALUE2)
        GIMarshallingTests.Flags.in_(GIMarshallingTests.Flags.VALUE2)
        # result of __or__() operation should still be valid instance, not an int.
        GIMarshallingTests.flags_in(
            GIMarshallingTests.Flags.VALUE2 | GIMarshallingTests.Flags.VALUE2
        )
        GIMarshallingTests.flags_in_zero(Number(0))
        GIMarshallingTests.Flags.in_zero(Number(0))

        self.assertRaises(TypeError, GIMarshallingTests.flags_in, 1 << 1)
        self.assertRaises(
            TypeError, GIMarshallingTests.flags_in, "GIMarshallingTests.Flags.VALUE2"
        )

    def test_flags_return(self):
        flags = GIMarshallingTests.flags_returnv()
        self.assertTrue(isinstance(flags, GIMarshallingTests.Flags))
        self.assertEqual(flags, GIMarshallingTests.Flags.VALUE2)

    def test_flags_return_method(self):
        flags = GIMarshallingTests.Flags.returnv()
        self.assertTrue(isinstance(flags, GIMarshallingTests.Flags))
        self.assertEqual(flags, GIMarshallingTests.Flags.VALUE2)

    def test_flags_out(self):
        flags = GIMarshallingTests.flags_out()
        self.assertTrue(isinstance(flags, GIMarshallingTests.Flags))
        self.assertEqual(flags, GIMarshallingTests.Flags.VALUE2)

    def test_flags_inout(self):
        flags = GIMarshallingTests.flags_inout(GIMarshallingTests.Flags.VALUE2)
        self.assertTrue(isinstance(flags, GIMarshallingTests.Flags))
        self.assertEqual(flags, GIMarshallingTests.Flags.VALUE1)

    def test_type_module_name(self):
        self.assertEqual(GIMarshallingTests.Flags.__name__, "Flags")
        self.assertEqual(
            GIMarshallingTests.Flags.__module__, "gi.repository.GIMarshallingTests"
        )

    def test_flags_values_property(self):
        flags_values = {
            1: GIMarshallingTests.Flags.VALUE1,
            2: GIMarshallingTests.Flags.VALUE2,
            4: GIMarshallingTests.Flags.VALUE3,
        }

        # Since Python 3.11 only primary values are considered.
        # See https://docs.python.org/3/whatsnew/3.11.html#enum
        if sys.version_info[:2] < (3, 11):
            flags_values[3] = GIMarshallingTests.Flags.MASK

        with warnings.catch_warnings():
            warnings.simplefilter("ignore", PyGIDeprecationWarning)

            assert GIMarshallingTests.Flags.__flags_values__ == flags_values

    def test_repr(self):
        self.assertEqual(repr(GIMarshallingTests.Flags.VALUE2), "<Flags.VALUE2: 2>")
        self.assertIn(
            repr(GIMarshallingTests.Flags.VALUE2 | GIMarshallingTests.Flags.VALUE3),
            {"<Flags.VALUE2|VALUE3: 6>", "<Flags.VALUE3|VALUE2: 6>"},
        )

    def test_hash(self):
        assert hash(GIMarshallingTests.Flags.VALUE2) == hash(
            GIMarshallingTests.Flags(GIMarshallingTests.Flags.VALUE2)
        )

    def test_flags_large_in(self):
        GIMarshallingTests.extra_flags_large_in(GIMarshallingTests.ExtraFlags.VALUE2)


class TestNoTypeFlags(unittest.TestCase):
    def test_flags(self):
        self.assertTrue(issubclass(GIMarshallingTests.NoTypeFlags, enum.IntFlag))
        self.assertTrue(
            isinstance(
                GIMarshallingTests.NoTypeFlags.VALUE1, GIMarshallingTests.NoTypeFlags
            )
        )
        self.assertTrue(
            isinstance(
                GIMarshallingTests.NoTypeFlags.VALUE2, GIMarshallingTests.NoTypeFlags
            )
        )
        self.assertTrue(
            isinstance(
                GIMarshallingTests.NoTypeFlags.VALUE3, GIMarshallingTests.NoTypeFlags
            )
        )
        # __or__() operation should still return an instance, not an int.
        self.assertTrue(
            isinstance(
                GIMarshallingTests.NoTypeFlags.VALUE1
                | GIMarshallingTests.NoTypeFlags.VALUE2,
                GIMarshallingTests.NoTypeFlags,
            )
        )
        self.assertEqual(1 << 1, GIMarshallingTests.NoTypeFlags.VALUE2)

    def test_flags_in(self):
        GIMarshallingTests.no_type_flags_in(GIMarshallingTests.NoTypeFlags.VALUE2)
        GIMarshallingTests.no_type_flags_in(
            GIMarshallingTests.NoTypeFlags.VALUE2
            | GIMarshallingTests.NoTypeFlags.VALUE2
        )
        GIMarshallingTests.no_type_flags_in_zero(Number(0))

        self.assertRaises(TypeError, GIMarshallingTests.no_type_flags_in, 1 << 1)
        self.assertRaises(
            TypeError,
            GIMarshallingTests.no_type_flags_in,
            "GIMarshallingTests.NoTypeFlags.VALUE2",
        )

    def test_flags_return(self):
        flags = GIMarshallingTests.no_type_flags_returnv()
        self.assertTrue(isinstance(flags, GIMarshallingTests.NoTypeFlags))
        self.assertEqual(flags, GIMarshallingTests.NoTypeFlags.VALUE2)

    def test_flags_out(self):
        flags = GIMarshallingTests.no_type_flags_out()
        self.assertTrue(isinstance(flags, GIMarshallingTests.NoTypeFlags))
        self.assertEqual(flags, GIMarshallingTests.NoTypeFlags.VALUE2)

    def test_flags_inout(self):
        flags = GIMarshallingTests.no_type_flags_inout(
            GIMarshallingTests.NoTypeFlags.VALUE2
        )
        self.assertTrue(isinstance(flags, GIMarshallingTests.NoTypeFlags))
        self.assertEqual(flags, GIMarshallingTests.NoTypeFlags.VALUE1)

    def test_flags_has_no_gtype(self):
        self.assertFalse(hasattr(GIMarshallingTests.NoTypeFlags, "__gtype__"))
        self.assertFalse(issubclass(GIMarshallingTests.NoTypeFlags, GObject.GFlags))

    def test_type_module_name(self):
        self.assertEqual(GIMarshallingTests.NoTypeFlags.__name__, "NoTypeFlags")
        self.assertEqual(
            GIMarshallingTests.NoTypeFlags.__module__,
            "gi.repository.GIMarshallingTests",
        )

    def test_repr(self):
        self.assertEqual(
            repr(GIMarshallingTests.NoTypeFlags.VALUE2), "<NoTypeFlags.VALUE2: 2>"
        )

    def test_flags_add_module_type_error(self):
        with self.assertRaises(TypeError):
            gi._gi.flags_add(
                "self",
                "Flags",
                GIMarshallingTests.Flags.__gtype__,
                GIMarshallingTests.Flags.__info__,
            )


class TestStructure(unittest.TestCase):
    def test_simple_struct(self):
        self.assertTrue(issubclass(GIMarshallingTests.SimpleStruct, GObject.GPointer))

        struct = GIMarshallingTests.SimpleStruct()
        self.assertTrue(isinstance(struct, GIMarshallingTests.SimpleStruct))

        self.assertEqual(0, struct.long_)
        self.assertEqual(0, struct.int8)

        struct.long_ = 6
        struct.int8 = 7

        self.assertEqual(6, struct.long_)
        self.assertEqual(7, struct.int8)

        del struct

    def test_nested_struct(self):
        struct = GIMarshallingTests.NestedStruct()

        self.assertTrue(
            isinstance(struct.simple_struct, GIMarshallingTests.SimpleStruct)
        )

        struct.simple_struct.long_ = 42
        self.assertEqual(42, struct.simple_struct.long_)

        del struct

    def test_not_simple_struct(self):
        struct = GIMarshallingTests.NotSimpleStruct()
        self.assertEqual(None, struct.pointer)

    def test_simple_struct_return(self):
        struct = GIMarshallingTests.simple_struct_returnv()

        self.assertTrue(isinstance(struct, GIMarshallingTests.SimpleStruct))
        self.assertEqual(6, struct.long_)
        self.assertEqual(7, struct.int8)

        del struct

    def test_simple_struct_in(self):
        struct = GIMarshallingTests.SimpleStruct()
        struct.long_ = 6
        struct.int8 = 7

        GIMarshallingTests.SimpleStruct.inv(struct)

        del struct

        struct = GIMarshallingTests.NestedStruct()

        self.assertRaises(TypeError, GIMarshallingTests.SimpleStruct.inv, struct)

        del struct

        self.assertRaises(TypeError, GIMarshallingTests.SimpleStruct.inv, None)

    def test_simple_struct_method(self):
        struct = GIMarshallingTests.SimpleStruct()
        struct.long_ = 6
        struct.int8 = 7

        struct.method()

        del struct

        self.assertRaises(TypeError, GIMarshallingTests.SimpleStruct.method)

    def test_pointer_struct(self):
        self.assertTrue(issubclass(GIMarshallingTests.PointerStruct, GObject.GPointer))

        struct = GIMarshallingTests.PointerStruct()
        self.assertTrue(isinstance(struct, GIMarshallingTests.PointerStruct))

        del struct

    def test_pointer_struct_return(self):
        struct = GIMarshallingTests.pointer_struct_returnv()

        self.assertTrue(isinstance(struct, GIMarshallingTests.PointerStruct))
        self.assertEqual(42, struct.long_)

        del struct

    def test_pointer_struct_in(self):
        struct = GIMarshallingTests.PointerStruct()
        struct.long_ = 42

        struct.inv()

        del struct

    def test_boxed_struct(self):
        self.assertTrue(issubclass(GIMarshallingTests.BoxedStruct, GObject.GBoxed))

        struct = GIMarshallingTests.BoxedStruct()
        self.assertTrue(isinstance(struct, GIMarshallingTests.BoxedStruct))

        self.assertEqual(0, struct.long_)
        self.assertEqual(None, struct.string_)
        self.assertEqual([], struct.g_strv)

        del struct

    def test_boxed_struct_new(self):
        struct = GIMarshallingTests.BoxedStruct.new()
        self.assertTrue(isinstance(struct, GIMarshallingTests.BoxedStruct))
        self.assertEqual(struct.long_, 0)
        self.assertEqual(struct.string_, None)

        del struct

    def test_boxed_struct_copy(self):
        struct = GIMarshallingTests.BoxedStruct()
        struct.long_ = 42
        struct.string_ = "hello"

        new_struct = struct.copy()
        self.assertTrue(isinstance(new_struct, GIMarshallingTests.BoxedStruct))
        self.assertEqual(new_struct.long_, 42)
        self.assertEqual(new_struct.string_, "hello")

        del new_struct
        del struct

    def test_boxed_struct_return(self):
        struct = GIMarshallingTests.boxed_struct_returnv()

        self.assertTrue(isinstance(struct, GIMarshallingTests.BoxedStruct))
        self.assertEqual(42, struct.long_)
        self.assertEqual("hello", struct.string_)
        self.assertEqual(["0", "1", "2"], struct.g_strv)

        del struct

    def test_boxed_struct_in(self):
        struct = GIMarshallingTests.BoxedStruct()
        struct.long_ = 42

        struct.inv()

        del struct

    def test_boxed_struct_out(self):
        struct = GIMarshallingTests.boxed_struct_out()

        self.assertTrue(isinstance(struct, GIMarshallingTests.BoxedStruct))
        self.assertEqual(42, struct.long_)

        del struct

    def test_boxed_struct_inout(self):
        in_struct = GIMarshallingTests.BoxedStruct()
        in_struct.long_ = 42

        out_struct = GIMarshallingTests.boxed_struct_inout(in_struct)

        self.assertTrue(isinstance(out_struct, GIMarshallingTests.BoxedStruct))
        self.assertEqual(0, out_struct.long_)

        del in_struct
        del out_struct

    def test_struct_field_assignment(self):
        struct = GIMarshallingTests.BoxedStruct()

        struct.long_ = 42
        struct.string_ = "hello"
        self.assertEqual(struct.long_, 42)
        self.assertEqual(struct.string_, "hello")

    def test_union_init(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            GIMarshallingTests.Union(42)

        self.assertTrue(issubclass(warn[0].category, DeprecationWarning))

        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            GIMarshallingTests.Union(f=42)

        self.assertTrue(issubclass(warn[0].category, DeprecationWarning))

    def test_union(self):
        union = GIMarshallingTests.Union()

        self.assertTrue(isinstance(union, GIMarshallingTests.Union))

        new_union = union.copy()
        self.assertTrue(isinstance(new_union, GIMarshallingTests.Union))

        del union
        del new_union

    def test_union_return(self):
        union = GIMarshallingTests.union_returnv()

        self.assertTrue(isinstance(union, GIMarshallingTests.Union))
        self.assertEqual(42, union.long_)

        del union

    def test_union_in(self):
        union = GIMarshallingTests.Union()
        union.long_ = 42

        union.inv()

        del union

    def test_union_method(self):
        union = GIMarshallingTests.Union()
        union.long_ = 42

        union.method()

        del union

        self.assertRaises(TypeError, GIMarshallingTests.Union.method)

    def test_repr(self):
        self.assertRegex(
            repr(GIMarshallingTests.PointerStruct()),
            r"<GIMarshallingTests.PointerStruct object at 0x[^\s]+ "
            r"\((void|GIMarshallingTestsPointerStruct) at 0x[^\s]+\)>",
        )

        self.assertRegex(
            repr(GIMarshallingTests.SimpleStruct()),
            r"<GIMarshallingTests.SimpleStruct object at 0x[^\s]+ "
            r"\((void|GIMarshallingTestsSimpleStruct) at 0x[^\s]+\)>",
        )

        self.assertRegex(
            repr(GIMarshallingTests.Union()),
            r"<GIMarshallingTests.Union object at 0x[^\s]+ "
            r"\(GIMarshallingTestsUnion at 0x[^\s]+\)>",
        )

        self.assertRegex(
            repr(GIMarshallingTests.BoxedStruct()),
            r"<GIMarshallingTests.BoxedStruct object at 0x[^\s]+ "
            r"\(GIMarshallingTestsBoxedStruct at 0x[^\s]+\)>",
        )


class TestGObject(unittest.TestCase):
    def test_object(self):
        self.assertTrue(issubclass(GIMarshallingTests.Object, GObject.GObject))

        object_ = GIMarshallingTests.Object()
        self.assertTrue(isinstance(object_, GIMarshallingTests.Object))
        self.assertEqual(object_.__grefcount__, 1)

    def test_object_new(self):
        object_ = GIMarshallingTests.Object.new(42)
        self.assertTrue(isinstance(object_, GIMarshallingTests.Object))
        self.assertEqual(object_.__grefcount__, 1)

    def test_object_int(self):
        object_ = GIMarshallingTests.Object(int=42)
        self.assertEqual(object_.int_, 42)

    # FIXME: Don't work yet.
    #        object_.int_ = 0
    #        self.assertEqual(object_.int_, 0)

    def test_object_static_method(self):
        GIMarshallingTests.Object.static_method()

    def test_object_method(self):
        GIMarshallingTests.Object(int=42).method()
        self.assertRaises(
            TypeError, GIMarshallingTests.Object.method, GObject.GObject()
        )
        self.assertRaises(TypeError, GIMarshallingTests.Object.method)

    def test_sub_object(self):
        self.assertTrue(
            issubclass(GIMarshallingTests.SubObject, GIMarshallingTests.Object)
        )

        object_ = GIMarshallingTests.SubObject()
        self.assertTrue(isinstance(object_, GIMarshallingTests.SubObject))

    def test_sub_object_new(self):
        self.assertRaises(TypeError, GIMarshallingTests.SubObject.new, 42)

    def test_sub_object_static_method(self):
        object_ = GIMarshallingTests.SubObject()
        object_.static_method()

    def test_sub_object_method(self):
        object_ = GIMarshallingTests.SubObject(int=42)
        object_.method()

    def test_sub_object_sub_method(self):
        object_ = GIMarshallingTests.SubObject()
        object_.sub_method()

    def test_sub_object_overwritten_method(self):
        object_ = GIMarshallingTests.SubObject()
        object_.overwritten_method()

        self.assertRaises(
            TypeError,
            GIMarshallingTests.SubObject.overwritten_method,
            GIMarshallingTests.Object(),
        )

    def test_sub_object_int(self):
        object_ = GIMarshallingTests.SubObject()
        self.assertEqual(object_.int_, 0)

    # FIXME: Don't work yet.
    #        object_.int_ = 42
    #        self.assertEqual(object_.int_, 42)

    def test_object_none_return(self):
        object_ = GIMarshallingTests.Object.none_return()
        self.assertTrue(isinstance(object_, GIMarshallingTests.Object))
        self.assertEqual(object_.__grefcount__, 2)

    def test_object_full_return(self):
        object_ = GIMarshallingTests.Object.full_return()
        self.assertTrue(isinstance(object_, GIMarshallingTests.Object))
        self.assertEqual(object_.__grefcount__, 1)

    def test_object_none_in(self):
        object_ = GIMarshallingTests.Object(int=42)
        GIMarshallingTests.Object.none_in(object_)
        self.assertEqual(object_.__grefcount__, 1)

        object_ = GIMarshallingTests.SubObject(int=42)
        GIMarshallingTests.Object.none_in(object_)

        object_ = GObject.GObject()
        self.assertRaises(TypeError, GIMarshallingTests.Object.none_in, object_)

        self.assertRaises(TypeError, GIMarshallingTests.Object.none_in, None)

    def test_object_none_out(self):
        object_ = GIMarshallingTests.Object.none_out()
        self.assertTrue(isinstance(object_, GIMarshallingTests.Object))
        self.assertEqual(object_.__grefcount__, 2)

        new_object = GIMarshallingTests.Object.none_out()
        self.assertTrue(new_object is object_)

    def test_object_full_out(self):
        object_ = GIMarshallingTests.Object.full_out()
        self.assertTrue(isinstance(object_, GIMarshallingTests.Object))
        self.assertEqual(object_.__grefcount__, 1)

    def test_object_none_inout(self):
        object_ = GIMarshallingTests.Object(int=42)
        new_object = GIMarshallingTests.Object.none_inout(object_)

        self.assertTrue(isinstance(new_object, GIMarshallingTests.Object))

        self.assertFalse(object_ is new_object)

        self.assertEqual(object_.__grefcount__, 1)
        self.assertEqual(new_object.__grefcount__, 2)

        new_new_object = GIMarshallingTests.Object.none_inout(object_)
        self.assertTrue(new_new_object is new_object)

        GIMarshallingTests.Object.none_inout(GIMarshallingTests.SubObject(int=42))

    def test_object_full_inout(self):
        # Using gimarshallingtests.c from GI versions > 1.38.0 will show this
        # test as an "unexpected success" due to reference leak fixes in that file.
        # TODO: remove the expectedFailure once PyGI relies on GI > 1.38.0.
        object_ = GIMarshallingTests.Object(int=42)
        new_object = GIMarshallingTests.Object.full_inout(object_)

        self.assertTrue(isinstance(new_object, GIMarshallingTests.Object))

        self.assertFalse(object_ is new_object)

        self.assertEqual(object_.__grefcount__, 1)
        self.assertEqual(new_object.__grefcount__, 1)

    def test_repr(self):
        self.assertRegex(
            repr(GIMarshallingTests.Object(int=42)),
            r"<GIMarshallingTests.Object object at 0x[^\s]+ "
            r"\(GIMarshallingTestsObject at 0x[^\s]+\)>",
        )

    def test_nongir_repr(self):
        self.assertRegex(
            repr(Gio.File.new_for_path("/")),
            r"<__gi__.GLocalFile object at 0x[^\s]+ " r"\(GLocalFile at 0x[^\s]+\)>",
        )

    def test_constructor_bad_cls_arg(self):
        # Get the unbound version of a constructor
        newv = GObject.GObject.newv.__func__

        class Foo:
            pass

        with self.assertRaisesRegex(AttributeError, r"has no attribute '__name__'"):
            foo = Foo()
            newv(foo)

        with self.assertRaisesRegex(TypeError, r"attribute is not a string"):
            foo = Foo()
            foo.__name__ = 42
            newv(foo)


# FIXME: Doesn't actually return the same object.
#    def test_object_inout_same(self):
#        object_ = GIMarshallingTests.Object()
#        new_object = GIMarshallingTests.object_full_inout(object_)
#        self.assertTrue(object_ is new_object)
#        self.assertEqual(object_.__grefcount__, 1)


class TestPythonGObject(unittest.TestCase):
    class Object(GIMarshallingTests.Object):
        return_for_caller_allocated_out_parameter = "test caller alloc return"

        def __init__(self, int):
            GIMarshallingTests.Object.__init__(self)
            self.val = None

        def method(self):
            # Don't call super, which asserts that self.int == 42.
            pass

        def do_method_int8_in(self, int8):
            self.val = int8

        def do_method_int8_out(self):
            return 42

        def do_method_int8_arg_and_out_caller(self, arg):
            return arg + 1

        def do_method_int8_arg_and_out_callee(self, arg):
            return arg + 1

        def do_method_str_arg_out_ret(self, arg):
            return (arg.upper(), len(arg))

        def do_method_with_default_implementation(self, int8):
            GIMarshallingTests.Object.do_method_with_default_implementation(self, int8)
            self.props.int += int8

        def do_vfunc_return_value_only(self):
            return 4242

        def do_vfunc_one_out_parameter(self):
            return 42.42

        def do_vfunc_multiple_out_parameters(self):
            return (42.42, 3.14)

        def do_vfunc_return_value_and_one_out_parameter(self):
            return (5, 42)

        def do_vfunc_return_value_and_multiple_out_parameters(self):
            return (5, 42, 99)

        def do_vfunc_caller_allocated_out_parameter(self):
            return self.return_for_caller_allocated_out_parameter

    class SubObject(GIMarshallingTests.SubObject):
        def __init__(self, int):
            GIMarshallingTests.SubObject.__init__(self)
            self.val = None

        def do_method_with_default_implementation(self, int8):
            self.val = int8

        def do_vfunc_return_value_only(self):
            return 2121

    class Interface3Impl(GObject.Object, GIMarshallingTests.Interface3):
        def __init__(self):
            GObject.Object.__init__(self)
            self.variants = None
            self.n_variants = None

        def do_test_variant_array_in(self, variants, n_variants):
            self.variants = variants
            self.n_variants = n_variants

    class ErrorObject(GIMarshallingTests.Object):
        def do_vfunc_return_value_only(self):
            raise ValueError("Return value should be 0")

    def test_object(self):
        self.assertTrue(issubclass(self.Object, GIMarshallingTests.Object))

        object_ = self.Object(int=42)
        self.assertTrue(isinstance(object_, self.Object))

    @unittest.skipUnless(
        hasattr(GIMarshallingTests.Object, "new_fail"), "Requires newer version of GI"
    )
    def test_object_fail(self):
        with self.assertRaises(GLib.Error):
            GIMarshallingTests.Object.new_fail(int_=42)

    def test_object_method(self):
        self.Object(int=0).method()

    def test_object_vfuncs(self):
        object_ = self.Object(int=42)
        object_.method_int8_in(84)
        self.assertEqual(object_.val, 84)
        self.assertEqual(object_.method_int8_out(), 42)

        # can be dropped when bumping g-i dependencies to >= 1.35.2
        if hasattr(object_, "method_int8_arg_and_out_caller"):
            self.assertEqual(object_.method_int8_arg_and_out_caller(42), 43)
            self.assertEqual(object_.method_int8_arg_and_out_callee(42), 43)
            self.assertEqual(object_.method_str_arg_out_ret("hello"), ("HELLO", 5))

        object_.method_with_default_implementation(42)
        self.assertEqual(object_.props.int, 84)

        self.assertEqual(object_.vfunc_return_value_only(), 4242)
        self.assertAlmostEqual(object_.vfunc_one_out_parameter(), 42.42, places=5)

        (a, b) = object_.vfunc_multiple_out_parameters()
        self.assertAlmostEqual(a, 42.42, places=5)
        self.assertAlmostEqual(b, 3.14, places=5)

        self.assertEqual(object_.vfunc_return_value_and_one_out_parameter(), (5, 42))
        self.assertEqual(
            object_.vfunc_return_value_and_multiple_out_parameters(), (5, 42, 99)
        )

        self.assertEqual(
            object_.vfunc_caller_allocated_out_parameter(),
            object_.return_for_caller_allocated_out_parameter,
        )

        class ObjectWithoutVFunc(GIMarshallingTests.Object):
            def __init__(self, int):
                GIMarshallingTests.Object.__init__(self)

        object_ = ObjectWithoutVFunc(int=42)
        object_.method_with_default_implementation(84)
        self.assertEqual(object_.props.int, 84)

    @unittest.skipUnless(hasattr(sys, "getrefcount"), "no sys.getrefcount")
    def test_vfunc_return_ref_count(self):
        obj = self.Object(int=42)
        ref_count = sys.getrefcount(obj.return_for_caller_allocated_out_parameter)
        ret = obj.vfunc_caller_allocated_out_parameter()
        gc.collect()

        # Make sure the return and what the vfunc returned
        # are equal but not the same object.
        self.assertEqual(ret, obj.return_for_caller_allocated_out_parameter)
        self.assertFalse(ret is obj.return_for_caller_allocated_out_parameter)
        self.assertEqual(
            sys.getrefcount(obj.return_for_caller_allocated_out_parameter), ref_count
        )

    def test_vfunc_return_no_ref_count(self):
        obj = self.Object(int=42)
        ret = obj.vfunc_caller_allocated_out_parameter()
        self.assertEqual(ret, obj.return_for_caller_allocated_out_parameter)
        self.assertFalse(ret is obj.return_for_caller_allocated_out_parameter)

    def test_subobject_parent_vfunc(self):
        object_ = self.SubObject(int=81)
        object_.method_with_default_implementation(87)
        self.assertEqual(object_.val, 87)

    def test_subobject_child_vfunc(self):
        object_ = self.SubObject(int=1)
        self.assertEqual(object_.vfunc_return_value_only(), 2121)

    def test_subobject_non_vfunc_do_method(self):
        class PythonObjectWithNonVFuncDoMethod:
            def do_not_a_vfunc(self):
                return 5

        class ObjectOverrideNonVFuncDoMethod(
            GIMarshallingTests.Object, PythonObjectWithNonVFuncDoMethod
        ):
            def do_not_a_vfunc(self):
                value = super().do_not_a_vfunc()
                return 13 + value

        object_ = ObjectOverrideNonVFuncDoMethod()
        self.assertEqual(18, object_.do_not_a_vfunc())

    def test_native_function_not_set_in_subclass_dict(self):
        # Previously, GI was setting virtual functions on the class as well
        # as any *native* class that subclasses it. Here we check that it is only
        # set on the class that the method is originally from.
        self.assertTrue(
            "do_method_with_default_implementation"
            in GIMarshallingTests.Object.__dict__
        )
        self.assertTrue(
            "do_method_with_default_implementation"
            not in GIMarshallingTests.SubObject.__dict__
        )

    def test_subobject_with_interface_and_non_vfunc_do_method(self):
        # There was a bug for searching for vfuncs in interfaces. It was
        # triggered by having a do_* method that wasn't overriding
        # a native vfunc, as well as inheriting from an interface.
        class GObjectSubclassWithInterface(
            GObject.GObject, GIMarshallingTests.Interface
        ):
            def do_method_not_a_vfunc(self):
                pass

    def test_subsubobject(self):
        class SubSubSubObject(GIMarshallingTests.SubSubObject):
            def do_method_deep_hierarchy(self, num):
                self.props.int = num * 2

        sub_sub_sub_object = SubSubSubObject()
        GIMarshallingTests.SubSubObject.do_method_deep_hierarchy(sub_sub_sub_object, 5)
        self.assertEqual(sub_sub_sub_object.props.int, 5)

    def test_interface3impl(self):
        iface3 = self.Interface3Impl()
        variants = [GLib.Variant("i", 27), GLib.Variant("s", "Hello")]
        iface3.test_variant_array_in(variants)
        self.assertEqual(iface3.n_variants, 2)
        self.assertEqual(iface3.variants[0].unpack(), 27)
        self.assertEqual(iface3.variants[1].unpack(), "Hello")

    def test_python_subsubobject_vfunc(self):
        class PySubObject(GIMarshallingTests.Object):
            def __init__(self):
                GIMarshallingTests.Object.__init__(self)
                self.sub_method_int8_called = 0

            def do_method_int8_in(self, int8):
                self.sub_method_int8_called += 1

        class PySubSubObject(PySubObject):
            def __init__(self):
                PySubObject.__init__(self)
                self.subsub_method_int8_called = 0

            def do_method_int8_in(self, int8):
                self.subsub_method_int8_called += 1

        so = PySubObject()
        so.method_int8_in(1)
        self.assertEqual(so.sub_method_int8_called, 1)

        # it should call the method on the SubSub object only
        sso = PySubSubObject()
        sso.method_int8_in(1)
        self.assertEqual(sso.subsub_method_int8_called, 1)
        self.assertEqual(sso.sub_method_int8_called, 0)

    def test_callback_in_vfunc(self):
        class SubObject(GIMarshallingTests.Object):
            def __init__(self):
                GObject.GObject.__init__(self)
                self.worked = False

            def do_vfunc_with_callback(self, callback):
                self.worked = callback(42) == 42

        object_ = SubObject()
        object_.call_vfunc_with_callback()
        self.assertTrue(object_.worked)
        object_.worked = False
        object_.call_vfunc_with_callback()
        self.assertTrue(object_.worked)

    def test_exception_in_vfunc_return_value(self):
        obj = self.ErrorObject()
        with capture_exceptions() as exc:
            self.assertEqual(obj.vfunc_return_value_only(), 0)
        self.assertEqual(len(exc), 1)
        self.assertEqual(exc[0].type, ValueError)

    @unittest.skipUnless(
        hasattr(GIMarshallingTests, "callback_owned_boxed"),
        "requires newer version of GI",
    )
    def test_callback_owned_box(self):
        def callback(box, data):
            self.box = box

        def nop_callback(box, data):
            pass

        GIMarshallingTests.callback_owned_boxed(callback, None)
        GIMarshallingTests.callback_owned_boxed(nop_callback, None)
        self.assertEqual(self.box.long_, 1)


class TestMultiOutputArgs(unittest.TestCase):
    def test_int_out_out(self):
        self.assertEqual((6, 7), GIMarshallingTests.int_out_out())

    def test_int_return_out(self):
        self.assertEqual((6, 7), GIMarshallingTests.int_return_out())


# Interface


class TestInterfaces(unittest.TestCase):
    class TestInterfaceImpl(GObject.GObject, GIMarshallingTests.Interface):
        def __init__(self):
            GObject.GObject.__init__(self)
            self.val = None

        def do_test_int8_in(self, int8):
            self.val = int8

    def setUp(self):
        self.instance = self.TestInterfaceImpl()

    def test_iface_impl(self):
        instance = GIMarshallingTests.InterfaceImpl()
        assert instance.get_as_interface() is instance
        instance.test_int8_in(42)

    def test_wrapper(self):
        self.assertTrue(issubclass(GIMarshallingTests.Interface, GObject.GInterface))
        self.assertEqual(
            GIMarshallingTests.Interface.__gtype__.name, "GIMarshallingTestsInterface"
        )
        self.assertRaises(NotImplementedError, GIMarshallingTests.Interface)

    def test_implementation(self):
        self.assertTrue(
            issubclass(self.TestInterfaceImpl, GIMarshallingTests.Interface)
        )
        self.assertTrue(isinstance(self.instance, GIMarshallingTests.Interface))

    def test_int8_int(self):
        GIMarshallingTests.test_interface_test_int8_in(self.instance, 42)
        self.assertEqual(self.instance.val, 42)

    def test_subclass(self):
        class TestInterfaceImplA(self.TestInterfaceImpl):
            pass

        class TestInterfaceImplB(TestInterfaceImplA):
            pass

        instance = TestInterfaceImplA()
        GIMarshallingTests.test_interface_test_int8_in(instance, 42)
        self.assertEqual(instance.val, 42)

    def test_subclass_override(self):
        class TestInterfaceImplD(TestInterfaces.TestInterfaceImpl):
            val2 = None

            def do_test_int8_in(self, int8):
                self.val2 = int8

        instance = TestInterfaceImplD()
        self.assertEqual(instance.val, None)
        self.assertEqual(instance.val2, None)

        GIMarshallingTests.test_interface_test_int8_in(instance, 42)
        self.assertEqual(instance.val, None)
        self.assertEqual(instance.val2, 42)

    def test_type_mismatch(self):
        obj = GIMarshallingTests.Object()

        # wrong type for first argument: interface
        enum = Gio.File.new_for_path(".").enumerate_children(
            "", Gio.FileQueryInfoFlags.NONE, None
        )
        try:
            enum.next_file(obj)
            self.fail("call with wrong type argument unexpectedly succeeded")
        except TypeError as e:
            # should have argument name
            self.assertTrue("cancellable" in str(e), e)
            # should have expected type
            self.assertTrue("xpected Gio.Cancellable" in str(e), e)
            # should have actual type
            self.assertTrue("GIMarshallingTests.Object" in str(e), e)

        # wrong type for self argument: interface
        try:
            Gio.FileEnumerator.next_file(obj, None)
            self.fail("call with wrong type argument unexpectedly succeeded")
        except TypeError as e:
            # should have argument name
            self.assertTrue("self" in str(e), e)
            # should have expected type
            self.assertTrue("xpected Gio.FileEnumerator" in str(e), e)
            # should have actual type
            self.assertTrue("GIMarshallingTests.Object" in str(e), e)

        # wrong type for first argument: GObject
        var = GLib.Variant("s", "mystring")
        action = Gio.SimpleAction.new("foo", var.get_type())
        try:
            action.activate(obj)
            self.fail("call with wrong type argument unexpectedly succeeded")
        except TypeError as e:
            # should have argument name
            self.assertTrue("parameter" in str(e), e)
            # should have expected type
            self.assertTrue("xpected GLib.Variant" in str(e), e)
            # should have actual type
            self.assertTrue("GIMarshallingTests.Object" in str(e), e)

        # wrong type for self argument: GObject
        try:
            Gio.SimpleAction.activate(obj, obj)
            self.fail("call with wrong type argument unexpectedly succeeded")
        except TypeError as e:
            # should have argument name
            self.assertTrue("self" in str(e), e)
            # should have expected type
            self.assertTrue("xpected Gio.Action" in str(e), e)
            # should have actual type
            self.assertTrue("GIMarshallingTests.Object" in str(e), e)


class TestMRO(unittest.TestCase):
    def test_mro(self):
        # check that our own MRO calculation matches what we would expect
        # from Python's own C3 calculations
        class A:
            pass

        class B(A):
            pass

        class C(A):
            pass

        class D(B, C):
            pass

        class E(D, GIMarshallingTests.Object):
            pass

        expected = (
            E,
            D,
            B,
            C,
            A,
            GIMarshallingTests.Object,
            GObject.Object,
            GObject.Object.__base__,
            gi._gi.GObject,
            object,
        )
        self.assertEqual(expected, E.__mro__)

    @unittest.skipIf(
        sys.implementation.name == "pypy", "PyPy 3.11 causes cycle among base classes"
    )
    def test_interface_collision(self):
        # there was a problem with Python bailing out because of
        # http://en.wikipedia.org/wiki/Diamond_problem with interfaces,
        # which shouldn't really be a problem.

        class TestInterfaceImpl(GObject.GObject, GIMarshallingTests.Interface):
            pass

        class TestInterfaceImpl2(GIMarshallingTests.Interface, TestInterfaceImpl):
            pass

        class TestInterfaceImpl3(TestInterfaceImpl, GIMarshallingTests.Interface2):
            pass

    def test_old_style_mixin(self):
        # Note: Old style classes don't exist in Python 3
        class Mixin:
            pass

        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")

            # Dynamically create a new gi based class with an old
            # style mixin.
            type("GIWithOldStyleMixin", (GIMarshallingTests.Object, Mixin), {})

            self.assertEqual(len(warn), 0)


class TestInterfaceClash(unittest.TestCase):
    def test_clash(self):
        def create_clash():
            class TestClash(
                GObject.GObject,
                GIMarshallingTests.Interface,
                GIMarshallingTests.Interface2,
            ):
                def do_test_int8_in(self, int8):
                    pass

            TestClash()

        self.assertRaises(TypeError, create_clash)


class TestOverrides(unittest.TestCase):
    def test_constant(self):
        self.assertEqual(GIMarshallingTests.OVERRIDES_CONSTANT, 7)

    def test_struct(self):
        # Test that the constructor has been overridden.
        struct = GIMarshallingTests.OverridesStruct(42)

        self.assertTrue(isinstance(struct, GIMarshallingTests.OverridesStruct))

        # Test that the method has been overridden.
        self.assertEqual(6, struct.method())

        del struct

        # Test that the overrides wrapper has been registered.
        struct = GIMarshallingTests.overrides_struct_returnv()

        self.assertTrue(isinstance(struct, GIMarshallingTests.OverridesStruct))

        del struct

    def test_object(self):
        # Test that the constructor has been overridden.
        object_ = GIMarshallingTests.OverridesObject(42)

        self.assertTrue(isinstance(object_, GIMarshallingTests.OverridesObject))

        # Test that the alternate constructor has been overridden.
        object_ = GIMarshallingTests.OverridesObject.new(42)

        self.assertTrue(isinstance(object_, GIMarshallingTests.OverridesObject))

        # Test that the method has been overridden.
        self.assertEqual(6, object_.method())

        # Test that the overrides wrapper has been registered.
        object_ = GIMarshallingTests.OverridesObject.returnv()

        self.assertTrue(isinstance(object_, GIMarshallingTests.OverridesObject))

    def test_module_name(self):
        # overridden types
        self.assertEqual(
            GIMarshallingTests.OverridesStruct.__module__,
            "gi.overrides.GIMarshallingTests",
        )
        self.assertEqual(
            GIMarshallingTests.OverridesObject.__module__,
            "gi.overrides.GIMarshallingTests",
        )
        self.assertEqual(GObject.Object.__module__, "gi.overrides.GObject")

        # not overridden
        self.assertEqual(
            GIMarshallingTests.SubObject.__module__, "gi.repository.GIMarshallingTests"
        )
        self.assertEqual(GObject.InitiallyUnowned.__module__, "gi.repository.GObject")


class TestDir(unittest.TestCase):
    def test_members_list(self):
        list = dir(GIMarshallingTests)
        self.assertTrue("OverridesStruct" in list)
        self.assertTrue("BoxedStruct" in list)
        self.assertTrue("OVERRIDES_CONSTANT" in list)
        self.assertTrue("GEnum" in list)
        self.assertTrue("int32_return_max" in list)

    def test_modules_list(self):
        import gi.repository

        list = dir(gi.repository)
        self.assertTrue("GIMarshallingTests" in list)

        # FIXME: test to see if a module which was not imported is in the list
        #        we should be listing every typelib we find, not just the ones
        #        which are imported
        #
        #        to test this I recommend we compile a fake module which
        #        our tests would never import and check to see if it is
        #        in the list:
        #
        # self.assertTrue('DoNotImportDummyTests' in list)


class TestParamSpec(unittest.TestCase):
    def test_param_spec_in_bool(self):
        ps = GObject.param_spec_boolean(
            "mybool", "test-bool", "boolblurb", True, GObject.ParamFlags.READABLE
        )
        GIMarshallingTests.param_spec_in_bool(ps)

    def test_param_spec_return(self):
        obj = GIMarshallingTests.param_spec_return()
        self.assertEqual(obj.name, "test-param")
        self.assertEqual(obj.nick, "test")
        self.assertEqual(obj.value_type, GObject.TYPE_STRING)

    def test_param_spec_out(self):
        obj = GIMarshallingTests.param_spec_out()
        self.assertEqual(obj.name, "test-param")
        self.assertEqual(obj.nick, "test")
        self.assertEqual(obj.value_type, GObject.TYPE_STRING)


class TestKeywordArgs(unittest.TestCase):
    def test_calling(self):
        kw_func = GIMarshallingTests.int_three_in_three_out

        self.assertEqual(kw_func(1, 2, 3), (1, 2, 3))
        self.assertEqual(kw_func(**{"a": 4, "b": 5, "c": 6}), (4, 5, 6))
        self.assertEqual(kw_func(1, **{"b": 7, "c": 8}), (1, 7, 8))
        self.assertEqual(kw_func(1, 7, **{"c": 8}), (1, 7, 8))
        self.assertEqual(kw_func(1, c=8, **{"b": 7}), (1, 7, 8))
        self.assertEqual(kw_func(2, c=4, b=3), (2, 3, 4))
        self.assertEqual(kw_func(a=2, c=4, b=3), (2, 3, 4))

    def assertRaisesMessage(self, exception, message, func, *args, **kwargs):
        try:
            func(*args, **kwargs)
        except exception:
            (_e_type, e) = sys.exc_info()[:2]
            if message is not None:
                self.assertEqual(str(e), message)
        except:
            raise
        else:
            msg = f"{func.__name__}() did not raise {exception.__name__}"
            raise AssertionError(msg)

    def test_type_errors(self):
        # test too few args
        self.assertRaisesMessage(
            TypeError,
            "GIMarshallingTests.int_three_in_three_out() takes exactly 3 arguments (0 given)",
            GIMarshallingTests.int_three_in_three_out,
        )
        self.assertRaisesMessage(
            TypeError,
            "GIMarshallingTests.int_three_in_three_out() takes exactly 3 arguments (1 given)",
            GIMarshallingTests.int_three_in_three_out,
            1,
        )
        self.assertRaisesMessage(
            TypeError,
            "GIMarshallingTests.int_three_in_three_out() takes exactly 3 arguments (0 given)",
            GIMarshallingTests.int_three_in_three_out,
            *(),
        )
        self.assertRaisesMessage(
            TypeError,
            "GIMarshallingTests.int_three_in_three_out() takes exactly 3 arguments (0 given)",
            GIMarshallingTests.int_three_in_three_out,
            *(),
            **{},
        )
        self.assertRaisesMessage(
            TypeError,
            "GIMarshallingTests.int_three_in_three_out() takes exactly 3 non-keyword arguments (0 given)",
            GIMarshallingTests.int_three_in_three_out,
            *(),
            **{"c": 4},
        )

        # test too many args
        self.assertRaisesMessage(
            TypeError,
            "GIMarshallingTests.int_three_in_three_out() takes exactly 3 arguments (4 given)",
            GIMarshallingTests.int_three_in_three_out,
            *(1, 2, 3, 4),
        )
        self.assertRaisesMessage(
            TypeError,
            "GIMarshallingTests.int_three_in_three_out() takes exactly 3 non-keyword arguments (4 given)",
            GIMarshallingTests.int_three_in_three_out,
            *(1, 2, 3, 4),
            c=6,
        )

        # test too many keyword args
        self.assertRaisesMessage(
            TypeError,
            "GIMarshallingTests.int_three_in_three_out() got multiple values for keyword argument 'a'",
            GIMarshallingTests.int_three_in_three_out,
            1,
            2,
            3,
            **{"a": 4, "b": 5},
        )
        self.assertRaisesMessage(
            TypeError,
            "GIMarshallingTests.int_three_in_three_out() got an unexpected keyword argument 'd'",
            GIMarshallingTests.int_three_in_three_out,
            d=4,
        )
        self.assertRaisesMessage(
            TypeError,
            "GIMarshallingTests.int_three_in_three_out() got an unexpected keyword argument 'e'",
            GIMarshallingTests.int_three_in_three_out,
            **{"e": 2},
        )

    def test_kwargs_are_not_modified(self):
        d = {"b": 2}
        d2 = d.copy()
        GIMarshallingTests.int_three_in_three_out(1, c=4, **d)
        self.assertEqual(d, d2)

    @unittest.skipUnless(
        hasattr(GIMarshallingTests, "int_one_in_utf8_two_in_one_allows_none"),
        "Requires newer GIMarshallingTests",
    )
    def test_allow_none_as_default(self):
        GIMarshallingTests.int_two_in_utf8_two_in_with_allow_none(1, 2, "3", "4")
        GIMarshallingTests.int_two_in_utf8_two_in_with_allow_none(1, 2, "3")
        GIMarshallingTests.int_two_in_utf8_two_in_with_allow_none(1, 2)
        GIMarshallingTests.int_two_in_utf8_two_in_with_allow_none(1, 2, d="4")

        GIMarshallingTests.array_in_utf8_two_in_out_of_order("1", [-1, 0, 1, 2])
        GIMarshallingTests.array_in_utf8_two_in_out_of_order("1", [-1, 0, 1, 2], "2")
        self.assertRaises(
            TypeError,
            GIMarshallingTests.array_in_utf8_two_in_out_of_order,
            [-1, 0, 1, 2],
            a="1",
        )
        self.assertRaises(
            TypeError,
            GIMarshallingTests.array_in_utf8_two_in_out_of_order,
            [-1, 0, 1, 2],
        )

        GIMarshallingTests.array_in_utf8_two_in([-1, 0, 1, 2], "1", "2")
        GIMarshallingTests.array_in_utf8_two_in([-1, 0, 1, 2], "1")
        GIMarshallingTests.array_in_utf8_two_in([-1, 0, 1, 2])
        GIMarshallingTests.array_in_utf8_two_in([-1, 0, 1, 2], b="2")

        GIMarshallingTests.int_one_in_utf8_two_in_one_allows_none(1, "2", "3")
        GIMarshallingTests.int_one_in_utf8_two_in_one_allows_none(1, c="3")

        self.assertRaises(
            TypeError, GIMarshallingTests.int_one_in_utf8_two_in_one_allows_none, 1, "3"
        )


class TestKeywords(unittest.TestCase):
    def test_method(self):
        # g_variant_print()
        v = GLib.Variant("i", 1)
        self.assertEqual(v.print_(False), "1")

    def test_function(self):
        # g_thread_yield()
        self.assertEqual(GLib.Thread.yield_(), None)

    def test_struct_method(self):
        # g_timer_continue()
        # we cannot currently instantiate GLib.Timer objects, so just ensure
        # the method exists
        self.assertTrue(callable(GLib.Timer.continue_))

    def test_uppercase(self):
        self.assertEqual(GLib.IOCondition.IN.value_nicks, ["in"])


class TestModule(unittest.TestCase):
    def test_type(self):
        self.assertIsInstance(GIMarshallingTests, types.ModuleType)

    def test_path(self):
        path = GIMarshallingTests.__path__
        assert isinstance(path, list)
        assert len(path) == 1
        assert path[0].endswith("GIMarshallingTests-1.0.typelib")

    def test_str(self):
        self.assertTrue(
            "'GIMarshallingTests' from '" in str(GIMarshallingTests),
            str(GIMarshallingTests),
        )

    def test_dir(self):
        dir_ = dir(GIMarshallingTests)
        self.assertGreater(len(dir_), 10)

        self.assertTrue("SimpleStruct" in dir_)
        self.assertTrue("Interface2" in dir_)
        self.assertTrue("CONSTANT_GERROR_CODE" in dir_)
        self.assertTrue("array_zero_terminated_inout" in dir_)

        # assert that dir() does not contain garbage
        for item_name in dir_:
            item = getattr(GIMarshallingTests, item_name)
            self.assertTrue(hasattr(item, "__class__"))

    def test_help(self):
        with capture_output() as (stdout, _stderr):
            help(GIMarshallingTests)
        output = stdout.getvalue()

        self.assertTrue("SimpleStruct" in output, output)
        self.assertTrue("Interface2" in output, output)
        self.assertTrue("method_array_inout" in output, output)


class TestProjectVersion(unittest.TestCase):
    def test_version_str(self):
        self.assertGreater(gi.__version__, "3.")

    def test_version_info(self):
        self.assertEqual(len(gi.version_info), 3)
        self.assertGreaterEqual(gi.version_info, (3, 3, 5))

    def test_check_version(self):
        self.assertRaises(ValueError, gi.check_version, (99, 0, 0))
        self.assertRaises(ValueError, gi.check_version, "99.0.0")
        gi.check_version((3, 3, 5))
        gi.check_version("3.3.5")


class TestGIWarning(unittest.TestCase):
    def test_warning(self):
        ignored_by_default = (
            DeprecationWarning,
            PendingDeprecationWarning,
            ImportWarning,
        )

        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            warnings.warn("test", PyGIWarning)
            self.assertTrue(issubclass(warn[0].category, Warning))
            # We don't want PyGIWarning get ignored by default
            self.assertFalse(issubclass(warn[0].category, ignored_by_default))


class TestDeprecation(unittest.TestCase):
    def test_method(self):
        d = GLib.Date.new()
        with self.assertWarns(DeprecationWarning) as w:
            d.set_time(1)
        self.assertEqual(str(w.warning), "GLib.Date.set_time is deprecated")
        self.assertIn("test_gi.py", w.filename)

    def test_function(self):
        with self.assertWarns(DeprecationWarning) as w:
            GLib.strcasecmp("foo", "bar")
        self.assertEqual(str(w.warning), "GLib.strcasecmp is deprecated")
        self.assertIn("test_gi.py", w.filename)

    def test_deprecated_attribute_compat(self):
        # test that deprecatied attributes behave like instance attributes

        # save the deprecation
        deprecation = GLib._deprecations["IO_STATUS_ERROR"]

        with self.assertWarns(PyGIDeprecationWarning) as w:
            self.assertTrue(hasattr(GLib, "IO_STATUS_ERROR"))
        self.assertEqual(
            str(w.warning),
            "GLib.IO_STATUS_ERROR is deprecated; use GLib.IOStatus.ERROR instead",
        )
        self.assertIn("test_gi.py", w.filename)

        # deprecated attributes appear in dir()
        self.assertIn("IO_STATUS_ERROR", dir(GLib))

        try:
            # check if replacing works
            GLib.IO_STATUS_ERROR = "foo"
            self.assertEqual(GLib.IO_STATUS_ERROR, "foo")
        finally:
            # restore deprecation
            with contextlib.suppress(AttributeError):
                del GLib.IO_STATUS_ERROR
            GLib._deprecations["IO_STATUS_ERROR"] = deprecation

        try:
            # check if deleting works
            del GLib.IO_STATUS_ERROR
            self.assertFalse(hasattr(GLib, "IO_STATUS_ERROR"))
        finally:
            # restore deprecation
            with contextlib.suppress(AttributeError):
                del GLib.IO_STATUS_ERROR
            GLib._deprecations["IO_STATUS_ERROR"] = deprecation

    def test_deprecated_attribute_warning(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            self.assertEqual(GLib.IO_STATUS_ERROR, GLib.IOStatus.ERROR)
            GLib.IO_STATUS_ERROR
            GLib.IO_STATUS_ERROR
            self.assertEqual(len(warn), 3)
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))
            self.assertRegex(
                str(warn[0].message), ".*GLib.IO_STATUS_ERROR.*GLib.IOStatus.ERROR.*"
            )

    def test_deprecated_attribute_warning_coverage(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            GObject.markup_escape_text
            GObject.PRIORITY_DEFAULT
            GObject.GError
            GObject.PARAM_CONSTRUCT
            GObject.SIGNAL_ACTION
            GObject.property
            GObject.IO_STATUS_ERROR
            GObject.G_MAXUINT64
            GLib.IO_STATUS_ERROR
            GLib.SPAWN_SEARCH_PATH
            GLib.OPTION_FLAG_HIDDEN
            GLib.IO_FLAG_IS_WRITEABLE
            GLib.IO_FLAG_NONBLOCK
            GLib.USER_DIRECTORY_DESKTOP
            GLib.OPTION_ERROR_BAD_VALUE
            GLib.glib_version
            GLib.pyglib_version
            self.assertEqual(len(warn), 17)

    def test_deprecated_init_no_keywords(self):
        def init(self, **kwargs):
            self.assertDictEqual(kwargs, {"a": 1, "b": 2, "c": 3})

        fn = gi.overrides.deprecated_init(init, arg_names=("a", "b", "c"))
        with self.assertWarns(PyGIDeprecationWarning) as w:
            fn(self, 1, 2, 3)
        self.assertRegex(str(w.warning), ".*keyword.*a, b, c.*")
        self.assertIn("test_gi.py", w.filename)

    def test_deprecated_init_no_keywords_out_of_order(self):
        def init(self, **kwargs):
            self.assertDictEqual(kwargs, {"a": 1, "b": 2, "c": 3})

        fn = gi.overrides.deprecated_init(init, arg_names=("b", "a", "c"))
        with self.assertWarns(PyGIDeprecationWarning) as w:
            fn(self, 2, 1, 3)
        self.assertRegex(str(w.warning), ".*keyword.*b, a, c.*")
        self.assertIn("test_gi.py", w.filename)

    def test_deprecated_init_ignored_keyword(self):
        def init(self, **kwargs):
            self.assertDictEqual(kwargs, {"a": 1, "c": 3})

        fn = gi.overrides.deprecated_init(
            init, arg_names=("a", "b", "c"), ignore=("b",)
        )
        with self.assertWarns(PyGIDeprecationWarning) as w:
            fn(self, 1, 2, 3)
        self.assertRegex(str(w.warning), ".*keyword.*a, b, c.*")
        self.assertIn("test_gi.py", w.filename)

    def test_deprecated_init_with_aliases(self):
        def init(self, **kwargs):
            self.assertDictEqual(kwargs, {"a": 1, "b": 2, "c": 3})

        fn = gi.overrides.deprecated_init(
            init, arg_names=("a", "b", "c"), deprecated_aliases={"b": "bb", "c": "cc"}
        )
        with self.assertWarns(PyGIDeprecationWarning) as w:
            fn(self, a=1, bb=2, cc=3)
        self.assertRegex(
            str(w.warning), '.*keyword.*"bb, cc".*deprecated.*"b, c" respectively'
        )
        self.assertIn("test_gi.py", w.filename)

    def test_deprecated_init_with_defaults(self):
        def init(self, **kwargs):
            self.assertDictEqual(kwargs, {"a": 1, "b": 2, "c": 3})

        fn = gi.overrides.deprecated_init(
            init, arg_names=("a", "b", "c"), deprecated_defaults={"b": 2, "c": 3}
        )
        with self.assertWarns(PyGIDeprecationWarning) as w:
            fn(self, a=1)
        self.assertRegex(
            str(w.warning),
            ".*relying on deprecated non-standard defaults.*explicitly use: b=2, c=3",
        )
        self.assertIn("test_gi.py", w.filename)

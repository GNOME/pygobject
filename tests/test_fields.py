# -*- Mode: Python; py-indent-offset: 4 -*-
# coding=utf-8

from __future__ import absolute_import

import math
import unittest

from gi.repository import GLib
from gi.repository import Regress
from gi.repository import GIMarshallingTests


class Number(object):

    def __init__(self, value):
        self.value = value

    def __int__(self):
        return int(self.value)

    def __float__(self):
        return float(self.value)


class TestFields(unittest.TestCase):

    def test_int8(self):
        s = Regress.TestStructA()
        s.some_int8 = 21
        self.assertEqual(s.some_int8, 21)

        s.some_int8 = b"\x42"
        self.assertEqual(s.some_int8, 0x42)

        self.assertRaises(TypeError, setattr, s, "some_int8", b"ab")
        self.assertRaises(TypeError, setattr, s, "some_int8", None)
        self.assertRaises(OverflowError, setattr, s, "some_int8", 128)
        self.assertRaises(OverflowError, setattr, s, "some_int8", -129)

        s.some_int8 = 3.6
        self.assertEqual(s.some_int8, 3)

        s.some_int8 = Number(55)
        self.assertEqual(s.some_int8, 55)

    def test_int(self):
        s = Regress.TestStructA()
        s.some_int = GLib.MAXINT
        self.assertEqual(s.some_int, GLib.MAXINT)

        self.assertRaises(TypeError, setattr, s, "some_int", b"a")
        self.assertRaises(TypeError, setattr, s, "some_int", None)
        self.assertRaises(
            OverflowError, setattr, s, "some_int", GLib.MAXINT + 1)
        self.assertRaises(
            OverflowError, setattr, s, "some_int", GLib.MININT - 1)

        s.some_int = 3.6
        self.assertEqual(s.some_int, 3)

        s.some_int = Number(GLib.MININT)
        self.assertEqual(s.some_int, GLib.MININT)

    def test_long(self):
        s = GIMarshallingTests.SimpleStruct()
        s.long_ = GLib.MAXLONG
        self.assertEqual(s.long_, GLib.MAXLONG)

        self.assertRaises(TypeError, setattr, s, "long_", b"a")
        self.assertRaises(TypeError, setattr, s, "long_", None)
        self.assertRaises(OverflowError, setattr, s, "long_", GLib.MAXLONG + 1)
        self.assertRaises(OverflowError, setattr, s, "long_", GLib.MINLONG - 1)

        s.long_ = 3.6
        self.assertEqual(s.long_, 3)

        s.long_ = Number(GLib.MINLONG)
        self.assertEqual(s.long_, GLib.MINLONG)

    def test_double(self):
        s = Regress.TestStructA()
        s.some_double = GLib.MAXDOUBLE
        self.assertEqual(s.some_double, GLib.MAXDOUBLE)
        s.some_double = GLib.MINDOUBLE
        self.assertEqual(s.some_double, GLib.MINDOUBLE)

        s.some_double = float("nan")
        self.assertTrue(math.isnan(s.some_double))

        self.assertRaises(TypeError, setattr, s, "some_double", b"a")
        self.assertRaises(TypeError, setattr, s, "some_double", None)

    def test_gtype(self):
        s = Regress.TestStructE()

        s.some_type = Regress.TestObj
        self.assertEqual(s.some_type, Regress.TestObj.__gtype__)

        self.assertRaises(TypeError, setattr, s, "some_type", 42)

    def test_unichar(self):
        # I can't find a unichar field..
        pass

    def test_utf8(self):
        s = GIMarshallingTests.BoxedStruct()
        s.string_ = "hello"
        self.assertEqual(s.string_, "hello")

        s.string_ = u"hello"
        self.assertEqual(s.string_, u"hello")

        s.string_ = None
        self.assertEqual(s.string_, None)

        self.assertRaises(TypeError, setattr, s, "string_", 42)

    def test_array_of_structs(self):
        s = Regress.TestStructD()
        self.assertEqual(s.array1, [])
        self.assertEqual(s.array2, [])

    def test_interface(self):
        s = Regress.TestStructC()

        obj = Regress.TestObj()
        s.obj = obj
        self.assertTrue(s.obj is obj)

        s.obj = None
        self.assertTrue(s.obj is None)

        self.assertRaises(TypeError, setattr, s, "obj", object())

    def test_glist(self):
        s = Regress.TestStructD()
        self.assertEqual(s.list, [])

        self.assertRaises(TypeError, setattr, s, "list", [object()])

    def test_gpointer(self):
        glist = GLib.List()

        glist.data = 123
        self.assertEqual(glist.data, 123)

        glist.data = None
        self.assertEqual(glist.data, 0)

    def test_gptrarray(self):
        s = Regress.TestStructD()
        self.assertEqual(s.garray, [])

        self.assertRaises(TypeError, setattr, s, "garray", [object()])

    def test_enum(self):
        s = Regress.TestStructA()

        s.some_enum = Regress.TestEnum.VALUE3
        self.assertEqual(s.some_enum, Regress.TestEnum.VALUE3)

        self.assertRaises(TypeError, setattr, s, "some_enum", object())

        s.some_enum = 0
        self.assertEqual(s.some_enum, Regress.TestEnum.VALUE1)

    def test_union(self):
        s = Regress.TestStructE()
        self.assertEqual(s.some_union, [None, None])

    def test_struct(self):
        s = GIMarshallingTests.NestedStruct()

        # FIXME: segfaults
        # https://bugzilla.gnome.org/show_bug.cgi?id=747002
        # s.simple_struct = None

        self.assertRaises(TypeError, setattr, s, "simple_struct", object())

        sub = GIMarshallingTests.SimpleStruct()
        sub.long_ = 42
        s.simple_struct = sub
        self.assertEqual(s.simple_struct.long_, 42)

    def test_ghashtable(self):
        obj = Regress.TestObj()
        self.assertTrue(obj.hash_table is None)

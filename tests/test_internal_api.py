# -*- coding: utf-8 -*-
# -*- Mode: Python -*-

from __future__ import absolute_import

import unittest
import pytest

from gi.repository import GLib, GObject
from gi._compat import PY3

from . import testhelper


class PyGObject(GObject.GObject):
    __gtype_name__ = 'PyGObject'
    __gproperties__ = {
        'label': (GObject.TYPE_STRING,
                  'label property',
                  'the label of the object',
                  'default',
                  GObject.ParamFlags.READABLE | GObject.ParamFlags.WRITABLE),
        }

    def __init__(self):
        self._props = {}
        GObject.GObject.__init__(self)
        self.set_property('label', 'hello')

    def do_set_property(self, name, value):
        self._props[name] = value

    def do_get_property(self, name):
        return self._props[name]


def test_parse_constructor_args():
    assert testhelper.test_parse_constructor_args("foo") == 1


class TestObject(unittest.TestCase):
    def test_create_ctor(self):
        o = PyGObject()
        self.assertTrue(isinstance(o, GObject.Object))
        self.assertTrue(isinstance(o, PyGObject))

        # has expected property
        self.assertEqual(o.props.label, 'hello')
        o.props.label = 'goodbye'
        self.assertEqual(o.props.label, 'goodbye')
        self.assertRaises(AttributeError, getattr, o.props, 'nosuchprop')

    def test_pyobject_new_test_type(self):
        o = testhelper.create_test_type()
        self.assertTrue(isinstance(o, PyGObject))

        # has expected property
        self.assertEqual(o.props.label, 'hello')
        o.props.label = 'goodbye'
        self.assertEqual(o.props.label, 'goodbye')
        self.assertRaises(AttributeError, getattr, o.props, 'nosuchprop')

    def test_new_refcount(self):
        # TODO: justify why this should be 2
        self.assertEqual(testhelper.test_g_object_new(), 2)


class TestGValueConversion(unittest.TestCase):
    def test_int(self):
        self.assertEqual(testhelper.test_value(0), 0)
        self.assertEqual(testhelper.test_value(5), 5)
        self.assertEqual(testhelper.test_value(-5), -5)
        self.assertEqual(testhelper.test_value(GLib.MAXINT32), GLib.MAXINT32)
        self.assertEqual(testhelper.test_value(GLib.MININT32), GLib.MININT32)

    def test_str(self):
        self.assertEqual(testhelper.test_value('hello'), 'hello')

    def test_int_array(self):
        self.assertEqual(testhelper.test_value_array([]), [])
        self.assertEqual(testhelper.test_value_array([0]), [0])
        ar = list(range(100))
        self.assertEqual(testhelper.test_value_array(ar), ar)

    def test_str_array(self):
        self.assertEqual(testhelper.test_value_array([]), [])
        self.assertEqual(testhelper.test_value_array(['a']), ['a'])
        ar = ('aa ' * 1000).split()
        self.assertEqual(testhelper.test_value_array(ar), ar)


class TestErrors(unittest.TestCase):
    def test_gerror(self):
        callable_ = lambda: GLib.file_get_contents('/nonexisting ')
        self.assertRaises(GLib.GError, testhelper.test_gerror_exception, callable_)

    def test_no_gerror(self):
        callable_ = lambda: GLib.file_get_contents(__file__)
        self.assertEqual(testhelper.test_gerror_exception(callable_), None)


def test_to_unichar_conv():
    assert testhelper.test_to_unichar_conv(u"A") == 65
    assert testhelper.test_to_unichar_conv(u"Ä") == 196

    if PY3:
        with pytest.raises(TypeError):
            assert testhelper.test_to_unichar_conv(b"\x65")
    else:
        assert testhelper.test_to_unichar_conv(b"\x65") == 0x65
        with pytest.raises(ValueError):
            assert testhelper.test_to_unichar_conv(b"\xff")

    with pytest.raises(TypeError):
        testhelper.test_to_unichar_conv(object())

    with pytest.raises(TypeError):
        testhelper.test_to_unichar_conv(u"AA")

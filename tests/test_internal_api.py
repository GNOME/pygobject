# -*- Mode: Python -*-

import unittest

from gi.repository import GLib, GObject

import testhelper
import testmodule


class TestObject(unittest.TestCase):
    def test_create_ctor(self):
        o = testmodule.PyGObject()
        self.assertTrue(isinstance(o, GObject.Object))
        self.assertTrue(isinstance(o, testmodule.PyGObject))

        # has expected property
        self.assertEqual(o.props.label, 'hello')
        o.props.label = 'goodbye'
        self.assertEqual(o.props.label, 'goodbye')
        self.assertRaises(AttributeError, getattr, o.props, 'nosuchprop')

    def test_pyobject_new_test_type(self):
        o = testhelper.create_test_type()
        self.assertTrue(isinstance(o, testmodule.PyGObject))

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
        self.assertEqual(testhelper.test_value(GObject.G_MAXINT32), GObject.G_MAXINT32)
        self.assertEqual(testhelper.test_value(GObject.G_MININT32), GObject.G_MININT32)

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


if __name__ == '__main__':
    unittest.main()

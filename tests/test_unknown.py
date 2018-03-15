# -*- Mode: Python -*-

from __future__ import absolute_import

import unittest

from gi.repository import GObject

import testhelper


TestInterface = GObject.GType.from_name('TestInterface')


class TestUnknown(unittest.TestCase):
    def test_unknown_interface(self):
        obj = testhelper.get_unknown()
        TestUnknownGType = GObject.GType.from_name('TestUnknown')
        TestUnknown = GObject.new(TestUnknownGType).__class__
        assert isinstance(obj, testhelper.Interface)
        assert isinstance(obj, TestUnknown)

    def test_property(self):
        obj = testhelper.get_unknown()
        self.assertEqual(obj.get_property('some-property'), None)
        obj.set_property('some-property', 'foo')

    def test_unknown_property(self):
        obj = testhelper.get_unknown()
        self.assertRaises(TypeError, obj.get_property, 'unknown')
        self.assertRaises(TypeError, obj.set_property, 'unknown', '1')

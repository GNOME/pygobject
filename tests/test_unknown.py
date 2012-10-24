# -*- Mode: Python -*-

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

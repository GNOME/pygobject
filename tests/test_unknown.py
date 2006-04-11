
import unittest
from gobject import GType, new

from common import gobject, testhelper

TestInterface = GType.from_name('TestInterface')

class TestUnknown(unittest.TestCase):
    def testFoo(self):
        obj = testhelper.get_unknown()
        TestUnknownGType = GType.from_name('TestUnknown')
        TestUnknown = new(TestUnknownGType).__class__
        assert isinstance(obj, testhelper.Interface)
        assert isinstance(obj, TestUnknown)

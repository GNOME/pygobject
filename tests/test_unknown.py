
import unittest

from common import gobject, testhelper

TestInterface = gobject.type_from_name('TestInterface')
    
class TestUnknown(unittest.TestCase):
    def testFoo(self):
        obj = testhelper.get_unknown()
        TestUnknownGType = gobject.type_from_name('TestUnknown')
        TestUnknown = gobject.new(TestUnknownGType).__class__
        assert isinstance(obj, testhelper.Interface)
        assert isinstance(obj, TestUnknown)

# -*- Mode: Python -*-

import unittest

from common import gobject, testhelper


class TestGObjectAPI(unittest.TestCase):
    def testGObjectModule(self):
        obj = gobject.GObject()
        self.assertEquals(obj.__module__,
                          'gobject._gobject')
        self.assertEquals(obj.__grefcount__, 1)


class TestFloating(unittest.TestCase):
    def testFloatingWithSinkFunc(self):
        obj = testhelper.FloatingWithSinkFunc()
        self.assertEquals(obj.__grefcount__, 1)

        obj = gobject.new(testhelper.FloatingWithSinkFunc)
        self.assertEquals(obj.__grefcount__, 1)

    def testFloatingWithoutSinkFunc(self):
        obj = testhelper.FloatingWithoutSinkFunc()
        self.assertEquals(obj.__grefcount__, 1)

        obj = gobject.new(testhelper.FloatingWithoutSinkFunc)
        self.assertEquals(obj.__grefcount__, 1)

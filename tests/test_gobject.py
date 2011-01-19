# -*- Mode: Python -*-

import unittest

import gobject
import testhelper


class TestGObjectAPI(unittest.TestCase):
    def testGObjectModule(self):
        obj = gobject.GObject()
        self.assertEquals(obj.__module__,
                          'gobject._gobject')


class TestReferenceCounting(unittest.TestCase):
    def testRegularObject(self):
        obj = gobject.GObject()
        self.assertEquals(obj.__grefcount__, 1)

        obj = gobject.new(gobject.GObject)
        self.assertEquals(obj.__grefcount__, 1)

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

# -*- Mode: Python -*-

from __future__ import absolute_import

import unittest

from gi.repository import GObject
import testhelper


GUnknown = GObject.type_from_name("TestUnknown")
Unknown = GUnknown.pytype


class MyUnknown(Unknown, testhelper.Interface):
    some_property = GObject.Property(type=str)

    def __init__(self):
        Unknown.__init__(self)
        self.called = False

    def do_iface_method(self):
        self.called = True
        Unknown.do_iface_method(self)


GObject.type_register(MyUnknown)


class MyObject(GObject.GObject, testhelper.Interface):
    some_property = GObject.Property(type=str)

    def __init__(self):
        GObject.GObject.__init__(self)
        self.called = False

    def do_iface_method(self):
        self.called = True


GObject.type_register(MyObject)


class TestIfaceImpl(unittest.TestCase):

    def test_reimplement_interface(self):
        m = MyUnknown()
        m.iface_method()
        self.assertEqual(m.called, True)

    def test_implement_interface(self):
        m = MyObject()
        m.iface_method()
        self.assertEqual(m.called, True)

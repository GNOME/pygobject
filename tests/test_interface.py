import unittest

import testmodule
from common import gobject, testhelper
from gobject import GObject, GInterface

GUnknown = gobject.type_from_name("TestUnknown")
Unknown = GUnknown.pytype

class MyUnknown(Unknown, testhelper.Interface):
    def __init__(self):
        Unknown.__init__(self)
        self.called = False

    def do_iface_method(self):
        self.called = True
        Unknown.do_iface_method(self)

gobject.type_register(MyUnknown)

class TestIfaceImpl(unittest.TestCase):
    def testMethodCall(self):
        m = MyUnknown()
        m.iface_method()
        self.assertEqual(m.called, True)

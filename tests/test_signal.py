# -*- Mode: Python -*-

import gc
import unittest

from common import gobject

class C(gobject.GObject):
    __gsignals__ = { 'my_signal': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                                   (gobject.TYPE_INT,)) }
    def do_my_signal(self, arg):
        self.arg = arg
gobject.type_register(C)

class D(C):
    def do_my_signal(self, arg2):
        self.arg2 = arg2
	C.do_my_signal(self, arg2)
gobject.type_register(D)

class TestChaining(unittest.TestCase):
    def setUp(self):
        self.inst = C()
        self.inst.connect("my_signal", self.my_signal_handler_cb, 1, 2, 3)

    def my_signal_handler_cb(self, *args):
        assert len(args) == 5
        assert isinstance(args[0], C)
        assert args[0] == self.inst
        
        assert isinstance(args[1], int)
        assert args[1] == 42

        assert args[2:] == (1, 2, 3)
        
    def testChaining(self):
        self.inst.emit("my_signal", 42)
        assert self.inst.arg == 42
        
    def testChaining(self):
        inst2 = D()
        inst2.emit("my_signal", 44)
        assert inst2.arg == 44
        assert inst2.arg2 == 44

# This is for bug 153718
class TestGSignalsError(unittest.TestCase):
    def testInvalidType(self, *args):
        class Foo(gobject.GObject):
            __gsignals__ = None
        self.assertRaises(TypeError, gobject.type_register, Foo)
        gc.collect()
        
    def testInvalidName(self, *args):
        class Foo(gobject.GObject):
            __gsignals__ = {'not-exists' : 'override'}
        self.assertRaises(TypeError, gobject.type_register, Foo)
        gc.collect()

class TestGPropertyError(unittest.TestCase):
    def testInvalidType(self, *args):
        class Foo(gobject.GObject):
            __gproperties__ = None
        self.assertRaises(TypeError, gobject.type_register, Foo)
        gc.collect()
        
    def testInvalidName(self, *args):
        class Foo(gobject.GObject):
            __gproperties__ = { None: None }
            
        self.assertRaises(TypeError, gobject.type_register, Foo)
        gc.collect()

if __name__ == '__main__':
    unittest.main()

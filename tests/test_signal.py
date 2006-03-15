# -*- Mode: Python -*-

import gc
import unittest

from common import gobject

class C(gobject.GObject):
    __gsignals__ = { 'my_signal': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                                   (gobject.TYPE_INT,)) }
    def do_my_signal(self, arg):
        self.arg = arg

class D(C):
    def do_my_signal(self, arg2):
        self.arg2 = arg2
	C.do_my_signal(self, arg2)

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
        def foo():
            class Foo(gobject.GObject):
                __gsignals__ = None
        self.assertRaises(TypeError, foo)
        gc.collect()

    def testInvalidName(self, *args):
        def foo():
            class Foo(gobject.GObject):
                __gsignals__ = {'not-exists' : 'override'}
        self.assertRaises(TypeError, foo)
        gc.collect()

class TestGPropertyError(unittest.TestCase):
    def testInvalidType(self, *args):
        def foo():
            class Foo(gobject.GObject):
                __gproperties__ = None
        self.assertRaises(TypeError, foo)
        gc.collect()

    def testInvalidName(self, *args):
        def foo():
            class Foo(gobject.GObject):
                __gproperties__ = { None: None }

        self.assertRaises(TypeError, foo)
        gc.collect()

class TestList(unittest.TestCase):
    def testListObject(self):
        self.assertEqual(gobject.signal_list_names(C), ('my-signal',))


def my_accumulator(ihint, return_accu, handler_return, user_data):
    """An accumulator that stops emission when the sum of handler
    returned values reaches 3"""
    assert user_data == "accum data"
    if return_accu >= 3:
        return False, return_accu
    return True, return_accu + handler_return

class Foo(gobject.GObject):
    __gsignals__ = {
        'my-acc-signal': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_INT,
                                   (), my_accumulator, "accum data"),
        'my-other-acc-signal': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_BOOLEAN,
                                (), gobject.signal_accumulator_true_handled)
        }

class TestAccumulator(unittest.TestCase):

    def testAccumulator(self):
        inst = Foo()
        inst.connect("my-acc-signal", lambda obj: 1)
        inst.connect("my-acc-signal", lambda obj: 2)
        ## the value returned in the following handler will not be
        ## considered, because at this point the accumulator already
        ## reached its limit.
        inst.connect("my-acc-signal", lambda obj: 3)
        retval = inst.emit("my-acc-signal")
        self.assertEqual(retval, 3)

    def testAccumulatorTrueHandled(self):
        inst = Foo()
        inst.connect("my-other-acc-signal", self._true_handler1)
        inst.connect("my-other-acc-signal", self._true_handler2)
        ## the following handler will not be called because handler2
        ## returns True, so it should stop the emission.
        inst.connect("my-other-acc-signal", self._true_handler3)
        self.__true_val = None
        inst.emit("my-other-acc-signal")
        self.assertEqual(self.__true_val, 2)

    def _true_handler1(self, obj):
        self.__true_val = 1
        return False
    def _true_handler2(self, obj):
        self.__true_val = 2
        return True
    def _true_handler3(self, obj):
        self.__true_val = 3
        return False

class E(gobject.GObject):
    __gsignals__ = { 'signal': (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                                ()) }
    def __init__(self):
        gobject.GObject.__init__(self)
        self.status = 0

    def do_signal(self):
        assert self.status == 0
        self.status = 1

class TestEmissionHook(unittest.TestCase):
    def testAdd(self):
        self.hook = True
        e = E()
        e.connect('signal', self._callback)
        gobject.add_emission_hook(E, "signal", self._emission_hook)
        e.emit('signal')
        self.assertEqual(e.status, 3)

    def testRemove(self):
        self.hook = False
        e = E()
        e.connect('signal', self._callback)
        hook_id = gobject.add_emission_hook(E, "signal", self._emission_hook)
        gobject.remove_emission_hook(E, "signal", hook_id)
        e.emit('signal')
        self.assertEqual(e.status, 3)

    def _emission_hook(self, e):
        self.assertEqual(e.status, 1)
        e.status = 2

    def _callback(self, e):
        if self.hook:
            self.assertEqual(e.status, 2)
        else:
            self.assertEqual(e.status, 1)
        e.status = 3

if __name__ == '__main__':
    unittest.main()

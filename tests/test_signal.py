# -*- Mode: Python -*-

import gc
import unittest
import sys

from gi.repository import GObject
from gi._gobject import signalhelper
import testhelper
from compathelper import _long


class C(GObject.GObject):
    __gsignals__ = {'my_signal': (GObject.SignalFlags.RUN_FIRST, None,
                                  (GObject.TYPE_INT,))}

    def do_my_signal(self, arg):
        self.arg = arg


class D(C):
    def do_my_signal(self, arg2):
        self.arg2 = arg2
        C.do_my_signal(self, arg2)


class TestSignalCreation(unittest.TestCase):
    # Bug 540376.
    def test_illegals(self):
        self.assertRaises(TypeError, lambda: GObject.signal_new('test',
                                                                None,
                                                                0,
                                                                None,
                                                                (GObject.TYPE_LONG,)))


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

    def testChaining2(self):
        inst2 = D()
        inst2.emit("my_signal", 44)
        assert inst2.arg == 44
        assert inst2.arg2 == 44

# This is for bug 153718


class TestGSignalsError(unittest.TestCase):
    def testInvalidType(self, *args):
        def foo():
            class Foo(GObject.GObject):
                __gsignals__ = None
        self.assertRaises(TypeError, foo)
        gc.collect()

    def testInvalidName(self, *args):
        def foo():
            class Foo(GObject.GObject):
                __gsignals__ = {'not-exists': 'override'}
        self.assertRaises(TypeError, foo)
        gc.collect()


class TestGPropertyError(unittest.TestCase):
    def testInvalidType(self, *args):
        def foo():
            class Foo(GObject.GObject):
                __gproperties__ = None
        self.assertRaises(TypeError, foo)
        gc.collect()

    def testInvalidName(self, *args):
        def foo():
            class Foo(GObject.GObject):
                __gproperties__ = {None: None}

        self.assertRaises(TypeError, foo)
        gc.collect()


class TestList(unittest.TestCase):
    def testListObject(self):
        self.assertEqual(GObject.signal_list_names(C), ('my-signal',))


def my_accumulator(ihint, return_accu, handler_return, user_data):
    """An accumulator that stops emission when the sum of handler
    returned values reaches 3"""
    assert user_data == "accum data"
    if return_accu >= 3:
        return False, return_accu
    return True, return_accu + handler_return


class Foo(GObject.GObject):
    __gsignals__ = {
        'my-acc-signal': (GObject.SignalFlags.RUN_LAST, GObject.TYPE_INT,
                          (), my_accumulator, "accum data"),
        'my-other-acc-signal': (GObject.SignalFlags.RUN_LAST, GObject.TYPE_BOOLEAN,
                                (), GObject.signal_accumulator_true_handled)
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


class E(GObject.GObject):
    __gsignals__ = {'signal': (GObject.SignalFlags.RUN_FIRST, None,
                               ())}

    def __init__(self):
        GObject.GObject.__init__(self)
        self.status = 0

    def do_signal(self):
        assert self.status == 0
        self.status = 1


class F(GObject.GObject):
    __gsignals__ = {'signal': (GObject.SignalFlags.RUN_FIRST, None,
                               ())}

    def __init__(self):
        GObject.GObject.__init__(self)
        self.status = 0

    def do_signal(self):
        self.status += 1


class TestEmissionHook(unittest.TestCase):
    def testAdd(self):
        self.hook = True
        e = E()
        e.connect('signal', self._callback)
        GObject.add_emission_hook(E, "signal", self._emission_hook)
        e.emit('signal')
        self.assertEqual(e.status, 3)

    def testRemove(self):
        self.hook = False
        e = E()
        e.connect('signal', self._callback)
        hook_id = GObject.add_emission_hook(E, "signal", self._emission_hook)
        GObject.remove_emission_hook(E, "signal", hook_id)
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

    def testCallbackReturnFalse(self):
        self.hook = False
        obj = F()

        def _emission_hook(obj):
            obj.status += 1
            return False
        GObject.add_emission_hook(obj, "signal", _emission_hook)
        obj.emit('signal')
        obj.emit('signal')
        self.assertEqual(obj.status, 3)

    def testCallbackReturnTrue(self):
        self.hook = False
        obj = F()

        def _emission_hook(obj):
            obj.status += 1
            return True
        hook_id = GObject.add_emission_hook(obj, "signal", _emission_hook)
        obj.emit('signal')
        obj.emit('signal')
        GObject.remove_emission_hook(obj, "signal", hook_id)
        self.assertEqual(obj.status, 4)

    def testCallbackReturnTrueButRemove(self):
        self.hook = False
        obj = F()

        def _emission_hook(obj):
            obj.status += 1
            return True
        hook_id = GObject.add_emission_hook(obj, "signal", _emission_hook)
        obj.emit('signal')
        GObject.remove_emission_hook(obj, "signal", hook_id)
        obj.emit('signal')
        self.assertEqual(obj.status, 3)


class TestClosures(unittest.TestCase):
    def setUp(self):
        self.count = 0

    def _callback(self, e):
        self.count += 1

    def testDisconnect(self):
        e = E()
        e.connect('signal', self._callback)
        e.disconnect_by_func(self._callback)
        e.emit('signal')
        self.assertEqual(self.count, 0)

    def testHandlerBlock(self):
        e = E()
        e.connect('signal', self._callback)
        e.handler_block_by_func(self._callback)
        e.emit('signal')
        self.assertEqual(self.count, 0)

    def testHandlerUnBlock(self):
        e = E()
        signal_id = e.connect('signal', self._callback)
        e.handler_block(signal_id)
        e.handler_unblock_by_func(self._callback)
        e.emit('signal')
        self.assertEqual(self.count, 1)

    def testHandlerBlockMethod(self):
        # Filed as #375589
        class A:
            def __init__(self):
                self.a = 0

            def callback(self, o):
                self.a = 1
                o.handler_block_by_func(self.callback)

        inst = A()
        e = E()
        e.connect("signal", inst.callback)
        e.emit('signal')
        self.assertEqual(inst.a, 1)
        gc.collect()

    def testGString(self):
        class C(GObject.GObject):
            __gsignals__ = {'my_signal': (GObject.SignalFlags.RUN_LAST, GObject.TYPE_GSTRING,
                                          (GObject.TYPE_GSTRING,))}

            def __init__(self, test):
                GObject.GObject.__init__(self)
                self.test = test

            def do_my_signal(self, data):
                self.data = data
                self.test.assertEqual(len(data), 3)
                return ''.join([data[2], data[1], data[0]])
        c = C(self)
        data = c.emit("my_signal", "\01\00\02")
        self.assertEqual(data, "\02\00\01")


class SigPropClass(GObject.GObject):
    __gsignals__ = {'my_signal': (GObject.SignalFlags.RUN_FIRST, None,
                                  (GObject.TYPE_INT,))}

    __gproperties__ = {
        'foo': (str, None, None, '', GObject.PARAM_WRITABLE | GObject.PARAM_CONSTRUCT),
        }

    signal_emission_failed = False

    def do_my_signal(self, arg):
        self.arg = arg

    def do_set_property(self, pspec, value):
        if pspec.name == 'foo':
            self._foo = value
        else:
            raise AttributeError('unknown property %s' % pspec.name)
        try:
            self.emit("my-signal", 1)
        except TypeError:
            self.signal_emission_failed = True


class TestSigProp(unittest.TestCase):
    def testEmitInPropertySetter(self):
        obj = SigPropClass()
        self.assertFalse(obj.signal_emission_failed)

f = GObject.SignalFlags.RUN_FIRST
l = GObject.SignalFlags.RUN_LAST
gfloat = GObject.TYPE_FLOAT
gdouble = GObject.TYPE_DOUBLE
guint = GObject.TYPE_UINT
gulong = GObject.TYPE_ULONG
gint64 = GObject.TYPE_INT64


class CM(GObject.GObject):
    __gsignals__ = dict(
        test1=(f, None, ()),
        test2=(l, None, (str,)),
        test3=(l, int, (gdouble,)),
        test4=(f, None, (bool, _long, gfloat, gdouble, int, guint, gulong)),
        test_float=(l, gfloat, (gfloat,)),
        test_double=(l, gdouble, (gdouble, )),
        test_int64=(l, gint64, (gint64, )),
        test_string=(l, str, (str, )),
        test_object=(l, object, (object, )),
        test_paramspec=(l, GObject.ParamSpec, ()),
    )

    testprop = GObject.Property(type=int)


class _TestCMarshaller:
    def setUp(self):
        self.obj = CM()
        testhelper.connectcallbacks(self.obj)

    def testTest1(self):
        self.obj.emit("test1")

    def testTest2(self):
        self.obj.emit("test2", "string")

    def testTest3(self):
        rv = self.obj.emit("test3", 42.0)
        self.assertEqual(rv, 20)

    def testTest4(self):
        self.obj.emit("test4", True, _long(10), 3.14, 1.78, 20, _long(30), _long(31))

    def testTestReturnFloat(self):
        rv = self.obj.emit("test-float", 1.234)
        self.assertTrue(rv >= 1.233999 and rv <= 1.2400001, rv)

    def testTestReturnDouble(self):
        rv = self.obj.emit("test-double", 1.234)
        self.assertEqual(rv, 1.234)

    def testTestReturnInt64(self):
        rv = self.obj.emit("test-int64", 102030405)
        self.assertEqual(rv, 102030405)

        rv = self.obj.emit("test-int64", GObject.G_MAXINT64)
        self.assertEqual(rv, GObject.G_MAXINT64 - 1)

        rv = self.obj.emit("test-int64", GObject.G_MININT64)
        self.assertEqual(rv, GObject.G_MININT64)

    def testTestReturnString(self):
        rv = self.obj.emit("test-string", "str")
        self.assertEqual(rv, "str")

    def testTestReturnObject(self):
        rv = self.obj.emit("test-object", self)
        self.assertEqual(rv, self)

    def testTestReturnParamSpec(self):
        rv = self.obj.emit("test-paramspec")
        self.assertEqual(rv.name, "test-param")
        self.assertEqual(rv.nick, "test")

    def testTestParamSpecArgFromC(self):
        self.notify_called = False

        def cb_notify(obj, prop):
            self.notify_called = True
            self.assertEqual(obj, self.obj)
            self.assertEqual(prop.name, "testprop")

        self.obj.connect("notify", cb_notify)
        self.obj.set_property("testprop", 42)
        self.assertTrue(self.notify_called)

if 'generic-c-marshaller' in GObject.features:
    class TestCMarshaller(_TestCMarshaller, unittest.TestCase):
        pass
else:
    print()
    print('** WARNING: LIBFFI disabled, not testing')
    print()

# Test for 374653


class TestPyGValue(unittest.TestCase):
    def testNoneNULLBoxedConversion(self):
        class C(GObject.GObject):
            __gsignals__ = dict(my_boxed_signal=(
                GObject.SignalFlags.RUN_LAST,
                GObject.TYPE_STRV, ()))

        obj = C()
        obj.connect('my-boxed-signal', lambda obj: None)
        sys.last_type = None
        obj.emit('my-boxed-signal')
        assert not sys.last_type


class TestSignalDecorator(unittest.TestCase):
    class Decorated(GObject.GObject):
        value = 0

        @GObject.Signal
        def pushed(self):
            """this will push"""
            self.value += 1

        @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST)
        def pulled(self):
            self.value -= 1

        stomped = GObject.Signal('stomped', arg_types=(int,), doc='this will stomp')
        unnamed = GObject.Signal()

    class DecoratedOverride(GObject.GObject):
        overridden_closure_called = False
        notify_called = False
        value = GObject.Property(type=int, default=0)

        @GObject.SignalOverride
        def notify(self, *args, **kargs):
            self.overridden_closure_called = True
            #GObject.GObject.notify(self, *args, **kargs)

        def on_notify(self, obj, prop):
            self.notify_called = True

    def setUp(self):
        self.unnamedCalled = False

    def onUnnamed(self, obj):
        self.unnamedCalled = True

    def testGetSignalArgs(self):
        self.assertEqual(self.Decorated.pushed.get_signal_args(),
                         (GObject.SignalFlags.RUN_FIRST, None, tuple()))
        self.assertEqual(self.Decorated.pulled.get_signal_args(),
                         (GObject.SignalFlags.RUN_LAST, None, tuple()))
        self.assertEqual(self.Decorated.stomped.get_signal_args(),
                         (GObject.SignalFlags.RUN_FIRST, None, (int,)))

    def testClosuresCalled(self):
        decorated = self.Decorated()
        self.assertEqual(decorated.value, 0)
        decorated.pushed.emit()
        self.assertEqual(decorated.value, 1)
        decorated.pulled.emit()
        self.assertEqual(decorated.value, 0)

    def testSignalCopy(self):
        blah = self.Decorated.stomped.copy('blah')
        self.assertEqual(str(blah), blah)
        self.assertEqual(blah.func, self.Decorated.stomped.func)
        self.assertEqual(blah.flags, self.Decorated.stomped.flags)
        self.assertEqual(blah.return_type, self.Decorated.stomped.return_type)
        self.assertEqual(blah.arg_types, self.Decorated.stomped.arg_types)
        self.assertEqual(blah.__doc__, self.Decorated.stomped.__doc__)

    def testDocString(self):
        # Test the two techniques for setting doc strings on the signals
        # class variables, through the "doc" keyword or as the getter doc string.
        self.assertEqual(self.Decorated.stomped.__doc__, 'this will stomp')
        self.assertEqual(self.Decorated.pushed.__doc__, 'this will push')

    def testUnnamedSignalGetsNamed(self):
        self.assertEqual(str(self.Decorated.unnamed), 'unnamed')

    def testUnnamedSignalGetsCalled(self):
        obj = self.Decorated()
        obj.connect('unnamed', self.onUnnamed)
        self.assertEqual(self.unnamedCalled, False)
        obj.emit('unnamed')
        self.assertEqual(self.unnamedCalled, True)

    def NOtestOverriddenSignal(self):
        # Test that the pushed signal is called in with super and the override
        # which should both increment the "value" to 3
        obj = self.DecoratedOverride()
        obj.connect("notify", obj.on_notify)
        self.assertEqual(obj.value, 0)
        #obj.notify.emit()
        obj.value = 1
        self.assertEqual(obj.value, 1)
        self.assertTrue(obj.overridden_closure_called)
        self.assertTrue(obj.notify_called)


class TestSignalConnectors(unittest.TestCase):
    class CustomButton(GObject.GObject):
        value = 0

        @GObject.Signal(arg_types=(int,))
        def clicked(self, value):
            self.value = value

    def setUp(self):
        self.obj = None
        self.value = None

    def onClicked(self, obj, value):
        self.obj = obj
        self.value = value

    def testSignalEmit(self):
        # standard callback connection with different forms of emit.
        obj = self.CustomButton()
        obj.connect('clicked', self.onClicked)

        # vanilla
        obj.emit('clicked', 1)
        self.assertEqual(obj.value, 1)
        self.assertEqual(obj, self.obj)
        self.assertEqual(self.value, 1)

        # using class signal as param
        self.obj = None
        self.value = None
        obj.emit(self.CustomButton.clicked, 1)
        self.assertEqual(obj, self.obj)
        self.assertEqual(self.value, 1)

        # using bound signal as param
        self.obj = None
        self.value = None
        obj.emit(obj.clicked, 1)
        self.assertEqual(obj, self.obj)
        self.assertEqual(self.value, 1)

        # using bound signal with emit
        self.obj = None
        self.value = None
        obj.clicked.emit(1)
        self.assertEqual(obj, self.obj)
        self.assertEqual(self.value, 1)

    def testSignalClassConnect(self):
        obj = self.CustomButton()
        obj.connect(self.CustomButton.clicked, self.onClicked)
        obj.emit('clicked', 2)
        self.assertEqual(obj, self.obj)
        self.assertEqual(self.value, 2)

    def testSignalBoundConnect(self):
        obj = self.CustomButton()
        obj.clicked.connect(self.onClicked)
        obj.emit('clicked', 3)
        self.assertEqual(obj, self.obj)
        self.assertEqual(self.value, 3)


# For this test to work with both python2 and 3 we need to dynamically
# exec the given code due to the new syntax causing an error in python 2.
annotated_class_code = """
class AnnotatedSignalClass(GObject.GObject):
    @GObject.Signal
    def sig1(self, a:int, b:float):
        pass

    @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST)
    def sig2_with_return(self, a:int, b:float) -> str:
        return "test"
"""


class TestPython3Signals(unittest.TestCase):
    AnnotatedClass = None

    def setUp(self):
        if sys.version_info >= (3, 0):
            exec(annotated_class_code, globals(), globals())
            self.assertTrue('AnnotatedSignalClass' in globals())
            self.AnnotatedClass = globals()['AnnotatedSignalClass']

    def testAnnotations(self):
        if self.AnnotatedClass:
            self.assertEqual(signalhelper.get_signal_annotations(self.AnnotatedClass.sig1.func),
                             (None, (int, float)))
            self.assertEqual(signalhelper.get_signal_annotations(self.AnnotatedClass.sig2_with_return.func),
                             (str, (int, float)))

            self.assertEqual(self.AnnotatedClass.sig2_with_return.get_signal_args(),
                             (GObject.SignalFlags.RUN_LAST, str, (int, float)))
            self.assertEqual(self.AnnotatedClass.sig2_with_return.arg_types,
                             (int, float))
            self.assertEqual(self.AnnotatedClass.sig2_with_return.return_type,
                             str)


if __name__ == '__main__':
    unittest.main()

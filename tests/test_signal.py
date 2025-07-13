import gc
import unittest
import sys
import weakref
import threading
import time

from gi.repository import GObject, GLib, Regress, Gio
from gi import _signalhelper as signalhelper
from gi.module import repository as repo

import testhelper
from .helper import capture_glib_warnings, capture_gi_deprecation_warnings


class C(GObject.GObject):
    __gsignals__ = {
        "my_signal": (GObject.SignalFlags.RUN_FIRST, None, (GObject.TYPE_INT,))
    }

    def do_my_signal(self, arg):
        self.arg = arg


class D(C):
    def do_my_signal(self, arg2):
        self.arg2 = arg2
        C.do_my_signal(self, arg2)


class TestSignalCreation(unittest.TestCase):
    # Bug 540376.
    def test_illegals(self):
        self.assertRaises(
            TypeError,
            lambda: GObject.signal_new("test", None, 0, None, (GObject.TYPE_LONG,)),
        )


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

    def test_chaining(self):
        self.inst.emit("my_signal", 42)
        assert self.inst.arg == 42

    def test_chaining2(self):
        inst2 = D()
        inst2.emit("my_signal", 44)
        assert inst2.arg == 44
        assert inst2.arg2 == 44


# This is for bug 153718


class TestGSignalsError(unittest.TestCase):
    def test_invalid_type(self, *args):
        def foo():
            class Foo(GObject.GObject):
                __gsignals__ = None

        self.assertRaises(TypeError, foo)
        gc.collect()

    def test_invalid_name(self, *args):
        def foo():
            class Foo(GObject.GObject):
                __gsignals__ = {"not-exists": "override"}

        with capture_glib_warnings(allow_warnings=True):
            self.assertRaises(TypeError, foo)
        gc.collect()


class TestGPropertyError(unittest.TestCase):
    def test_invalid_type(self, *args):
        def foo():
            class Foo(GObject.GObject):
                __gproperties__ = None

        self.assertRaises(TypeError, foo)
        gc.collect()

    def test_invalid_name(self, *args):
        def foo():
            class Foo(GObject.GObject):
                __gproperties__ = {None: None}

        self.assertRaises(TypeError, foo)
        gc.collect()


class TestList(unittest.TestCase):
    def test_list_names(self):
        self.assertEqual(GObject.signal_list_names(C), ("my-signal",))


def my_accumulator(ihint, return_accu, handler_return, user_data):
    """An accumulator that stops emission when the sum of handler
    returned values reaches 3.
    """
    assert user_data == "accum data"
    if return_accu >= 3:
        return False, return_accu
    return True, return_accu + handler_return


class Foo(GObject.GObject):
    my_acc_signal = GObject.Signal(
        return_type=GObject.TYPE_INT,
        flags=GObject.SignalFlags.RUN_LAST,
        accumulator=my_accumulator,
        accu_data="accum data",
    )

    my_other_acc_signal = GObject.Signal(
        return_type=GObject.TYPE_BOOLEAN,
        flags=GObject.SignalFlags.RUN_LAST,
        accumulator=GObject.signal_accumulator_true_handled,
    )

    my_acc_first_wins = GObject.Signal(
        return_type=GObject.TYPE_BOOLEAN,
        flags=GObject.SignalFlags.RUN_LAST,
        accumulator=GObject.signal_accumulator_first_wins,
    )


class TestAccumulator(unittest.TestCase):
    def test_accumulator(self):
        inst = Foo()
        inst.my_acc_signal.connect(lambda obj: 1)
        inst.my_acc_signal.connect(lambda obj: 2)
        # the value returned in the following handler will not be
        # considered, because at this point the accumulator already
        # reached its limit.
        inst.my_acc_signal.connect(lambda obj: 3)
        retval = inst.my_acc_signal.emit()
        self.assertEqual(retval, 3)

    def test_accumulator_true_handled(self):
        inst = Foo()
        inst.my_other_acc_signal.connect(self._true_handler1)
        inst.my_other_acc_signal.connect(self._true_handler2)
        # the following handler will not be called because handler2
        # returns True, so it should stop the emission.
        inst.my_other_acc_signal.connect(self._true_handler3)
        self.__true_val = None
        inst.my_other_acc_signal.emit()
        self.assertEqual(self.__true_val, 2)

    def test_accumulator_first_wins(self):
        # First signal hit will always win
        inst = Foo()
        inst.my_acc_first_wins.connect(self._true_handler3)
        inst.my_acc_first_wins.connect(self._true_handler1)
        inst.my_acc_first_wins.connect(self._true_handler2)
        self.__true_val = None
        inst.my_acc_first_wins.emit()
        self.assertEqual(self.__true_val, 3)

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
    __gsignals__ = {"signal": (GObject.SignalFlags.RUN_FIRST, None, ())}

    # Property used to test detailed signal
    prop = GObject.Property(type=int, default=0)

    def __init__(self):
        GObject.GObject.__init__(self)
        self.status = 0

    def do_signal(self):
        assert self.status == 0
        self.status = 1


class F(GObject.GObject):
    __gsignals__ = {"signal": (GObject.SignalFlags.RUN_FIRST, None, ())}

    def __init__(self):
        GObject.GObject.__init__(self)
        self.status = 0

    def do_signal(self):
        self.status += 1


class TestEmissionHook(unittest.TestCase):
    def test_add(self):
        self.hook = True
        e = E()
        e.connect("signal", self._callback)
        GObject.add_emission_hook(E, "signal", self._emission_hook)
        e.emit("signal")
        self.assertEqual(e.status, 3)

    def test_remove(self):
        self.hook = False
        e = E()
        e.connect("signal", self._callback)
        hook_id = GObject.add_emission_hook(E, "signal", self._emission_hook)
        GObject.remove_emission_hook(E, "signal", hook_id)
        e.emit("signal")
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

    def test_callback_return_false(self):
        self.hook = False
        obj = F()

        def _emission_hook(obj):
            obj.status += 1
            return False

        GObject.add_emission_hook(obj, "signal", _emission_hook)
        obj.emit("signal")
        obj.emit("signal")
        self.assertEqual(obj.status, 3)

    def test_callback_return_true(self):
        self.hook = False
        obj = F()

        def _emission_hook(obj):
            obj.status += 1
            return True

        hook_id = GObject.add_emission_hook(obj, "signal", _emission_hook)
        obj.emit("signal")
        obj.emit("signal")
        GObject.remove_emission_hook(obj, "signal", hook_id)
        self.assertEqual(obj.status, 4)

    def test_callback_return_true_but_remove(self):
        self.hook = False
        obj = F()

        def _emission_hook(obj):
            obj.status += 1
            return True

        hook_id = GObject.add_emission_hook(obj, "signal", _emission_hook)
        obj.emit("signal")
        GObject.remove_emission_hook(obj, "signal", hook_id)
        obj.emit("signal")
        self.assertEqual(obj.status, 3)


class TestMatching(unittest.TestCase):
    class Object(GObject.Object):
        status = 0
        prop = GObject.Property(type=int, default=0)

        @GObject.Signal()
        def my_signal(self):
            pass

    @unittest.expectedFailure  # https://bugzilla.gnome.org/show_bug.cgi?id=692918
    def test_signal_handler_block_matching(self):
        def dummy(*args):
            """Hack to work around:"""

        def foo(obj):
            obj.status += 1

        obj = self.Object()
        handler_id = GObject.signal_connect_closure(obj, "my-signal", foo, after=False)
        handler_id

        self.assertEqual(obj.status, 0)
        obj.emit("my-signal")
        self.assertEqual(obj.status, 1)

        # Blocking by match criteria disables the foo callback
        signal_id, detail = GObject.signal_parse_name("my-signal", obj, True)
        count = GObject.signal_handlers_block_matched(
            obj,
            GObject.SignalMatchType.ID | GObject.SignalMatchType.CLOSURE,
            signal_id=signal_id,
            detail=detail,
            closure=foo,
            func=dummy,
            data=dummy,
        )
        self.assertEqual(count, 1)
        obj.emit("my-signal")
        self.assertEqual(obj.status, 1)

        # Unblocking by the same match criteria allows callback to work again
        count = GObject.signal_handlers_unblock_matched(
            obj,
            GObject.SignalMatchType.ID | GObject.SignalMatchType.CLOSURE,
            signal_id=signal_id,
            detail=detail,
            closure=foo,
            func=dummy,
            data=dummy,
        )
        self.assertEqual(count, 1)
        obj.emit("my-signal")
        self.assertEqual(obj.status, 2)

        # Disconnecting by match criteria completely removes the handler
        count = GObject.signal_handlers_disconnect_matched(
            obj,
            GObject.SignalMatchType.ID | GObject.SignalMatchType.CLOSURE,
            signal_id=signal_id,
            detail=detail,
            closure=foo,
            func=dummy,
            data=dummy,
        )
        self.assertEqual(count, 1)
        obj.emit("my-signal")
        self.assertEqual(obj.status, 2)

    def test_signal_handler_find(self):
        def foo(obj):
            obj.status += 1

        obj = self.Object()
        handler_id = GObject.signal_connect_closure(obj, "my-signal", foo, after=False)

        signal_id, detail = GObject.signal_parse_name("my-signal", obj, True)
        found_id = GObject.signal_handler_find(
            obj,
            GObject.SignalMatchType.ID,
            signal_id=signal_id,
            detail=detail,
            closure=None,
            func=0,
            data=0,
        )
        self.assertEqual(handler_id, found_id)


class TestClosures(unittest.TestCase):
    def setUp(self):
        self.count = 0
        self.emission_stopped = False
        self.emission_error = False
        self.handler_pending = False

    def _callback_handler_pending(self, e):
        signal_id, detail = GObject.signal_parse_name("signal", e, True)
        self.handler_pending = GObject.signal_has_handler_pending(
            e, signal_id, detail, may_be_blocked=False
        )

    def _callback(self, e):
        self.count += 1

    def _callback_stop_emission(self, obj, prop, stop_it):
        if stop_it:
            obj.stop_emission_by_name("notify::prop")
            self.emission_stopped = True
        else:
            self.count += 1

    def _callback_invalid_stop_emission_name(self, obj, prop):
        with capture_glib_warnings(allow_warnings=True, allow_criticals=True) as warn:
            obj.stop_emission_by_name("notasignal::baddetail")
            self.emission_error = True
            self.assertTrue(warn)

    def test_disconnect_by_func(self):
        e = E()
        e.connect("signal", self._callback)
        e.disconnect_by_func(self._callback)
        e.emit("signal")
        self.assertEqual(self.count, 0)

    def test_disconnect(self):
        e = E()
        handler_id = e.connect("signal", self._callback)
        self.assertTrue(e.handler_is_connected(handler_id))
        e.disconnect(handler_id)
        e.emit("signal")
        self.assertEqual(self.count, 0)
        self.assertFalse(e.handler_is_connected(handler_id))

    def test_stop_emission_by_name(self):
        e = E()

        # Sandwich a callback that stops emission in between a callback that increments
        e.connect("notify::prop", self._callback_stop_emission, False)
        e.connect("notify::prop", self._callback_stop_emission, True)
        e.connect("notify::prop", self._callback_stop_emission, False)

        e.set_property("prop", 1234)
        self.assertEqual(e.get_property("prop"), 1234)
        self.assertEqual(self.count, 1)
        self.assertTrue(self.emission_stopped)

    def test_stop_emission_by_name_error(self):
        e = E()

        e.connect("notify::prop", self._callback_invalid_stop_emission_name)
        with capture_glib_warnings():
            e.set_property("prop", 1234)
        self.assertTrue(self.emission_error)

    def test_handler_block(self):
        e = E()
        e.connect("signal", self._callback)
        e.handler_block_by_func(self._callback)
        e.emit("signal")
        self.assertEqual(self.count, 0)

    def test_handler_unblock(self):
        e = E()
        handler_id = e.connect("signal", self._callback)
        e.handler_block(handler_id)
        e.handler_unblock_by_func(self._callback)
        e.emit("signal")
        self.assertEqual(self.count, 1)

    def test_handler_block_method(self):
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
        e.emit("signal")
        self.assertEqual(inst.a, 1)
        gc.collect()

    def test_gstring(self):
        class C(GObject.GObject):
            __gsignals__ = {
                "my_signal": (
                    GObject.SignalFlags.RUN_LAST,
                    GObject.TYPE_GSTRING,
                    (GObject.TYPE_GSTRING,),
                )
            }

            def __init__(self, test):
                GObject.GObject.__init__(self)
                self.test = test

            def do_my_signal(self, data):
                self.data = data
                self.test.assertEqual(len(data), 3)
                return "".join([data[2], data[1], data[0]])

        c = C(self)
        data = c.emit("my_signal", "\01\00\02")
        self.assertEqual(data, "\02\00\01")

    def test_handler_pending(self):
        obj = F()
        obj.connect("signal", self._callback_handler_pending)
        obj.connect("signal", self._callback)

        self.assertEqual(self.count, 0)
        self.assertEqual(self.handler_pending, False)

        obj.emit("signal")
        self.assertEqual(self.count, 1)
        self.assertEqual(self.handler_pending, True)

    def test_signal_handlers_destroy(self):
        obj = F()
        obj.connect("signal", self._callback)
        obj.connect("signal", self._callback)
        obj.connect("signal", self._callback)

        obj.emit("signal")
        self.assertEqual(self.count, 3)

        # count should remain at 3 after all handlers are destroyed
        GObject.signal_handlers_destroy(obj)
        obj.emit("signal")
        self.assertEqual(self.count, 3)


class SigPropClass(GObject.GObject):
    __gsignals__ = {
        "my_signal": (GObject.SignalFlags.RUN_FIRST, None, (GObject.TYPE_INT,))
    }

    __gproperties__ = {
        "foo": (
            str,
            None,
            None,
            "",
            GObject.ParamFlags.WRITABLE | GObject.ParamFlags.CONSTRUCT,
        ),
    }

    signal_emission_failed = False

    def do_my_signal(self, arg):
        self.arg = arg

    def do_set_property(self, pspec, value):
        if pspec.name == "foo":
            self._foo = value
        else:
            raise AttributeError(f"unknown property {pspec.name}")
        try:
            self.emit("my-signal", 1)
        except TypeError:
            self.signal_emission_failed = True


class TestSigProp(unittest.TestCase):
    def test_emit_in_property_setter(self):
        obj = SigPropClass()
        self.assertFalse(obj.signal_emission_failed)


class CM(GObject.GObject):
    __gsignals__ = {
        "test1": (GObject.SignalFlags.RUN_FIRST, None, ()),
        "test2": (GObject.SignalFlags.RUN_LAST, None, (str,)),
        "test3": (GObject.SignalFlags.RUN_LAST, int, (GObject.TYPE_DOUBLE,)),
        "test4": (
            GObject.SignalFlags.RUN_FIRST,
            None,
            (
                bool,
                int,
                GObject.TYPE_FLOAT,
                GObject.TYPE_DOUBLE,
                int,
                GObject.TYPE_UINT,
                GObject.TYPE_ULONG,
            ),
        ),
        "test_float": (
            GObject.SignalFlags.RUN_LAST,
            GObject.TYPE_FLOAT,
            (GObject.TYPE_FLOAT,),
        ),
        "test_double": (
            GObject.SignalFlags.RUN_LAST,
            GObject.TYPE_DOUBLE,
            (GObject.TYPE_DOUBLE,),
        ),
        "test_int64": (
            GObject.SignalFlags.RUN_LAST,
            GObject.TYPE_INT64,
            (GObject.TYPE_INT64,),
        ),
        "test_string": (GObject.SignalFlags.RUN_LAST, str, (str,)),
        "test_object": (GObject.SignalFlags.RUN_LAST, object, (object,)),
        "test_paramspec": (GObject.SignalFlags.RUN_LAST, GObject.ParamSpec, ()),
        "test_paramspec_in": (
            GObject.SignalFlags.RUN_LAST,
            GObject.ParamSpec,
            (GObject.ParamSpec,),
        ),
        "test_gvalue": (GObject.SignalFlags.RUN_LAST, GObject.Value, (GObject.Value,)),
        "test_gvalue_ret": (
            GObject.SignalFlags.RUN_LAST,
            GObject.Value,
            (GObject.TYPE_GTYPE,),
        ),
    }

    testprop = GObject.Property(type=int)


class _TestCMarshaller:
    def setUp(self):
        self.obj = CM()
        testhelper.connectcallbacks(self.obj)

    def test_test1(self):
        self.obj.emit("test1")

    def test_test2(self):
        self.obj.emit("test2", "string")

    def test_test3(self):
        rv = self.obj.emit("test3", 42.0)
        self.assertEqual(rv, 20)

    def test_test4(self):
        self.obj.emit("test4", True, 10, 3.14, 1.78, 20, 30, 31)

    def test_float(self):
        rv = self.obj.emit("test-float", 1.234)
        self.assertTrue(rv >= 1.233999 and rv <= 1.2400001, rv)

    def test_double(self):
        rv = self.obj.emit("test-double", 1.234)
        self.assertEqual(rv, 1.234)

    def test_int64(self):
        rv = self.obj.emit("test-int64", 102030405)
        self.assertEqual(rv, 102030405)

        rv = self.obj.emit("test-int64", GLib.MAXINT64)
        self.assertEqual(rv, GLib.MAXINT64 - 1)

        rv = self.obj.emit("test-int64", GLib.MININT64)
        self.assertEqual(rv, GLib.MININT64)

    def test_string(self):
        rv = self.obj.emit("test-string", "str")
        self.assertEqual(rv, "str")

    def test_object(self):
        rv = self.obj.emit("test-object", self)
        self.assertEqual(rv, self)

    def test_paramspec(self):
        rv = self.obj.emit("test-paramspec")
        self.assertEqual(rv.name, "test-param")
        self.assertEqual(rv.nick, "test")

    @unittest.skipUnless(
        hasattr(GObject, "param_spec_boolean"), "too old gobject-introspection"
    )
    def test_paramspec_in(self):
        rv = GObject.param_spec_boolean(
            "mybool", "test-bool", "do something", True, GObject.ParamFlags.READABLE
        )

        rv2 = self.obj.emit("test-paramspec-in", rv)
        self.assertEqual(type(rv), type(rv2))
        self.assertEqual(rv2.name, "mybool")
        self.assertEqual(rv2.nick, "test-bool")

    def test_C_paramspec(self):
        self.notify_called = False

        def cb_notify(obj, prop):
            self.notify_called = True
            self.assertEqual(obj, self.obj)
            self.assertEqual(prop.name, "testprop")

        self.obj.connect("notify", cb_notify)
        self.obj.set_property("testprop", 42)
        self.assertTrue(self.notify_called)

    def test_gvalue(self):
        # implicit int
        rv = self.obj.emit("test-gvalue", 42)
        self.assertEqual(rv, 42)

        # explicit float
        v = GObject.Value(GObject.TYPE_FLOAT, 1.234)
        rv = self.obj.emit("test-gvalue", v)
        self.assertAlmostEqual(rv, 1.234, places=4)

        # implicit float
        rv = self.obj.emit("test-gvalue", 1.234)
        self.assertAlmostEqual(rv, 1.234, places=4)

        # explicit int64
        v = GObject.Value(GObject.TYPE_INT64, GLib.MAXINT64)
        rv = self.obj.emit("test-gvalue", v)
        self.assertEqual(rv, GLib.MAXINT64)

        # explicit uint64
        v = GObject.Value(GObject.TYPE_UINT64, GLib.MAXUINT64)
        rv = self.obj.emit("test-gvalue", v)
        self.assertEqual(rv, GLib.MAXUINT64)

    @unittest.expectedFailure  # https://bugzilla.gnome.org/show_bug.cgi?id=705291
    def test_gvalue_implicit_int64(self):
        # implicit int64
        rv = self.obj.emit("test-gvalue", GLib.MAXINT64)
        self.assertEqual(rv, GLib.MAXINT64)

        # implicit uint64
        rv = self.obj.emit("test-gvalue", GLib.MAXUINT64)
        self.assertEqual(rv, GLib.MAXUINT64)

    def test_gvalue_ret(self):
        self.assertEqual(
            self.obj.emit("test-gvalue-ret", GObject.TYPE_INT), GLib.MAXINT
        )
        self.assertEqual(
            self.obj.emit("test-gvalue-ret", GObject.TYPE_UINT), GLib.MAXUINT
        )
        self.assertEqual(
            self.obj.emit("test-gvalue-ret", GObject.TYPE_INT64), GLib.MAXINT64
        )
        self.assertEqual(
            self.obj.emit("test-gvalue-ret", GObject.TYPE_UINT64), GLib.MAXUINT64
        )
        self.assertEqual(self.obj.emit("test-gvalue-ret", GObject.TYPE_STRING), "hello")


class TestCMarshaller(_TestCMarshaller, unittest.TestCase):
    pass


# Test for 374653


class TestPyGValue(unittest.TestCase):
    def test_none_null_boxed_conversion(self):
        class C(GObject.GObject):
            __gsignals__ = {
                "my_boxed_signal": (GObject.SignalFlags.RUN_LAST, GObject.TYPE_STRV, ())
            }

        obj = C()
        obj.connect("my-boxed-signal", lambda obj: None)
        sys.last_type = None
        obj.emit("my-boxed-signal")
        assert not sys.last_type


class TestSignalDecorator(unittest.TestCase):
    class Decorated(GObject.GObject):
        value = 0

        @GObject.Signal
        def pushed(self):
            """This will push."""
            self.value += 1

        @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST)
        def pulled(self):
            self.value -= 1

        @GObject.Signal(flags=GObject.SignalFlags.DETAILED)
        def detailed(self):
            self.value -= 1

        stomped = GObject.Signal("stomped", arg_types=(int,), doc="This will stomp.")
        unnamed = GObject.Signal()

    class DecoratedOverride(GObject.GObject):
        overridden_closure_called = False
        notify_called = False
        value = GObject.Property(type=int, default=0)

        @GObject.SignalOverride
        def notify(self, *args, **kargs):
            self.overridden_closure_called = True

        def on_notify(self, obj, prop):
            self.notify_called = True

    def setUp(self):
        self.unnamedCalled = False

    def onUnnamed(self, obj):
        self.unnamedCalled = True

    def test_disconnect(self):
        decorated = self.Decorated()
        id_ = decorated.pushed.connect(lambda *args: None)
        decorated.pushed.disconnect(id_)

    def test_signal_repr(self):
        decorated = self.Decorated()
        assert repr(decorated.pushed) == 'BoundSignal("pushed")'

    def test_signal_call(self):
        decorated = self.Decorated()
        assert decorated.value == 0
        decorated.pushed()
        assert decorated.value == 1

    def test_connect_detailed(self):
        decorated = self.Decorated()
        id_ = decorated.detailed.connect_detailed(lambda *args: None, "foo")
        decorated.pushed.disconnect(id_)

    def test_get_signal_args(self):
        self.assertEqual(
            self.Decorated.pushed.get_signal_args(),
            (GObject.SignalFlags.RUN_FIRST, None, (), None, None),
        )
        self.assertEqual(
            self.Decorated.pulled.get_signal_args(),
            (GObject.SignalFlags.RUN_LAST, None, (), None, None),
        )
        self.assertEqual(
            self.Decorated.stomped.get_signal_args(),
            (GObject.SignalFlags.RUN_FIRST, None, (int,), None, None),
        )

    def test_closures_called(self):
        decorated = self.Decorated()
        self.assertEqual(decorated.value, 0)
        decorated.pushed.emit()
        self.assertEqual(decorated.value, 1)
        decorated.pulled.emit()
        self.assertEqual(decorated.value, 0)

    def test_signal_copy(self):
        blah = self.Decorated.stomped.copy("blah")
        self.assertEqual(str(blah), blah)
        self.assertEqual(blah.func, self.Decorated.stomped.func)
        self.assertEqual(blah.flags, self.Decorated.stomped.flags)
        self.assertEqual(blah.return_type, self.Decorated.stomped.return_type)
        self.assertEqual(blah.arg_types, self.Decorated.stomped.arg_types)
        self.assertEqual(blah.__doc__, self.Decorated.stomped.__doc__)

    def test_doc_string(self):
        # Test the two techniques for setting doc strings on the signals
        # class variables, through the "doc" keyword or as the getter doc string.
        self.assertEqual(self.Decorated.stomped.__doc__, "This will stomp.")
        self.assertEqual(self.Decorated.pushed.__doc__, "This will push.")

    def test_unnamed_signal_gets_named(self):
        self.assertEqual(str(self.Decorated.unnamed), "unnamed")

    def test_unnamed_signal_gets_called(self):
        obj = self.Decorated()
        obj.connect("unnamed", self.onUnnamed)
        self.assertEqual(self.unnamedCalled, False)
        obj.emit("unnamed")
        self.assertEqual(self.unnamedCalled, True)

    def test_overridden_signal(self):
        # Test that the pushed signal is called in with super and the override
        # which should both increment the "value" to 3
        obj = self.DecoratedOverride()
        obj.connect("notify", obj.on_notify)
        self.assertEqual(obj.value, 0)
        obj.value = 1
        self.assertEqual(obj.value, 1)
        self.assertTrue(obj.overridden_closure_called)
        self.assertTrue(obj.notify_called)


class TestSignalConnectors(unittest.TestCase):
    class CustomButton(GObject.GObject):
        on_notify_called = False
        value = GObject.Property(type=int)

        @GObject.Signal(arg_types=(int,))
        def clicked(self, value):
            self.value = value

    def setUp(self):
        self.obj = None
        self.value = None

    def on_clicked(self, obj, value):
        self.obj = obj
        self.value = value

    def test_signal_notify(self):
        def on_notify(obj, param):
            obj.on_notify_called = True

        obj = self.CustomButton()
        obj.connect("notify", on_notify)
        self.assertFalse(obj.on_notify_called)
        obj.notify("value")
        self.assertTrue(obj.on_notify_called)

    def test_signal_emit(self):
        # standard callback connection with different forms of emit.
        obj = self.CustomButton()
        obj.connect("clicked", self.on_clicked)

        # vanilla
        obj.emit("clicked", 1)
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

    def test_signal_class_connect(self):
        obj = self.CustomButton()
        obj.connect(self.CustomButton.clicked, self.on_clicked)
        obj.emit("clicked", 2)
        self.assertEqual(obj, self.obj)
        self.assertEqual(self.value, 2)

    def test_signal_bound_connect(self):
        obj = self.CustomButton()
        obj.clicked.connect(self.on_clicked)
        obj.emit("clicked", 3)
        self.assertEqual(obj, self.obj)
        self.assertEqual(self.value, 3)


class _ConnectDataTestBase:
    # Notes:
    #  - self.Object is overridden in sub-classes.
    #  - Numeric suffixes indicate the number of user data args passed in.
    Object = None

    def run_connect_test(self, emit_args, user_data, flags=0):
        obj = self.Object()
        callback_args = []

        def callback(*args):
            callback_args.append(args)
            return 0

        obj.connect_data(
            "sig-with-int64-prop", callback, connect_flags=flags, *user_data
        )
        obj.emit("sig-with-int64-prop", *emit_args)
        self.assertEqual(len(callback_args), 1)
        return callback_args[0]

    def test_0(self):
        obj, value = self.run_connect_test([GLib.MAXINT64], user_data=[])
        self.assertIsInstance(obj, self.Object)
        self.assertEqual(value, GLib.MAXINT64)

    def test_1(self):
        obj, value, data = self.run_connect_test([GLib.MAXINT64], user_data=["mydata"])
        self.assertIsInstance(obj, self.Object)
        self.assertEqual(value, GLib.MAXINT64)
        self.assertEqual(data, "mydata")

    def test_after_0(self):
        obj, value = self.run_connect_test(
            [GLib.MAXINT64], user_data=[], flags=GObject.ConnectFlags.AFTER
        )
        self.assertIsInstance(obj, self.Object)
        self.assertEqual(value, GLib.MAXINT64)

    def test_after_1(self):
        obj, value, data = self.run_connect_test(
            [GLib.MAXINT64], user_data=["mydata"], flags=GObject.ConnectFlags.AFTER
        )
        self.assertIsInstance(obj, self.Object)
        self.assertEqual(value, GLib.MAXINT64)
        self.assertEqual(data, "mydata")

    def test_swaped_0(self):
        # Swapped only works with a single user data argument.
        with self.assertRaises(ValueError):
            self.run_connect_test(
                [GLib.MAXINT64], user_data=[], flags=GObject.ConnectFlags.SWAPPED
            )

    def test_swaped_1(self):
        # Notice obj and data are reversed in the return.
        data, value, obj = self.run_connect_test(
            [GLib.MAXINT64], user_data=["mydata"], flags=GObject.ConnectFlags.SWAPPED
        )
        self.assertIsInstance(obj, self.Object)
        self.assertEqual(value, GLib.MAXINT64)
        self.assertEqual(data, "mydata")

    def test_swaped_2(self):
        # Swapped only works with a single user data argument.
        with self.assertRaises(ValueError):
            self.run_connect_test(
                [GLib.MAXINT64], user_data=[1, 2], flags=GObject.ConnectFlags.SWAPPED
            )

    def test_after_and_swapped_0(self):
        # Swapped only works with a single user data argument.
        with self.assertRaises(ValueError):
            self.run_connect_test(
                [GLib.MAXINT64],
                user_data=[],
                flags=GObject.ConnectFlags.AFTER | GObject.ConnectFlags.SWAPPED,
            )

    def test_after_and_swapped_1(self):
        # Notice obj and data are reversed in the return.
        data, value, obj = self.run_connect_test(
            [GLib.MAXINT64],
            user_data=["mydata"],
            flags=GObject.ConnectFlags.AFTER | GObject.ConnectFlags.SWAPPED,
        )
        self.assertIsInstance(obj, self.Object)
        self.assertEqual(value, GLib.MAXINT64)
        self.assertEqual(data, "mydata")

    def test_after_and_swapped_2(self):
        # Swapped only works with a single user data argument.
        with self.assertRaises(ValueError):
            self.run_connect_test(
                [GLib.MAXINT64],
                user_data=[],
                flags=GObject.ConnectFlags.AFTER | GObject.ConnectFlags.SWAPPED,
            )


class TestConnectDataNonIntrospected(unittest.TestCase, _ConnectDataTestBase):
    # This tests connect_data with non-introspected signals
    # (created in Python in this case).
    class Object(GObject.Object):
        test = GObject.Signal()
        sig_with_int64_prop = GObject.Signal(
            return_type=GObject.TYPE_INT64,
            arg_types=[GObject.TYPE_INT64],
            flags=GObject.SignalFlags.RUN_LAST,
        )


class TestConnectDataIntrospected(unittest.TestCase, _ConnectDataTestBase):
    # This tests connect_data with introspected signals brought in from Regress.
    Object = Regress.TestObj


class TestInstallSignals(unittest.TestCase):
    # These tests only test how signalhelper.install_signals works
    # with the __gsignals__ dict and therefore does not need to use
    # GObject as a base class because that would automatically call
    # install_signals within the meta-class.
    class Base:
        __gsignals__ = {"test": (0, None, ())}

    class Sub1(Base):
        pass

    class Sub2(Base):
        @GObject.Signal
        def sub2test(self):
            pass

    def setUp(self):
        self.assertEqual(len(self.Base.__gsignals__), 1)
        signalhelper.install_signals(self.Base)
        self.assertEqual(len(self.Base.__gsignals__), 1)

    def test_subclass_gets_empty_gsignals_dict(self):
        # Installing signals will add the __gsignals__ dict to a class
        # if it doesn't already exists.
        self.assertFalse("__gsignals__" in self.Sub1.__dict__)
        signalhelper.install_signals(self.Sub1)
        self.assertTrue("__gsignals__" in self.Sub1.__dict__)
        # Sub1 should only contain an empty signals dict, this tests:
        # https://bugzilla.gnome.org/show_bug.cgi?id=686496
        self.assertEqual(self.Sub1.__dict__["__gsignals__"], {})

    def test_subclass_with_decorator_gets_gsignals_dict(self):
        self.assertFalse("__gsignals__" in self.Sub2.__dict__)
        signalhelper.install_signals(self.Sub2)
        self.assertTrue("__gsignals__" in self.Sub2.__dict__)
        self.assertEqual(len(self.Base.__gsignals__), 1)
        self.assertEqual(len(self.Sub2.__gsignals__), 1)
        self.assertTrue("sub2test" in self.Sub2.__gsignals__)

        # Make sure the vfunc was added
        self.assertTrue(hasattr(self.Sub2, "do_sub2test"))


class TestPython3Signals(unittest.TestCase):
    class AnnotatedClass(GObject.GObject):
        @GObject.Signal
        def sig1(self, a: int, b: float):
            pass

        @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST)
        def sig2_with_return(self, a: int, b: float) -> str:
            return "test"

    def test_annotations(self):
        self.assertEqual(
            signalhelper.get_signal_annotations(self.AnnotatedClass.sig1.func),
            (None, (int, float)),
        )
        self.assertEqual(
            signalhelper.get_signal_annotations(
                self.AnnotatedClass.sig2_with_return.func
            ),
            (str, (int, float)),
        )

        self.assertEqual(
            self.AnnotatedClass.sig2_with_return.get_signal_args(),
            (GObject.SignalFlags.RUN_LAST, str, (int, float), None, None),
        )
        self.assertEqual(self.AnnotatedClass.sig2_with_return.arg_types, (int, float))
        self.assertEqual(self.AnnotatedClass.sig2_with_return.return_type, str)

    def test_emit_return(self):
        obj = self.AnnotatedClass()
        self.assertEqual(obj.sig2_with_return.emit(1, 2.0), "test")


class TestSignalModuleLevelFunctions(unittest.TestCase):
    def test_signal_list_ids_with_invalid_type(self):
        with self.assertRaisesRegex(
            TypeError, "type must be instantiable or an interface.*"
        ):
            GObject.signal_list_ids(GObject.TYPE_INVALID)

    def test_signal_list_ids(self):
        with self.assertRaisesRegex(
            TypeError, "type must be instantiable or an interface.*"
        ):
            GObject.signal_list_ids(GObject.TYPE_INT)

        ids = GObject.signal_list_ids(C)
        self.assertEqual(len(ids), 1)
        # Note canonicalized names
        self.assertEqual(GObject.signal_name(ids[0]), "my-signal")
        # There is no signal 0 in gobject
        self.assertEqual(GObject.signal_name(0), None)

    def test_signal_lookup_with_invalid_type(self):
        with self.assertRaisesRegex(
            TypeError, "type must be instantiable or an interface.*"
        ):
            GObject.signal_lookup("NOT_A_SIGNAL_NAME", GObject.TYPE_INVALID)

    def test_signal_lookup(self):
        ids = GObject.signal_list_ids(C)
        self.assertEqual(ids[0], GObject.signal_lookup("my_signal", C))
        self.assertEqual(ids[0], GObject.signal_lookup("my-signal", C))

        with self.assertRaisesRegex(
            TypeError, "type must be instantiable or an interface.*"
        ):
            GObject.signal_lookup("NOT_A_SIGNAL_NAME", GObject.TYPE_INT)

        # Invalid signal names return 0 instead of raising
        self.assertEqual(GObject.signal_lookup("NOT_A_SIGNAL_NAME", C), 0)

    def test_signal_query(self):
        (my_signal_id,) = GObject.signal_list_ids(C)

        # Form is: (id, name, gtype, arg_count, return_type, (arg_type1, ...))
        my_signal_expected_query_result = [
            my_signal_id,
            "my-signal",
            C.__gtype__,
            1,
            GObject.TYPE_NONE,
            (GObject.TYPE_INT,),
        ]
        # signal_query(name, type)
        self.assertEqual(
            list(GObject.signal_query("my-signal", C)), my_signal_expected_query_result
        )
        # signal_query(signal_id)
        self.assertEqual(
            list(GObject.signal_query(my_signal_id)), my_signal_expected_query_result
        )
        # invalid query returns None instead of raising
        self.assertEqual(GObject.signal_query(0), None)
        self.assertEqual(GObject.signal_query("NOT_A_SIGNAL", C), None)


class TestIntrospectedSignals(unittest.TestCase):
    def test_object_param_signal(self):
        obj = Regress.TestObj()

        def callback(obj, obj_param):
            self.assertEqual(obj_param.props.int, 3)
            self.assertGreater(obj_param.__grefcount__, 1)
            obj.called = True

        obj.called = False
        obj.connect("sig-with-obj", callback)
        obj.emit_sig_with_obj()
        self.assertTrue(obj.called)

    def test_connect_after(self):
        obj = Regress.TestObj()

        def callback(obj, obj_param):
            obj.called = True

        obj.called = False
        obj.connect_after("sig-with-obj", callback)
        obj.emit_sig_with_obj()
        self.assertTrue(obj.called)

    def test_int64_param_from_py(self):
        obj = Regress.TestObj()

        def callback(obj, i):
            obj.callback_i = i
            return i

        obj.callback_i = None
        obj.connect("sig-with-int64-prop", callback)
        rv = obj.emit("sig-with-int64-prop", GLib.MAXINT64)
        self.assertEqual(rv, GLib.MAXINT64)
        self.assertEqual(obj.callback_i, GLib.MAXINT64)

    def test_uint64_param_from_py(self):
        obj = Regress.TestObj()

        def callback(obj, i):
            obj.callback_i = i
            return i

        obj.callback_i = None
        obj.connect("sig-with-uint64-prop", callback)
        rv = obj.emit("sig-with-uint64-prop", GLib.MAXUINT64)
        self.assertEqual(rv, GLib.MAXUINT64)
        self.assertEqual(obj.callback_i, GLib.MAXUINT64)

    def test_int64_param_from_c(self):
        obj = Regress.TestObj()

        def callback(obj, i):
            obj.callback_i = i
            return i

        obj.callback_i = None

        obj.connect("sig-with-int64-prop", callback)
        obj.emit_sig_with_int64()
        self.assertEqual(obj.callback_i, GLib.MAXINT64)

    def test_uint64_param_from_c(self):
        obj = Regress.TestObj()

        def callback(obj, i):
            obj.callback_i = i
            return i

        obj.callback_i = None

        obj.connect("sig-with-uint64-prop", callback)
        obj.emit_sig_with_uint64()
        self.assertEqual(obj.callback_i, GLib.MAXUINT64)

    def test_intarray_ret(self):
        obj = Regress.TestObj()

        def callback(obj, i):
            obj.callback_i = i
            return [i, i + 1]

        obj.callback_i = None

        try:
            obj.connect("sig-with-intarray-ret", callback)
        except TypeError as e:
            # compat with g-i 1.34.x
            if "unknown signal" in str(e):
                return
            raise

        rv = obj.emit("sig-with-intarray-ret", 42)
        self.assertEqual(obj.callback_i, 42)
        self.assertEqual(type(rv), GLib.Array)
        self.assertEqual(rv.len, 2)

    @unittest.skip("https://bugzilla.gnome.org/show_bug.cgi?id=669496")
    def test_array_parm(self):
        obj = Regress.TestObj()

        def callback(obj, arr):
            obj.callback_arr = arr

        obj.connect("sig-with-array-prop", callback)
        obj.callback_arr = None
        self.assertEqual(obj.emit("sig-with-array-prop", [1, 2, GLib.MAXUINT]), None)
        self.assertEqual(obj.callback_arr, [1, 2, GLib.MAXUINT])

    def test_held_struct_ref(self):
        held_structs = []

        def callback(obj, struct):
            # The struct held by Python will become a copy after this callback exits.
            struct.some_int = 42
            struct.some_int8 = 42
            held_structs.append(struct)

        struct = Regress.TestSimpleBoxedA()
        obj = Regress.TestObj()

        self.assertEqual(struct.some_int, 0)
        self.assertEqual(struct.some_int8, 0)

        obj.connect("test-with-static-scope-arg", callback)
        obj.emit("test-with-static-scope-arg", struct)

        # The held struct will be a copy of the modified struct.
        self.assertEqual(len(held_structs), 1)
        held_struct = held_structs[0]
        self.assertEqual(held_struct.some_int, 42)
        self.assertEqual(held_struct.some_int8, 42)

        # Boxed equality checks pointers by default.
        self.assertNotEqual(struct, held_struct)

    def test_action(self):
        obj = Regress.TestAction()
        other_obj = obj.emit("action")
        self.assertEqual(other_obj.__grefcount__, 1)
        other_obj2 = obj.emit("action2")
        self.assertIsNone(other_obj2)


class TestIntrospectedSignalsIssue158(unittest.TestCase):
    """The test for https://gitlab.gnome.org/GNOME/pygobject/issues/158."""

    _obj_sig_names = [
        sig.get_name() for sig in repo.find_by_name("Regress", "TestObj").get_signals()
    ]

    def __init__(self, *args):
        unittest.TestCase.__init__(self, *args)
        self._gc_thread_stop = False

    def _gc_thread(self):
        while not self._gc_thread_stop:
            gc.collect()
            time.sleep(0.010)

    def _callback(self, *args):
        pass

    def test_run(self):
        """Manually trigger GC from a different thread periodicaly
        while the main thread keeps connecting/disconnecting to/from signals.

        It takes a lot of time to reproduce the issue. It is possible to make it
        fail reliably by changing the code of pygobject_unwatch_closure slightly from:
          PyGObjectData *inst_data = data;
          inst_data->closures = g_slist_remove (inst_data->closures, closure);
        to
          PyGObjectData *inst_data = data;
          GSList *tmp = g_slist_remove (inst_data->closures, closure);
          g_usleep(G_USEC_PER_SEC/10);
          inst_data->closures = tmp;
        """
        obj = Regress.TestObj()
        gc_thread = threading.Thread(target=self._gc_thread)
        gc_thread.start()

        for _ in range(8):
            handlers = [obj.connect(sig, self._callback) for sig in self._obj_sig_names]
            time.sleep(0.010)
            while len(handlers) > 0:
                obj.disconnect(handlers.pop())

        self._gc_thread_stop = True
        gc_thread.join()


class _ConnectObjectTestBase:
    # Notes:
    #  - self.Object is overridden in sub-classes.
    #  - Numeric suffixes indicate the number of user data args passed in.
    Object = None
    SwapObject = None

    def run_connect_test(self, emit_args, user_data, flags=0):
        obj = self.Object()
        callback_args = []
        swap_obj = self.SwapObject()

        def callback(*args):
            callback_args.append(args)
            return 0

        if flags & GObject.ConnectFlags.AFTER:
            connect_func = obj.connect_object_after
        else:
            connect_func = obj.connect_object

        with capture_gi_deprecation_warnings():
            connect_func("sig-with-int64-prop", callback, swap_obj, *user_data)
        obj.emit("sig-with-int64-prop", *emit_args)
        self.assertEqual(len(callback_args), 1)
        return callback_args[0]

    def test_0(self):
        obj, value = self.run_connect_test([GLib.MAXINT64], user_data=[])
        self.assertIsInstance(obj, self.SwapObject)
        self.assertEqual(value, GLib.MAXINT64)

    def test_1(self):
        obj, value, data = self.run_connect_test([GLib.MAXINT64], user_data=["mydata"])
        self.assertIsInstance(obj, self.SwapObject)
        self.assertEqual(value, GLib.MAXINT64)
        self.assertEqual(data, "mydata")

    def test_2(self):
        obj, value, data1, data2 = self.run_connect_test(
            [GLib.MAXINT64], user_data=["mydata1", "mydata2"]
        )
        self.assertIsInstance(obj, self.SwapObject)
        self.assertEqual(value, GLib.MAXINT64)
        self.assertEqual(data1, "mydata1")
        self.assertEqual(data2, "mydata2")

    def test_after_0(self):
        obj, value = self.run_connect_test(
            [GLib.MAXINT64], user_data=[], flags=GObject.ConnectFlags.AFTER
        )
        self.assertIsInstance(obj, self.SwapObject)
        self.assertEqual(value, GLib.MAXINT64)

    def test_after_1(self):
        obj, value, data = self.run_connect_test(
            [GLib.MAXINT64], user_data=["mydata"], flags=GObject.ConnectFlags.AFTER
        )
        self.assertIsInstance(obj, self.SwapObject)
        self.assertEqual(value, GLib.MAXINT64)
        self.assertEqual(data, "mydata")

    def test_after_2(self):
        obj, value, data1, data2 = self.run_connect_test(
            [GLib.MAXINT64],
            user_data=["mydata1", "mydata2"],
            flags=GObject.ConnectFlags.AFTER,
        )
        self.assertIsInstance(obj, self.SwapObject)
        self.assertEqual(value, GLib.MAXINT64)
        self.assertEqual(data1, "mydata1")
        self.assertEqual(data2, "mydata2")


class TestConnectGObjectNonIntrospected(unittest.TestCase, _ConnectObjectTestBase):
    # This tests connect_object with non-introspected signals
    # (created in Python in this case).
    class Object(GObject.Object):
        test = GObject.Signal()
        sig_with_int64_prop = GObject.Signal(
            return_type=GObject.TYPE_INT64,
            arg_types=[GObject.TYPE_INT64],
            flags=GObject.SignalFlags.RUN_LAST,
        )

    # Object passed for swapping is GObject based.
    class SwapObject(GObject.Object):
        pass


class TestConnectGObjectIntrospected(unittest.TestCase, _ConnectObjectTestBase):
    # This tests connect_object with introspected signals brought in from Regress.
    Object = Regress.TestObj

    # Object passed for swapping is GObject based.
    class SwapObject(GObject.Object):
        pass


class TestConnectPyObjectNonIntrospected(unittest.TestCase, _ConnectObjectTestBase):
    # This tests connect_object with non-introspected signals
    # (created in Python in this case).
    class Object(GObject.Object):
        test = GObject.Signal()
        sig_with_int64_prop = GObject.Signal(
            return_type=GObject.TYPE_INT64,
            arg_types=[GObject.TYPE_INT64],
            flags=GObject.SignalFlags.RUN_LAST,
        )

    # Object passed for swapping is pure Python
    SwapObject = object


class TestConnectPyObjectIntrospected(unittest.TestCase, _ConnectObjectTestBase):
    # This tests connect_object with introspected signals brought in from Regress.
    Object = Regress.TestObj

    # Object passed for swapping is pure Python
    SwapObject = object


class _RefCountTestBase:
    # NOTE: ref counts are always one more than expected because the getrefcount()
    # function adds a ref for the input argument.

    # Sub-classes set this
    Object = None

    class PyData:
        pass

    @unittest.skipUnless(hasattr(sys, "getrefcount"), "no sys.getrefcount")
    def test_callback_ref_count_del(self):
        def callback(obj, value):
            return value // 2

        callback_ref = weakref.ref(callback)
        self.assertEqual(
            sys.getrefcount(callback), 2 if sys.version_info < (3, 14) else 1
        )

        obj = self.Object()
        obj.connect("sig-with-int64-prop", callback)
        self.assertEqual(
            sys.getrefcount(callback), 3 if sys.version_info < (3, 14) else 2
        )

        del callback
        self.assertEqual(sys.getrefcount(callback_ref()), 2)

        res = obj.emit("sig-with-int64-prop", 42)
        self.assertEqual(res, 21)
        self.assertEqual(
            sys.getrefcount(callback_ref), 2 if sys.version_info < (3, 14) else 1
        )

        del obj
        self.assertIsNone(callback_ref())

    @unittest.skipUnless(hasattr(sys, "getrefcount"), "no sys.getrefcount")
    def test_callback_ref_count_disconnect(self):
        def callback(obj, value):
            return value // 2

        callback_ref = weakref.ref(callback)
        self.assertEqual(
            sys.getrefcount(callback), 2 if sys.version_info < (3, 14) else 1
        )

        obj = self.Object()
        handler_id = obj.connect("sig-with-int64-prop", callback)
        self.assertEqual(
            sys.getrefcount(callback), 3 if sys.version_info < (3, 14) else 2
        )

        del callback
        self.assertEqual(sys.getrefcount(callback_ref()), 2)

        res = obj.emit("sig-with-int64-prop", 42)
        self.assertEqual(res, 21)
        self.assertEqual(
            sys.getrefcount(callback_ref), 2 if sys.version_info < (3, 14) else 1
        )

        obj.disconnect(handler_id)
        self.assertIsNone(callback_ref())

    @unittest.skipUnless(hasattr(sys, "getrefcount"), "no sys.getrefcount")
    def test_callback_ref_count_disconnect_by_func(self):
        def callback(obj, value):
            return value // 2

        callback_ref = weakref.ref(callback)
        self.assertEqual(
            sys.getrefcount(callback), 2 if sys.version_info < (3, 14) else 1
        )

        obj = self.Object()
        obj.connect("sig-with-int64-prop", callback)
        self.assertEqual(
            sys.getrefcount(callback), 3 if sys.version_info < (3, 14) else 2
        )

        del callback
        self.assertEqual(sys.getrefcount(callback_ref()), 2)

        res = obj.emit("sig-with-int64-prop", 42)
        self.assertEqual(res, 21)
        self.assertEqual(
            sys.getrefcount(callback_ref), 2 if sys.version_info < (3, 14) else 1
        )

        obj.disconnect_by_func(callback_ref())
        self.assertIsNone(callback_ref())

    @unittest.skipUnless(hasattr(sys, "getrefcount"), "no sys.getrefcount")
    def test_user_data_ref_count(self):
        def callback(obj, value, data):
            return value // 2

        data = self.PyData()
        data_ref = weakref.ref(data)
        self.assertEqual(sys.getrefcount(data), 2 if sys.version_info < (3, 14) else 1)

        obj = self.Object()
        obj.connect("sig-with-int64-prop", callback, data)
        self.assertEqual(sys.getrefcount(data), 3 if sys.version_info < (3, 14) else 2)

        del data
        self.assertEqual(sys.getrefcount(data_ref()), 2)

        res = obj.emit("sig-with-int64-prop", 42)
        self.assertEqual(res, 21)
        self.assertEqual(sys.getrefcount(data_ref()), 2)

        del obj
        self.assertIsNone(data_ref())

    @unittest.skipUnless(hasattr(sys, "getrefcount"), "no sys.getrefcount")
    @unittest.expectedFailure  # https://bugzilla.gnome.org/show_bug.cgi?id=688064
    def test_object_ref_count(self):
        # connect_object() should only weakly reference the object passed in
        # and auto-disconnect the signal when the object is destroyed.
        def callback(data, value):
            return value // 2

        data = GObject.Object()
        data_ref = weakref.ref(data)
        self.assertEqual(sys.getrefcount(data), 2)

        obj = self.Object()
        handler_id = obj.connect_object("sig-with-int64-prop", callback, data)
        self.assertEqual(sys.getrefcount(data), 2)

        res = obj.emit("sig-with-int64-prop", 42)
        self.assertEqual(res, 21)
        self.assertEqual(sys.getrefcount(data), 2)

        del data

        self.assertIsNone(data_ref())
        self.assertFalse(obj.handler_is_connected(handler_id))


class TestRefCountsNonIntrospected(unittest.TestCase, _RefCountTestBase):
    class Object(GObject.Object):
        sig_with_int64_prop = GObject.Signal(
            return_type=GObject.TYPE_INT64,
            arg_types=[GObject.TYPE_INT64],
            flags=GObject.SignalFlags.RUN_LAST,
        )


class TestRefCountsIntrospected(unittest.TestCase, _RefCountTestBase):
    Object = Regress.TestObj


class TestClosureRefCycle(unittest.TestCase):
    def test_closure_ref_cycle_unreachable(self):
        # https://bugzilla.gnome.org/show_bug.cgi?id=731501

        called = []

        def on_add(store, *args):
            called.append(store)

        store = Gio.ListStore()
        store.connect_object("items-changed", on_add, store)

        # Remove all Python references to the object and keep it alive
        # on the C level.
        x = Gio.FileInfo()
        x.set_attribute_object("store", store)
        del store
        gc.collect()

        # get it back and trigger the signal
        x.get_attribute_object("store").append(Gio.FileInfo())

        self.assertEqual(len(called), 1)
        self.assertTrue(called[0].__grefcount__ > 0)

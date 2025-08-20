import sys
import gc
import unittest
import warnings

from gi.repository import GLib
from gi import PyGIDeprecationWarning

from .helper import capture_glib_warnings


class Idle(GLib.Idle):
    def __init__(self, loop):
        GLib.Idle.__init__(self)
        self.count = 0
        self.set_callback(self.callback, loop)

    def callback(self, loop):
        self.count += 1
        return True


class MySource(GLib.Source):
    def __init__(self):
        GLib.Source.__init__(self)

    def prepare(self):
        return True, 0

    def check(self):
        return True

    def dispatch(self, callback, args):
        return callback(*args)


class TestSource(unittest.TestCase):
    def timeout_callback(self, loop):
        loop.quit()

    def my_callback(self, loop):
        self.pos += 1
        return True

    def setup_timeout(self, loop):
        timeout = GLib.Timeout(500)
        timeout.set_callback(self.timeout_callback, loop)
        timeout.attach()

    def test_sources(self):
        loop = GLib.MainLoop()

        self.setup_timeout(loop)

        idle = Idle(loop)
        self.assertEqual(idle.get_context(), None)
        idle.attach()
        self.assertEqual(idle.get_context(), GLib.main_context_default())

        self.pos = 0

        m = MySource()
        self.assertEqual(m.get_context(), None)
        m.set_callback(self.my_callback, loop)
        m.attach()
        self.assertEqual(m.get_context(), GLib.main_context_default())

        loop.run()

        m.destroy()
        idle.destroy()

        self.assertGreater(self.pos, 0)
        self.assertGreaterEqual(idle.count, 0)
        self.assertTrue(m.is_destroyed())
        self.assertTrue(idle.is_destroyed())

    def test_source_prepare(self):
        # this test may not terminate if prepare() is wrapped incorrectly
        dispatched = [False]
        loop = GLib.MainLoop()

        class CustomTimeout(GLib.Source):
            def prepare(self):
                return (False, 10)

            def check(self):
                return True

            def dispatch(self, callback, args):
                dispatched[0] = True

                loop.quit()

                return False

        source = CustomTimeout()

        source.attach()
        source.set_callback(dir)

        loop.run()

        assert dispatched[0]

    def test_is_destroyed_simple(self):
        s = GLib.Source()

        self.assertFalse(s.is_destroyed())
        s.destroy()
        self.assertTrue(s.is_destroyed())

        c = GLib.MainContext()
        s = GLib.Source()
        s.attach(c)
        self.assertFalse(s.is_destroyed())
        s.destroy()
        self.assertTrue(s.is_destroyed())

    def test_is_destroyed_context(self):
        def f():
            c = GLib.MainContext()
            s = GLib.Source()
            s.attach(c)
            return s

        s = f()
        gc.collect()
        gc.collect()
        self.assertTrue(s.is_destroyed())

    def test_remove(self):
        s = GLib.idle_add(dir)
        self.assertEqual(GLib.source_remove(s), True)

        # Removing sources not found cause critical
        with capture_glib_warnings(allow_criticals=True):
            # s is now removed, should fail now
            self.assertEqual(GLib.source_remove(s), False)

            # accepts large source IDs (they are unsigned)
            self.assertEqual(GLib.source_remove(GLib.MAXINT32), False)
            self.assertEqual(GLib.source_remove(GLib.MAXINT32 + 1), False)
            self.assertEqual(GLib.source_remove(GLib.MAXUINT32), False)

    def test_recurse_property(self):
        s = GLib.Idle()
        self.assertTrue(s.can_recurse in [False, True])
        s.can_recurse = False
        self.assertFalse(s.can_recurse)

    def test_priority(self):
        s = GLib.Idle()
        self.assertEqual(s.priority, GLib.PRIORITY_DEFAULT_IDLE)
        s.priority = GLib.PRIORITY_HIGH
        self.assertEqual(s.priority, GLib.PRIORITY_HIGH)

        s = GLib.Idle(GLib.PRIORITY_LOW)
        self.assertEqual(s.priority, GLib.PRIORITY_LOW)

        s = GLib.Timeout(1, GLib.PRIORITY_LOW)
        self.assertEqual(s.priority, GLib.PRIORITY_LOW)

        s = GLib.Source()
        self.assertEqual(s.priority, GLib.PRIORITY_DEFAULT)

    def test_get_current_time(self):
        # Note, deprecated API
        s = GLib.Idle()
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            time = s.get_current_time()
            self.assertTrue(issubclass(w[0].category, PyGIDeprecationWarning))

        self.assertTrue(isinstance(time, float))
        # plausibility check, and check magnitude of result
        self.assertGreater(time, 1300000000.0)
        self.assertLess(time, 2000000000.0)

    def test_add_remove_poll(self):
        # FIXME: very shallow test, only verifies the API signature
        pollfd = GLib.PollFD(99, GLib.IOCondition.IN | GLib.IOCondition.HUP)
        self.assertEqual(pollfd.fd, 99)
        source = GLib.Source()
        source.add_poll(pollfd)
        source.remove_poll(pollfd)

    def test_out_of_scope_before_dispatch(self):
        # https://bugzilla.gnome.org/show_bug.cgi?id=504337
        GLib.Timeout(20)
        GLib.Idle()

    @unittest.skipIf(sys.platform == "darwin", "hangs")
    def test_finalize(self):
        self.dispatched = False
        self.finalized = False

        class S(GLib.Source):
            def prepare(s):
                return (True, 1)

            def dispatch(s, callback, args):
                self.dispatched = True
                return False

            def finalize(s):
                self.finalized = True

        source = S()
        source.attach()
        self.assertFalse(self.finalized)
        self.assertFalse(source.is_destroyed())

        while source.get_context().iteration(False):
            pass

        source.destroy()
        self.assertTrue(self.dispatched)
        self.assertFalse(self.finalized)
        self.assertTrue(source.is_destroyed())
        del source
        gc.collect()
        gc.collect()
        self.assertTrue(self.finalized)

    def test_python_unref_with_active_source(self):
        # Tests a Python derived Source which is free'd in the context of
        # Python, but which was attached to a MainContext (via source.attach())
        self.dispatched = False
        self.finalized = False

        class S(GLib.Source):
            def prepare(s):
                return (True, 1)

            def check(s):
                pass

            def dispatch(s, callback, args):
                self.dispatched = True
                return False

            def finalize(s):
                self.finalized = True

        context = GLib.MainContext.new()
        source = S()
        id_ = source.attach(context)
        self.assertFalse(self.finalized)
        self.assertFalse(source.is_destroyed())

        # Delete the source from Python, it should detach
        del source
        gc.collect()
        gc.collect()

        while context.iteration(may_block=False):
            pass

        assert self.finalized
        assert not self.dispatched
        assert context.find_source_by_id(id_) is None

    @unittest.skipIf(
        sys.implementation.name == "pypy", "PyPy doesn't __del__ immediately"
    )
    def test_python_unref_during_dispatch(self):
        # Tests a Python derived Source which is free'd in the context of
        # Python, while being dispatched
        # NOTE: Finalize is explicitly called by __del__ for consistency
        self.dispatched = False
        self.finalized = False

        class S(GLib.Source):
            def __init__(s, func=None):
                s.func = func

            def prepare(s):
                return (True, 1)

            def check(s):
                pass

            def dispatch(s, callback, args):
                self.dispatched = True
                self.source = None
                return False

            def finalize(s):
                self.finalized = True

        context = GLib.MainContext.new()
        self.source = S()
        id_ = self.source.attach(context)

        while context.iteration(may_block=False):
            pass

        assert self.source is None
        assert context.find_source_by_id(id_) is None
        assert self.finalized
        assert self.dispatched

    def test_extra_init_args(self):
        class SourceWithInitArgs(GLib.Source):
            def __init__(self, arg, kwarg=None):
                super().__init__()
                self.arg = arg
                self.kwarg = kwarg

        source = SourceWithInitArgs(1, kwarg=2)
        self.assertEqual(source.arg, 1)
        self.assertEqual(source.kwarg, 2)


@unittest.skipIf(sys.platform == "darwin", "hangs")
class TestUserData(unittest.TestCase):
    def test_idle_no_data(self):
        ml = GLib.MainLoop()

        def cb():
            ml.quit()

        id = GLib.idle_add(cb)
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_DEFAULT_IDLE
        )
        ml.run()

    def test_timeout_no_data(self):
        ml = GLib.MainLoop()

        def cb():
            ml.quit()

        id = GLib.timeout_add(1, cb)
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_DEFAULT
        )
        ml.run()

    def test_idle_data(self):
        ml = GLib.MainLoop()

        def cb(data):
            data["called"] = True
            ml.quit()

        data = {}
        id = GLib.idle_add(cb, data)
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_DEFAULT_IDLE
        )
        ml.run()
        self.assertTrue(data["called"])

    def test_idle_multidata(self):
        ml = GLib.MainLoop()

        def cb(data, data2):
            data["called"] = True
            data["data2"] = data2
            ml.quit()

        data = {}
        id = GLib.idle_add(cb, data, "hello")
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_DEFAULT_IDLE
        )
        ml.run()
        self.assertTrue(data["called"])
        self.assertEqual(data["data2"], "hello")

    def test_timeout_data(self):
        ml = GLib.MainLoop()

        def cb(data):
            data["called"] = True
            ml.quit()

        data = {}
        id = GLib.timeout_add(1, cb, data)
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_DEFAULT
        )
        ml.run()
        self.assertTrue(data["called"])

    def test_timeout_multidata(self):
        ml = GLib.MainLoop()

        def cb(data, data2):
            data["called"] = True
            data["data2"] = data2
            ml.quit()

        data = {}
        id = GLib.timeout_add(1, cb, data, "hello")
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_DEFAULT
        )
        ml.run()
        self.assertTrue(data["called"])
        self.assertEqual(data["data2"], "hello")

    def test_idle_no_data_priority(self):
        ml = GLib.MainLoop()

        def cb():
            ml.quit()

        id = GLib.idle_add(cb, priority=GLib.PRIORITY_HIGH)
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )
        ml.run()

    def test_timeout_no_data_priority(self):
        ml = GLib.MainLoop()

        def cb():
            ml.quit()

        id = GLib.timeout_add(1, cb, priority=GLib.PRIORITY_HIGH)
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )
        ml.run()

    def test_idle_data_priority(self):
        ml = GLib.MainLoop()

        def cb(data):
            data["called"] = True
            ml.quit()

        data = {}
        id = GLib.idle_add(cb, data, priority=GLib.PRIORITY_HIGH)
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )
        ml.run()
        self.assertTrue(data["called"])

    def test_timeout_data_priority(self):
        ml = GLib.MainLoop()

        def cb(data):
            data["called"] = True
            ml.quit()

        data = {}
        id = GLib.timeout_add(1, cb, data, priority=GLib.PRIORITY_HIGH)
        self.assertEqual(
            ml.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )
        ml.run()
        self.assertTrue(data["called"])

    def cb_no_data(self):
        self.loop.quit()

    def test_idle_method_callback_no_data(self):
        self.loop = GLib.MainLoop()
        GLib.idle_add(self.cb_no_data)
        self.loop.run()

    def cb_with_data(self, data):
        data["called"] = True
        self.loop.quit()

    def test_idle_method_callback_with_data(self):
        self.loop = GLib.MainLoop()
        data = {}
        GLib.idle_add(self.cb_with_data, data)
        self.loop.run()
        self.assertTrue(data["called"])

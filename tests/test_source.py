# -*- Mode: Python -*-

import unittest
import warnings

from gi.repository import GLib, GObject


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
        self.assertTrue(s.is_destroyed())

    def test_remove(self):
        s = GLib.idle_add(dir)
        self.assertEqual(GLib.source_remove(s), True)
        # s is now removed, should fail now
        self.assertEqual(GLib.source_remove(s), False)

        # accepts large source IDs (they are unsigned)
        self.assertEqual(GLib.source_remove(GObject.G_MAXINT32), False)
        self.assertEqual(GLib.source_remove(GObject.G_MAXINT32 + 1), False)
        self.assertEqual(GLib.source_remove(GObject.G_MAXUINT32), False)

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
            warnings.simplefilter('always')
            time = s.get_current_time()
            self.assertTrue(issubclass(w[0].category, DeprecationWarning))

        self.assertTrue(isinstance(time, float))
        # plausibility check, and check magnitude of result
        self.assertGreater(time, 1300000000.0)
        self.assertLess(time, 2000000000.0)

    def test_add_remove_poll(self):
        # FIXME: very shallow test, only verifies the API signature
        pollfd = GLib.PollFD(99, GLib.IO_IN | GLib.IO_HUP)
        self.assertEqual(pollfd.fd, 99)
        source = GLib.Source()
        source.add_poll(pollfd)
        source.remove_poll(pollfd)

    def test_out_of_scope_before_dispatch(self):
        # https://bugzilla.gnome.org/show_bug.cgi?id=504337
        GLib.Timeout(20)
        GLib.Idle()


class TestUserData(unittest.TestCase):
    def test_idle_no_data(self):
        ml = GLib.MainLoop()
        def cb():
            ml.quit()
        GLib.idle_add(cb)
        ml.run()

    def test_timeout_no_data(self):
        ml = GLib.MainLoop()
        def cb():
            ml.quit()
        GLib.timeout_add(50, cb)
        ml.run()

    def test_idle_data(self):
        ml = GLib.MainLoop()
        def cb(data):
            data['called'] = True
            ml.quit()
        data = {}
        GLib.idle_add(cb, data)
        ml.run()
        self.assertTrue(data['called'])

    def test_timeout_data(self):
        ml = GLib.MainLoop()
        def cb(data):
            data['called'] = True
            ml.quit()
        data = {}
        GLib.timeout_add(50, cb, data)
        ml.run()
        self.assertTrue(data['called'])

if __name__ == '__main__':
    unittest.main()

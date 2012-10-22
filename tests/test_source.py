# -*- Mode: Python -*-

import unittest

from gi.repository import GLib


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

    def testSources(self):
        loop = GLib.MainLoop()

        self.setup_timeout(loop)

        idle = Idle(loop)
        idle.attach()

        self.pos = 0

        m = MySource()
        m.set_callback(self.my_callback, loop)
        m.attach()

        loop.run()

        m.destroy()
        idle.destroy()

        self.assertGreater(self.pos, 0)
        self.assertGreaterEqual(idle.count, 0)
        self.assertTrue(m.is_destroyed())
        self.assertTrue(idle.is_destroyed())

    def testSourcePrepare(self):
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

    def testIsDestroyedSimple(self):
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

    def testIsDestroyedContext(self):
        def f():
            c = GLib.MainContext()
            s = GLib.Source()
            s.attach(c)
            return s

        s = f()
        self.assertTrue(s.is_destroyed())


class TestTimeout(unittest.TestCase):
    def test504337(self):
        GLib.Timeout(20)
        GLib.Idle()


if __name__ == '__main__':
    unittest.main()

#!/usr/bin/env python

import exceptions
import os
import sys
import select
import unittest

from common import gobject

class Idle(gobject.Idle):
    def __init__(self, loop):
        gobject.Idle.__init__(self)
        self.count = 0
        self.set_callback(self.callback, loop)

    def callback(self, loop):
        self.count += 1
        return True

class MySource(gobject.Source):
    def __init__(self):
        gobject.Source.__init__(self)

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
        timeout = gobject.Timeout(500)
        timeout.set_callback(self.timeout_callback, loop)
        timeout.attach()

    def testSources(self):
        loop = gobject.MainLoop()

        self.setup_timeout(loop)

        idle = Idle(loop)
        idle.attach()

        self.pos = 0

        m = MySource()
        m.set_callback(self.my_callback, loop)
        m.attach()

        loop.run()

        assert self.pos >= 0 and idle.count >= 0

    def testSourcePrepare(self):
        # this test may not terminate if prepare() is wrapped incorrectly
        dispatched = [False]
        loop = gobject.MainLoop()

        class CustomTimeout(gobject.Source):
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


class TestTimeout(unittest.TestCase):
     def test504337(self):
	timeout_source = gobject.Timeout(20)
	idle_source = gobject.Idle()


if __name__ == '__main__':
    unittest.main()

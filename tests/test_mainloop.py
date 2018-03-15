# -*- Mode: Python -*-

from __future__ import absolute_import

import os
import select
import signal
import unittest

from gi.repository import GLib

from .helper import capture_exceptions


class TestMainLoop(unittest.TestCase):

    @unittest.skipUnless(hasattr(os, "fork"), "no os.fork available")
    def test_exception_handling(self):
        pipe_r, pipe_w = os.pipe()

        pid = os.fork()
        if pid == 0:
            os.close(pipe_w)
            select.select([pipe_r], [], [])
            os.close(pipe_r)
            os._exit(1)

        def child_died(pid, status, loop):
            loop.quit()
            raise Exception("deadbabe")

        loop = GLib.MainLoop()
        GLib.child_watch_add(GLib.PRIORITY_DEFAULT, pid, child_died, loop)

        os.close(pipe_r)
        os.write(pipe_w, b"Y")
        os.close(pipe_w)

        with capture_exceptions() as exc:
            loop.run()

        assert len(exc) == 1
        assert exc[0].type is Exception
        assert exc[0].value.args[0] == "deadbabe"

    @unittest.skipUnless(hasattr(os, "fork"), "no os.fork available")
    @unittest.skipIf(os.environ.get("PYGI_TEST_GDB"), "SIGINT stops gdb")
    def test_sigint(self):
        r, w = os.pipe()
        pid = os.fork()
        if pid == 0:
            # wait for the parent process loop to start
            os.read(r, 1)
            os.close(r)

            os.kill(os.getppid(), signal.SIGINT)
            os._exit(0)

        def notify_child():
            # tell the child that it can kill the parent
            os.write(w, b"X")
            os.close(w)

        GLib.idle_add(notify_child)
        loop = GLib.MainLoop()
        try:
            loop.run()
            self.fail('expected KeyboardInterrupt exception')
        except KeyboardInterrupt:
            pass
        self.assertFalse(loop.is_running())
        os.waitpid(pid, 0)

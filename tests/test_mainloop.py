# -*- Mode: Python -*-

import os
import sys
import select
import signal
import thread
import time
import unittest

from gi.repository import GLib

from compathelper import _bytes


class TestMainLoop(unittest.TestCase):
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
        GLib.child_watch_add(pid, child_died, loop)

        os.close(pipe_r)
        os.write(pipe_w, _bytes("Y"))
        os.close(pipe_w)

        def excepthook(type, value, traceback):
            assert type is Exception
            assert value.args[0] == "deadbabe"
        sys.excepthook = excepthook

        got_exception = False
        try:
            loop.run()
        except:
            got_exception = True

        #
        # The exception should be handled (by printing it)
        # immediately on return from child_died() rather
        # than here. See bug #303573
        #
        sys.excepthook = sys.__excepthook__
        assert not got_exception

    def test_concurrency(self):
        def on_usr1(signum, frame):
            pass

        try:
            # create a thread which will terminate upon SIGUSR1 by way of
            # interrupting sleep()
            orig_handler = signal.signal(signal.SIGUSR1, on_usr1)
            thread.start_new_thread(time.sleep, (10,))

            # now create two main loops
            loop1 = GLib.MainLoop()
            loop2 = GLib.MainLoop()
            GLib.timeout_add(100, lambda: os.kill(os.getpid(), signal.SIGUSR1))
            GLib.timeout_add(500, loop1.quit)
            loop1.run()
            loop2.quit()
        finally:
            signal.signal(signal.SIGUSR1, orig_handler)

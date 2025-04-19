import sys
import unittest

try:
    if sys.platform != 'win32':
        from test.test_asyncio.test_events import UnixEventLoopTestsMixin as GLibEventLoopTestsMixin
    else:
        from test.test_asyncio.test_events import EventLoopTestsMixin

        # There is Mixin for the ProactorEventLoop, so copy the skips
        class GLibEventLoopTestsMixin(EventLoopTestsMixin):
            def test_reader_callback(self):
                raise unittest.SkipTest("IocpEventLoop does not have add_reader()")

            def test_reader_callback_cancel(self):
                raise unittest.SkipTest("IocpEventLoop does not have add_reader()")

            def test_writer_callback(self):
                raise unittest.SkipTest("IocpEventLoop does not have add_writer()")

            def test_writer_callback_cancel(self):
                raise unittest.SkipTest("IocpEventLoop does not have add_writer()")

            def test_remove_fds_after_closing(self):
                raise unittest.SkipTest("IocpEventLoop does not have add_reader()")

    from test.test_asyncio.test_subprocess import SubprocessMixin
    from test.test_asyncio.utils import TestCase
except:
    class GLibEventLoopTestsMixin():
        def test_unix_event_loop_tests_missing(self):
            import warnings
            warnings.warn('UnixEventLoopTestsMixin is unavailable, not running tests!')
            self.skipTest('UnixEventLoopTestsMixin is unavailable, not running tests!')

    class SubprocessMixin():
        def test_subprocess_mixin_tests_missing(self):
            import warnings
            warnings.warn('SubprocessMixin is unavailable, not running tests!')
            self.skipTest('SubprocessMixin is unavailable, not running tests!')

    from unittest import TestCase

import sys
import gi
import gi.events
import asyncio
import socket
import threading
from gi.repository import GLib

try:
    from gi.repository import Gtk
except ImportError:
    Gtk = None


GTK4 = (Gtk and Gtk._version == "4.0")


class GLibEventLoopTests(GLibEventLoopTestsMixin, TestCase):

    def __init__(self, *args):
        super().__init__(*args)
        self.loop = None

    def create_event_loop(self):
        return gi.events.GLibEventLoop(GLib.MainContext())


class SubprocessWatcherTests(SubprocessMixin, TestCase):

    def setUp(self):
        super().setUp()
        policy = gi.events.GLibEventLoopPolicy()
        asyncio.set_event_loop_policy(policy)
        self.loop = policy.get_event_loop()

    def tearDown(self):
        asyncio.set_event_loop_policy(None)
        self.loop.close()
        super().tearDown()

    # Fix broken test for Python 3.12
    @unittest.skipUnless(sys.version_info >= (3, 12), "test is added in Python 3.12")
    def test_subprocess_consistent_callbacks(self):
        # gh-108973: Test that all subprocess protocol methods are called.
        # The protocol methods are not called in a determistic order.
        # The order depends on the event loop and the operating system.
        events = []
        fds = [1, 2]
        expected = [
            ('pipe_data_received', 1, b'stdout'),
            ('pipe_data_received', 2, b'stderr'),
            ('pipe_connection_lost', 1),
            ('pipe_connection_lost', 2),
            'process_exited',
        ]
        per_fd_expected = [
            'pipe_data_received',
            'pipe_connection_lost',
        ]

        class MyProtocol(asyncio.SubprocessProtocol):
            def __init__(self, exit_future: asyncio.Future) -> None:
                self.exit_future = exit_future

            def pipe_data_received(self, fd, data) -> None:
                events.append(('pipe_data_received', fd, data))
                self.exit_maybe()

            def pipe_connection_lost(self, fd, exc) -> None:
                events.append(('pipe_connection_lost', fd))
                self.exit_maybe()

            def process_exited(self) -> None:
                events.append('process_exited')
                self.exit_maybe()

            def exit_maybe(self):
                # Only exit when we got all expected events
                if len(events) >= len(expected):
                    self.exit_future.set_result(True)

        async def main() -> None:
            loop = asyncio.get_running_loop()
            exit_future = asyncio.Future()
            code = 'import sys; sys.stdout.write("stdout"); sys.stderr.write("stderr")'
            transport, _ = await loop.subprocess_exec(lambda: MyProtocol(exit_future),
                                                      sys.executable, '-c', code, stdin=None)
            await exit_future
            transport.close()

            return events

        events = self.loop.run_until_complete(main())

        # First, make sure that we received all events
        self.assertSetEqual(set(events), set(expected))

        # Second, check order of pipe events per file descriptor
        per_fd_events = {fd: [] for fd in fds}
        for event in events:
            if event == 'process_exited':
                continue
            name, fd = event[:2]
            per_fd_events[fd].append(name)

        for fd in fds:
            self.assertEqual(per_fd_events[fd], per_fd_expected, (fd, events))


class GLibEventLoopPolicyTests(unittest.TestCase):

    def create_policy(self):
        return gi.events.GLibEventLoopPolicy()

    def test_get_event_loop(self):
        policy = self.create_policy()
        loop = policy.get_event_loop()
        self.assertIsInstance(loop, gi.events.GLibEventLoop)
        self.assertIs(loop, policy.get_event_loop())
        loop.close()

    def test_new_event_loop(self):
        policy = self.create_policy()
        loop = policy.new_event_loop()
        self.assertIsInstance(loop, gi.events.GLibEventLoop)
        loop.close()

        # Attaching a loop to the main thread fails
        with self.assertRaises(RuntimeError):
            policy.set_event_loop(loop)

    def test_nested_context_iteration(self):
        policy = self.create_policy()
        loop = policy.new_event_loop()

        called = False

        def cb():
            nonlocal called
            called = True

        async def run():
            nonlocal loop, called

            loop.call_soon(cb)
            self.assertEqual(called, False)

            # Iterating the main context does not cause cb to be called
            while loop._context.iteration(False):
                pass
            self.assertEqual(called, False)

            # Awaiting on anything *does* cause the cb to fire
            await asyncio.sleep(0)
            self.assertEqual(called, True)

        loop.run_until_complete(run())
        loop.close()

    def test_thread_event_loop(self):
        policy = self.create_policy()
        loop = policy.new_event_loop()

        res = []

        def thread_func(res):
            try:
                # We cannot get an event loop for the current thread
                with self.assertRaises(RuntimeError):
                    policy.get_event_loop()

                # We can attach our loop
                policy.set_event_loop(loop)
                # Now we can get it, and it is the same
                self.assertIs(policy.get_event_loop(), loop)

                # Simple call_soon test
                results = []

                def callback(arg1, arg2):
                    results.append((arg1, arg2))
                    loop.stop()

                loop.call_soon(callback, 'hello', 'world')
                loop.run_forever()
                self.assertEqual(results, [('hello', 'world')])

                # We can detach it again
                policy.set_event_loop(None)

                # Which means we have none and get a runtime error
                with self.assertRaises(RuntimeError):
                    policy.get_event_loop()
            except:
                res += sys.exc_info()

        # Initially, the thread has no event loop
        thread = threading.Thread(target=lambda: thread_func(res))
        thread.start()
        thread.join()

        if res:
            t, v, tb = res
            raise t(v).with_traceback(tb)

        loop.close()

    def test_outside_context_iteration(self):
        """Iterating the main context from the outside, does not cause the
        EventLoop to dispatch."""
        policy = self.create_policy()
        loop = policy.new_event_loop()

        called = False

        def cb():
            nonlocal called
            called = True

        loop.call_soon(cb)
        while loop._context.iteration(False):
            pass
        loop.close()
        self.assertEqual(called, False)

    def test_inside_context_iteration(self):
        """Iterating the main context from the inside, does not cause the
        EventLoop to dispatch."""
        policy = self.create_policy()
        loop = policy.get_event_loop()

        done = asyncio.Future(loop=loop)

        called = False

        def cb():
            nonlocal called
            called = True

        def ctx_iterate():
            nonlocal called

            loop.call_soon(cb)
            while loop._context.iteration(False):
                pass
            self.assertEqual(called, False)

            # If we by-pass the override, then the callback is called
            while super(GLib.MainContext, loop._context).iteration(False):
                pass
            self.assertEqual(called, True)

            # It'll also be called (again) before run_until_complete finishes
            called = False
            loop.call_soon(cb)

            done.set_result(True)

            return GLib.SOURCE_REMOVE

        GLib.idle_add(ctx_iterate)
        loop.run_until_complete(done)
        loop.close()
        self.assertEqual(called, True)

    @unittest.skipUnless(Gtk, 'no Gtk')
    def test_recursive_stop(self):
        """Calling stop() on the EventLoop will quit it, even if iteration
        is done recursively."""
        policy = self.create_policy()
        asyncio.set_event_loop_policy(policy)
        self.addCleanup(asyncio.set_event_loop_policy, None)
        loop = policy.get_event_loop()

        if not GTK4:
            def main_gtk():
                GLib.idle_add(loop.stop)
                Gtk.main()

            GLib.idle_add(main_gtk)
            Gtk.main()

        def main_glib():
            GLib.idle_add(loop.stop)
            GLib.MainLoop().run()

        GLib.idle_add(main_glib)
        GLib.MainLoop().run()

        loop.close()

    @unittest.skipIf(sys.platform == 'win32', 'add reader/writer not implemented')
    def test_source_fileobj_fd(self):
        """Regression test for
        https://gitlab.gnome.org/GNOME/pygobject/-/issues/689
        """
        class Echo:
            def __init__(self, sock, expect_bytes):
                self.sock = sock
                self.sent_bytes = 0
                self.expect_bytes = expect_bytes
                self.done = asyncio.Future()
                self.data = bytes()

            def send(self):
                if self.done.done():
                    return
                if self.sent_bytes < len(self.data):
                    self.sent_bytes += self.sock.send(
                        self.data[self.sent_bytes:])
                    print('sent', self.data)
                if self.sent_bytes >= self.expect_bytes:
                    self.done.set_result(None)
                    self.sock.shutdown(socket.SHUT_WR)

            def recv(self):
                if self.done.done():
                    return
                self.data += self.sock.recv(self.expect_bytes)
                print('received', self.data)
                if len(self.data) >= self.expect_bytes:
                    self.sock.shutdown(socket.SHUT_RD)

        async def run():
            loop = asyncio.get_running_loop()
            s1, s2 = socket.socketpair()
            sample = b'Hello!'
            e = Echo(s1, len(sample))
            # register using file object and file descriptor
            loop.add_reader(s1, e.recv)
            loop.add_writer(s1.fileno(), e.send)
            s2.sendall(sample)
            await asyncio.wait_for(e.done, timeout=2.0)
            echo = bytes()
            for _ in range(len(sample)):
                echo += s2.recv(len(sample))
                if len(echo) == len(sample):
                    break
            # remove using file object and file descriptor
            loop.remove_reader(s1)
            loop.remove_writer(s1.fileno())
            s1.close()
            s2.close()
            # check if the data was echoed correctly
            self.assertEqual(sample, echo)

        policy = self.create_policy()
        loop = policy.get_event_loop()
        loop.run_until_complete(run())
        loop.close()

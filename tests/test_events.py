import unittest

try:
    from test.test_asyncio.test_events import UnixEventLoopTestsMixin
    from test.test_asyncio.test_subprocess import SubprocessMixin
    from test.test_asyncio.utils import TestCase
except:
    class UnixEventLoopTestsMixin():
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
import threading
from gi.repository import GLib


# None of this currently works on Windows
if sys.platform != 'win32':

    class GLibEventLoopTests(UnixEventLoopTestsMixin, TestCase):

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

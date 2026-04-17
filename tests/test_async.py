import sys
import pytest
import platform
import unittest

import asyncio
from gi.repository import GLib, Gio
from gi.events import GLibEventLoopPolicy


class TestAsync(unittest.TestCase):
    def setUp(self):
        policy = GLibEventLoopPolicy()
        asyncio.set_event_loop_policy(policy)
        self.addCleanup(asyncio.set_event_loop_policy, None)
        self.loop = policy.get_event_loop()
        self.addCleanup(self.loop.close)

    def test_async_enumerate(self):
        f = Gio.file_new_for_path("./")

        called = False

        def cb():
            nonlocal called
            called = True

        async def run():
            nonlocal called, self

            self.loop.call_soon(cb)

            iter_info = [
                info.get_name()
                for info in await f.enumerate_children_async(
                    "standard::*", 0, GLib.PRIORITY_DEFAULT
                )
            ]

            # The await runs the mainloop and cb is called.
            self.assertEqual(called, True)

            next_info = []
            enumerator = f.enumerate_children("standard::*", 0, None)
            while True:
                info = enumerator.next_file(None)
                if info is None:
                    break
                next_info.append(info.get_name())

            self.assertEqual(iter_info, next_info)

        self.loop.run_until_complete(run())

    def test_async_cancellation(self):
        """Cancellation raises G_IO_ERROR_CANCELLED."""
        f = Gio.file_new_for_path("./")

        async def run():
            nonlocal self

            # cancellable created implicitly
            res = f.enumerate_children_async("standard::*", 0, GLib.PRIORITY_DEFAULT)
            res.cancel()
            with self.assertRaisesRegex(GLib.GError, "Operation was cancelled"):
                await res

            # cancellable passed explicitly
            cancel = Gio.Cancellable()
            res = f.enumerate_children_async(
                "standard::*", 0, GLib.PRIORITY_DEFAULT, cancel
            )
            self.assertEqual(res.cancellable, cancel)
            cancel.cancel()
            with self.assertRaisesRegex(GLib.GError, "Operation was cancelled"):
                await res

        self.loop.run_until_complete(run())

    def test_cancel_async_task(self):
        # Regression test for https://gitlab.gnome.org/GNOME/pygobject/-/issues/755
        async def _long_task(proc):
            await proc.wait_async()

        async def run():
            nonlocal self

            # Create a process which waits for input on stdin, without ever
            # actually providing any such input.  This process won't exit unless
            # we explicitly kill it.
            flags = (
                Gio.SubprocessFlags.STDOUT_SILENCE
                | Gio.SubprocessFlags.STDERR_SILENCE
                | Gio.SubprocessFlags.STDIN_PIPE
            )
            proc = Gio.Subprocess.new([sys.executable], flags)

            try:
                # Create a task which waits for said process to exit; but since
                # the process won't actually do so we've created a task which
                # indefinitely awaits on a Gio cancellable.
                res = asyncio.create_task(_long_task(proc))
                # Briefly yield control to make sure the task we just created
                # starts running on the loop and polls on a Gio task.
                await asyncio.sleep(0.1)
                # Now cancel the task while it's awaiting on a Gio task; this
                # will now hit the async_cancel implementation on the C side.
                #
                # Also, explicitly assert that the task was really cancelled
                # (.cancel() returns True), and hasn't already finished by this
                # time (.cancel() returns False).  This makes sure that we're
                # really testing the cancellation here.
                assert res.cancel(), "Test invalid, task already finished"
            finally:
                proc.force_exit()

        self.loop.run_until_complete(run())

    def test_async_task_cancellation(self):
        """Cancellation works on wrapping task."""
        f = Gio.file_new_for_path("./")

        async def run():
            nonlocal self, task

            # Queue cancellation of ourselves with default idle priority
            GLib.idle_add(lambda: task.cancel("custom cancellation message") and False)

            cancel = Gio.Cancellable()

            try:
                # Start low priority task which will be cancelled (lower than default idle)
                async_call = f.enumerate_children_async(
                    "standard::*", 0, GLib.PRIORITY_LOW, cancel
                )
                await async_call

                self.fail("await did not raise!")
            except asyncio.CancelledError:
                self.assertTrue(cancel.is_cancelled())

                gio_exception = async_call.exception()
                self.assertIsInstance(gio_exception, GLib.GError)
                self.assertTrue(
                    gio_exception.matches(
                        Gio.io_error_quark(), Gio.IOErrorEnum.CANCELLED
                    )
                )

                # re-raise cancellation
                raise

        task = self.loop.create_task(run())
        with self.assertRaisesRegex(
            asyncio.CancelledError, "custom cancellation message"
        ):
            self.loop.run_until_complete(task)

    def test_not_completed(self):
        """Querying an uncompleted task raises exceptions."""
        f = Gio.file_new_for_path("./")

        async def run():
            nonlocal self

            # cancellable created implicitly
            res = f.enumerate_children_async("standard::*", 0, GLib.PRIORITY_DEFAULT)
            with self.assertRaises(asyncio.InvalidStateError):
                res.result()

            with self.assertRaises(asyncio.InvalidStateError):
                res.exception()

            # And, await it
            await res

        self.loop.run_until_complete(run())

    def test_async_cancel_completed(self):
        """Cancelling a completed task just cancels the cancellable."""
        f = Gio.file_new_for_path("./")

        async def run():
            nonlocal self

            res = f.enumerate_children_async("standard::*", 0, GLib.PRIORITY_DEFAULT)
            await res
            assert not res.cancellable.is_cancelled()
            res.cancel()
            assert res.cancellable.is_cancelled()

        self.loop.run_until_complete(run())

    def test_async_completed_add_cb(self):
        """Adding a done cb to a completed future queues it with call_soon."""
        f = Gio.file_new_for_path("./")

        called = False

        def cb():
            nonlocal called
            called = True

        async def run():
            nonlocal called, self

            res = f.enumerate_children_async("standard::*", 0, GLib.PRIORITY_DEFAULT)
            await res
            self.loop.call_soon(cb)

            # Python await is smart and does not iterate the EventLoop
            await res
            assert not called

            # So create a new future and wait on that
            fut = asyncio.Future()

            def done_cb(res):
                nonlocal fut
                fut.set_result(res.result())

            res.add_done_callback(done_cb)
            await fut
            assert called

        self.loop.run_until_complete(run())

    @pytest.mark.xfail(
        platform.python_implementation() == "PyPy",
        reason="Exception reporting does not work in pypy",
    )
    def test_deleting_failed_logs(self):
        f = Gio.file_new_for_path("./")

        async def run():
            nonlocal self

            res = f.enumerate_children_async("standard::*", 0, GLib.PRIORITY_DEFAULT)
            res.cancel()
            # Cancellation in Gio is not immediate, so sleep for a bit
            await asyncio.sleep(0.5)

        exc = None
        msg = None

        def handler(loop, context):
            nonlocal exc, msg
            msg = context["message"]
            exc = context["exception"]

        self.loop.set_exception_handler(handler)
        self.loop.run_until_complete(run())

        self.assertRegex(msg, ".*exception was never retrieved")
        self.assertIsInstance(exc, GLib.GError)
        assert exc.matches(Gio.io_error_quark(), Gio.IOErrorEnum.CANCELLED)

    def test_no_running_loop(self):
        f = Gio.file_new_for_path("./")

        res = f.enumerate_children_async("standard::*", 0, GLib.PRIORITY_DEFAULT)
        self.assertIsNone(res)

    def test_wrong_default_context(self):
        f = Gio.file_new_for_path("./")

        async def run():  # noqa: RUF029
            nonlocal self

            ctx = GLib.MainContext.new()
            GLib.MainContext.push_thread_default(ctx)
            try:
                res = f.enumerate_children_async(
                    "standard::*", 0, GLib.PRIORITY_DEFAULT
                )
                self.assertIsNone(res)
            finally:
                GLib.MainContext.pop_thread_default(ctx)

        self.loop.run_until_complete(run())

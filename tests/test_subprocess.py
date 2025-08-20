import sys
import os
import unittest
import warnings

import pytest

from gi.repository import GLib
from gi import PyGIDeprecationWarning

from .helper import capture_gi_deprecation_warnings


def test_child_watch_add_get_args_various():
    def cb(pid, status):
        return None

    get_args = GLib._child_watch_add_get_args
    pid = 42
    with capture_gi_deprecation_warnings():
        assert get_args(pid, cb, 2) == (0, pid, cb, (2,))

        with pytest.raises(TypeError):
            get_args(pid, cb, 2, 3, 4)

        assert get_args(0, pid, 2, 3, function=cb) == (0, pid, cb, (2, 3))
        assert get_args(0, pid, cb, 2, 3) == (0, pid, cb, (2, 3))
        assert get_args(0, pid, cb, data=99) == (0, pid, cb, (99,))

        with pytest.raises(TypeError):
            get_args(0, pid, 24)

        with pytest.raises(TypeError):
            get_args(0, pid, cb, 2, 3, data=99)


@unittest.skipIf(os.name == "nt", "not on Windows")
class TestProcess(unittest.TestCase):
    def test_deprecated_child_watch_no_data(self):
        def cb(pid, status):
            return None

        pid = object()
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            res = GLib._child_watch_add_get_args(pid, cb)
            self.assertTrue(issubclass(w[0].category, PyGIDeprecationWarning))

        self.assertEqual(len(res), 4)
        self.assertEqual(res[0], GLib.PRIORITY_DEFAULT)
        self.assertEqual(res[1], pid)
        self.assertTrue(callable(cb))
        self.assertSequenceEqual(res[3], [])

    def test_deprecated_child_watch_data_priority(self):
        def cb(pid, status):
            return None

        pid = object()
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            res = GLib._child_watch_add_get_args(pid, cb, 12345, GLib.PRIORITY_HIGH)
            self.assertTrue(issubclass(w[0].category, PyGIDeprecationWarning))

        self.assertEqual(len(res), 4)
        self.assertEqual(res[0], GLib.PRIORITY_HIGH)
        self.assertEqual(res[1], pid)
        self.assertEqual(res[2], cb)
        self.assertSequenceEqual(res[3], [12345])

    def test_deprecated_child_watch_data_priority_kwargs(self):
        def cb(pid, status):
            return None

        pid = object()
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            res = GLib._child_watch_add_get_args(
                pid, cb, priority=GLib.PRIORITY_HIGH, data=12345
            )
            self.assertTrue(issubclass(w[0].category, PyGIDeprecationWarning))

        self.assertEqual(len(res), 4)
        self.assertEqual(res[0], GLib.PRIORITY_HIGH)
        self.assertEqual(res[1], pid)
        self.assertEqual(res[2], cb)
        self.assertSequenceEqual(res[3], [12345])

    @unittest.expectedFailure  # using keyword args is fully supported by PyGObject machinery
    def test_child_watch_all_kwargs(self):
        def cb(pid, status):
            return None

        pid = object()

        res = GLib._child_watch_add_get_args(
            priority=GLib.PRIORITY_HIGH, pid=pid, function=cb, data=12345
        )
        self.assertEqual(len(res), 4)
        self.assertEqual(res[0], GLib.PRIORITY_HIGH)
        self.assertEqual(res[1], pid)
        self.assertEqual(res[2], cb)
        self.assertSequenceEqual(res[3], [12345])

    def test_child_watch_no_data(self):
        def cb(pid, status):
            self.status = status
            self.loop.quit()

        self.status = None
        self.loop = GLib.MainLoop()
        argv = [sys.executable, "-c", "import sys"]
        pid, _stdin, _stdout, _stderr = GLib.spawn_async(
            argv, flags=GLib.SpawnFlags.DO_NOT_REAP_CHILD
        )
        pid.close()
        id = GLib.child_watch_add(GLib.PRIORITY_HIGH, pid, cb)
        self.assertEqual(
            self.loop.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )
        self.loop.run()
        self.assertEqual(self.status, 0)

    def test_child_watch_with_data(self):
        def cb(pid, status, data):
            self.status = status
            self.data = data
            self.loop.quit()

        self.data = None
        self.status = None
        self.loop = GLib.MainLoop()
        argv = [sys.executable, "-c", "import sys"]
        pid, stdin, stdout, stderr = GLib.spawn_async(
            argv, flags=GLib.SpawnFlags.DO_NOT_REAP_CHILD
        )
        self.assertEqual(stdin, None)
        self.assertEqual(stdout, None)
        self.assertEqual(stderr, None)
        self.assertNotEqual(pid, 0)
        pid.close()
        id = GLib.child_watch_add(GLib.PRIORITY_HIGH, pid, cb, 12345)
        self.assertEqual(
            self.loop.get_context().find_source_by_id(id).priority, GLib.PRIORITY_HIGH
        )
        self.loop.run()
        self.assertEqual(self.data, 12345)
        self.assertEqual(self.status, 0)

    def test_spawn_async_fds(self):
        pid, stdin, stdout, stderr = GLib.spawn_async(
            ["cat"],
            flags=GLib.SpawnFlags.SEARCH_PATH,
            standard_input=True,
            standard_output=True,
            standard_error=True,
        )
        os.write(stdin, b"hello world!\n")
        os.close(stdin)
        out = os.read(stdout, 50)
        os.close(stdout)
        err = os.read(stderr, 50)
        os.close(stderr)
        pid.close()
        self.assertEqual(out, b"hello world!\n")
        self.assertEqual(err, b"")

    def test_spawn_async_with_pipes(self):
        _res, pid, stdin, stdout, stderr = GLib.spawn_async_with_pipes(
            working_directory=None,
            argv=["cat"],
            envp=None,
            flags=GLib.SpawnFlags.SEARCH_PATH,
        )

        os.write(stdin, b"hello world!\n")
        os.close(stdin)
        out = os.read(stdout, 50)
        os.close(stdout)
        err = os.read(stderr, 50)
        os.close(stderr)
        GLib.spawn_close_pid(pid)
        self.assertEqual(out, b"hello world!\n")
        self.assertEqual(err, b"")

    def test_spawn_async_envp(self):
        pid, stdin, stdout, stderr = GLib.spawn_async(
            ["sh", "-c", "echo $TEST_VAR"],
            ["TEST_VAR=moo!"],
            flags=GLib.SpawnFlags.SEARCH_PATH,
            standard_output=True,
        )
        self.assertEqual(stdin, None)
        self.assertEqual(stderr, None)
        out = os.read(stdout, 50)
        os.close(stdout)
        pid.close()
        self.assertEqual(out, b"moo!\n")

    def test_backwards_compat_flags(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", PyGIDeprecationWarning)

            self.assertEqual(
                GLib.SpawnFlags.DO_NOT_REAP_CHILD, GLib.SPAWN_DO_NOT_REAP_CHILD
            )

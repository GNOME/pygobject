import contextlib
import os
import sys
import unittest
import os.path
import warnings
import subprocess

import pytest
from gi.repository import GLib
from gi import PyGIDeprecationWarning


PLATFORM_SPLIT_VERSION = (2, 88)


GLibUnix = None
with contextlib.suppress(ImportError):
    from gi.repository import GLibUnix


class TestGLib(unittest.TestCase):
    @pytest.mark.xfail()
    def test_pytest_capture_error_in_closure(self):
        # this test is supposed to fail
        ml = GLib.MainLoop()

        def callback():
            ml.quit()
            raise Exception("expected")

        GLib.idle_add(callback)
        ml.run()

    @unittest.skipIf(os.name == "nt", "no bash on Windows")
    def test_find_program_in_path(self):
        bash_path = GLib.find_program_in_path("bash")
        self.assertTrue(bash_path.endswith(os.path.sep + "bash"))
        self.assertTrue(os.path.exists(bash_path))

        self.assertEqual(GLib.find_program_in_path("non existing"), None)

    def test_markup_escape_text(self):
        self.assertEqual(GLib.markup_escape_text("a&bä"), "a&amp;bä")
        self.assertEqual(GLib.markup_escape_text(b"a&b\x05"), "a&amp;b&#x5;")

        # with explicit length argument
        self.assertEqual(GLib.markup_escape_text(b"a\x05\x01\x02", 2), "a&#x5;")

    def test_progname(self):
        GLib.set_prgname("moo")
        self.assertEqual(GLib.get_prgname(), "moo")

    def test_appname(self):
        GLib.set_application_name("moo")
        self.assertEqual(GLib.get_application_name(), "moo")

    def test_xdg_dirs(self):
        d = GLib.get_user_data_dir()
        self.assertTrue(os.path.sep in d, d)
        d = GLib.get_user_special_dir(GLib.UserDirectory.DIRECTORY_DESKTOP)
        self.assertTrue(os.path.sep in d, d)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", PyGIDeprecationWarning)

            # also works with backwards compatible enum names
            self.assertEqual(
                GLib.get_user_special_dir(GLib.UserDirectory.DIRECTORY_MUSIC),
                GLib.get_user_special_dir(GLib.USER_DIRECTORY_MUSIC),
            )

        for d in GLib.get_system_config_dirs():
            self.assertTrue(os.path.sep in d, d)
        for d in GLib.get_system_data_dirs():
            self.assertTrue(isinstance(d, str), d)

    def test_main_depth(self):
        self.assertEqual(GLib.main_depth(), 0)

    def test_filenames(self):
        self.assertEqual(GLib.filename_display_name("foo"), "foo")
        self.assertEqual(GLib.filename_display_basename("bar/foo"), "foo")

        def glibfsencode(f):
            # the annotations of filename_from_utf8() was changed in
            # https://bugzilla.gnome.org/show_bug.cgi?id=756128
            if isinstance(f, bytes):
                return f
            if os.name == "nt":
                return f.encode("utf-8", "surrogatepass")
            return os.fsencode(f)

        # this is locale dependent, so we cannot completely verify the result
        res = GLib.filename_from_utf8("aäb")
        res = glibfsencode(res)
        self.assertTrue(isinstance(res, bytes))
        self.assertGreaterEqual(len(res), 3)

        # with explicit length argument
        res = GLib.filename_from_utf8("aäb", 1)
        res = glibfsencode(res)
        self.assertEqual(res, b"a")

    def test_uri_extract(self):
        res = GLib.uri_list_extract_uris("""# some comment
http://example.com
https://my.org/q?x=1&y=2
            http://gnome.org/new""")
        self.assertEqual(
            res,
            ["http://example.com", "https://my.org/q?x=1&y=2", "http://gnome.org/new"],
        )

    def test_current_time(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            tm = GLib.get_current_time()
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        self.assertTrue(isinstance(tm, float))
        self.assertGreater(tm, 1350000000.0)

    @unittest.skipIf(sys.platform == "darwin", "fails on OSX")
    def test_main_loop(self):
        # note we do not test run() here, as we use this in countless other
        # tests
        ml = GLib.MainLoop()
        self.assertFalse(ml.is_running())

        context = ml.get_context()
        self.assertEqual(context, GLib.MainContext.default())
        self.assertTrue(context.is_owner() in [True, False])
        self.assertTrue(context.pending() in [True, False])
        self.assertFalse(context.iteration(False))

    def test_main_loop_with_context(self):
        context = GLib.MainContext()
        ml = GLib.MainLoop(context)
        self.assertFalse(ml.is_running())
        self.assertEqual(ml.get_context(), context)

    def test_main_context(self):
        # constructor
        context = GLib.MainContext()
        self.assertTrue(context.is_owner() in [True, False])
        self.assertFalse(context.pending())
        self.assertFalse(context.iteration(False))

        # GLib API
        context = GLib.MainContext.default()
        self.assertTrue(context.is_owner() in [True, False])
        self.assertTrue(context.pending() in [True, False])
        self.assertTrue(context.iteration(False) in [True, False])

        # backwards compatible API
        context = GLib.main_context_default()
        self.assertTrue(context.is_owner() in [True, False])
        self.assertTrue(context.pending() in [True, False])
        self.assertTrue(context.iteration(False) in [True, False])

    def test_main_context_query(self):
        context = GLib.MainContext()
        assert context.acquire()

        timeout_msec, fds = context.query(0)

        assert timeout_msec == 0
        assert len(fds) == 1

    @unittest.skipIf(os.name == "nt", "hangs")
    def test_io_add_watch_no_data(self):
        (r, w) = os.pipe()
        call_data = []

        def cb(fd, condition):
            call_data.append((fd, condition, os.read(fd, 1)))
            if len(call_data) == 2:
                ml.quit()
            return True

        # io_add_watch() takes an IOChannel, calling with an fd is deprecated
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            GLib.io_add_watch(r, GLib.IOCondition.IN, cb, priority=GLib.PRIORITY_HIGH)
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        def write():
            os.write(w, b"a")
            GLib.idle_add(lambda: os.write(w, b"b") and False)

        ml = GLib.MainLoop()
        GLib.idle_add(write)
        GLib.timeout_add(2000, ml.quit)
        ml.run()

        self.assertEqual(
            call_data, [(r, GLib.IOCondition.IN, b"a"), (r, GLib.IOCondition.IN, b"b")]
        )

    @unittest.skipIf(os.name == "nt", "hangs")
    def test_io_add_watch_with_data(self):
        (r, w) = os.pipe()
        call_data = []

        def cb(fd, condition, data):
            call_data.append((fd, condition, os.read(fd, 1), data))
            if len(call_data) == 2:
                ml.quit()
            return True

        # io_add_watch() takes an IOChannel, calling with an fd is deprecated
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            GLib.io_add_watch(
                r, GLib.IOCondition.IN, cb, "moo", priority=GLib.PRIORITY_HIGH
            )
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        def write():
            os.write(w, b"a")
            GLib.idle_add(lambda: os.write(w, b"b") and False)

        ml = GLib.MainLoop()
        GLib.idle_add(write)
        GLib.timeout_add(2000, ml.quit)
        ml.run()

        self.assertEqual(
            call_data,
            [
                (r, GLib.IOCondition.IN, b"a", "moo"),
                (r, GLib.IOCondition.IN, b"b", "moo"),
            ],
        )

    @unittest.skipIf(os.name == "nt", "hangs")
    def test_io_add_watch_with_multiple_data(self):
        (r, w) = os.pipe()
        call_data = []

        def cb(fd, condition, *user_data):
            call_data.append((fd, condition, os.read(fd, 1), user_data))
            ml.quit()
            return True

        # io_add_watch() takes an IOChannel, calling with an fd is deprecated
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            GLib.io_add_watch(r, GLib.IOCondition.IN, cb, "moo", "foo")
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        ml = GLib.MainLoop()
        GLib.idle_add(lambda: os.write(w, b"a") and False)
        GLib.timeout_add(2000, ml.quit)
        ml.run()

        self.assertEqual(call_data, [(r, GLib.IOCondition.IN, b"a", ("moo", "foo"))])

    @unittest.skipIf(sys.platform == "darwin", "fails")
    @unittest.skipIf(os.name == "nt", "no shell on Windows")
    def test_io_add_watch_pyfile(self):
        call_data = []

        cmd = subprocess.Popen(
            "echo hello; echo world", shell=True, bufsize=0, stdout=subprocess.PIPE
        )

        def cb(file, condition):
            call_data.append((file, condition, file.readline()))
            if len(call_data) == 2:
                # avoid having to wait for the full timeout
                ml.quit()
            return True

        # io_add_watch() takes an IOChannel, calling with a Python file is deprecated
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            GLib.io_add_watch(cmd.stdout, GLib.IOCondition.IN, cb)
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        ml = GLib.MainLoop()
        GLib.timeout_add(2000, ml.quit)
        ml.run()

        cmd.wait()

        self.assertEqual(
            call_data,
            [
                (cmd.stdout, GLib.IOCondition.IN, b"hello\n"),
                (cmd.stdout, GLib.IOCondition.IN, b"world\n"),
            ],
        )

    def test_glib_version(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", PyGIDeprecationWarning)

            (major, minor, micro) = GLib.glib_version
            self.assertGreaterEqual(major, 2)
            self.assertGreaterEqual(minor, 0)
            self.assertGreaterEqual(micro, 0)

    def test_pyglib_version(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", PyGIDeprecationWarning)

            (major, minor, micro) = GLib.pyglib_version
            self.assertGreaterEqual(major, 3)
            self.assertGreaterEqual(minor, 0)
            self.assertGreaterEqual(micro, 0)

    def test_timezone_constructor(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            timezone = GLib.TimeZone("+05:21")
            self.assertEqual(timezone.get_offset(0), ((5 * 60) + 21) * 60)

    def test_source_attach_implicit_context(self):
        context = GLib.MainContext.default()
        source = GLib.Idle()
        source_id = source.attach()
        self.assertEqual(context, source.get_context())
        self.assertTrue(GLib.Source.remove(source_id))


class TestGLibPlatform(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # TODO: Make this more dynamic in case we'll support Win32
        cls.platform_glib = GLibUnix

        if cls.platform_glib:
            cls.platform_full_name = cls.platform_glib._namespace
            cls.platform_name = f"{cls.platform_full_name[len(GLib._namespace) :]}"

        unittest.TestCase.setUpClass()

    def assertPlatformTypeFallback(self, symbol_name):
        self.assertIn(symbol_name, dir(GLib))

        platform_symbol_name = symbol_name
        if symbol_name.startswith(f"{self.platform_name}"):
            platform_symbol_name = symbol_name[len(self.platform_name) :]
        elif symbol_name.startswith(f"{self.platform_name.lower()}_"):
            platform_symbol_name = symbol_name[len(self.platform_name) + 1 :]

        with warnings.catch_warnings(record=True) as warn:
            symbol = getattr(GLib, symbol_name)
            self.assertIsNotNone(symbol)

            platform_symbol = getattr(self.platform_glib, platform_symbol_name)
            self.assertIsNotNone(platform_symbol)
            self.maybeAssertDeprecationWarning(warn, symbol_name, platform_symbol_name)

            if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) >= PLATFORM_SPLIT_VERSION:
                self.assertEqual(platform_symbol, symbol)

    def maybeAssertDeprecationWarning(self, warn, symbol_name, new_symbol_name):
        if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) >= PLATFORM_SPLIT_VERSION:
            self.assertDeprecationWarning(
                warn,
                "GLib",
                self.platform_full_name,
                symbol_name,
                new_symbol_name,
            )

    def assertDeprecationWarning(
        self, warn, namespace, new_namespace, symbol_name, new_symbol_name
    ):
        self.assertGreaterEqual(len(warn), 1)
        self.assertTrue(
            any(issubclass(w.category, PyGIDeprecationWarning) for w in warn)
        )

        expected_message = (
            f"{namespace}.{symbol_name} is deprecated; "
            + f"use {new_namespace}.{new_symbol_name} instead"
        )
        if len(warn) == 1:
            self.assertEqual(expected_message, str(warn[0].message))
            return

        self.assertIn(
            expected_message,
            [str(w.message) for w in warn],
        )

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_unix_pipe(self):
        self.assertPlatformTypeFallback("UnixPipe")

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_unix_pipe_end(self):
        self.assertPlatformTypeFallback("UnixPipeEnd")

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_closefrom(self):
        self.assertPlatformTypeFallback("closefrom")

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_unix_error_quark(self):
        self.assertPlatformTypeFallback("unix_error_quark")

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_unix_fd_add_full(self):
        self.assertPlatformTypeFallback("unix_fd_add_full")

    @unittest.skipIf(
        GLibUnix is None or (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) < (2, 87),
        "Not supported",
    )
    def test_fallback_unix_fd_query_path(self):
        self.assertPlatformTypeFallback("unix_fd_query_path")

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_unix_fd_source_new(self):
        self.assertPlatformTypeFallback("unix_fd_source_new")

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_fdwalk_set_cloexec(self):
        self.assertPlatformTypeFallback("fdwalk_set_cloexec")

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_unix_get_passwd_entry(self):
        self.assertPlatformTypeFallback("unix_get_passwd_entry")

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_unix_open_pipe(self):
        self.assertPlatformTypeFallback("unix_open_pipe")

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_unix_set_fd_nonblocking(self):
        self.assertPlatformTypeFallback("unix_set_fd_nonblocking")

    @unittest.skipIf(
        GLibUnix is None
        or (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) < PLATFORM_SPLIT_VERSION,
        "Not supported",
    )
    def test_fallback_unix_signal_add(self):
        self.assertPlatformTypeFallback("unix_signal_add")

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_unix_signal_add_full(self):
        self.assertPlatformTypeFallback(
            "unix_signal_add"
            if "signal_add" in dir(GLibUnix)
            else "unix_signal_add_full"
        )

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_fallback_unix_signal_source_new(self):
        self.assertPlatformTypeFallback("unix_signal_source_new")

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_glib_unix_pipe(self):
        pipe = GLibUnix.Pipe()
        self.assertEqual(pipe.fds, [0, 0])

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_glib_unix_pipe_fallback(self):
        with warnings.catch_warnings(record=True) as warn:
            pipe = GLib.UnixPipe()
            self.maybeAssertDeprecationWarning(warn, "UnixPipe", "Pipe")
        self.assertEqual(pipe.fds, [0, 0])

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_glib_unix_error_quark(self):
        self.assertGreater(GLibUnix.error_quark(), 0)

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_glib_unix_error_quark_fallback(self):
        with warnings.catch_warnings(record=True) as warn:
            self.assertGreater(GLib.unix_error_quark(), 0)
            self.maybeAssertDeprecationWarning(warn, "unix_error_quark", "error_quark")

    @unittest.skipIf(
        GLibUnix is None
        or (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) < PLATFORM_SPLIT_VERSION,
        "Not supported",
    )
    def test_glib_unix_signal_add_full_deprecation(self):
        with warnings.catch_warnings(record=True) as warn:
            self.assertIsNotNone(GLibUnix.signal_add_full)
            self.assertDeprecationWarning(
                warn, "GLibUnix", "GLibUnix", "signal_add_full", "signal_add"
            )

    @unittest.skipIf(GLibUnix is None, "Not supported")
    def test_glib_unix_signal_add_full_wrapper_deprecation(self):
        with warnings.catch_warnings(record=True) as warn:
            self.assertIsNotNone(GLib.unix_signal_add_full)
            new_namespace = "GLibUnix" if "signal_add" in dir(GLibUnix) else "GLib"
            new_symbol = (
                "signal_add" if "signal_add" in dir(GLibUnix) else "unix_signal_add"
            )
            self.assertDeprecationWarning(
                warn, "GLib", new_namespace, "unix_signal_add_full", new_symbol
            )

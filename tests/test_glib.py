# -*- Mode: Python -*-
# encoding: UTF-8

import unittest
import os.path
import warnings
import subprocess

from gi.repository import GLib
from gi import PyGIDeprecationWarning

from compathelper import _unicode, _bytes


class TestGLib(unittest.TestCase):
    def test_find_program_in_path(self):
        bash_path = GLib.find_program_in_path('bash')
        self.assertTrue(bash_path.endswith('/bash'))
        self.assertTrue(os.path.exists(bash_path))

        self.assertEqual(GLib.find_program_in_path('non existing'), None)

    def test_markup_escape_text(self):
        self.assertEqual(GLib.markup_escape_text(_unicode('a&bä')), 'a&amp;bä')
        self.assertEqual(GLib.markup_escape_text(_bytes('a&b\x05')), 'a&amp;b&#x5;')

        # with explicit length argument
        self.assertEqual(GLib.markup_escape_text(_bytes('a\x05\x01\x02'), 2), 'a&#x5;')

    def test_progname(self):
        GLib.set_prgname('moo')
        self.assertEqual(GLib.get_prgname(), 'moo')

    def test_appname(self):
        GLib.set_application_name('moo')
        self.assertEqual(GLib.get_application_name(), 'moo')

    def test_xdg_dirs(self):
        d = GLib.get_user_data_dir()
        self.assertTrue('/' in d, d)
        d = GLib.get_user_special_dir(GLib.USER_DIRECTORY_DESKTOP)
        self.assertTrue('/' in d, d)
        # also works with backwards compatible enum names
        self.assertEqual(GLib.get_user_special_dir(GLib.UserDirectory.DIRECTORY_MUSIC),
                         GLib.get_user_special_dir(GLib.USER_DIRECTORY_MUSIC))

        for d in GLib.get_system_config_dirs():
            self.assertTrue('/' in d, d)
        for d in GLib.get_system_data_dirs():
            self.assertTrue(isinstance(d, str), d)

    def test_main_depth(self):
        self.assertEqual(GLib.main_depth(), 0)

    def test_filenames(self):
        self.assertEqual(GLib.filename_display_name('foo'), 'foo')
        self.assertEqual(GLib.filename_display_basename('bar/foo'), 'foo')

        # this is locale dependent, so we cannot completely verify the result
        res = GLib.filename_from_utf8(_unicode('aäb'))
        self.assertTrue(isinstance(res, bytes))
        self.assertGreaterEqual(len(res), 3)

        # with explicit length argument
        self.assertEqual(GLib.filename_from_utf8(_unicode('aäb'), 1), b'a')

    def test_uri_extract(self):
        res = GLib.uri_list_extract_uris('''# some comment
http://example.com
https://my.org/q?x=1&y=2
            http://gnome.org/new''')
        self.assertEqual(res, ['http://example.com',
                               'https://my.org/q?x=1&y=2',
                               'http://gnome.org/new'])

    def test_current_time(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            tm = GLib.get_current_time()
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        self.assertTrue(isinstance(tm, float))
        self.assertGreater(tm, 1350000000.0)

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

    def test_io_add_watch_no_data(self):
        (r, w) = os.pipe()
        call_data = []

        def cb(fd, condition):
            call_data.append((fd, condition, os.read(fd, 1)))
            return True

        # io_add_watch() takes an IOChannel, calling with an fd is deprecated
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            GLib.io_add_watch(r, GLib.IOCondition.IN, cb)
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        ml = GLib.MainLoop()
        GLib.timeout_add(10, lambda: os.write(w, b'a') and False)
        GLib.timeout_add(100, lambda: os.write(w, b'b') and False)
        GLib.timeout_add(200, ml.quit)
        ml.run()

        self.assertEqual(call_data, [(r, GLib.IOCondition.IN, b'a'),
                                     (r, GLib.IOCondition.IN, b'b')])

    def test_io_add_watch_with_data(self):
        (r, w) = os.pipe()
        call_data = []

        def cb(fd, condition, data):
            call_data.append((fd, condition, os.read(fd, 1), data))
            return True

        # io_add_watch() takes an IOChannel, calling with an fd is deprecated
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            GLib.io_add_watch(r, GLib.IOCondition.IN, cb, 'moo')
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        ml = GLib.MainLoop()
        GLib.timeout_add(10, lambda: os.write(w, b'a') and False)
        GLib.timeout_add(100, lambda: os.write(w, b'b') and False)
        GLib.timeout_add(200, ml.quit)
        ml.run()

        self.assertEqual(call_data, [(r, GLib.IOCondition.IN, b'a', 'moo'),
                                     (r, GLib.IOCondition.IN, b'b', 'moo')])

    def test_io_add_watch_with_multiple_data(self):
        (r, w) = os.pipe()
        call_data = []

        def cb(fd, condition, *user_data):
            call_data.append((fd, condition, os.read(fd, 1), user_data))
            return True

        # io_add_watch() takes an IOChannel, calling with an fd is deprecated
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            GLib.io_add_watch(r, GLib.IOCondition.IN, cb, 'moo', 'foo')
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        ml = GLib.MainLoop()
        GLib.timeout_add(10, lambda: os.write(w, b'a') and False)
        GLib.timeout_add(100, ml.quit)
        ml.run()

        self.assertEqual(call_data, [(r, GLib.IOCondition.IN, b'a', ('moo', 'foo'))])

    def test_io_add_watch_pyfile(self):
        call_data = []

        cmd = subprocess.Popen('sleep 0.1; echo hello; sleep 0.2; echo world',
                               shell=True, stdout=subprocess.PIPE)

        def cb(file, condition):
            call_data.append((file, condition, file.readline()))
            if len(call_data) == 2:
                # avoid having to wait for the full timeout
                ml.quit()
            return True

        # io_add_watch() takes an IOChannel, calling with a Python file is deprecated
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            GLib.io_add_watch(cmd.stdout, GLib.IOCondition.IN, cb)
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))

        ml = GLib.MainLoop()
        GLib.timeout_add(2000, ml.quit)
        ml.run()

        cmd.wait()

        self.assertEqual(call_data, [(cmd.stdout, GLib.IOCondition.IN, b'hello\n'),
                                     (cmd.stdout, GLib.IOCondition.IN, b'world\n')])

    def test_glib_version(self):
        (major, minor, micro) = GLib.glib_version
        self.assertGreaterEqual(major, 2)
        self.assertGreaterEqual(minor, 0)
        self.assertGreaterEqual(micro, 0)

    def test_pyglib_version(self):
        (major, minor, micro) = GLib.pyglib_version
        self.assertGreaterEqual(major, 3)
        self.assertGreaterEqual(minor, 0)
        self.assertGreaterEqual(micro, 0)

    def test_timezone_constructor_error(self):
        self.assertRaisesRegexp(TypeError, '.*constructor.*help\(GLib.TimeZone\).*',
                                GLib.TimeZone)

    def test_source_attach_implicit_context(self):
        context = GLib.MainContext.default()
        source = GLib.Idle()
        source_id = source.attach()
        self.assertEqual(context, source.get_context())
        self.assertTrue(GLib.Source.remove(source_id))

import contextlib
import os
import unittest
import warnings

import pytest

GioUnix = None
with contextlib.suppress(ImportError):
    from gi.repository import GioUnix

import gi.overrides
from gi import PyGIWarning, PyGIDeprecationWarning
from gi.repository import GLib, Gio

from .helper import ignore_gi_deprecation_warnings


class TestGio(unittest.TestCase):
    def test_file_enumerator(self):
        self.assertEqual(Gio.FileEnumerator, gi.overrides.Gio.FileEnumerator)
        f = Gio.file_new_for_path("./")

        iter_info = [
            info.get_name() for info in f.enumerate_children("standard::*", 0, None)
        ]

        next_info = []
        enumerator = f.enumerate_children("standard::*", 0, None)
        while True:
            info = enumerator.next_file(None)
            if info is None:
                break
            next_info.append(info.get_name())

        self.assertEqual(iter_info, next_info)

    def test_menu_item(self):
        menu = Gio.Menu()
        item = Gio.MenuItem()
        item.set_attribute([("label", "s", "Test"), ("action", "s", "app.test")])
        menu.append_item(item)
        value = menu.get_item_attribute_value(0, "label", GLib.VariantType.new("s"))
        self.assertEqual("Test", value.unpack())
        value = menu.get_item_attribute_value(0, "action", GLib.VariantType.new("s"))
        self.assertEqual("app.test", value.unpack())

    def test_volume_monitor_warning(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            Gio.VolumeMonitor()
            self.assertEqual(len(warn), 1)
            self.assertTrue(issubclass(warn[0].category, PyGIWarning))
            self.assertRegex(
                str(warn[0].message), ".*Gio\\.VolumeMonitor\\.get\\(\\).*"
            )


class TestGioPlatform(unittest.TestCase):
    desktopFileContent = f"""[Desktop Entry]
Version=1.0
Type=Application
Name=Some Application
Exec={GLib.find_program_in_path("true")} %u
Actions=act;
Categories=Development;Profiling;
GenericName=Test
Hidden=true
Keywords=test;example;
NoDisplay=true
OnlyShowIn=GNOME;
StartupWMClass=testTest
X-Bool=true

[Desktop Action act]
Name=Take action!
Exec={GLib.find_program_in_path("true")} action
"""

    def setUp(self):
        super().setUp()

        if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) < (2, 80):
            self.skipTest("Installed Gio is not new enough for this test")

        if GioUnix:
            self.key_file = GLib.KeyFile()
            self.key_file.load_from_data(
                self.desktopFileContent,
                len(self.desktopFileContent),
                GLib.KeyFileFlags.NONE,
            )

    def assert_expected_desktop_app_info(self, app_info):
        # All functionality is available as ordinary methods, without
        # needing extraneous arguments
        # https://gitlab.gnome.org/GNOME/pygobject/-/issues/719
        self.assertEqual(app_info.get_action_name("act"), "Take action!")
        self.assertEqual(app_info.get_boolean("X-Bool"), True)
        self.assertEqual(app_info.get_categories(), "Development;Profiling;")
        self.assertIsNone(app_info.get_filename())
        self.assertEqual(app_info.get_generic_name(), "Test")
        self.assertEqual(app_info.get_is_hidden(), True)
        self.assertEqual(app_info.get_keywords(), ["test", "example"])
        self.assertEqual(app_info.get_locale_string("Keywords"), "test;example;")
        self.assertEqual(app_info.get_nodisplay(), True)
        self.assertEqual(app_info.get_show_in("GNOME"), True)
        self.assertEqual(app_info.get_show_in("KDE"), False)
        # get_show_in() can be called without an argument, which means NULL
        self.assertIn(app_info.get_show_in(), [True, False])
        self.assertIn(app_info.get_show_in(None), [True, False])
        self.assertEqual(app_info.get_startup_wm_class(), "testTest")
        self.assertEqual(app_info.get_string("StartupWMClass"), "testTest")
        self.assertEqual(app_info.get_string_list("Keywords"), ["test", "example"])
        self.assertEqual(app_info.has_key("Keywords"), True)
        self.assertEqual(app_info.list_actions(), ["act"])

        # All functionality is also available via unbound methods:
        # for example Cinnamon relies on this as of October 2025
        self.assertEqual(
            GioUnix.DesktopAppInfo.get_action_name(app_info, "act"), "Take action!"
        )
        self.assertEqual(
            Gio.DesktopAppInfo.get_action_name(app_info, "act"), "Take action!"
        )

        # Exercise methods that actually launch something
        app_info.launch_action("act")
        app_info.launch_action("act", None)
        self.assertTrue(
            app_info.launch_uris_as_manager(
                ["about:blank"],
                None,
                GLib.SpawnFlags.DEFAULT,
                None,
                None,
                None,
                None,
            ),
        )
        self.assertTrue(
            app_info.launch_uris_as_manager(
                ["about:blank"],
                None,
                GLib.SpawnFlags.DEFAULT,
            ),
        )
        self.assertTrue(
            app_info.launch_uris_as_manager_with_fds(
                ["about:blank"],
                None,
                GLib.SpawnFlags.DEFAULT,
                None,
                None,
                None,
                None,
                -1,
                -1,
                -1,
            ),
        )

    @unittest.skipIf(
        GioUnix is None or "DesktopAppInfo" not in dir(GioUnix), "Not supported"
    )
    def test_desktop_app_info_can_be_created_from_gio_unix(self):
        app_info = GioUnix.DesktopAppInfo.new_from_keyfile(self.key_file)
        self.assertIsNotNone(app_info)
        self.assertIsInstance(app_info, GioUnix.DesktopAppInfo)
        self.assertIsInstance(app_info, Gio.AppInfo)
        self.assert_expected_desktop_app_info(app_info)

    @unittest.skipIf(
        GioUnix is None or "DesktopAppInfo" not in dir(GioUnix), "Not supported"
    )
    def test_desktop_app_info_can_be_created_from_gio(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            app_info = Gio.DesktopAppInfo.new_from_keyfile(self.key_file)

            if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) >= (2, 86):
                self.assertEqual(len(warn), 1)
                self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))
                self.assertEqual(
                    str(warn[0].message),
                    "Gio.DesktopAppInfo is deprecated; "
                    + "use GioUnix.DesktopAppInfo instead",
                )

            self.assertIsNotNone(app_info)
            self.assertIsInstance(app_info, Gio.DesktopAppInfo)
            self.assertIsInstance(app_info, Gio.AppInfo)

    @unittest.skipIf(
        GioUnix is None or "FDMessage" not in dir(GioUnix), "Not supported"
    )
    def test_fd_message(self):
        message = GioUnix.FDMessage.new()
        self.assertIsNotNone(message)

        with open("/dev/null", "r+b") as devnull:
            self.assertTrue(message.append_fd(devnull.fileno()))
            self.assertIsNotNone(message.get_fd_list())
            fds = message.steal_fds()
            self.assertEqual(len(fds), 1)
            os.close(fds[0])

    @unittest.skipIf(
        GioUnix is None or "DesktopAppInfo" not in dir(GioUnix), "Not supported"
    )
    def test_gio_unix_desktop_app_info_provides_platform_independent_functions(self):
        app_info = GioUnix.DesktopAppInfo.new_from_keyfile(self.key_file)
        self.assertEqual(app_info.get_name(), "Some Application")

    @unittest.skipIf(
        GioUnix is None or "DesktopAppInfo" not in dir(GioUnix), "Not supported"
    )
    @ignore_gi_deprecation_warnings
    def test_gio_desktop_app_info_provides_platform_independent_functions(self):
        app_info = Gio.DesktopAppInfo.new_from_keyfile(self.key_file)
        self.assertEqual(app_info.get_name(), "Some Application")

    @unittest.skipIf(
        GioUnix is None or "DesktopAppInfo" not in dir(GioUnix), "Not supported"
    )
    def test_gio_unix_desktop_app_info_provides_unix_only_functions(self):
        app_info = GioUnix.DesktopAppInfo.new_from_keyfile(self.key_file)
        self.assertTrue(app_info.has_key("Name"))
        self.assertEqual(app_info.get_string("Name"), "Some Application")

    @unittest.skipIf(
        GioUnix is None or "DesktopAppInfo" not in dir(GioUnix), "Not supported"
    )
    @ignore_gi_deprecation_warnings
    def test_gio_desktop_app_info_provides_unix_only_functions(self):
        app_info = Gio.DesktopAppInfo.new_from_keyfile(self.key_file)
        self.assertTrue(app_info.has_key("Name"))
        self.assertEqual(app_info.get_string("Name"), "Some Application")

    @unittest.skipIf(GioUnix is None, "Not supported")
    def test_deprecated_unix_function_can_be_called_from_gio(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            mount_points = Gio.unix_mount_points_get()

            if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) >= (2, 86):
                self.assertEqual(len(warn), 1)
                self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))
                self.assertEqual(
                    str(warn[0].message),
                    "Gio.unix_mount_points_get is deprecated; "
                    + "use GioUnix.mount_points_get instead",
                )

            self.assertIsNotNone(mount_points)

            if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) >= (2, 86):
                self.assertEqual(GioUnix.mount_points_get, Gio.unix_mount_points_get)

    def assert_expected_unix_stream_type(self, stream_type):
        with open("/dev/null", "r+b") as devnull:
            stream = stream_type.new(devnull.fileno(), False)
            self.assertIsNotNone(stream)
            self.assertEqual(stream.get_close_fd(), False)
            stream.set_close_fd(False)
            self.assertNotEqual(stream.get_fd(), -1)

    @unittest.skipIf(GioUnix is None, "Not supported")
    def test_deprecated_unix_type_can_be_used_if_equal_exists_in_gio(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")

            input_stream = Gio.UnixInputStream

            if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) >= (2, 86):
                self.assertEqual(len(warn), 1)
                self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))
                self.assertEqual(
                    str(warn[0].message),
                    "Gio.UnixInputStream is deprecated; "
                    + "use GioUnix.InputStream instead",
                )

            self.assertIsNotNone(input_stream)
            self.assertEqual(Gio.UnixInputStream, GioUnix.InputStream)
            self.assertNotEqual(Gio.InputStream, GioUnix.InputStream)

            self.assert_expected_unix_stream_type(Gio.UnixInputStream)
            self.assert_expected_unix_stream_type(GioUnix.InputStream)

            self.assert_expected_unix_stream_type(Gio.UnixOutputStream)
            self.assert_expected_unix_stream_type(GioUnix.OutputStream)

    @unittest.skipIf(GioUnix is None, "Not supported")
    def test_deprecated_unix_class_can_be_used_from_gio(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")
            monitor = Gio.UnixMountMonitor.get()

            if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) >= (2, 86):
                self.assertEqual(len(warn), 1)
                self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))
                self.assertEqual(
                    str(warn[0].message),
                    "Gio.UnixMountMonitor is deprecated; "
                    + "use GioUnix.MountMonitor instead",
                )

            self.assertIsNotNone(monitor)
            self.assertEqual(Gio.UnixMountMonitor.get, GioUnix.MountMonitor.get)

    @unittest.skipIf(GioUnix is None, "Not supported")
    def test_deprecated_unix_gobject_class_can_be_used_from_gio(self):
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter("always")

            input_stream_class = Gio.UnixInputStreamClass

            if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) >= (2, 86):
                self.assertEqual(len(warn), 1)
                self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))
                self.assertEqual(
                    str(warn[0].message),
                    "Gio.UnixInputStreamClass is deprecated; "
                    + "use GioUnix.InputStreamClass instead",
                )

            self.assertIsNotNone(input_stream_class)

            if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) >= (2, 86):
                self.assertEqual(Gio.UnixInputStreamClass, GioUnix.InputStreamClass)

            self.assertNotEqual(Gio.InputStreamClass, GioUnix.InputStreamClass)


class TestGSettings(unittest.TestCase):
    def setUp(self):
        self.settings = Gio.Settings.new("org.gnome.test")
        # we change the values in the tests, so set them to predictable start
        # value
        self.settings.reset("test-string")
        self.settings.reset("test-array")
        self.settings.reset("test-boolean")
        self.settings.reset("test-enum")

    def test_iter(self):
        assert set(self.settings) == {
            "test-tuple",
            "test-array",
            "test-boolean",
            "test-string",
            "test-enum",
            "test-range",
        }

    def test_get_set(self):
        for key in self.settings:
            old_value = self.settings[key]
            self.settings[key] = old_value
            assert self.settings[key] == old_value

    def test_native(self):
        self.assertTrue("test-array" in self.settings.list_keys())

        # get various types
        v = self.settings.get_value("test-boolean")
        self.assertEqual(v.get_boolean(), True)
        self.assertEqual(self.settings.get_boolean("test-boolean"), True)

        v = self.settings.get_value("test-string")
        self.assertEqual(v.get_string(), "Hello")
        self.assertEqual(self.settings.get_string("test-string"), "Hello")

        v = self.settings.get_value("test-array")
        self.assertEqual(v.unpack(), [1, 2])

        v = self.settings.get_value("test-tuple")
        self.assertEqual(v.unpack(), (1, 2))

        v = self.settings.get_value("test-range")
        assert v.unpack() == 123

        # set a value
        self.settings.set_string("test-string", "World")
        self.assertEqual(self.settings.get_string("test-string"), "World")

        self.settings.set_value("test-string", GLib.Variant("s", "Goodbye"))
        self.assertEqual(self.settings.get_string("test-string"), "Goodbye")

    def test_constructor(self):
        # default constructor uses path from schema
        self.assertEqual(self.settings.get_property("path"), "/tests/")

        # optional constructor arguments
        with_path = Gio.Settings.new_with_path("org.gnome.nopathtest", "/mypath/")
        self.assertEqual(with_path.get_property("path"), "/mypath/")
        self.assertEqual(with_path["np-int"], 42)

    def test_dictionary_api(self):
        self.assertEqual(len(self.settings), 6)
        self.assertTrue("test-array" in self.settings)
        self.assertTrue("test-array" in self.settings)
        self.assertFalse("nonexisting" in self.settings)
        self.assertFalse(4 in self.settings)
        self.assertEqual(bool(self.settings), True)

    def test_get(self):
        self.assertEqual(self.settings["test-boolean"], True)
        self.assertEqual(self.settings["test-string"], "Hello")
        self.assertEqual(self.settings["test-enum"], "banana")
        self.assertEqual(self.settings["test-array"], [1, 2])
        self.assertEqual(self.settings["test-tuple"], (1, 2))

        self.assertRaises(KeyError, self.settings.__getitem__, "unknown")
        self.assertRaises(KeyError, self.settings.__getitem__, 2)

    def test_set(self):
        self.settings["test-boolean"] = False
        self.assertEqual(self.settings["test-boolean"], False)
        self.settings["test-string"] = "Goodbye"
        self.assertEqual(self.settings["test-string"], "Goodbye")
        self.settings["test-array"] = [3, 4, 5]
        self.assertEqual(self.settings["test-array"], [3, 4, 5])
        self.settings["test-enum"] = "pear"
        self.assertEqual(self.settings["test-enum"], "pear")

        self.assertRaises(TypeError, self.settings.__setitem__, "test-string", 1)
        self.assertRaises(ValueError, self.settings.__setitem__, "test-enum", "plum")
        self.assertRaises(KeyError, self.settings.__setitem__, "unknown", "moo")

    def test_set_range(self):
        self.settings["test-range"] = 7
        assert self.settings["test-range"] == 7
        self.settings["test-range"] = 65535
        assert self.settings["test-range"] == 65535

        with pytest.raises(ValueError, match=r".*7 - 65535.*"):
            self.settings["test-range"] = 7 - 1

        with pytest.raises(ValueError, match=r".*7 - 65535.*"):
            self.settings["test-range"] = 65535 + 1

    def test_empty(self):
        empty = Gio.Settings.new_with_path("org.gnome.empty", "/tests/")
        self.assertEqual(len(empty), 0)
        self.assertEqual(bool(empty), True)
        self.assertEqual(empty.keys(), [])

    @ignore_gi_deprecation_warnings
    def test_change_event(self):
        changed_log = []
        change_event_log = []

        def on_changed(settings, key):
            changed_log.append((settings, key))

        def on_change_event(settings, keys, n_keys):
            change_event_log.append((settings, keys, n_keys))

        self.settings.connect("changed", on_changed)
        self.settings.connect("change-event", on_change_event)
        self.settings["test-string"] = "Moo"
        self.assertEqual(changed_log, [(self.settings, "test-string")])
        self.assertEqual(
            change_event_log,
            [(self.settings, [GLib.quark_from_static_string("test-string")], 1)],
        )


@unittest.skipIf(os.name == "nt", "FIXME")
class TestGFile(unittest.TestCase):
    def setUp(self):
        self.file, self.io_stream = Gio.File.new_tmp("TestGFile.XXXXXX")

    def tearDown(self):
        with contextlib.suppress(GLib.GError):
            # test_delete and test_delete_async already remove it
            self.file.delete(None)

    def test_replace_contents(self):
        content = b"hello\0world\x7f!"
        succ, etag = self.file.replace_contents(
            content, None, False, Gio.FileCreateFlags.NONE, None
        )
        new_succ, new_content, new_etag = self.file.load_contents(None)

        self.assertTrue(succ)
        self.assertTrue(new_succ)
        self.assertEqual(etag, new_etag)
        self.assertEqual(content, new_content)

    # https://bugzilla.gnome.org/show_bug.cgi?id=690525
    def disabled_test_replace_contents_async(self):
        content = b"".join(bytes(chr(i), "utf-8") for i in range(128))

        def callback(f, result, d):
            # Quit so in case of failed assertations loop doesn't keep running.
            main_loop.quit()
            _succ, _etag = self.file.replace_contents_finish(result)
            _new_succ, _new_content, _new_etag = self.file.load_contents(None)
            d["succ"], d["etag"] = self.file.replace_contents_finish(result)
            load = self.file.load_contents(None)
            d["new_succ"], d["new_content"], d["new_etag"] = load

        data = {}
        self.file.replace_contents_async(
            content, None, False, Gio.FileCreateFlags.NONE, None, callback, data
        )
        main_loop = GLib.MainLoop()
        main_loop.run()
        self.assertTrue(data["succ"])
        self.assertTrue(data["new_succ"])
        self.assertEqual(data["etag"], data["new_etag"])
        self.assertEqual(content, data["new_content"])

    def test_tmp_exists(self):
        # A simple test to check if Gio.File.new_tmp is working correctly.
        self.assertTrue(self.file.query_exists(None))

    def test_delete(self):
        self.file.delete(None)
        self.assertFalse(self.file.query_exists(None))

    def test_delete_async(self):
        def callback(f, result, data):
            main_loop.quit()

        self.file.delete_async(0, None, callback, None)
        main_loop = GLib.MainLoop()
        main_loop.run()
        self.assertFalse(self.file.query_exists(None))


@unittest.skipIf(os.name == "nt", "crashes on Windows")
class TestGApplication(unittest.TestCase):
    def test_command_line(self):
        class App(Gio.Application):
            args = None

            def __init__(self):
                super().__init__(flags=Gio.ApplicationFlags.HANDLES_COMMAND_LINE)

            def do_command_line(self, cmdline):
                self.args = cmdline.get_arguments()
                return 42

        app = App()
        res = app.run(["spam", "eggs"])

        self.assertEqual(res, 42)
        self.assertSequenceEqual(app.args, ["spam", "eggs"])

    def test_local_command_line(self):
        class App(Gio.Application):
            local_args = None

            def __init__(self):
                super().__init__(flags=Gio.ApplicationFlags.HANDLES_COMMAND_LINE)

            def do_local_command_line(self, args):
                self.local_args = args[:]  # copy
                args.remove("eggs")

                # True skips do_command_line being called.
                return True, args, 42

        app = App()
        res = app.run(["spam", "eggs"])

        self.assertEqual(res, 42)
        self.assertSequenceEqual(app.local_args, ["spam", "eggs"])

    def test_local_and_remote_command_line(self):
        class App(Gio.Application):
            args = None
            local_args = None

            def __init__(self):
                super().__init__(flags=Gio.ApplicationFlags.HANDLES_COMMAND_LINE)

            def do_command_line(self, cmdline):
                self.args = cmdline.get_arguments()
                return 42

            def do_local_command_line(self, args):
                self.local_args = args[:]  # copy
                args.remove("eggs")

                # False causes do_command_line to be called with args.
                return False, args, 0

        app = App()
        res = app.run(["spam", "eggs"])

        self.assertEqual(res, 42)
        self.assertSequenceEqual(app.args, ["spam"])
        self.assertSequenceEqual(app.local_args, ["spam", "eggs"])

    @unittest.skipUnless(
        hasattr(Gio.Application, "add_main_option"), "Requires newer version of GLib"
    )
    def test_add_main_option(self):
        stored_options = []

        def on_handle_local_options(app, options):
            stored_options.append(options)
            return 0  # Return 0 if options have been handled

        def on_activate(app):
            pass

        app = Gio.Application()
        app.add_main_option(
            long_name="string",
            short_name=b"s",
            flags=0,
            arg=GLib.OptionArg.STRING,
            description="some string",
        )

        app.connect("activate", on_activate)
        app.connect("handle-local-options", on_handle_local_options)
        app.run(["app", "-s", "test string"])

        self.assertEqual(len(stored_options), 1)
        options = stored_options[0]
        self.assertTrue(options.contains("string"))
        self.assertEqual(options.lookup_value("string").unpack(), "test string")

    def test_add_action_entries_override(self):
        app = Gio.Application()

        app.add_action_entries(
            [
                ("app.new", print),
                ("app.quit", print),
            ]
        )

        assert app.lookup_action("app.new").get_name() == "app.new"

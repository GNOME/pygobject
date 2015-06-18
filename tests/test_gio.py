# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest

import gi.overrides
from gi.repository import GLib, Gio

from helper import ignore_gi_deprecation_warnings


class TestGio(unittest.TestCase):
    def test_file_enumerator(self):
        self.assertEqual(Gio.FileEnumerator, gi.overrides.Gio.FileEnumerator)
        f = Gio.file_new_for_path("./")

        iter_info = []
        for info in f.enumerate_children("standard::*", 0, None):
            iter_info.append(info.get_name())

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
        item.set_attribute([("label", "s", "Test"),
                            ("action", "s", "app.test")])
        menu.append_item(item)
        value = menu.get_item_attribute_value(0, "label", GLib.VariantType.new("s"))
        self.assertEqual("Test", value.unpack())
        value = menu.get_item_attribute_value(0, "action", GLib.VariantType.new("s"))
        self.assertEqual("app.test", value.unpack())


class TestGSettings(unittest.TestCase):
    def setUp(self):
        self.settings = Gio.Settings.new('org.gnome.test')
        # we change the values in the tests, so set them to predictable start
        # value
        self.settings.reset('test-string')
        self.settings.reset('test-array')
        self.settings.reset('test-boolean')
        self.settings.reset('test-enum')

    def test_native(self):
        self.assertTrue('test-array' in self.settings.list_keys())

        # get various types
        v = self.settings.get_value('test-boolean')
        self.assertEqual(v.get_boolean(), True)
        self.assertEqual(self.settings.get_boolean('test-boolean'), True)

        v = self.settings.get_value('test-string')
        self.assertEqual(v.get_string(), 'Hello')
        self.assertEqual(self.settings.get_string('test-string'), 'Hello')

        v = self.settings.get_value('test-array')
        self.assertEqual(v.unpack(), [1, 2])

        v = self.settings.get_value('test-tuple')
        self.assertEqual(v.unpack(), (1, 2))

        # set a value
        self.settings.set_string('test-string', 'World')
        self.assertEqual(self.settings.get_string('test-string'), 'World')

        self.settings.set_value('test-string', GLib.Variant('s', 'Goodbye'))
        self.assertEqual(self.settings.get_string('test-string'), 'Goodbye')

    def test_constructor(self):
        # default constructor uses path from schema
        self.assertEqual(self.settings.get_property('path'), '/tests/')

        # optional constructor arguments
        with_path = Gio.Settings.new_with_path('org.gnome.nopathtest', '/mypath/')
        self.assertEqual(with_path.get_property('path'), '/mypath/')
        self.assertEqual(with_path['np-int'], 42)

    def test_dictionary_api(self):
        self.assertEqual(len(self.settings), 5)
        self.assertTrue('test-array' in self.settings)
        self.assertTrue('test-array' in self.settings.keys())
        self.assertFalse('nonexisting' in self.settings)
        self.assertFalse(4 in self.settings)
        self.assertEqual(bool(self.settings), True)

    def test_get(self):
        self.assertEqual(self.settings['test-boolean'], True)
        self.assertEqual(self.settings['test-string'], 'Hello')
        self.assertEqual(self.settings['test-enum'], 'banana')
        self.assertEqual(self.settings['test-array'], [1, 2])
        self.assertEqual(self.settings['test-tuple'], (1, 2))

        self.assertRaises(KeyError, self.settings.__getitem__, 'unknown')
        self.assertRaises(KeyError, self.settings.__getitem__, 2)

    def test_set(self):
        self.settings['test-boolean'] = False
        self.assertEqual(self.settings['test-boolean'], False)
        self.settings['test-string'] = 'Goodbye'
        self.assertEqual(self.settings['test-string'], 'Goodbye')
        self.settings['test-array'] = [3, 4, 5]
        self.assertEqual(self.settings['test-array'], [3, 4, 5])
        self.settings['test-enum'] = 'pear'
        self.assertEqual(self.settings['test-enum'], 'pear')

        self.assertRaises(TypeError, self.settings.__setitem__, 'test-string', 1)
        self.assertRaises(ValueError, self.settings.__setitem__, 'test-enum', 'plum')
        self.assertRaises(KeyError, self.settings.__setitem__, 'unknown', 'moo')

    def test_empty(self):
        empty = Gio.Settings.new_with_path('org.gnome.empty', '/tests/')
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

        self.settings.connect('changed', on_changed)
        self.settings.connect('change-event', on_change_event)
        self.settings['test-string'] = 'Moo'
        self.assertEqual(changed_log, [(self.settings, 'test-string')])
        self.assertEqual(change_event_log, [(self.settings,
                                             [GLib.quark_from_static_string('test-string')],
                                             1)])


class TestGFile(unittest.TestCase):
    def setUp(self):
        self.file, self.io_stream = Gio.File.new_tmp('TestGFile.XXXXXX')

    def tearDown(self):
        try:
            self.file.delete(None)
            # test_delete and test_delete_async already remove it
        except GLib.GError:
            pass

    def test_replace_contents(self):
        content = b'hello\0world\x7F!'
        succ, etag = self.file.replace_contents(content, None, False,
                                                Gio.FileCreateFlags.NONE, None)
        new_succ, new_content, new_etag = self.file.load_contents(None)

        self.assertTrue(succ)
        self.assertTrue(new_succ)
        self.assertEqual(etag, new_etag)
        self.assertEqual(content, new_content)

    # https://bugzilla.gnome.org/show_bug.cgi?id=690525
    def disabled_test_replace_contents_async(self):
        content = b''.join(bytes(chr(i), 'utf-8') for i in range(128))

        def callback(f, result, d):
            # Quit so in case of failed assertations loop doesn't keep running.
            main_loop.quit()
            succ, etag = self.file.replace_contents_finish(result)
            new_succ, new_content, new_etag = self.file.load_contents(None)
            d['succ'], d['etag'] = self.file.replace_contents_finish(result)
            load = self.file.load_contents(None)
            d['new_succ'], d['new_content'], d['new_etag'] = load

        data = {}
        self.file.replace_contents_async(content, None, False,
                                         Gio.FileCreateFlags.NONE, None,
                                         callback, data)
        main_loop = GLib.MainLoop()
        main_loop.run()
        self.assertTrue(data['succ'])
        self.assertTrue(data['new_succ'])
        self.assertEqual(data['etag'], data['new_etag'])
        self.assertEqual(content, data['new_content'])

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


class TestGApplication(unittest.TestCase):
    def test_command_line(self):
        class App(Gio.Application):
            args = None

            def __init__(self):
                super(App, self).__init__(flags=Gio.ApplicationFlags.HANDLES_COMMAND_LINE)

            def do_command_line(self, cmdline):
                self.args = cmdline.get_arguments()
                return 42

        app = App()
        res = app.run(['spam', 'eggs'])

        self.assertEqual(res, 42)
        self.assertSequenceEqual(app.args, ['spam', 'eggs'])

    def test_local_command_line(self):
        class App(Gio.Application):
            local_args = None

            def __init__(self):
                super(App, self).__init__(flags=Gio.ApplicationFlags.HANDLES_COMMAND_LINE)

            def do_local_command_line(self, args):
                self.local_args = args[:]  # copy
                args.remove('eggs')

                # True skips do_command_line being called.
                return True, args, 42

        app = App()
        res = app.run(['spam', 'eggs'])

        self.assertEqual(res, 42)
        self.assertSequenceEqual(app.local_args, ['spam', 'eggs'])

    def test_local_and_remote_command_line(self):
        class App(Gio.Application):
            args = None
            local_args = None

            def __init__(self):
                super(App, self).__init__(flags=Gio.ApplicationFlags.HANDLES_COMMAND_LINE)

            def do_command_line(self, cmdline):
                self.args = cmdline.get_arguments()
                return 42

            def do_local_command_line(self, args):
                self.local_args = args[:]  # copy
                args.remove('eggs')

                # False causes do_command_line to be called with args.
                return False, args, 0

        app = App()
        res = app.run(['spam', 'eggs'])

        self.assertEqual(res, 42)
        self.assertSequenceEqual(app.args, ['spam'])
        self.assertSequenceEqual(app.local_args, ['spam', 'eggs'])

    @unittest.skipUnless(hasattr(Gio.Application, 'add_main_option'),
                         'Requires newer version of GLib')
    def test_add_main_option(self):
        stored_options = []

        def on_handle_local_options(app, options):
            stored_options.append(options)
            return 0  # Return 0 if options have been handled

        def on_activate(app):
            pass

        app = Gio.Application()
        app.add_main_option(long_name='string',
                            short_name=b's',
                            flags=0,
                            arg=GLib.OptionArg.STRING,
                            description='some string')

        app.connect('activate', on_activate)
        app.connect('handle-local-options', on_handle_local_options)
        app.run(['app', '-s', 'test string'])

        self.assertEqual(len(stored_options), 1)
        options = stored_options[0]
        self.assertTrue(options.contains('string'))
        self.assertEqual(options.lookup_value('string').unpack(),
                         'test string')

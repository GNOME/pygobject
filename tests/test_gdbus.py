# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest

import sys
sys.path.insert(0, "../")

import gobject
from gi.repository import GLib
from gi.repository import Gio

class TestGDBusClient(unittest.TestCase):
    def setUp(self):
        self.bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)

        self.dbus_proxy = Gio.DBusProxy.new_sync(self.bus,
                Gio.DBusProxyFlags.NONE, None, 
                'org.freedesktop.DBus',
                '/org/freedesktop/DBus',
                'org.freedesktop.DBus', None)

    def test_native_calls_sync(self):
        result = self.dbus_proxy.call_sync('ListNames', None, 
                Gio.DBusCallFlags.NO_AUTO_START, 500, None)
        self.assertTrue(isinstance(result, GLib.Variant))
        result = result.unpack()[0] # result is always a tuple
        self.assertTrue(len(result) > 1)
        self.assertTrue('org.freedesktop.DBus' in result)

        result = self.dbus_proxy.call_sync('GetNameOwner', 
                GLib.Variant('(s)', ('org.freedesktop.DBus',)),
                Gio.DBusCallFlags.NO_AUTO_START, 500, None)
        self.assertTrue(isinstance(result, GLib.Variant))
        self.assertEqual(type(result.unpack()[0]), type(''))

    def test_native_calls_sync_errors(self):
        # error case: invalid argument types
        try:
            self.dbus_proxy.call_sync('GetConnectionUnixProcessID', None,
                    Gio.DBusCallFlags.NO_AUTO_START, 500, None)
            self.fail('call with invalid arguments should raise an exception')
        except Exception as e:
            self.assertTrue('InvalidArgs' in str(e))

        # error case: invalid argument
        try:
            self.dbus_proxy.call_sync('GetConnectionUnixProcessID', 
                    GLib.Variant('(s)', (' unknown',)),
                    Gio.DBusCallFlags.NO_AUTO_START, 500, None)
            self.fail('call with invalid arguments should raise an exception')
        except Exception as e:
            self.assertTrue('NameHasNoOwner' in str(e))

        # error case: unknown method
        try:
            self.dbus_proxy.call_sync('UnknownMethod', None,
                    Gio.DBusCallFlags.NO_AUTO_START, 500, None)
            self.fail('call for unknown method should raise an exception')
        except Exception as e:
            self.assertTrue('UnknownMethod' in str(e))

    def test_native_calls_async(self):
        def call_done(obj, result, user_data):
            user_data['result'] = self.dbus_proxy.call_finish(result)
            user_data['main_loop'].quit()

        main_loop = gobject.MainLoop()
        data = {'main_loop': main_loop}
        self.dbus_proxy.call('ListNames', None, 
                Gio.DBusCallFlags.NO_AUTO_START, 500, None,
                call_done, data)
        main_loop.run()

        self.assertTrue(isinstance(data['result'], GLib.Variant))
        result = data['result'].unpack()[0] # result is always a tuple
        self.assertTrue(len(result) > 1)
        self.assertTrue('org.freedesktop.DBus' in result)

    def test_native_calls_async_errors(self):
        def call_done(obj, result, user_data):
            try:
                self.dbus_proxy.call_finish(result)
                self.fail('call_finish() for unknown method should raise an exception')
            except Exception as e:
                self.assertTrue('UnknownMethod' in str(e))
            finally:
                user_data['main_loop'].quit()

        main_loop = gobject.MainLoop()
        data = {'main_loop': main_loop}
        self.dbus_proxy.call('UnknownMethod', None,
                Gio.DBusCallFlags.NO_AUTO_START, 500, None, call_done, data)
        main_loop.run()

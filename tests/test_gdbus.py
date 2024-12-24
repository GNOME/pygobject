import asyncio
import unittest

import pytest
from gi.events import GLibEventLoopPolicy
from gi.repository import GLib
from gi.repository import Gio


try:
    Gio.bus_get_sync(Gio.BusType.SESSION, None)
except GLib.Error:
    has_dbus = False
else:
    has_dbus = True


class TestDBusNodeInfo(unittest.TestCase):
    def test_new_for_xml(self):
        info = Gio.DBusNodeInfo.new_for_xml("""
<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN'
    'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>
    <interface name='org.freedesktop.DBus.Introspectable'>
        <method name='Introspect'>
            <arg name='data' direction='out' type='s'/>
        </method>
    </interface>
</node>
""")

        interfaces = info.interfaces
        del info
        assert len(interfaces) == 1
        assert interfaces[0].name == "org.freedesktop.DBus.Introspectable"
        methods = interfaces[0].methods
        del interfaces
        assert len(methods) == 1
        assert methods[0].name == "Introspect"
        out_args = methods[0].out_args
        assert len(out_args)
        del methods
        assert out_args[0].name == "data"


@unittest.skipUnless(has_dbus, "no dbus running")
class TestGDBusClient(unittest.TestCase):
    def setUp(self):
        self.bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)

        self.dbus_proxy = Gio.DBusProxy.new_sync(
            self.bus,
            Gio.DBusProxyFlags.NONE,
            None,
            "org.freedesktop.DBus",
            "/org/freedesktop/DBus",
            "org.freedesktop.DBus",
            None,
        )

    def test_native_calls_sync(self):
        result = self.dbus_proxy.call_sync(
            "ListNames", None, Gio.DBusCallFlags.NO_AUTO_START, 500, None
        )
        self.assertTrue(isinstance(result, GLib.Variant))
        result = result.unpack()[0]  # result is always a tuple
        self.assertTrue(len(result) > 1)
        self.assertTrue("org.freedesktop.DBus" in result)

        result = self.dbus_proxy.call_sync(
            "GetNameOwner",
            GLib.Variant("(s)", ("org.freedesktop.DBus",)),
            Gio.DBusCallFlags.NO_AUTO_START,
            500,
            None,
        )
        self.assertTrue(isinstance(result, GLib.Variant))
        self.assertEqual(type(result.unpack()[0]), str)

    def test_native_calls_sync_errors(self):
        # error case: invalid argument types
        try:
            self.dbus_proxy.call_sync(
                "GetConnectionUnixProcessID",
                None,
                Gio.DBusCallFlags.NO_AUTO_START,
                500,
                None,
            )
            self.fail("call with invalid arguments should raise an exception")
        except Exception as e:
            self.assertTrue("InvalidArgs" in str(e))

        # error case: invalid argument
        try:
            self.dbus_proxy.call_sync(
                "GetConnectionUnixProcessID",
                GLib.Variant("(s)", (" unknown",)),
                Gio.DBusCallFlags.NO_AUTO_START,
                500,
                None,
            )
            self.fail("call with invalid arguments should raise an exception")
        except Exception as e:
            self.assertTrue("NameHasNoOwner" in str(e))

        # error case: unknown method
        try:
            self.dbus_proxy.call_sync(
                "UnknownMethod", None, Gio.DBusCallFlags.NO_AUTO_START, 500, None
            )
            self.fail("call for unknown method should raise an exception")
        except Exception as e:
            self.assertTrue("UnknownMethod" in str(e))

    def test_native_calls_async(self):
        def call_done(obj, result, user_data):
            try:
                user_data["result"] = obj.call_finish(result)
            finally:
                user_data["main_loop"].quit()

        main_loop = GLib.MainLoop()
        data = {"main_loop": main_loop}
        self.dbus_proxy.call(
            "ListNames",
            None,
            Gio.DBusCallFlags.NO_AUTO_START,
            500,
            None,
            call_done,
            data,
        )
        main_loop.run()

        self.assertTrue(isinstance(data["result"], GLib.Variant))
        result = data["result"].unpack()[0]  # result is always a tuple
        self.assertTrue(len(result) > 1)
        self.assertTrue("org.freedesktop.DBus" in result)

    def test_native_calls_async_errors(self):
        def call_done(obj, result, user_data):
            try:
                obj.call_finish(result)
                self.fail("call_finish() for unknown method should raise an exception")
            except Exception as e:
                self.assertTrue("UnknownMethod" in str(e))
            finally:
                user_data["main_loop"].quit()

        main_loop = GLib.MainLoop()
        data = {"main_loop": main_loop}
        self.dbus_proxy.call(
            "UnknownMethod",
            None,
            Gio.DBusCallFlags.NO_AUTO_START,
            500,
            None,
            call_done,
            data,
        )
        main_loop.run()

    def test_python_calls_sync(self):
        # single value return tuples get unboxed to the one element
        result = self.dbus_proxy.ListNames("()")
        self.assertTrue(isinstance(result, list))
        self.assertTrue(len(result) > 1)
        self.assertTrue("org.freedesktop.DBus" in result)

        result = self.dbus_proxy.GetNameOwner("(s)", "org.freedesktop.DBus")
        self.assertEqual(type(result), str)

        # empty return tuples get unboxed to None
        self.assertEqual(self.dbus_proxy.ReloadConfig("()"), None)

        # multiple return values remain a tuple; unfortunately D-BUS itself
        # does not have any method returning multiple results, so try talking
        # to notification-daemon (and don't fail the test if it does not exist)
        try:
            nd = Gio.DBusProxy.new_sync(
                self.bus,
                Gio.DBusProxyFlags.NONE,
                None,
                "org.freedesktop.Notifications",
                "/org/freedesktop/Notifications",
                "org.freedesktop.Notifications",
                None,
            )
            result = nd.GetServerInformation("()")
            self.assertTrue(isinstance(result, tuple))
            self.assertEqual(len(result), 4)
            for i in result:
                self.assertEqual(type(i), str)
        except Exception as e:
            if "Error.ServiceUnknown" not in str(e):
                raise

        # test keyword argument; timeout=0 will fail immediately
        try:
            self.dbus_proxy.GetConnectionUnixProcessID("(s)", "1", timeout=0)
            self.fail("call with timeout=0 should raise an exception")
        except Exception as e:
            # FIXME: this is not very precise, but in some environments we
            # do not always get an actual timeout
            self.assertTrue(isinstance(e, GLib.GError), str(e))

    def test_python_calls_sync_noargs(self):
        # methods without arguments don't need an explicit signature
        result = self.dbus_proxy.ListNames()
        self.assertTrue(isinstance(result, list))
        self.assertTrue(len(result) > 1)
        self.assertTrue("org.freedesktop.DBus" in result)

    def test_python_calls_sync_errors(self):
        # error case: invalid argument types
        try:
            self.dbus_proxy.GetConnectionUnixProcessID("()")
            self.fail("call with invalid arguments should raise an exception")
        except Exception as e:
            self.assertTrue("InvalidArgs" in str(e), str(e))

        try:
            self.dbus_proxy.GetConnectionUnixProcessID(None, "foo")
            self.fail("call with None signature should raise an exception")
        except TypeError as e:
            self.assertTrue("signature" in str(e), str(e))

    def test_python_calls_async(self):
        def call_done(obj, result, user_data):
            user_data["result"] = result
            user_data["main_loop"].quit()

        main_loop = GLib.MainLoop()
        data = {"main_loop": main_loop}
        self.dbus_proxy.ListNames("()", result_handler=call_done, user_data=data)
        main_loop.run()

        result = data["result"]
        self.assertEqual(type(result), type([]))
        self.assertTrue(len(result) > 1)
        self.assertTrue("org.freedesktop.DBus" in result)

    def test_python_calls_async_error_result(self):
        # when only specifying a result handler, this will get the error
        def call_done(obj, result, user_data):
            user_data["result"] = result
            user_data["main_loop"].quit()

        main_loop = GLib.MainLoop()
        data = {"main_loop": main_loop}
        self.dbus_proxy.ListNames(
            "(s)", "invalid_argument", result_handler=call_done, user_data=data
        )
        main_loop.run()

        self.assertTrue(isinstance(data["result"], Exception))
        self.assertTrue("InvalidArgs" in str(data["result"]), str(data["result"]))

    def test_python_calls_async_error(self):
        # when specifying an explicit error handler, this will get the error
        def call_done(obj, result, user_data):
            user_data["main_loop"].quit()
            self.fail("result handler should not be called")

        def call_error(obj, error, user_data):
            user_data["error"] = error
            user_data["main_loop"].quit()

        main_loop = GLib.MainLoop()
        data = {"main_loop": main_loop}
        self.dbus_proxy.ListNames(
            "(s)",
            "invalid_argument",
            result_handler=call_done,
            error_handler=call_error,
            user_data=data,
        )
        main_loop.run()

        self.assertTrue(isinstance(data["error"], Exception))
        self.assertTrue("InvalidArgs" in str(data["error"]), str(data["error"]))

    def test_instantiate_custom_proxy(self):
        class SomeProxy(Gio.DBusProxy):
            def __init__(self):
                Gio.DBusProxy.__init__(self)

        SomeProxy()


class TestDBusConnection:
    @unittest.skipUnless(has_dbus, "no dbus running")
    def test_register_object(self):
        object_path = "/pygobject/Test"
        interface_xml = """
            <node>
                <interface name='org.pygobject.Test'>
                    <method name='test' />
                </interface>
            </node>"""
        interface_info = Gio.DBusNodeInfo.new_for_xml(interface_xml).interfaces[0]
        bus = Gio.bus_get_sync(Gio.BusType.SESSION)

        for args in (
            (object_path, interface_info),
            (object_path, interface_info, None, None, None),
        ):
            reg_id = bus.register_object(*args)
            bus.unregister_object(reg_id)

        for kwargs in (
            {"object_path": object_path, "interface_info": interface_info},
            {
                "object_path": object_path,
                "interface_info": interface_info,
                "method_call_closure": None,
                "get_property_closure": None,
                "set_property_closure": None,
            },
        ):
            reg_id = bus.register_object(**kwargs)
            bus.unregister_object(reg_id)

    @unittest.skipUnless(has_dbus, "no dbus running")
    @pytest.mark.xfail()
    def test_connection_invocation_ref_count(self):
        """Invocation object should not leak a reference."""
        invocation, errors = self.run_server(self.client_call)

        assert not errors
        assert invocation
        assert invocation.ref_count == 1

    def run_server(self, client_callback):
        self.invocation = None
        self.errors = []
        self.loop = GLib.MainLoop()

        def on_name_acquired(bus, name):
            client_callback(bus)

        self.reg_id = None
        bus = Gio.bus_get_sync(Gio.BusType.SESSION)
        owner_id = Gio.bus_own_name(
            Gio.BusType.SESSION,
            "org.pygobject.Test",
            Gio.BusNameOwnerFlags.NONE,
            self.on_bus_acquired,
            on_name_acquired,
            self.on_name_lost,
        )
        try:
            self.loop.run()
        finally:
            Gio.bus_unown_name(owner_id)
            if self.reg_id:
                bus.unregister_object(self.reg_id)

        return (self.invocation, self.errors)

    def on_name_lost(self, _bus, name):
        self.errors.append(f"Name {name} lost")
        self.loop.quit()

    def on_bus_acquired(self, bus, name):
        interface_xml = """
            <node>
                <interface name='org.pygobject.Test'>
                    <method name='test' />
                </interface>
            </node>"""
        self.reg_id = bus.register_object(
            "/pygobject/Test",
            Gio.DBusNodeInfo.new_for_xml(interface_xml).interfaces[0],
            self.on_incoming_method_call,
            None,
            None,
        )

    def on_incoming_method_call(
        self,
        bus,
        sender,
        object_path,
        interface_name,
        method_name,
        parameters,
        invocation,
    ):
        invocation.return_value(GLib.Variant("()", ()))
        self.invocation = invocation

    def client_call(self, bus):
        def call_done(obj, result):
            try:
                obj.call_finish(result)
            finally:
                self.loop.quit()

        bus.call(
            "org.pygobject.Test",
            "/pygobject/Test",
            "org.pygobject.Test",
            "test",
            parameters=None,
            reply_type=None,
            flags=Gio.DBusCallFlags.NONE,
            timeout_msec=5000,
            callback=call_done,
        )


@unittest.skipUnless(has_dbus, "no dbus running")
class AsyncDBusTests(unittest.TestCase):
    def setUp(self):
        policy = GLibEventLoopPolicy()
        asyncio.set_event_loop_policy(policy)
        self.addCleanup(asyncio.set_event_loop_policy, None)
        self.loop = policy.get_event_loop()
        self.addCleanup(self.loop.close)

    def test_async_bus_get(self):
        async def run():
            bus = await Gio.bus_get(Gio.BusType.SESSION)
            self.assertIsInstance(bus, Gio.DBusConnection)

        self.loop.run_until_complete(run())

    def test_async_proxy(self):
        async def run():
            proxy = await Gio.DBusProxy.new_for_bus(
                Gio.BusType.SESSION,
                Gio.DBusProxyFlags.DO_NOT_LOAD_PROPERTIES,
                None,
                "org.freedesktop.DBus",
                "/org/freedesktop/DBus",
                "org.freedesktop.DBus",
            )
            self.assertIsInstance(proxy, Gio.DBusProxy)

        self.loop.run_until_complete(run())

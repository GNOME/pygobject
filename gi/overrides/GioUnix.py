# Copyright 2025 Simon McVittie
# SPDX-License-Identifier: LGPL-2.1-or-later

from ..module import get_introspection_module
from ..overrides import override

from gi.repository import GLib

GioUnix = get_introspection_module("GioUnix")

__all__ = []

if (GLib.MAJOR_VERSION, GLib.MINOR_VERSION) < (2, 86):
    # In older versions of GLib there was some confusion between the
    # platform-specific classes in GioUnix and their equivalents in Gio,
    # resulting in functions like g_desktop_app_info_get_action_name()
    # being assumed to be a global function that happened to take a
    # Gio.DesktopAppInfo first parameter, instead of being a method on a
    # GioUnix.DesktopAppInfo instance. There are not very many classes and
    # methods in GioUnix, so we can wrap them and provide the intended API.

    @override
    class DesktopAppInfo(GioUnix.DesktopAppInfo):
        def get_action_name(self, action_name):
            return GioUnix.DesktopAppInfo.get_action_name(self, action_name)

        def get_boolean(self, key):
            return GioUnix.DesktopAppInfo.get_boolean(self, key)

        def get_categories(self):
            return GioUnix.DesktopAppInfo.get_categories(self)

        def get_filename(self):
            return GioUnix.DesktopAppInfo.get_filename(self)

        def get_generic_name(self):
            return GioUnix.DesktopAppInfo.get_generic_name(self)

        def get_is_hidden(self):
            return GioUnix.DesktopAppInfo.get_is_hidden(self)

        def get_keywords(self):
            return GioUnix.DesktopAppInfo.get_keywords(self)

        def get_locale_string(self, key):
            return GioUnix.DesktopAppInfo.get_locale_string(self, key)

        def get_nodisplay(self):
            return GioUnix.DesktopAppInfo.get_nodisplay(self)

        def get_show_in(self, desktop_env=None):
            return GioUnix.DesktopAppInfo.get_show_in(self, desktop_env)

        def get_startup_wm_class(self):
            return GioUnix.DesktopAppInfo.get_startup_wm_class(self)

        def get_string(self, key):
            return GioUnix.DesktopAppInfo.get_string(self, key)

        def get_string_list(self, key):
            return GioUnix.DesktopAppInfo.get_string_list(self, key)

        def has_key(self, key):
            return GioUnix.DesktopAppInfo.has_key(self, key)

        def launch_action(self, action_name, launch_context=None):
            GioUnix.DesktopAppInfo.launch_action(
                self,
                action_name,
                launch_context,
            )

        def launch_uris_as_manager(
            self,
            uris,
            launch_context,
            spawn_flags,
            user_setup=None,
            user_setup_data=None,
            pid_callback=None,
            pid_callback_data=None,
        ):
            return GioUnix.DesktopAppInfo.launch_uris_as_manager(
                self,
                uris,
                launch_context,
                spawn_flags,
                user_setup,
                user_setup_data,
                pid_callback,
                pid_callback_data,
            )

        def launch_uris_as_manager_with_fds(
            self,
            uris,
            launch_context,
            spawn_flags,
            user_setup,
            user_setup_data,
            pid_callback,
            pid_callback_data,
            stdin_fd,
            stdout_fd,
            stderr_fd,
        ):
            return GioUnix.DesktopAppInfo.launch_uris_as_manager_with_fds(
                self,
                uris,
                launch_context,
                spawn_flags,
                user_setup,
                user_setup_data,
                pid_callback,
                pid_callback_data,
                stdin_fd,
                stdout_fd,
                stderr_fd,
            )

        def list_actions(self):
            return GioUnix.DesktopAppInfo.list_actions(self)

    __all__.append("DesktopAppInfo")

    @override
    class FDMessage(GioUnix.FDMessage):
        def append_fd(self, fd):
            return GioUnix.FDMessage.append_fd(self, fd)

        def get_fd_list(self):
            return GioUnix.FDMessage.get_fd_list(self)

        def steal_fds(self):
            return GioUnix.FDMessage.steal_fds(self)

    __all__.append("FDMessage")

    @override
    class InputStream(GioUnix.InputStream):
        def get_close_fd(self):
            return GioUnix.InputStream.get_close_fd(self)

        def get_fd(self):
            return GioUnix.InputStream.get_fd(self)

        def set_close_fd(self, close_fd):
            GioUnix.InputStream.set_close_fd(self, close_fd)

    __all__.append("InputStream")

    @override
    class MountMonitor(GioUnix.MountMonitor):
        def set_rate_limit(self, limit_msec):
            GioUnix.MountMonitor.set_rate_limit(limit_msec)

    __all__.append("MountMonitor")

    @override
    class OutputStream(GioUnix.OutputStream):
        def get_close_fd(self):
            return GioUnix.OutputStream.get_close_fd(self)

        def get_fd(self):
            return GioUnix.OutputStream.get_fd(self)

        def set_close_fd(self, close_fd):
            GioUnix.OutputStream.set_close_fd(self, close_fd)

    __all__.append("OutputStream")

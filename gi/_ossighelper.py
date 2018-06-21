# -*- coding: utf-8 -*-
# Copyright 2017 Christoph Reiter
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

from __future__ import print_function

import os
import sys
import socket
import signal
import threading
from contextlib import closing, contextmanager

from . import _gi


def ensure_socket_not_inheritable(sock):
    """Ensures that the socket is not inherited by child processes

    Raises:
        EnvironmentError
        NotImplementedError: With Python <3.4 on Windows
    """

    if hasattr(sock, "set_inheritable"):
        sock.set_inheritable(False)
    else:
        try:
            import fcntl
        except ImportError:
            raise NotImplementedError(
                "Not implemented for older Python on Windows")
        else:
            fd = sock.fileno()
            flags = fcntl.fcntl(fd, fcntl.F_GETFD)
            fcntl.fcntl(fd, fcntl.F_SETFD, flags | fcntl.FD_CLOEXEC)


_wakeup_fd_is_active = False
"""Since we can't check if set_wakeup_fd() is already used for nested event
loops without introducing a race condition we keep track of it globally.
"""


@contextmanager
def wakeup_on_signal():
    """A decorator for functions which create a glib event loop to keep
    Python signal handlers working while the event loop is idling.

    In case an OS signal is received will wake the default event loop up
    shortly so that any registered Python signal handlers registered through
    signal.signal() can run.

    Works on Windows but needs Python 3.5+.

    In case the wrapped function is not called from the main thread it will be
    called as is and it will not wake up the default loop for signals.
    """

    global _wakeup_fd_is_active

    if _wakeup_fd_is_active:
        yield
        return

    from gi.repository import GLib

    # On Windows only Python 3.5+ supports passing sockets to set_wakeup_fd
    set_wakeup_fd_supports_socket = (
        os.name != "nt" or sys.version_info[:2] >= (3, 5))
    # On Windows only Python 3 has an implementation of socketpair()
    has_socketpair = hasattr(socket, "socketpair")

    if not has_socketpair or not set_wakeup_fd_supports_socket:
        yield
        return

    read_socket, write_socket = socket.socketpair()
    with closing(read_socket), closing(write_socket):

        for sock in [read_socket, write_socket]:
            sock.setblocking(False)
            ensure_socket_not_inheritable(sock)

        try:
            orig_fd = signal.set_wakeup_fd(write_socket.fileno())
        except ValueError:
            # Raised in case this is not the main thread -> give up.
            yield
            return
        else:
            _wakeup_fd_is_active = True

        def signal_notify(source, condition):
            if condition & GLib.IO_IN:
                try:
                    return bool(read_socket.recv(1))
                except EnvironmentError as e:
                    print(e)
                    return False
                return True
            else:
                return False

        try:
            if os.name == "nt":
                channel = GLib.IOChannel.win32_new_socket(
                    read_socket.fileno())
            else:
                channel = GLib.IOChannel.unix_new(read_socket.fileno())

            source_id = GLib.io_add_watch(
                channel,
                GLib.PRIORITY_DEFAULT,
                (GLib.IOCondition.IN | GLib.IOCondition.HUP |
                 GLib.IOCondition.NVAL | GLib.IOCondition.ERR),
                signal_notify)
            try:
                yield
            finally:
                GLib.source_remove(source_id)
        finally:
            write_fd = signal.set_wakeup_fd(orig_fd)
            if write_fd != write_socket.fileno():
                # Someone has called set_wakeup_fd while func() was active,
                # so let's re-revert again.
                signal.set_wakeup_fd(write_fd)
            _wakeup_fd_is_active = False


PyOS_getsig = _gi.pyos_getsig

# We save the signal pointer so we can detect if glib has changed the
# signal handler behind Python's back (GLib.unix_signal_add)
if signal.getsignal(signal.SIGINT) is signal.default_int_handler:
    _startup_sigint_ptr = PyOS_getsig(signal.SIGINT)
else:
    # Something has set the handler before import, we can't get a ptr
    # for the default handler so make sure the pointer will never match.
    _startup_sigint_ptr = -1


_callback_stack = [signal.default_int_handler]
_sigint_called = False


def _sigint_handler(sig_num, frame):
    global _callback_stack, _sigint_called, _startup_sigint_ptr

    # Only execute if no changes to the SIGINT handler
    if PyOS_getsig(signal.SIGINT) == _startup_sigint_ptr:
        _sigint_called = True
        _callback_stack.pop()()
    else:
        signal.default_int_handler(sig_num, frame)


def is_main_thread():
    """Returns True in case the function is called from the main thread"""

    return threading.current_thread().name == "MainThread"


@contextmanager
def register_sigint_fallback(callback):
    """Installs a SIGINT signal handler in case the default
    Python one is active.
    Also, adds 'callback' to the SIGINT handler callback stack.

    Only does something if called from the main thread.

    In case of nested context managers the signal handler will be only
    installed once and the callbacks on the stack will be called in the
    reverse order of their registration.

    The old signal handler will be restored in case no signal handler is
    registered while the context is active.
    """

    # To handle multiple levels of event loops we need to call the last
    # callback first, wait until the innermost event loop returns control
    # and only then call the next callback, and so on... until we
    # reach the outer most which manages the signal handler and raises
    # in the end

    global _callback_stack, _sigint_called

    assert not _sigint_called

    if not is_main_thread():
        yield
        return

    _callback_stack.append(callback)
    install = signal.getsignal(signal.SIGINT) is signal.default_int_handler \
        and PyOS_getsig(signal.SIGINT) == _startup_sigint_ptr
    if install:
        signal.signal(signal.SIGINT, _sigint_handler)
    try:
        yield
    finally:
        if install and signal.getsignal(signal.SIGINT) is _sigint_handler:
            signal.signal(signal.SIGINT, signal.default_int_handler)

        # If _sigint_called is True, then this is the callback of the outer context
        # If _sigint_called is False, then this is the callback of the current context
        next_callback = _callback_stack.pop()
        assert _sigint_called or callback == next_callback
        if _sigint_called:
            if not _callback_stack:  # Handling is over, revert to initial setting
                _callback_stack = [signal.default_int_handler]
                _sigint_called = False
            next_callback()

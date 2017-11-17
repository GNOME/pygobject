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

import os
import signal
import unittest
import threading
from contextlib import contextmanager

from gi.repository import Gtk, Gio, GLib
from gi._ossighelper import wakeup_on_signal


class TestOverridesWakeupOnAlarm(unittest.TestCase):

    @contextmanager
    def _run_with_timeout(self, timeout, abort_func):
        failed = []

        def fail():
            abort_func()
            failed.append(1)
            return True

        fail_id = GLib.timeout_add(timeout, fail)
        try:
            yield
        finally:
            GLib.source_remove(fail_id)
        self.assertFalse(failed)

    def test_basic(self):
        self.assertEqual(signal.set_wakeup_fd(-1), -1)
        with wakeup_on_signal():
            pass
        self.assertEqual(signal.set_wakeup_fd(-1), -1)

    def test_in_thread(self):
        failed = []

        def target():
            try:
                with wakeup_on_signal():
                    pass
            except:
                failed.append(1)

        t = threading.Thread(target=target)
        t.start()
        t.join(5)
        self.assertFalse(failed)

    @unittest.skipIf(os.name == "nt", "not on Windows")
    def test_glib_mainloop(self):
        loop = GLib.MainLoop()
        signal.signal(signal.SIGALRM, lambda *args: loop.quit())
        GLib.idle_add(signal.setitimer, signal.ITIMER_REAL, 0.001)

        with self._run_with_timeout(2000, loop.quit):
            loop.run()

    @unittest.skipIf(os.name == "nt", "not on Windows")
    def test_gio_application(self):
        app = Gio.Application()
        signal.signal(signal.SIGALRM, lambda *args: app.quit())
        GLib.idle_add(signal.setitimer, signal.ITIMER_REAL, 0.001)

        with self._run_with_timeout(2000, app.quit):
            app.hold()
            app.connect("activate", lambda *args: None)
            app.run()

    @unittest.skipIf(os.name == "nt", "not on Windows")
    def test_gtk_main(self):
        signal.signal(signal.SIGALRM, lambda *args: Gtk.main_quit())
        GLib.idle_add(signal.setitimer, signal.ITIMER_REAL, 0.001)

        with self._run_with_timeout(2000, Gtk.main_quit):
            Gtk.main()

    @unittest.skipIf(os.name == "nt", "not on Windows")
    def test_gtk_dialog_run(self):
        w = Gtk.Window()
        d = Gtk.Dialog(transient_for=w)
        signal.signal(signal.SIGALRM, lambda *args: d.destroy())
        GLib.idle_add(signal.setitimer, signal.ITIMER_REAL, 0.001)

        with self._run_with_timeout(2000, d.destroy):
            d.run()

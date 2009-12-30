# -*- Mode: Python -*-

import os
import unittest

from common import gio, glib, gobject


class TestSocket(unittest.TestCase):
    def setUp(self):
        self.sock = gio.Socket(gio.SOCKET_FAMILY_IPV4,
                               gio.SOCKET_TYPE_STREAM,
                               gio.SOCKET_PROTOCOL_TCP)

    def test_socket_condition(self):
        check = self.sock.condition_check(glib.IO_OUT)
        self.failUnless(isinstance(check, gobject.GFlags))
        self.failUnlessEqual(check, glib.IO_OUT | glib.IO_HUP)

    def tearDown(self):
        self.sock.close()

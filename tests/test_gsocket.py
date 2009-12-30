# -*- Mode: Python -*-

import os
import unittest

from common import gio, glib, gobject


class TestSocket(unittest.TestCase):
    def setUp(self):
        self.sock = gio.Socket(gio.SOCKET_FAMILY_IPV4,
                               gio.SOCKET_TYPE_STREAM,
                               gio.SOCKET_PROTOCOL_TCP)

    def test_socket_condition_check(self):
        check = self.sock.condition_check(glib.IO_OUT)
        self.failUnless(isinstance(check, gobject.GFlags))
        self.failUnlessEqual(check, glib.IO_OUT | glib.IO_HUP)

    def test_socket_condition_wait(self):
        res = self.sock.condition_wait(glib.IO_OUT)
        self.failUnless(res)

    def tearDown(self):
        self.sock.close()

class TestSocketAddress(unittest.TestCase):
    def test_socket_address_enumerator_next_async(self):
        def callback(enumerator, result):
            try:
                address = enumerator.next_finish(result)
                self.failUnless(isinstance(address, gio.SocketAddress))
            finally:
                loop.quit()

        socket = gio.NetworkAddress("www.pygtk.org", 80)
        enumerator = socket.enumerate()
        enumerator.next_async(callback)

        loop = glib.MainLoop()
        loop.run()

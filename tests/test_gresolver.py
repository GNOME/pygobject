# -*- Mode: Python -*-

import os
import unittest

from common import gio


class TestResolver(unittest.TestCase):
    def setUp(self):
        self.resolver = gio.resolver_get_default()

    def test_resolver_lookup_by_name(self):
        addresses = self.resolver.lookup_by_name("pygtk.org", cancellable=None)
        self.failUnless(isinstance(addresses[0], gio.InetAddress))

    def test_resolver_lookup_by_address(self):
        address = gio.inet_address_new_from_string("8.8.8.8")
        dns = self.resolver.lookup_by_address(address, cancellable=None)
        self.failUnlessEqual(dns, "google-public-dns-a.google.com")
        

# -*- Mode: Python -*-

import os
import unittest

from common import gio, glib


class TestResolver(unittest.TestCase):
    def setUp(self):
        self.cancellable = gio.Cancellable()
    
    def test_make_poll_fd(self):
        poll = self.cancellable.make_pollfd()
        self.failUnless(isinstance(poll, glib.PollFD))

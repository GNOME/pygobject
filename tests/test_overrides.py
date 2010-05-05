# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import pygtk
pygtk.require("2.0")

import unittest
import gobject

import sys
sys.path.insert(0, "../")

from gi.repository import Gdk

class TestGdk(unittest.TestCase):

    def test_color(self):
        color = Gdk.Color(100, 200, 300)
        self.assertEquals(color.r, 100)
        self.assertEquals(color.g, 200)
        self.assertEquals(color.b, 300)


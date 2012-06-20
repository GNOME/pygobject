# -*- Mode: Python -*-

import unittest
import os.path

from gi.repository import GLib


class TestGLib(unittest.TestCase):
    def test_find_program_in_path(self):
        bash_path = GLib.find_program_in_path('bash')
        self.assertTrue(bash_path.endswith('/bash'))
        self.assertTrue(os.path.exists(bash_path))

        self.assertEqual(GLib.find_program_in_path('non existing'), None)

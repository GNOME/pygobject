# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest

import gi.overrides
try:
    from gi.repository import Regress
    Regress  # pyflakes
except ImportError:
    Regress = None


class TestRegistry(unittest.TestCase):
    def test_non_gi(self):
        class MyClass:
            pass

        try:
            gi.overrides.override(MyClass)
            self.fail('unexpected success of overriding non-GI class')
        except TypeError as e:
            self.assertTrue('Can not override a type MyClass' in str(e))

    @unittest.skipUnless(Regress, 'built without cairo support')
    def test_separate_path(self):
        # Regress override is in tests/gi/overrides, separate from gi/overrides
        # https://bugzilla.gnome.org/show_bug.cgi?id=680913
        self.assertEqual(Regress.REGRESS_OVERRIDE, 42)

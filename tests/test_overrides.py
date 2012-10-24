# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest

import gi.overrides
import gi.module

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


class TestModule(unittest.TestCase):
    # Tests for gi.module

    def test_get_introspection_module_caching(self):
        # This test attempts to minimize side effects by
        # using a DynamicModule directly instead of going though:
        # from gi.repository import Foo

        # Clear out introspection module cache before running this test.
        old_modules = gi.module._introspection_modules
        gi.module._introspection_modules = {}

        mod_name = 'GIMarshallingTests'
        mod1 = gi.module.get_introspection_module(mod_name)
        mod2 = gi.module.get_introspection_module(mod_name)
        self.assertTrue(mod1 is mod2)

        # Using a DynamicModule will use get_introspection_module internally
        # in its _load method.
        mod_overridden = gi.module.DynamicModule(mod_name)
        mod_overridden._load()
        self.assertTrue(mod1 is mod_overridden._introspection_module)

        # Restore the previous cache
        gi.module._introspection_modules = old_modules

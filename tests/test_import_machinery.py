# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

from __future__ import absolute_import

import sys
import unittest

import gi.overrides
import gi.module
import gi.importer
from gi._compat import PY2, PY3

from gi.repository import Regress


class TestOverrides(unittest.TestCase):

    def test_non_gi(self):
        class MyClass:
            pass

        try:
            gi.overrides.override(MyClass)
            self.fail('unexpected success of overriding non-GI class')
        except TypeError as e:
            self.assertTrue('Can not override a type MyClass' in str(e))

    def test_separate_path(self):
        # Regress override is in tests/gi/overrides, separate from gi/overrides
        # https://bugzilla.gnome.org/show_bug.cgi?id=680913
        self.assertEqual(Regress.REGRESS_OVERRIDE, 42)

    def test_load_overrides(self):
        mod = gi.module.get_introspection_module('GIMarshallingTests')
        mod_override = gi.overrides.load_overrides(mod)
        self.assertTrue(mod_override is not mod)
        self.assertTrue(mod_override._introspection_module is mod)
        self.assertEqual(mod_override.OVERRIDES_CONSTANT, 7)
        self.assertEqual(mod.OVERRIDES_CONSTANT, 42)

    def test_load_no_overrides(self):
        mod_key = "gi.overrides.GIMarshallingTests"
        had_mod = mod_key in sys.modules
        old_mod = sys.modules.get(mod_key)
        try:
            # this makes override import fail
            sys.modules[mod_key] = None
            mod = gi.module.get_introspection_module('GIMarshallingTests')
            mod_override = gi.overrides.load_overrides(mod)
            self.assertTrue(mod_override is mod)
        finally:
            del sys.modules[mod_key]
            if had_mod:
                sys.modules[mod_key] = old_mod


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

        # Restore the previous cache
        gi.module._introspection_modules = old_modules

    def test_module_dependency_loading(self):
        # Difficult to because this generally need to run in isolation to make
        # sure GIMarshallingTests has not yet been loaded. But we can do this with:
        #  make check TEST_NAMES=test_import_machinery.TestModule.test_module_dependency_loading
        if 'gi.repository.Gio' in sys.modules:
            return

        from gi.repository import GIMarshallingTests
        GIMarshallingTests  # PyFlakes

        self.assertIn('gi.repository.Gio', sys.modules)
        self.assertIn('gi.repository.GIMarshallingTests', sys.modules)

    def test_static_binding_protection(self):
        # Importing old static bindings once gi has been imported should not
        # crash but instead give back a dummy module which produces RuntimeErrors
        # on access.
        with self.assertRaises(AttributeError):
            import gobject
            gobject.anything

        with self.assertRaises(AttributeError):
            import glib
            glib.anything

        with self.assertRaises(AttributeError):
            import gio
            gio.anything

        with self.assertRaises(AttributeError):
            import gtk
            gtk.anything

        with self.assertRaises(AttributeError):
            import gtk.gdk
            gtk.gdk.anything


class TestImporter(unittest.TestCase):
    def test_invalid_repository_module_name(self):
        with self.assertRaises(ImportError) as context:
            from gi.repository import InvalidGObjectRepositoryModuleName
            InvalidGObjectRepositoryModuleName  # pyflakes

        exception_string = str(context.exception)

        self.assertTrue('InvalidGObjectRepositoryModuleName' in exception_string)

        # The message of the custom exception in gi/importer.py is eaten in Python <3.3
        if sys.version_info >= (3, 3):
            self.assertTrue('introspection typelib' in exception_string)

    def test_require_version_warning(self):
        check = gi.importer._check_require_version

        # make sure it doesn't fail at least
        with check("GLib", 1):
            from gi.repository import GLib
            GLib

        # make sure the exception propagates
        with self.assertRaises(ImportError):
            with check("InvalidGObjectRepositoryModuleName", 1):
                from gi.repository import InvalidGObjectRepositoryModuleName
                InvalidGObjectRepositoryModuleName

    def test_require_version_versiontype(self):
        import gi
        with self.assertRaises(ValueError):
            gi.require_version('GLib', 2.0)

        # Test that unicode strings work in python 2
        if PY2:
            gi.require_version('GLib', u'2.0')

        if PY3:
            with self.assertRaises(ValueError):
                gi.require_version('GLib', b'2.0')

    def test_require_versions(self):
        import gi
        gi.require_versions({'GLib': '2.0', 'Gio': '2.0', 'GObject': '2.0'})
        from gi.repository import GLib
        GLib

    def test_get_import_stacklevel(self):
        gi.importer.get_import_stacklevel(import_hook=True)
        gi.importer.get_import_stacklevel(import_hook=False)

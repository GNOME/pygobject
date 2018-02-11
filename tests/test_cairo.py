# -*- Mode: Python; py-indent-offset: 4 -*-
# coding=utf-8
# vim: tabstop=4 shiftwidth=4 expandtab

from __future__ import absolute_import

import unittest

import gi

try:
    gi.require_foreign('cairo')
    import cairo
    has_cairo = True
except ImportError:
    has_cairo = False

has_region = has_cairo and hasattr(cairo, "Region")

try:
    from gi.repository import Gtk, Gdk
    Gtk, Gdk  # pyflakes
except:
    Gtk = None
    Gdk = None

from gi.repository import GObject, Regress


@unittest.skipUnless(has_cairo, 'built without cairo support')
class Test(unittest.TestCase):
    def test_cairo_context(self):
        context = Regress.test_cairo_context_full_return()
        self.assertTrue(isinstance(context, cairo.Context))

        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        context = cairo.Context(surface)
        Regress.test_cairo_context_none_in(context)

    def test_cairo_surface(self):
        surface = Regress.test_cairo_surface_none_return()
        self.assertTrue(isinstance(surface, cairo.ImageSurface))
        self.assertTrue(isinstance(surface, cairo.Surface))
        self.assertEqual(surface.get_format(), cairo.FORMAT_ARGB32)
        self.assertEqual(surface.get_width(), 10)
        self.assertEqual(surface.get_height(), 10)

        surface = Regress.test_cairo_surface_full_return()
        self.assertTrue(isinstance(surface, cairo.ImageSurface))
        self.assertTrue(isinstance(surface, cairo.Surface))
        self.assertEqual(surface.get_format(), cairo.FORMAT_ARGB32)
        self.assertEqual(surface.get_width(), 10)
        self.assertEqual(surface.get_height(), 10)

        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        Regress.test_cairo_surface_none_in(surface)

        surface = Regress.test_cairo_surface_full_out()
        self.assertTrue(isinstance(surface, cairo.ImageSurface))
        self.assertTrue(isinstance(surface, cairo.Surface))
        self.assertEqual(surface.get_format(), cairo.FORMAT_ARGB32)
        self.assertEqual(surface.get_width(), 10)
        self.assertEqual(surface.get_height(), 10)

    def test_require_foreign(self):
        self.assertEqual(gi.require_foreign('cairo'), None)
        self.assertEqual(gi.require_foreign('cairo', 'Context'), None)
        self.assertRaises(ImportError, gi.require_foreign, 'invalid_module')
        self.assertRaises(ImportError, gi.require_foreign, 'invalid_module', 'invalid_symbol')
        self.assertRaises(ImportError, gi.require_foreign, 'cairo', 'invalid_symbol')


@unittest.skipUnless(has_cairo, 'built without cairo support')
@unittest.skipUnless(has_region, 'built without cairo.Region support')
@unittest.skipUnless(Gdk, 'Gdk not available')
class TestRegion(unittest.TestCase):

    def test_region_to_py(self):
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        context = cairo.Context(surface)
        context.paint()
        region = Gdk.cairo_region_create_from_surface(surface)
        r = region.get_extents()
        self.assertEqual((r.height, r.width), (10, 10))

    def test_region_from_py(self):
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        context = cairo.Context(surface)
        region = cairo.Region(cairo.RectangleInt(0, 0, 42, 42))
        Gdk.cairo_region(context, region)
        self.assertTrue("42" in repr(list(context.copy_path())))


@unittest.skipUnless(has_cairo, 'built without cairo support')
@unittest.skipUnless(Gtk, 'Gtk not available')
class TestPango(unittest.TestCase):
    def test_cairo_font_options(self):
        screen = Gtk.Window().get_screen()
        font_opts = screen.get_font_options()
        self.assertTrue(isinstance(font_opts.get_subpixel_order(), int))


if has_cairo:
    from gi.repository import cairo as CairoGObject

    # Use PyGI signals to test non-introspected foreign marshaling.
    class CairoSignalTester(GObject.Object):
        sig_context = GObject.Signal(arg_types=[CairoGObject.Context])
        sig_surface = GObject.Signal(arg_types=[CairoGObject.Surface])
        sig_font_face = GObject.Signal(arg_types=[CairoGObject.FontFace])
        sig_scaled_font = GObject.Signal(arg_types=[CairoGObject.ScaledFont])
        sig_pattern = GObject.Signal(arg_types=[CairoGObject.Pattern])


@unittest.skipUnless(has_cairo, 'built without cairo support')
class TestSignalMarshaling(unittest.TestCase):
    # Tests round tripping of cairo objects through non-introspected signals.

    def setUp(self):
        self.surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        self.context = cairo.Context(self.surface)
        self.tester = CairoSignalTester()

    def pass_object_through_signal(self, obj, signal):
        """Pass the given `obj` through the `signal` emission storing the
        `obj` passed through the signal and returning it."""
        passthrough_result = []

        def callback(instance, passthrough):
            passthrough_result.append(passthrough)

        signal.connect(callback)
        signal.emit(obj)

        return passthrough_result[0]

    def test_context(self):
        result = self.pass_object_through_signal(self.context, self.tester.sig_context)
        self.assertTrue(isinstance(result, cairo.Context))

    def test_surface(self):
        result = self.pass_object_through_signal(self.surface, self.tester.sig_surface)
        self.assertTrue(isinstance(result, cairo.Surface))

    def test_font_face(self):
        font_face = self.context.get_font_face()
        result = self.pass_object_through_signal(font_face, self.tester.sig_font_face)
        self.assertTrue(isinstance(result, cairo.FontFace))

    def test_scaled_font(self):
        scaled_font = cairo.ScaledFont(self.context.get_font_face(),
                                       cairo.Matrix(),
                                       cairo.Matrix(),
                                       self.context.get_font_options())
        result = self.pass_object_through_signal(scaled_font, self.tester.sig_scaled_font)
        self.assertTrue(isinstance(result, cairo.ScaledFont))

    def test_pattern(self):
        pattern = cairo.SolidPattern(1, 1, 1, 1)
        result = self.pass_object_through_signal(pattern, self.tester.sig_pattern)
        self.assertTrue(isinstance(result, cairo.Pattern))
        self.assertTrue(isinstance(result, cairo.SolidPattern))

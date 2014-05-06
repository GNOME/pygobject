# -*- Mode: Python; py-indent-offset: 4 -*-
# coding=utf-8
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest

try:
    import cairo
    from gi.repository import Regress
    has_cairo = True
except ImportError:
    has_cairo = False

try:
    from gi.repository import Gtk
    Gtk  # pyflakes
except:
    Gtk = None


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


@unittest.skipUnless(has_cairo, 'built without cairo support')
@unittest.skipUnless(Gtk, 'Gtk not available')
class TestPango(unittest.TestCase):
    def test_cairo_font_options(self):
        screen = Gtk.Window().get_screen()
        font_opts = screen.get_font_options()
        self.assertEqual(type(font_opts.get_subpixel_order()), int)


if __name__ == '__main__':
    unittest.main()

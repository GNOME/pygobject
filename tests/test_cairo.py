import unittest
import pytest

import gi

try:
    gi.require_foreign("cairo")
    import cairo

    has_cairo = True
except ImportError:
    has_cairo = False

has_region = has_cairo and hasattr(cairo, "Region")

try:
    from gi.repository import Gtk, Gdk

    Gtk, Gdk
except ImportError:
    Gtk = None
    Gdk = None

from gi.repository import GObject, Regress


@unittest.skipUnless(has_cairo, "built without cairo support")
class Test(unittest.TestCase):
    def test_gvalue_converters(self):
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        context = cairo.Context(surface)
        matrix = cairo.Matrix()
        objects = {
            "CairoContext": context,
            "CairoSurface": surface,
            "CairoFontFace": context.get_font_face(),
            "CairoScaledFont": context.get_scaled_font(),
            "CairoPattern": context.get_source(),
            "CairoMatrix": matrix,
        }
        for type_name, cairo_obj in objects.items():
            gtype = GObject.type_from_name(type_name)
            v = GObject.Value()
            assert v.init(gtype) is None
            assert v.get_value() is None
            v.set_value(None)
            assert v.get_value() is None
            v.set_value(cairo_obj)
            assert v.get_value() == cairo_obj

    def test_cairo_context(self):
        context = Regress.test_cairo_context_full_return()
        self.assertTrue(isinstance(context, cairo.Context))

        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        context = cairo.Context(surface)
        Regress.test_cairo_context_none_in(context)

    def test_cairo_context_full_in(self):
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        context = cairo.Context(surface)
        Regress.test_cairo_context_full_in(context)

        with pytest.raises(TypeError):
            Regress.test_cairo_context_full_in(object())

    def test_cairo_context_none_return(self):
        context = Regress.test_cairo_context_none_return()
        self.assertTrue(isinstance(context, cairo.Context))

    def test_cairo_path_full_return(self):
        path = Regress.test_cairo_path_full_return()
        if hasattr(cairo, "Path"):  # pycairo 1.15.1+
            assert isinstance(path, cairo.Path)

    def test_cairo_path_none_in(self):
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        context = cairo.Context(surface)
        path = context.copy_path()
        Regress.test_cairo_path_none_in(path)
        surface.finish()

        with pytest.raises(TypeError):
            Regress.test_cairo_path_none_in(object())

    def test_cairo_path_full_in_full_return(self):
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        context = cairo.Context(surface)
        context.move_to(10, 10)
        context.curve_to(10, 10, 3, 4, 5, 6)
        path = context.copy_path()
        new_path = Regress.test_cairo_path_full_in_full_return(path)
        assert list(path) == list(new_path)
        surface.finish()

    def test_cairo_font_options_full_return(self):
        options = Regress.test_cairo_font_options_full_return()
        assert isinstance(options, cairo.FontOptions)

    def test_cairo_font_options_none_return(self):
        options = Regress.test_cairo_font_options_none_return()
        assert isinstance(options, cairo.FontOptions)

    def test_cairo_font_options_full_in(self):
        options = cairo.FontOptions()
        Regress.test_cairo_font_options_full_in(options)

        with pytest.raises(TypeError):
            Regress.test_cairo_font_options_full_in(object())

    def test_cairo_font_options_none_in(self):
        options = cairo.FontOptions()
        Regress.test_cairo_font_options_none_in(options)

    def test_cairo_font_face_full_return(self):
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        context = cairo.Context(surface)
        font_face = Regress.test_cairo_font_face_full_return(context)

        assert font_face

    def test_cairo_scaled_font_full_return(self):
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        context = cairo.Context(surface)
        scaled_font = Regress.test_cairo_scaled_font_full_return(context)

        assert scaled_font

    def test_cairo_pattern_full_in(self):
        pattern = cairo.SolidPattern(1, 1, 1, 1)
        Regress.test_cairo_pattern_full_in(pattern)

        with pytest.raises(TypeError):
            Regress.test_cairo_pattern_full_in(object())

    def test_cairo_pattern_none_in(self):
        pattern = cairo.SolidPattern(1, 1, 1, 1)
        Regress.test_cairo_pattern_none_in(pattern)

    def test_cairo_pattern_full_return(self):
        pattern = Regress.test_cairo_pattern_full_return()
        self.assertTrue(isinstance(pattern, cairo.Pattern))
        self.assertTrue(isinstance(pattern, cairo.SolidPattern))

    def test_cairo_pattern_none_return(self):
        pattern = Regress.test_cairo_pattern_none_return()
        self.assertTrue(isinstance(pattern, cairo.Pattern))
        self.assertTrue(isinstance(pattern, cairo.SolidPattern))

    def test_cairo_region_full_in(self):
        region = cairo.Region()
        Regress.test_cairo_region_full_in(region)

        with pytest.raises(TypeError):
            Regress.test_cairo_region_full_in(object())

    def test_cairo_matrix_none_in(self):
        matrix = cairo.Matrix()
        Regress.test_cairo_matrix_none_in(matrix)

        with pytest.raises(TypeError):
            Regress.test_cairo_matrix_none_in(object())

    def test_cairo_matrix_none_return(self):
        matrix = Regress.test_cairo_matrix_none_return()
        assert matrix == cairo.Matrix()

    def test_cairo_matrix_out_caller_allocates(self):
        matrix = Regress.test_cairo_matrix_out_caller_allocates()
        assert matrix == cairo.Matrix()

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

    def test_cairo_surface_full_in(self):
        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        Regress.test_cairo_surface_full_in(surface)

        with pytest.raises(TypeError):
            Regress.test_cairo_surface_full_in(object())

    def test_require_foreign(self):
        self.assertEqual(gi.require_foreign("cairo"), None)
        self.assertEqual(gi.require_foreign("cairo", "Context"), None)
        self.assertRaises(ImportError, gi.require_foreign, "invalid_module")
        self.assertRaises(
            ImportError, gi.require_foreign, "invalid_module", "invalid_symbol"
        )
        self.assertRaises(ImportError, gi.require_foreign, "cairo", "invalid_symbol")


@unittest.skipUnless(has_cairo, "built without cairo support")
@unittest.skipUnless(has_region, "built without cairo.Region support")
@unittest.skipUnless(Gdk, "Gdk not available")
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


@unittest.skipUnless(has_cairo, "built without cairo support")
@unittest.skipUnless(Gtk, "Gtk not available")
class TestPango(unittest.TestCase):
    def test_cairo_font_options(self):
        window = Gtk.Window()
        if Gtk._version == "4.0":
            window.set_font_options(cairo.FontOptions())
            font_opts = window.get_font_options()
        else:
            screen = window.get_screen()
            font_opts = screen.get_font_options()
        assert font_opts is not None
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


@unittest.skipUnless(has_cairo, "built without cairo support")
class TestSignalMarshaling(unittest.TestCase):
    # Tests round tripping of cairo objects through non-introspected signals.

    def setUp(self):
        self.surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        self.context = cairo.Context(self.surface)
        self.tester = CairoSignalTester()

    def pass_object_through_signal(self, obj, signal):
        """Pass the given `obj` through the `signal` emission storing the
        `obj` passed through the signal and returning it.
        """
        passthrough_result = []

        def callback(instance, passthrough):
            passthrough_result.append(passthrough)

        signal.connect(callback)
        signal.emit(obj)

        return passthrough_result[0]

    def test_context(self):
        result = self.pass_object_through_signal(self.context, self.tester.sig_context)
        self.assertTrue(isinstance(result, cairo.Context))

        with pytest.raises(TypeError):
            self.pass_object_through_signal(object(), self.tester.sig_context)

    def test_surface(self):
        result = self.pass_object_through_signal(self.surface, self.tester.sig_surface)
        self.assertTrue(isinstance(result, cairo.Surface))

    def test_font_face(self):
        font_face = self.context.get_font_face()
        result = self.pass_object_through_signal(font_face, self.tester.sig_font_face)
        self.assertTrue(isinstance(result, cairo.FontFace))

        with pytest.raises(TypeError):
            self.pass_object_through_signal(object(), self.tester.sig_font_face)

    def test_scaled_font(self):
        scaled_font = cairo.ScaledFont(
            self.context.get_font_face(),
            cairo.Matrix(),
            cairo.Matrix(),
            self.context.get_font_options(),
        )
        result = self.pass_object_through_signal(
            scaled_font, self.tester.sig_scaled_font
        )
        self.assertTrue(isinstance(result, cairo.ScaledFont))

        with pytest.raises(TypeError):
            result = self.pass_object_through_signal(
                object(), self.tester.sig_scaled_font
            )

    def test_pattern(self):
        pattern = cairo.SolidPattern(1, 1, 1, 1)
        result = self.pass_object_through_signal(pattern, self.tester.sig_pattern)
        self.assertTrue(isinstance(result, cairo.Pattern))
        self.assertTrue(isinstance(result, cairo.SolidPattern))

        with pytest.raises(TypeError):
            result = self.pass_object_through_signal(object(), self.tester.sig_pattern)

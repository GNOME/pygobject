import re
import os
import sys
import unittest
import pytest

import gi
import gi.overrides
from gi import PyGIDeprecationWarning

try:
    from gi.repository import Gio, Gdk, GdkPixbuf, Gtk

    GDK4 = Gdk._version == "4.0"
except ImportError:
    Gdk = None
    Gtk = None
    GDK4 = False


def gtkver():
    if Gtk is None:
        return (0, 0, 0)
    return (Gtk.get_major_version(), Gtk.get_minor_version(), Gtk.get_micro_version())


try:
    gi.require_foreign("cairo")
    has_cairo = True
except ImportError:
    has_cairo = False

from .helper import capture_glib_deprecation_warnings


@unittest.skipUnless(Gdk, "Gdk not available")
class TestGdk(unittest.TestCase):
    @unittest.skipIf(sys.platform == "darwin" or os.name == "nt", "crashes")
    @unittest.skipIf(GDK4, "not in gdk4")
    def test_constructor(self):
        attribute = Gdk.WindowAttr()
        attribute.window_type = Gdk.WindowType.CHILD
        attributes_mask = Gdk.WindowAttributesType.X | Gdk.WindowAttributesType.Y
        window = Gdk.Window(None, attribute, attributes_mask)
        self.assertEqual(window.get_window_type(), Gdk.WindowType.CHILD)

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_color(self):
        color = Gdk.Color(100, 200, 300)
        self.assertEqual(color.red, 100)
        self.assertEqual(color.green, 200)
        self.assertEqual(color.blue, 300)
        with capture_glib_deprecation_warnings():
            self.assertEqual(color, Gdk.Color(100, 200, 300))
        self.assertNotEqual(color, Gdk.Color(1, 2, 3))
        self.assertNotEqual(color, None)
        # assertNotEqual only tests __ne__. Following line explicitly
        # tests __eq__ with objects of other types
        self.assertFalse(color == object())

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_color_floats(self):
        self.assertEqual(
            Gdk.Color(13107, 21845, 65535), Gdk.Color.from_floats(0.2, 1.0 / 3.0, 1.0)
        )

        self.assertEqual(
            Gdk.Color(13107, 21845, 65535).to_floats(), (0.2, 1.0 / 3.0, 1.0)
        )

        self.assertEqual(
            Gdk.RGBA(0.2, 1.0 / 3.0, 1.0, 0.5).to_color(),
            Gdk.Color.from_floats(0.2, 1.0 / 3.0, 1.0),
        )

        self.assertEqual(
            Gdk.RGBA.from_color(Gdk.Color(13107, 21845, 65535)),
            Gdk.RGBA(0.2, 1.0 / 3.0, 1.0, 1.0),
        )

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_color_to_floats_attrs(self):
        color = Gdk.Color(13107, 21845, 65535)
        assert color.red_float == 0.2
        color.red_float = 0
        assert color.red_float == 0
        assert color.green_float == 1.0 / 3.0
        color.green_float = 0
        assert color.green_float == 0
        assert color.blue_float == 1.0
        color.blue_float = 0
        assert color.blue_float == 0

    def test_rgba(self):
        self.assertEqual(Gdk.RGBA, gi.overrides.Gdk.RGBA)
        rgba = Gdk.RGBA(0.1, 0.2, 0.3, 0.4)
        self.assertEqual(rgba, Gdk.RGBA(0.1, 0.2, 0.3, 0.4))
        self.assertNotEqual(rgba, Gdk.RGBA(0.0, 0.2, 0.3, 0.4))
        self.assertAlmostEqual(rgba.red, 0.1)
        self.assertAlmostEqual(rgba.green, 0.2)
        self.assertAlmostEqual(rgba.blue, 0.3)
        self.assertAlmostEqual(rgba.alpha, 0.4)
        rgba.green = 0.9
        self.assertAlmostEqual(rgba.green, 0.9)
        self.assertNotEqual(rgba, None)
        # assertNotEqual only tests __ne__. Following line explicitly
        # tests __eq__ with objects of other types
        self.assertFalse(rgba == object())

        # Iterator/tuple convsersion
        assert tuple(Gdk.RGBA(0.1, 0.2, 0.3, 0.4)) == pytest.approx(
            (0.1, 0.2, 0.3, 0.4)
        )

    @unittest.skipUnless(GDK4, "only in gdk4")
    def test_rgba_gtk4(self):
        c = Gdk.RGBA(0, 0, 0, 0)
        assert c.to_string() == "rgba(0,0,0,0)"

    @unittest.skipIf(not has_cairo or GDK4, "not in gdk4")
    def test_window(self):
        w = Gtk.Window()
        w.realize()
        window = w.get_window()
        with capture_glib_deprecation_warnings():
            assert window.cairo_create() is not None

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_drag_context(self):
        context = Gdk.DragContext()
        # using it this way crashes..
        assert hasattr(context, "finish")

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_event(self):
        event = Gdk.Event.new(Gdk.EventType.CONFIGURE)
        self.assertEqual(event.type, Gdk.EventType.CONFIGURE)
        self.assertEqual(event.send_event, 0)

        event = Gdk.Event()
        event.type = Gdk.EventType.SCROLL
        self.assertRaises(AttributeError, lambda: getattr(event, "foo_bar"))

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_scroll_event(self):
        event = Gdk.Event.new(Gdk.EventType.SCROLL)
        assert event.direction == Gdk.ScrollDirection.UP

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_event_strip_boolean(self):
        ev = Gdk.EventButton()
        ev.type = Gdk.EventType.BUTTON_PRESS
        assert ev.get_coords() == (0.0, 0.0)

        # https://gitlab.gnome.org/GNOME/pygobject/issues/85
        ev = Gdk.Event.new(Gdk.EventType.BUTTON_PRESS)
        assert ev.get_coords() == (True, 0.0, 0.0)

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_event_touch(self):
        event = Gdk.Event.new(Gdk.EventType.TOUCH_BEGIN)
        self.assertEqual(event.type, Gdk.EventType.TOUCH_BEGIN)

        # emulating_pointer is unique to touch events
        self.assertFalse(event.emulating_pointer)
        self.assertFalse(event.touch.emulating_pointer)

        event.emulating_pointer = True
        self.assertTrue(event.emulating_pointer)
        self.assertTrue(event.touch.emulating_pointer)

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_event_setattr(self):
        event = Gdk.Event.new(Gdk.EventType.DRAG_MOTION)
        event.x_root, event.y_root = 0, 5
        self.assertEqual(event.dnd.x_root, 0)
        self.assertEqual(event.dnd.y_root, 5)
        self.assertEqual(event.x_root, 0)
        self.assertEqual(event.y_root, 5)

        # this used to work, keep it that way
        self.assertFalse(hasattr(event, "foo_bar"))
        event.foo_bar = 42

        # unhandled type
        event.type = Gdk.EventType.EVENT_LAST
        with pytest.raises(AttributeError):
            event.foo_bar
        event.foo_bar = 42
        assert event.foo_bar == 42

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_event_repr(self):
        event = Gdk.Event.new(Gdk.EventType.CONFIGURE)
        self.assertTrue("CONFIGURE" in repr(event))

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_event_structures(self):
        def button_press_cb(button, event):
            self.assertTrue(isinstance(event, Gdk.EventButton))
            self.assertTrue(event.type == Gdk.EventType.BUTTON_PRESS)
            self.assertEqual(event.send_event, 0)
            self.assertEqual(event.get_state(), Gdk.ModifierType.CONTROL_MASK)
            self.assertEqual(event.get_root_coords(), (2, 5))

            event.time = 12345
            self.assertEqual(event.get_time(), 12345)

        w = Gtk.Window()
        b = Gtk.Button()
        b.connect("button-press-event", button_press_cb)
        w.add(b)
        b.show()
        b.realize()
        Gdk.test_simulate_button(
            b.get_window(),
            2,
            5,
            0,
            Gdk.ModifierType.CONTROL_MASK,
            Gdk.EventType.BUTTON_PRESS,
        )

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_cursor(self):
        self.assertEqual(Gdk.Cursor, gi.overrides.Gdk.Cursor)
        with capture_glib_deprecation_warnings():
            c = Gdk.Cursor(Gdk.CursorType.WATCH)
        self.assertNotEqual(c, None)
        with capture_glib_deprecation_warnings():
            c = Gdk.Cursor(cursor_type=Gdk.CursorType.WATCH)
        self.assertNotEqual(c, None)

        display_manager = Gdk.DisplayManager.get()
        display = display_manager.get_default_display()

        test_pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB, False, 8, 5, 10)

        with capture_glib_deprecation_warnings() as warn:
            c = Gdk.Cursor(display, test_pixbuf, y=0, x=0)
            self.assertNotEqual(c, None)

            self.assertEqual(len(warn), 1)
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))
            self.assertRegex(str(warn[0].message), ".*new_from_pixbuf.*")

        self.assertRaises(ValueError, Gdk.Cursor, 1, 2, 3)

        with capture_glib_deprecation_warnings() as warn:
            c = Gdk.Cursor(display, Gdk.CursorType.WATCH)
        assert len(warn) == 1
        # on macOS the type is switched to PIXMAP behind the scenes
        assert c.props.cursor_type in (
            Gdk.CursorType.WATCH,
            Gdk.CursorType.CURSOR_IS_PIXMAP,
        )
        assert c.props.display == display

    @unittest.skipUnless(GDK4, "only gdk4")
    def test_cursor_gdk4(self):
        Gdk.Cursor()
        Gdk.Cursor(name="foo")
        Gdk.Cursor(fallback=Gdk.Cursor())

    def test_flags(self):
        self.assertEqual(Gdk.ModifierType.META_MASK | 0, 0x10000000)
        self.assertEqual(hex(Gdk.ModifierType.META_MASK), "0x10000000")
        self.assertEqual(
            repr(Gdk.ModifierType.META_MASK), "<ModifierType.META_MASK: 268435456>"
        )

        # RELEASE_MASK does not exist in gdk4
        if not GDK4:
            self.assertEqual(Gdk.ModifierType.RELEASE_MASK | 0, 0x40000000)
            self.assertEqual(hex(Gdk.ModifierType.RELEASE_MASK), "0x40000000")
            self.assertEqual(
                repr(Gdk.ModifierType.RELEASE_MASK),
                "<ModifierType.RELEASE_MASK: 1073741824>",
            )

            self.assertEqual(
                Gdk.ModifierType.RELEASE_MASK | Gdk.ModifierType.META_MASK, 0x50000000
            )
            self.assertIn(
                repr(Gdk.ModifierType.RELEASE_MASK | Gdk.ModifierType.META_MASK),
                {
                    "<ModifierType.META_MASK|RELEASE_MASK: 1342177280>",
                    "<ModifierType.RELEASE_MASK|META_MASK: 1342177280>",
                },
            )

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_color_parse(self):
        with capture_glib_deprecation_warnings():
            c = Gdk.color_parse("#00FF80")
        self.assertEqual(c.red, 0)
        self.assertEqual(c.green, 65535)
        self.assertEqual(c.blue, 32896)
        self.assertEqual(Gdk.color_parse("bogus"), None)

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_color_representations(self):
        # __repr__ should generate a string which is parsable when possible
        # http://docs.python.org/2/reference/datamodel.html#object.__repr__
        color = Gdk.Color(red=65535, green=32896, blue=1)
        self.assertEqual(eval(repr(color)), color)

    def test_rgba_representations(self):
        rgba = Gdk.RGBA(red=1.0, green=0.8, blue=0.6, alpha=0.4)
        self.assertEqual(eval(repr(rgba)), rgba)

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_rectangle_functions(self):
        # https://bugzilla.gnome.org/show_bug.cgi?id=756364
        a = Gdk.Rectangle()
        b = Gdk.Rectangle()
        self.assertTrue(isinstance(Gdk.rectangle_union(a, b), Gdk.Rectangle))
        intersect, rect = Gdk.rectangle_intersect(a, b)
        self.assertTrue(isinstance(rect, Gdk.Rectangle))
        self.assertTrue(isinstance(intersect, bool))

    @unittest.skipIf(GDK4, "not in gdk4")
    def test_atom_repr_str(self):
        atom = Gdk.atom_intern("", True)
        assert re.match(r"<Gdk.Atom\(\d+\)>", repr(atom))
        assert re.match(r"Gdk.Atom<\d+>", str(atom))

    @unittest.skipUnless(GDK4, "only in gdk4")
    @unittest.skipUnless(gtkver() >= (4, 8, 0), "constructor available since 4.8")
    def test_file_list(self):
        f = Gio.File.new_for_path("/tmp")
        filelist = Gdk.FileList([f])
        self.assertTrue(isinstance(filelist, Gdk.FileList))
        self.assertEqual(len(filelist), 1)
        self.assertEqual(filelist[0], f)

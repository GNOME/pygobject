# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest
import warnings

import gi.overrides
from gi import PyGIDeprecationWarning

try:
    from gi.repository import Gdk, GdkPixbuf, Gtk
    Gdk  # pyflakes
except ImportError:
    Gdk = None


@unittest.skipUnless(Gdk, 'Gdk not available')
class TestGdk(unittest.TestCase):
    def test_constructor(self):
        attribute = Gdk.WindowAttr()
        attribute.window_type = Gdk.WindowType.CHILD
        attributes_mask = Gdk.WindowAttributesType.X | \
            Gdk.WindowAttributesType.Y
        window = Gdk.Window(None, attribute, attributes_mask)
        self.assertEqual(window.get_window_type(), Gdk.WindowType.CHILD)

    def test_color(self):
        color = Gdk.Color(100, 200, 300)
        self.assertEqual(color.red, 100)
        self.assertEqual(color.green, 200)
        self.assertEqual(color.blue, 300)
        self.assertEqual(color, Gdk.Color(100, 200, 300))
        self.assertNotEqual(color, Gdk.Color(1, 2, 3))

    def test_color_floats(self):
        self.assertEqual(Gdk.Color(13107, 21845, 65535),
                         Gdk.Color.from_floats(0.2, 1.0 / 3.0, 1.0))

        self.assertEqual(Gdk.Color(13107, 21845, 65535).to_floats(),
                         (0.2, 1.0 / 3.0, 1.0))

        self.assertEqual(Gdk.RGBA(0.2, 1.0 / 3.0, 1.0, 0.5).to_color(),
                         Gdk.Color.from_floats(0.2, 1.0 / 3.0, 1.0))

        self.assertEqual(Gdk.RGBA.from_color(Gdk.Color(13107, 21845, 65535)),
                         Gdk.RGBA(0.2, 1.0 / 3.0, 1.0, 1.0))

    def test_rgba(self):
        self.assertEqual(Gdk.RGBA, gi.overrides.Gdk.RGBA)
        rgba = Gdk.RGBA(0.1, 0.2, 0.3, 0.4)
        self.assertEqual(rgba, Gdk.RGBA(0.1, 0.2, 0.3, 0.4))
        self.assertNotEqual(rgba, Gdk.RGBA(0.0, 0.2, 0.3, 0.4))
        self.assertEqual(rgba.red, 0.1)
        self.assertEqual(rgba.green, 0.2)
        self.assertEqual(rgba.blue, 0.3)
        self.assertEqual(rgba.alpha, 0.4)
        rgba.green = 0.9
        self.assertEqual(rgba.green, 0.9)

        # Iterator/tuple convsersion
        self.assertEqual(tuple(Gdk.RGBA(0.1, 0.2, 0.3, 0.4)),
                         (0.1, 0.2, 0.3, 0.4))

    def test_event(self):
        event = Gdk.Event.new(Gdk.EventType.CONFIGURE)
        self.assertEqual(event.type, Gdk.EventType.CONFIGURE)
        self.assertEqual(event.send_event, 0)

        event = Gdk.Event()
        event.type = Gdk.EventType.SCROLL
        self.assertRaises(AttributeError, lambda: getattr(event, 'foo_bar'))

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

    def test_event_repr(self):
        event = Gdk.Event.new(Gdk.EventType.CONFIGURE)
        self.assertTrue("CONFIGURE" in repr(event))

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
        b.connect('button-press-event', button_press_cb)
        w.add(b)
        w.show_all()
        Gdk.test_simulate_button(b.get_window(),
                                 2, 5,
                                 0,
                                 Gdk.ModifierType.CONTROL_MASK,
                                 Gdk.EventType.BUTTON_PRESS)

    def test_cursor(self):
        self.assertEqual(Gdk.Cursor, gi.overrides.Gdk.Cursor)
        c = Gdk.Cursor(Gdk.CursorType.WATCH)
        self.assertNotEqual(c, None)
        c = Gdk.Cursor(cursor_type=Gdk.CursorType.WATCH)
        self.assertNotEqual(c, None)

        display_manager = Gdk.DisplayManager.get()
        display = display_manager.get_default_display()

        test_pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB,
                                           False,
                                           8,
                                           5,
                                           10)

        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            c = Gdk.Cursor(display,
                           test_pixbuf,
                           y=0, x=0)
            self.assertNotEqual(c, None)

            self.assertEqual(len(warn), 1)
            self.assertTrue(issubclass(warn[0].category, PyGIDeprecationWarning))
            self.assertRegexpMatches(str(warn[0].message),
                                     '.*new_from_pixbuf.*')

        self.assertRaises(ValueError, Gdk.Cursor, 1, 2, 3)

    def test_flags(self):
        self.assertEqual(Gdk.ModifierType.META_MASK | 0, 0x10000000)
        self.assertEqual(hex(Gdk.ModifierType.META_MASK), '0x10000000')
        self.assertEqual(str(Gdk.ModifierType.META_MASK),
                         '<flags GDK_META_MASK of type GdkModifierType>')

        self.assertEqual(Gdk.ModifierType.RELEASE_MASK | 0, 0x40000000)
        self.assertEqual(hex(Gdk.ModifierType.RELEASE_MASK), '0x40000000')
        self.assertEqual(str(Gdk.ModifierType.RELEASE_MASK),
                         '<flags GDK_RELEASE_MASK of type GdkModifierType>')

        self.assertEqual(Gdk.ModifierType.RELEASE_MASK | Gdk.ModifierType.META_MASK, 0x50000000)
        self.assertEqual(str(Gdk.ModifierType.RELEASE_MASK | Gdk.ModifierType.META_MASK),
                         '<flags GDK_META_MASK | GDK_RELEASE_MASK of type GdkModifierType>')

    def test_color_parse(self):
        c = Gdk.color_parse('#00FF80')
        self.assertEqual(c.red, 0)
        self.assertEqual(c.green, 65535)
        self.assertEqual(c.blue, 32896)
        self.assertEqual(Gdk.color_parse('bogus'), None)

    def test_color_representations(self):
        # __repr__ should generate a string which is parsable when possible
        # http://docs.python.org/2/reference/datamodel.html#object.__repr__
        color = Gdk.Color(red=65535, green=32896, blue=1)
        self.assertEqual(eval(repr(color)), color)

        rgba = Gdk.RGBA(red=1.0, green=0.8, blue=0.6, alpha=0.4)
        self.assertEqual(eval(repr(rgba)), rgba)

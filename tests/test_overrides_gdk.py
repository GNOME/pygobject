# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

from __future__ import absolute_import

import os
import sys
import unittest

import gi.overrides
from gi import PyGIDeprecationWarning

try:
    from gi.repository import Gdk, GdkPixbuf, Gtk
    Gdk_version = Gdk._version
except ImportError:
    Gdk = None
    Gdk_version = None

from .helper import capture_glib_deprecation_warnings


@unittest.skipUnless(Gdk, 'Gdk not available')
class TestGdk(unittest.TestCase):

    @unittest.skipIf(sys.platform == "darwin" or os.name == "nt", "crashes")
    @unittest.skipIf(Gdk_version == "4.0", "not in gdk4")
    def test_constructor(self):
        attribute = Gdk.WindowAttr()
        attribute.window_type = Gdk.WindowType.CHILD
        attributes_mask = Gdk.WindowAttributesType.X | \
            Gdk.WindowAttributesType.Y
        window = Gdk.Window(None, attribute, attributes_mask)
        self.assertEqual(window.get_window_type(), Gdk.WindowType.CHILD)

    @unittest.skipIf(Gdk_version == "4.0", "not in gdk4")
    def test_color(self):
        color = Gdk.Color(100, 200, 300)
        self.assertEqual(color.red, 100)
        self.assertEqual(color.green, 200)
        self.assertEqual(color.blue, 300)
        with capture_glib_deprecation_warnings():
            self.assertEqual(color, Gdk.Color(100, 200, 300))
        self.assertNotEqual(color, Gdk.Color(1, 2, 3))

    @unittest.skipIf(Gdk_version == "4.0", "not in gdk4")
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

    @unittest.skipIf(Gdk_version == "4.0", "not in gdk4")
    def test_event(self):
        event = Gdk.Event.new(Gdk.EventType.CONFIGURE)
        self.assertEqual(event.type, Gdk.EventType.CONFIGURE)
        self.assertEqual(event.send_event, 0)

        event = Gdk.Event()
        event.type = Gdk.EventType.SCROLL
        self.assertRaises(AttributeError, lambda: getattr(event, 'foo_bar'))

    @unittest.skipIf(Gdk_version == "4.0", "not in gdk4")
    def test_event_touch(self):
        event = Gdk.Event.new(Gdk.EventType.TOUCH_BEGIN)
        self.assertEqual(event.type, Gdk.EventType.TOUCH_BEGIN)

        # emulating_pointer is unique to touch events
        self.assertFalse(event.emulating_pointer)
        self.assertFalse(event.touch.emulating_pointer)

        event.emulating_pointer = True
        self.assertTrue(event.emulating_pointer)
        self.assertTrue(event.touch.emulating_pointer)

    @unittest.skipIf(Gdk_version == "4.0", "not in gdk4")
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

    @unittest.skipIf(Gdk_version == "4.0", "not in gdk4")
    def test_event_repr(self):
        event = Gdk.Event.new(Gdk.EventType.CONFIGURE)
        self.assertTrue("CONFIGURE" in repr(event))

    @unittest.skipIf(Gdk_version == "4.0", "not in gdk4")
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
        b.show()
        b.realize()
        Gdk.test_simulate_button(b.get_window(),
                                 2, 5,
                                 0,
                                 Gdk.ModifierType.CONTROL_MASK,
                                 Gdk.EventType.BUTTON_PRESS)

    @unittest.skipIf(Gdk_version == "4.0", "not in gdk4")
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

        test_pixbuf = GdkPixbuf.Pixbuf.new(GdkPixbuf.Colorspace.RGB,
                                           False,
                                           8,
                                           5,
                                           10)

        with capture_glib_deprecation_warnings() as warn:
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
                         '<flags GDK_META_MASK of type Gdk.ModifierType>')

        self.assertEqual(Gdk.ModifierType.RELEASE_MASK | 0, 0x40000000)
        self.assertEqual(hex(Gdk.ModifierType.RELEASE_MASK), '0x40000000')
        self.assertEqual(str(Gdk.ModifierType.RELEASE_MASK),
                         '<flags GDK_RELEASE_MASK of type Gdk.ModifierType>')

        self.assertEqual(Gdk.ModifierType.RELEASE_MASK | Gdk.ModifierType.META_MASK, 0x50000000)
        self.assertEqual(str(Gdk.ModifierType.RELEASE_MASK | Gdk.ModifierType.META_MASK),
                         '<flags GDK_META_MASK | GDK_RELEASE_MASK of type Gdk.ModifierType>')

    @unittest.skipIf(Gdk_version == "4.0", "not in gdk4")
    def test_color_parse(self):
        with capture_glib_deprecation_warnings():
            c = Gdk.color_parse('#00FF80')
        self.assertEqual(c.red, 0)
        self.assertEqual(c.green, 65535)
        self.assertEqual(c.blue, 32896)
        self.assertEqual(Gdk.color_parse('bogus'), None)

    @unittest.skipIf(Gdk_version == "4.0", "not in gdk4")
    def test_color_representations(self):
        # __repr__ should generate a string which is parsable when possible
        # http://docs.python.org/2/reference/datamodel.html#object.__repr__
        color = Gdk.Color(red=65535, green=32896, blue=1)
        self.assertEqual(eval(repr(color)), color)

        rgba = Gdk.RGBA(red=1.0, green=0.8, blue=0.6, alpha=0.4)
        self.assertEqual(eval(repr(rgba)), rgba)

    def test_rectangle_functions(self):
        # https://bugzilla.gnome.org/show_bug.cgi?id=756364
        a = Gdk.Rectangle()
        b = Gdk.Rectangle()
        self.assertTrue(isinstance(Gdk.rectangle_union(a, b), Gdk.Rectangle))
        intersect, rect = Gdk.rectangle_intersect(a, b)
        self.assertTrue(isinstance(rect, Gdk.Rectangle))
        self.assertTrue(isinstance(intersect, bool))

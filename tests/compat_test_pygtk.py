# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest
import contextlib

from gi.repository import GLib

try:
    from gi.repository import Pango
    from gi.repository import Atk
    from gi.repository import Gdk
    from gi.repository import Gtk
    (Atk, Gtk, Pango)  # pyflakes

    import pygtkcompat

    pygtkcompat.enable()
    pygtkcompat.enable_gtk(version='3.0')

    import atk
    import pango
    import pangocairo
    import gtk
    import gtk.gdk
except ImportError:
    Gtk = None


@contextlib.contextmanager
def ignore_glib_warnings():
    """Temporarily change GLib logging to not bail on warnings."""
    old_mask = GLib.log_set_always_fatal(
        GLib.LogLevelFlags.LEVEL_CRITICAL | GLib.LogLevelFlags.LEVEL_ERROR)
    yield
    GLib.log_set_always_fatal(old_mask)


@unittest.skipUnless(Gtk, 'Gtk not available')
class TestATKCompat(unittest.TestCase):
    def test_object(self):
        self.assertTrue(hasattr(atk, 'Object'))


@unittest.skipUnless(Gtk, 'Gtk not available')
class TestPangoCompat(unittest.TestCase):
    def test_layout(self):
        self.assertTrue(hasattr(pango, 'Layout'))


@unittest.skipUnless(Gtk, 'Gtk not available')
class TestPangoCairoCompat(unittest.TestCase):
    def test_error_underline_path(self):
        self.assertTrue(hasattr(pangocairo, 'error_underline_path'))


@unittest.skipUnless(Gtk, 'Gtk not available')
class TestGTKCompat(unittest.TestCase):
    def test_buttons(self):
        self.assertEqual(Gdk._2BUTTON_PRESS, 5)
        self.assertEqual(Gdk.BUTTON_PRESS, 4)

    def test_enums(self):
        self.assertEqual(gtk.WINDOW_TOPLEVEL, Gtk.WindowType.TOPLEVEL)
        self.assertEqual(gtk.PACK_START, Gtk.PackType.START)

    def test_flags(self):
        self.assertEqual(gtk.EXPAND, Gtk.AttachOptions.EXPAND)
        self.assertEqual(gtk.gdk.SHIFT_MASK, Gdk.ModifierType.SHIFT_MASK)

    def test_keysyms(self):
        import gtk.keysyms
        self.assertEqual(gtk.keysyms.Escape, Gdk.KEY_Escape)
        self.assertTrue(gtk.keysyms._0, Gdk.KEY_0)

    def test_style(self):
        widget = gtk.Button()
        self.assertTrue(isinstance(widget.style.base[gtk.STATE_NORMAL],
                                   gtk.gdk.Color))

    def test_alignment(self):
        # Creation of pygtk.Alignment causes hard warnings, ignore this in testing.
        with ignore_glib_warnings():
            a = gtk.Alignment()

        self.assertEqual(a.props.xalign, 0.0)
        self.assertEqual(a.props.yalign, 0.0)
        self.assertEqual(a.props.xscale, 0.0)
        self.assertEqual(a.props.yscale, 0.0)

    def test_box(self):
        box = gtk.Box()
        child = gtk.Button()

        box.pack_start(child)
        expand, fill, padding, pack_type = box.query_child_packing(child)
        self.assertTrue(expand)
        self.assertTrue(fill)
        self.assertEqual(padding, 0)
        self.assertEqual(pack_type, gtk.PACK_START)

        child = gtk.Button()
        box.pack_end(child)
        expand, fill, padding, pack_type = box.query_child_packing(child)
        self.assertTrue(expand)
        self.assertTrue(fill)
        self.assertEqual(padding, 0)
        self.assertEqual(pack_type, gtk.PACK_END)

    def test_combobox_entry(self):
        liststore = gtk.ListStore(int, str)
        liststore.append((1, 'One'))
        liststore.append((2, 'Two'))
        liststore.append((3, 'Three'))
        # might cause a Pango warning, do not break on this
        old_mask = GLib.log_set_always_fatal(
            GLib.LogLevelFlags.LEVEL_CRITICAL | GLib.LogLevelFlags.LEVEL_ERROR)
        try:
            combo = gtk.ComboBoxEntry(model=liststore)
        finally:
            GLib.log_set_always_fatal(old_mask)
        combo.set_text_column(1)
        combo.set_active(0)
        self.assertEqual(combo.get_text_column(), 1)
        self.assertEqual(combo.get_child().get_text(), 'One')
        combo = gtk.combo_box_entry_new()
        combo.set_model(liststore)
        combo.set_text_column(1)
        combo.set_active(0)
        self.assertEqual(combo.get_text_column(), 1)
        self.assertEqual(combo.get_child().get_text(), 'One')
        combo = gtk.combo_box_entry_new_with_model(liststore)
        combo.set_text_column(1)
        combo.set_active(0)
        self.assertEqual(combo.get_text_column(), 1)
        self.assertEqual(combo.get_child().get_text(), 'One')

    def test_size_request(self):
        box = gtk.Box()
        self.assertEqual(box.size_request(), [0, 0])

    def test_pixbuf(self):
        gtk.gdk.Pixbuf()

    def test_pixbuf_loader(self):
        loader = gtk.gdk.PixbufLoader('png')
        loader.close()

    def test_pixbuf_formats(self):
        formats = gtk.gdk.pixbuf_get_formats()
        self.assertEqual(type(formats[0]), dict)
        self.assertTrue('name' in formats[0])
        self.assertTrue('description' in formats[0])
        self.assertTrue('mime_types' in formats[0])
        self.assertEqual(type(formats[0]['extensions']), list)

    def test_gdk_window(self):
        w = gtk.Window()
        w.realize()
        self.assertEqual(w.get_window().get_origin(), (0, 0))

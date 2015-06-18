# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest
import base64

import gi

try:
    try:
        gi.require_version("Gtk", "3.0")
    except ValueError as e:
        raise ImportError(e)
    from gi.repository import Gtk
    from gi.repository import Pango
    from gi.repository import Atk
    from gi.repository import Gdk
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

from helper import capture_gi_deprecation_warnings, capture_glib_warnings


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
        with capture_gi_deprecation_warnings():
            self.assertTrue(isinstance(widget.style.base[gtk.STATE_NORMAL],
                                       gtk.gdk.Color))

    def test_alignment(self):
        # Creation of pygtk.Alignment causes hard warnings, ignore this in testing.
        with capture_glib_warnings(allow_warnings=True):
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
        with capture_glib_warnings(allow_warnings=True):
            combo = gtk.ComboBoxEntry(model=liststore)
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
        with capture_gi_deprecation_warnings():
            self.assertEqual(box.size_request(), [0, 0])

    def test_pixbuf(self):
        gtk.gdk.Pixbuf()

    def test_pixbuf_loader(self):
        # load a 1x1 pixel PNG from memory
        data = base64.b64decode('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP4n8Dw'
                                'HwAGIAJf85Z3XgAAAABJRU5ErkJggg==')
        loader = gtk.gdk.PixbufLoader('png')
        loader.write(data)
        loader.close()

        pixbuf = loader.get_pixbuf()
        self.assertEqual(pixbuf.get_width(), 1)
        self.assertEqual(pixbuf.get_height(), 1)

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

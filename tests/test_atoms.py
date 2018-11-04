from __future__ import absolute_import

import os
import unittest

try:
    from gi.repository import Gtk, Atk, Gdk
except ImportError:
    Gdk = None
    Atk = None
    Gtk = None

from .helper import capture_glib_deprecation_warnings


def is_X11():
    try:
        from gi.repository import Gdk, GdkX11
    except ImportError:
        return False

    display = Gdk.Display.get_default()
    return isinstance(display, GdkX11.X11Display)


@unittest.skipUnless(Gdk, 'Gdk not available')
@unittest.skipIf(Gdk._version == "4.0", 'Gdk4 doesn\'t have GdkAtom')
class TestGdkAtom(unittest.TestCase):
    def test_create(self):
        atom = Gdk.Atom.intern('my_string', False)
        self.assertEqual(atom.name(), 'my_string')

    def test_str(self):
        atom = Gdk.Atom.intern('my_string', False)
        self.assertEqual(str(atom), 'my_string')

        self.assertEqual(str(Gdk.SELECTION_CLIPBOARD), 'CLIPBOARD')

    def test_repr(self):
        # __repr__ should generate a string which is parsable when possible
        # http://docs.python.org/2/reference/datamodel.html#object.__repr__
        atom = Gdk.Atom.intern('my_string', False)
        self.assertEqual(repr(atom), 'Gdk.Atom.intern("my_string", False)')
        self.assertEqual(eval(repr(atom)), atom)

        self.assertEqual(repr(Gdk.SELECTION_CLIPBOARD), 'Gdk.Atom.intern("CLIPBOARD", False)')

    @unittest.skipUnless(os.name != "nt", "not on Windows")
    def test_in_single(self):
        a_selection = Gdk.Atom.intern('test_clipboard', False)
        clipboard = Gtk.Clipboard.get(a_selection)
        clipboard.set_text('hello', 5)

        # needs a Gdk.Atom, not a string
        self.assertRaises(TypeError, Gtk.Clipboard.get, 'CLIPBOARD')

    def test_in_array(self):
        a_plain = Gdk.Atom.intern('text/plain', False)
        a_html = Gdk.Atom.intern('text/html', False)
        a_jpeg = Gdk.Atom.intern('image/jpeg', False)

        self.assertFalse(Gtk.targets_include_text([]))
        self.assertTrue(Gtk.targets_include_text([a_plain, a_html]))
        self.assertFalse(Gtk.targets_include_text([a_jpeg]))
        self.assertTrue(Gtk.targets_include_text([a_jpeg, a_plain]))

        self.assertFalse(Gtk.targets_include_image([], False))
        self.assertFalse(Gtk.targets_include_image([a_plain, a_html], False))
        self.assertTrue(Gtk.targets_include_image([a_jpeg], False))
        self.assertTrue(Gtk.targets_include_image([a_jpeg, a_plain], False))

    @unittest.skipUnless(is_X11(), "only on X11")
    def test_out_array(self):
        a_selection = Gdk.Atom.intern('my_clipboard', False)
        clipboard = Gtk.Clipboard.get(a_selection)

        # empty
        (res, targets) = clipboard.wait_for_targets()
        self.assertEqual(res, False)
        self.assertEqual(targets, [])

        # text
        clipboard.set_text('hello', 5)
        (res, targets) = clipboard.wait_for_targets()
        self.assertEqual(res, True)
        self.assertNotEqual(targets, [])
        self.assertEqual(type(targets[0]), Gdk.Atom)
        names = [t.name() for t in targets]
        self.assertFalse(None in names, names)
        self.assertTrue('TEXT' in names, names)

    @unittest.skipUnless(is_X11(), "only on X11")
    @unittest.skipIf(not Gdk or Gdk._version == "4.0", "not in gdk4")
    def test_out_glist(self):
        display = Gdk.Display.get_default()
        with capture_glib_deprecation_warnings():
            dm = display.get_device_manager()
            device = dm.get_client_pointer()
        axes = device.list_axes()
        axes_names = [atom.name() for atom in axes]
        self.assertNotEqual(axes_names, [])

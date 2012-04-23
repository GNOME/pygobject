import unittest

from gi.repository import Gdk, Gtk


class TestGdkAtom(unittest.TestCase):
    def test_create(self):
        atom = Gdk.Atom.intern('my_string', False)
        self.assertEqual(atom.name(), 'my_string')

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

    def test_out_glist(self):
        display = Gdk.Display.get_default()
        dm = display.get_device_manager()
        device = dm.get_client_pointer()
        axes = device.list_axes()
        axes_names = [atom.name() for atom in axes]
        self.assertNotEqual(axes_names, [])
        self.assertTrue('Rel X' in axes_names)

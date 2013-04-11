# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest

try:
    from gi.repository import Pango
    Pango
except ImportError:
    Pango = None


@unittest.skipUnless(Pango, 'Pango not available')
class TestPango(unittest.TestCase):

    def test_default_font_description(self):
        desc = Pango.FontDescription()
        self.assertEqual(desc.get_variant(), Pango.Variant.NORMAL)

    def test_font_description(self):
        desc = Pango.FontDescription('monospace')
        self.assertEqual(desc.get_family(), 'monospace')
        self.assertEqual(desc.get_variant(), Pango.Variant.NORMAL)

    def test_layout(self):
        self.assertRaises(TypeError, Pango.Layout)
        context = Pango.Context()
        layout = Pango.Layout(context)
        self.assertEqual(layout.get_context(), context)

        layout.set_markup("Foobar")
        self.assertEqual(layout.get_text(), "Foobar")

    def test_break_keyword_escape(self):
        # https://bugzilla.gnome.org/show_bug.cgi?id=697363
        self.assertTrue(hasattr(Pango, 'break_'))
        self.assertTrue(Pango.break_ is not None)

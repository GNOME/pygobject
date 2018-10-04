# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

from __future__ import absolute_import

import unittest

try:
    from gi.repository import Pango
    from gi.repository import PangoCairo
except ImportError:
    Pango = None
    PangoCairo = None


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

    def test_layout_set_markup(self):
        context = Pango.Context()
        layout = Pango.Layout(context)
        layout.set_markup("abc")
        assert layout.get_text() == "abc"
        layout.set_markup("abc", -1)
        assert layout.get_text() == "abc"
        layout.set_markup("abc", 2)
        assert layout.get_text() == "ab"

    def test_layout_set_test(self):
        context = Pango.Context()
        layout = Pango.Layout(context)
        layout.set_text("abc")
        assert layout.get_text() == "abc"
        layout.set_text("abc", -1)
        assert layout.get_text() == "abc"
        layout.set_text("abc", 2)
        assert layout.get_text() == "ab"

    def test_break_keyword_escape(self):
        # https://bugzilla.gnome.org/show_bug.cgi?id=697363
        self.assertTrue(hasattr(Pango, 'break_'))
        self.assertTrue(Pango.break_ is not None)

    def test_context_get_metrics(self):
        # Test default "language" argument
        font_map = PangoCairo.font_map_get_default()
        context = font_map.create_context()
        desc = Pango.FontDescription('monospace')
        metrics1 = context.get_metrics(desc)
        metrics2 = context.get_metrics(desc, context.get_language())
        self.assertEqual(metrics1.get_ascent(), metrics2.get_ascent())

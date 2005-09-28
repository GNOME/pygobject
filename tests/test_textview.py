import sys
import unittest

from common import gtk

class TextViewTest(unittest.TestCase):
    def test_default_attributes(self):
        textview = gtk.TextView()
        attrs = textview.get_default_attributes()
        textview.destroy()
        self.assertEqual(attrs.font_scale, 1.0)

if __name__ == '__main__':
    unittest.main()

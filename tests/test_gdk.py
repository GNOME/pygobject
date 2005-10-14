import unittest

from common import gtk

class GdkTest(unittest.TestCase):
    def testBitmapCreateFromData(self):
        gtk.gdk.bitmap_create_from_data(None, '\x00', 1, 1)

    def testPixmapCreateFromData(self):
        black = gtk.gdk.color_parse('black')
        gtk.gdk.pixmap_create_from_data(None, '\x00', 1, 1, 1,
                                        black, black)

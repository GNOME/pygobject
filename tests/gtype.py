import unittest

from common import gobject, gtk

class GTypeTest(unittest.TestCase):
    def testBoolType(self):
        store = gtk.ListStore(gobject.TYPE_BOOLEAN)
        assert store.get_column_type(0) == gobject.TYPE_BOOLEAN
        store = gtk.ListStore('gboolean')
        assert store.get_column_type(0) == gobject.TYPE_BOOLEAN
        store = gtk.ListStore(bool)
        assert store.get_column_type(0) == gobject.TYPE_BOOLEAN

if __name__ == '__main__':
    unittest.main()

import unittest

from common import gobject, gtk

class GTypeTest(unittest.TestCase):
    def checkType(self, expected, *objects):
        # Silly method to check pyg_type_from_object
        
        store = gtk.ListStore(expected)
        val = store.get_column_type(0)
        assert val == expected, \
               'got %r while %r was expected' % (val, expected)

        for object in objects:
            store = gtk.ListStore(object)
            val = store.get_column_type(0)
            assert val == expected, \
                   'got %r while %r was expected' % (val, expected)
        
    def testBool(self):
        self.checkType(gobject.TYPE_BOOLEAN, 'gboolean', bool)

    def testInt(self):
        self.checkType(gobject.TYPE_INT, 'gint', int)
        
    def testInt64(self):
        self.checkType(gobject.TYPE_INT64, 'gint64')

    def testUint(self):
        self.checkType(gobject.TYPE_UINT, 'guint')

    def testUint64(self):
        self.checkType(gobject.TYPE_UINT64, 'guint64')

    def testLong(self):
        self.checkType(gobject.TYPE_LONG, 'glong', long)

    def testUlong(self):
        self.checkType(gobject.TYPE_ULONG, 'gulong')

    def testDouble(self):
        self.checkType(gobject.TYPE_DOUBLE, 'gdouble', float)

    def testFloat(self):
        self.checkType(gobject.TYPE_FLOAT, 'gfloat')

    def testPyObject(self):
        self.checkType(gobject.TYPE_PYOBJECT, object)
        
    def testObject(self):
        self.checkType(gobject.TYPE_OBJECT)
        
    # XXX: Flags, Enums
    
if __name__ == '__main__':
    unittest.main()

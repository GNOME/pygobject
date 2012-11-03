import unittest

from gi.repository import GObject
from gi.repository import GIMarshallingTests
from gi._gobject import _gobject  # pull in the python DSO directly


class CustomBase(GObject.GObject):
    pass


class CustomChild(CustomBase, GIMarshallingTests.Interface):
    pass


class TestTypeModuleLevelFunctions(unittest.TestCase):
    def test_type_name(self):
        self.assertEqual(GObject.type_name(GObject.TYPE_NONE), 'void')
        self.assertEqual(GObject.type_name(GObject.TYPE_INTERFACE), 'GInterface')
        self.assertEqual(GObject.type_name(GObject.TYPE_CHAR), 'gchar')
        self.assertEqual(GObject.type_name(GObject.TYPE_UCHAR), 'guchar')
        self.assertEqual(GObject.type_name(GObject.TYPE_BOOLEAN), 'gboolean')
        self.assertEqual(GObject.type_name(GObject.TYPE_INT), 'gint')
        self.assertEqual(GObject.type_name(GObject.TYPE_UINT), 'guint')
        self.assertEqual(GObject.type_name(GObject.TYPE_LONG), 'glong')
        self.assertEqual(GObject.type_name(GObject.TYPE_ULONG), 'gulong')
        self.assertEqual(GObject.type_name(GObject.TYPE_INT64), 'gint64')
        self.assertEqual(GObject.type_name(GObject.TYPE_UINT64), 'guint64')
        self.assertEqual(GObject.type_name(GObject.TYPE_ENUM), 'GEnum')
        self.assertEqual(GObject.type_name(GObject.TYPE_FLAGS), 'GFlags')
        self.assertEqual(GObject.type_name(GObject.TYPE_FLOAT), 'gfloat')
        self.assertEqual(GObject.type_name(GObject.TYPE_DOUBLE), 'gdouble')
        self.assertEqual(GObject.type_name(GObject.TYPE_STRING), 'gchararray')
        self.assertEqual(GObject.type_name(GObject.TYPE_POINTER), 'gpointer')
        self.assertEqual(GObject.type_name(GObject.TYPE_BOXED), 'GBoxed')
        self.assertEqual(GObject.type_name(GObject.TYPE_PARAM), 'GParam')
        self.assertEqual(GObject.type_name(GObject.TYPE_OBJECT), 'GObject')
        self.assertEqual(GObject.type_name(GObject.TYPE_PYOBJECT), 'PyObject')
        self.assertEqual(GObject.type_name(GObject.TYPE_GTYPE), 'GType')
        self.assertEqual(GObject.type_name(GObject.TYPE_STRV), 'GStrv')
        self.assertEqual(GObject.type_name(GObject.TYPE_VARIANT), 'GVariant')
        self.assertEqual(GObject.type_name(GObject.TYPE_UNICHAR), 'guint')

    def test_gi_types_equal_static_type_from_name(self):
        # Note this test should be changed to use GObject.type_from_name
        # if the _gobject.type_from_name binding is ever removed.

        self.assertEqual(GObject.TYPE_NONE, _gobject.type_from_name('void'))
        self.assertEqual(GObject.TYPE_INTERFACE, _gobject.type_from_name('GInterface'))
        self.assertEqual(GObject.TYPE_CHAR, _gobject.type_from_name('gchar'))
        self.assertEqual(GObject.TYPE_UCHAR, _gobject.type_from_name('guchar'))
        self.assertEqual(GObject.TYPE_BOOLEAN, _gobject.type_from_name('gboolean'))
        self.assertEqual(GObject.TYPE_INT, _gobject.type_from_name('gint'))
        self.assertEqual(GObject.TYPE_UINT, _gobject.type_from_name('guint'))
        self.assertEqual(GObject.TYPE_LONG, _gobject.type_from_name('glong'))
        self.assertEqual(GObject.TYPE_ULONG, _gobject.type_from_name('gulong'))
        self.assertEqual(GObject.TYPE_INT64, _gobject.type_from_name('gint64'))
        self.assertEqual(GObject.TYPE_UINT64, _gobject.type_from_name('guint64'))
        self.assertEqual(GObject.TYPE_ENUM, _gobject.type_from_name('GEnum'))
        self.assertEqual(GObject.TYPE_FLAGS, _gobject.type_from_name('GFlags'))
        self.assertEqual(GObject.TYPE_FLOAT, _gobject.type_from_name('gfloat'))
        self.assertEqual(GObject.TYPE_DOUBLE, _gobject.type_from_name('gdouble'))
        self.assertEqual(GObject.TYPE_STRING, _gobject.type_from_name('gchararray'))
        self.assertEqual(GObject.TYPE_POINTER, _gobject.type_from_name('gpointer'))
        self.assertEqual(GObject.TYPE_BOXED, _gobject.type_from_name('GBoxed'))
        self.assertEqual(GObject.TYPE_PARAM, _gobject.type_from_name('GParam'))
        self.assertEqual(GObject.TYPE_OBJECT, _gobject.type_from_name('GObject'))
        self.assertEqual(GObject.TYPE_PYOBJECT, _gobject.type_from_name('PyObject'))
        self.assertEqual(GObject.TYPE_GTYPE, _gobject.type_from_name('GType'))
        self.assertEqual(GObject.TYPE_STRV, _gobject.type_from_name('GStrv'))
        self.assertEqual(GObject.TYPE_VARIANT, _gobject.type_from_name('GVariant'))
        self.assertEqual(GObject.TYPE_UNICHAR, _gobject.type_from_name('guint'))

    def test_type_from_name(self):
        # A complete test is not needed here since the TYPE_* defines are created
        # using this method.
        self.assertRaises(RuntimeError, GObject.type_from_name, '!NOT_A_REAL_TYPE!')
        self.assertEqual(GObject.type_from_name('GObject'), GObject.TYPE_OBJECT)
        self.assertEqual(GObject.type_from_name('GObject'), GObject.GObject.__gtype__)

    def test_type_is_a(self):
        self.assertTrue(GObject.type_is_a(CustomBase, GObject.TYPE_OBJECT))
        self.assertTrue(GObject.type_is_a(CustomChild, CustomBase))
        self.assertTrue(GObject.type_is_a(CustomBase, GObject.GObject))
        self.assertTrue(GObject.type_is_a(CustomBase.__gtype__, GObject.TYPE_OBJECT))
        self.assertFalse(GObject.type_is_a(GObject.TYPE_OBJECT, CustomBase))
        self.assertFalse(GObject.type_is_a(CustomBase, int))  # invalid type
        self.assertRaises(TypeError, GObject.type_is_a, CustomBase, 1)
        self.assertRaises(TypeError, GObject.type_is_a, 2, GObject.TYPE_OBJECT)
        self.assertRaises(TypeError, GObject.type_is_a, 1, 2)

    def test_type_children(self):
        self.assertSequenceEqual(GObject.type_children(CustomBase),
                                 [CustomChild.__gtype__])
        self.assertEqual(len(GObject.type_children(CustomChild)), 0)

    def test_type_interfaces(self):
        self.assertEqual(len(GObject.type_interfaces(CustomBase)), 0)
        self.assertEqual(len(GObject.type_interfaces(CustomChild)), 1)
        self.assertSequenceEqual(GObject.type_interfaces(CustomChild),
                                 [GIMarshallingTests.Interface.__gtype__])

    def test_type_parent(self):
        self.assertEqual(GObject.type_parent(CustomChild), CustomBase.__gtype__)
        self.assertEqual(GObject.type_parent(CustomBase), GObject.TYPE_OBJECT)
        self.assertRaises(RuntimeError, GObject.type_parent, GObject.GObject)

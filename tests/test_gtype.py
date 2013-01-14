import unittest

from gi.repository import GObject
from gi.repository import GIMarshallingTests


class CustomBase(GObject.GObject):
    pass


class CustomChild(CustomBase, GIMarshallingTests.Interface):
    pass


class TestTypeModuleLevelFunctions(unittest.TestCase):
    def test_type_name(self):
        self.assertEqual(GObject.type_name(GObject.TYPE_NONE), 'void')
        self.assertEqual(GObject.type_name(GObject.TYPE_OBJECT), 'GObject')
        self.assertEqual(GObject.type_name(GObject.TYPE_PYOBJECT), 'PyObject')

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
        self.assertEqual(GObject.type_children(CustomBase), [CustomChild.__gtype__])
        self.assertEqual(len(GObject.type_children(CustomChild)), 0)

    def test_type_interfaces(self):
        self.assertEqual(len(GObject.type_interfaces(CustomBase)), 0)
        self.assertEqual(len(GObject.type_interfaces(CustomChild)), 1)
        self.assertEqual(GObject.type_interfaces(CustomChild), [GIMarshallingTests.Interface.__gtype__])

    def test_type_parent(self):
        self.assertEqual(GObject.type_parent(CustomChild), CustomBase.__gtype__)
        self.assertEqual(GObject.type_parent(CustomBase), GObject.TYPE_OBJECT)
        self.assertRaises(RuntimeError, GObject.type_parent, GObject.GObject)

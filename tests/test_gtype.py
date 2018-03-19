from __future__ import absolute_import

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


def test_gtype_has_value_table():
    assert CustomBase.__gtype__.has_value_table()
    assert not GIMarshallingTests.Interface.__gtype__.has_value_table()
    assert CustomChild.__gtype__.has_value_table()


def test_gtype_is_abstract():
    assert not CustomBase.__gtype__.is_abstract()
    assert not GIMarshallingTests.Interface.__gtype__.is_abstract()
    assert not CustomChild.__gtype__.is_abstract()


def test_gtype_is_classed():
    assert CustomBase.__gtype__.is_classed()
    assert not GIMarshallingTests.Interface.__gtype__.is_classed()
    assert CustomChild.__gtype__.is_classed()


def test_gtype_is_deep_derivable():
    assert CustomBase.__gtype__.is_deep_derivable()
    assert not GIMarshallingTests.Interface.__gtype__.is_deep_derivable()
    assert CustomChild.__gtype__.is_deep_derivable()


def test_gtype_is_derivable():
    assert CustomBase.__gtype__.is_derivable()
    assert GIMarshallingTests.Interface.__gtype__.is_derivable()
    assert CustomChild.__gtype__.is_derivable()


def test_gtype_is_value_abstract():
    assert not CustomBase.__gtype__.is_value_abstract()
    assert not GIMarshallingTests.Interface.__gtype__.is_value_abstract()
    assert not CustomChild.__gtype__.is_value_abstract()


def test_gtype_is_value_type():
    assert CustomBase.__gtype__.is_value_type()
    assert not GIMarshallingTests.Interface.__gtype__.is_value_type()
    assert CustomChild.__gtype__.is_value_type()


def test_gtype_children():
    assert CustomBase.__gtype__.children == [CustomChild.__gtype__]
    assert GIMarshallingTests.Interface.__gtype__.children == []
    assert CustomChild.__gtype__.children == []


def test_gtype_depth():
    assert CustomBase.__gtype__.depth == 2
    assert GIMarshallingTests.Interface.__gtype__.depth == 2
    assert CustomChild.__gtype__.depth == 3


def test_gtype_interfaces():
    assert CustomBase.__gtype__.interfaces == []
    assert GIMarshallingTests.Interface.__gtype__.interfaces == []
    assert CustomChild.__gtype__.interfaces == \
        [GIMarshallingTests.Interface.__gtype__]

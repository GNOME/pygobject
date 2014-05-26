# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# test_typeclass.py: Tests for GTypeClass related methods and marshalling.
#
# Copyright (C) 2014 Simon Feltman <sfeltman@gnome.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
# USA

import unittest

from gi.repository import GObject
from gi.repository import GIMarshallingTests


class TestCoercion(unittest.TestCase):
    def test_coerce_from_class(self):
        prop = GObject.ObjectClass.find_property(GIMarshallingTests.PropertiesObject,
                                                 'some-int')

        self.assertIsInstance(prop, GObject.GParamSpec)
        self.assertEqual(prop.name, 'some-int')
        self.assertEqual(prop.value_type, GObject.TYPE_INT)
        self.assertEqual(prop.owner_type,
                         GIMarshallingTests.PropertiesObject.__gtype__)

    def test_coerce_from_gtype(self):
        gtype = GIMarshallingTests.PropertiesObject.__gtype__
        prop = GObject.ObjectClass.find_property(gtype, 'some-int')

        self.assertIsInstance(prop, GObject.GParamSpec)
        self.assertEqual(prop.name, 'some-int')
        self.assertEqual(prop.value_type, GObject.TYPE_INT)
        self.assertEqual(prop.owner_type, gtype)

    def test_coerce_from_instance(self):
        obj = GIMarshallingTests.PropertiesObject()
        prop = GObject.ObjectClass.find_property(obj, 'some-int')

        self.assertIsInstance(prop, GObject.GParamSpec)
        self.assertEqual(prop.name, 'some-int')
        self.assertEqual(prop.value_type, GObject.TYPE_INT)
        self.assertEqual(prop.owner_type, obj.__gtype__)

    def test_marshalling_error(self):
        with self.assertRaises(TypeError):
            GObject.ObjectClass.find_property(object, 'some-int')

        with self.assertRaises(TypeError):
            GObject.ObjectClass.find_property(42, 'some-int')


if __name__ == '__main__':
    unittest.main()

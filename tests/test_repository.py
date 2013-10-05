# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2013 Simon Feltman <sfeltman@gnome.org>
#
#   test_repository.py: Test for the GIRepository module
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
import collections

import gi._gi as GIRepository
from gi.module import repository as repo
from gi.repository import GObject

try:
    import cairo
    cairo
    has_cairo = True
except ImportError:
    has_cairo = False


def find_child_info(info, getter_name, name):
    getter = getattr(info, getter_name)
    for child in getter():
        if child.get_name() == name:
            return child
    else:
        raise ValueError('child info %s not found' % name)


class Test(unittest.TestCase):
    def setUp(self):
        repo.require('GObject')
        repo.require('GIMarshallingTests')

    def test_arg_info(self):
        func_info = repo.find_by_name('GIMarshallingTests', 'array_fixed_out_struct')
        args = func_info.get_arguments()
        self.assertTrue(len(args), 1)

        arg = args[0]
        self.assertEqual(arg.get_container(), func_info)
        self.assertEqual(arg.get_direction(), GIRepository.DIRECTION_OUT)
        self.assertEqual(arg.get_name(), 'structs')
        self.assertEqual(arg.get_namespace(), 'GIMarshallingTests')
        self.assertEqual(arg.get_pytype_hint(), 'list')
        self.assertFalse(arg.is_caller_allocates())
        self.assertFalse(arg.is_optional())
        self.assertFalse(arg.is_return_value())
        self.assertFalse(arg.may_be_null())

    def test_base_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        self.assertEqual(info.__name__, 'Object')
        self.assertEqual(info.get_name(), 'Object')
        self.assertEqual(info.__module__, 'gi.repository.GIMarshallingTests')
        self.assertEqual(info.get_name_unescaped(), 'Object')
        self.assertEqual(info.get_namespace(), 'GIMarshallingTests')
        self.assertEqual(info.get_container(), None)
        info2 = repo.find_by_name('GIMarshallingTests', 'Object')
        self.assertFalse(info is info2)
        self.assertEqual(info, info2)

    def test_object_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        self.assertEqual(info.get_parent(), repo.find_by_name('GObject', 'Object'))
        self.assertTrue(isinstance(info.get_methods(), collections.Iterable))
        self.assertTrue(isinstance(info.get_fields(), collections.Iterable))
        self.assertTrue(isinstance(info.get_interfaces(), collections.Iterable))
        self.assertTrue(isinstance(info.get_constants(), collections.Iterable))
        self.assertTrue(isinstance(info.get_vfuncs(), collections.Iterable))
        self.assertFalse(info.get_abstract())
        self.assertEqual(info.get_class_struct(), repo.find_by_name('GIMarshallingTests', 'ObjectClass'))

    def test_registered_type_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        # Call these from the class because GIObjectInfo overrides them
        self.assertEqual(GIRepository.RegisteredTypeInfo.get_g_type(info),
                         GObject.type_from_name('GIMarshallingTestsObject'))

    @unittest.skipUnless(has_cairo, 'Regress needs cairo')
    def test_fundamental_object_info(self):
        repo.require('Regress')
        info = repo.find_by_name('Regress', 'TestFundamentalObject')
        self.assertTrue(info.get_abstract())

    def test_interface_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Interface')
        self.assertTrue(isinstance(info.get_methods(), collections.Iterable))
        self.assertTrue(isinstance(info.get_vfuncs(), collections.Iterable))
        self.assertTrue(isinstance(info.get_constants(), collections.Iterable))

    def test_struct_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'InterfaceIface')
        self.assertTrue(isinstance(info, GIRepository.StructInfo))
        self.assertTrue(isinstance(info.get_fields(), collections.Iterable))
        self.assertTrue(isinstance(info.get_methods(), collections.Iterable))

    def test_enum_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Enum')
        self.assertTrue(isinstance(info, GIRepository.EnumInfo))
        self.assertTrue(isinstance(info.get_values(), collections.Iterable))
        self.assertFalse(info.is_flags())

    def test_union_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Union')
        self.assertTrue(isinstance(info, GIRepository.UnionInfo))
        self.assertTrue(isinstance(info.get_fields(), collections.Iterable))
        self.assertTrue(isinstance(info.get_methods(), collections.Iterable))

    def test_field_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'InterfaceIface')
        field = find_child_info(info, 'get_fields', 'test_int8_in')
        self.assertEqual(field.get_name(), 'test_int8_in')

    def test_callable_info(self):
        func_info = repo.find_by_name('GIMarshallingTests', 'array_fixed_out_struct')
        self.assertTrue(hasattr(func_info, 'invoke'))
        self.assertTrue(isinstance(func_info.get_arguments(), collections.Iterable))

    def test_object_constructor(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        method = find_child_info(info, 'get_methods', 'new')

        self.assertTrue(isinstance(method, GIRepository.CallableInfo))
        self.assertTrue(isinstance(method, GIRepository.FunctionInfo))
        self.assertTrue(method in info.get_methods())
        self.assertEqual(method.get_name(), 'new')
        self.assertFalse(method.is_method())
        self.assertTrue(method.is_constructor())

    def test_method_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        method = find_child_info(info, 'get_methods', 'vfunc_return_value_only')

        self.assertTrue(isinstance(method, GIRepository.CallableInfo))
        self.assertTrue(isinstance(method, GIRepository.FunctionInfo))
        self.assertTrue(method in info.get_methods())
        self.assertEqual(method.get_name(), 'vfunc_return_value_only')
        self.assertFalse(method.is_constructor())

    def test_vfunc_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        invoker = find_child_info(info, 'get_methods', 'vfunc_return_value_only')
        vfunc = find_child_info(info, 'get_vfuncs', 'vfunc_return_value_only')

        self.assertTrue(isinstance(vfunc, GIRepository.CallableInfo))
        self.assertTrue(isinstance(vfunc, GIRepository.VFuncInfo))
        self.assertEqual(vfunc.get_name(), 'vfunc_return_value_only')
        self.assertEqual(vfunc.get_invoker(), invoker)


if __name__ == '__main__':
    unittest.main()

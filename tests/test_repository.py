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
from gi.repository import GLib
from gi.repository import GIMarshallingTests
from gi.repository import GIRepository as IntrospectedRepository

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
        self.assertEqual(arg.get_direction(), GIRepository.Direction.OUT)
        self.assertEqual(arg.get_name(), 'structs')
        self.assertEqual(arg.get_namespace(), 'GIMarshallingTests')
        self.assertFalse(arg.is_caller_allocates())
        self.assertFalse(arg.is_optional())
        self.assertFalse(arg.is_return_value())
        self.assertFalse(arg.may_be_null())
        self.assertEqual(arg.get_destroy(), -1)
        self.assertEqual(arg.get_ownership_transfer(), GIRepository.Transfer.NOTHING)
        self.assertEqual(arg.get_scope(), GIRepository.ScopeType.INVALID)
        self.assertEqual(arg.get_type().get_tag(), GIRepository.TypeTag.ARRAY)

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
        self.assertTrue(info.equal(info2))

    def test_object_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        self.assertEqual(info.get_parent(), repo.find_by_name('GObject', 'Object'))
        self.assertTrue(isinstance(info.get_methods(), collections.Iterable))
        self.assertTrue(isinstance(info.get_fields(), collections.Iterable))
        self.assertTrue(isinstance(info.get_interfaces(), collections.Iterable))
        self.assertTrue(isinstance(info.get_constants(), collections.Iterable))
        self.assertTrue(isinstance(info.get_vfuncs(), collections.Iterable))
        self.assertTrue(isinstance(info.get_properties(), collections.Iterable))
        self.assertFalse(info.get_abstract())
        self.assertEqual(info.get_class_struct(), repo.find_by_name('GIMarshallingTests', 'ObjectClass'))
        self.assertEqual(info.get_type_name(), 'GIMarshallingTestsObject')
        self.assertEqual(info.get_type_init(), 'gi_marshalling_tests_object_get_type')
        self.assertFalse(info.get_fundamental())
        self.assertEqual(info.get_parent(), repo.find_by_name('GObject', 'Object'))

    def test_callable_inheritance(self):
        self.assertTrue(issubclass(GIRepository.CallableInfo, GIRepository.BaseInfo))
        self.assertTrue(issubclass(GIRepository.CallbackInfo, GIRepository.CallableInfo))
        self.assertTrue(issubclass(GIRepository.FunctionInfo, GIRepository.CallableInfo))
        self.assertTrue(issubclass(GIRepository.VFuncInfo, GIRepository.CallableInfo))
        self.assertTrue(issubclass(GIRepository.SignalInfo, GIRepository.CallableInfo))

    def test_registered_type_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        # Call these from the class because GIObjectInfo overrides them
        self.assertEqual(GIRepository.RegisteredTypeInfo.get_g_type(info),
                         GObject.type_from_name('GIMarshallingTestsObject'))
        self.assertEqual(GIRepository.RegisteredTypeInfo.get_type_name(info),
                         'GIMarshallingTestsObject')
        self.assertEqual(GIRepository.RegisteredTypeInfo.get_type_init(info),
                         'gi_marshalling_tests_object_get_type')

    @unittest.skipUnless(has_cairo, 'Regress needs cairo')
    def test_fundamental_object_info(self):
        repo.require('Regress')
        info = repo.find_by_name('Regress', 'TestFundamentalObject')
        self.assertTrue(info.get_abstract())
        self.assertTrue(info.get_fundamental())
        self.assertEqual(info.get_ref_function(), 'regress_test_fundamental_object_ref')
        self.assertEqual(info.get_unref_function(), 'regress_test_fundamental_object_unref')
        self.assertEqual(info.get_get_value_function(), 'regress_test_value_get_fundamental_object')
        self.assertEqual(info.get_set_value_function(), 'regress_test_value_set_fundamental_object')

    def test_interface_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Interface')
        self.assertTrue(isinstance(info.get_methods(), collections.Iterable))
        self.assertTrue(isinstance(info.get_vfuncs(), collections.Iterable))
        self.assertTrue(isinstance(info.get_constants(), collections.Iterable))
        self.assertTrue(isinstance(info.get_prerequisites(), collections.Iterable))
        self.assertTrue(isinstance(info.get_properties(), collections.Iterable))
        self.assertTrue(isinstance(info.get_signals(), collections.Iterable))

        method = info.find_method('test_int8_in')
        vfunc = info.find_vfunc('test_int8_in')
        self.assertEqual(method.get_name(), 'test_int8_in')
        self.assertEqual(vfunc.get_invoker(), method)
        self.assertEqual(method.get_vfunc(), vfunc)

        iface = info.get_iface_struct()
        self.assertEqual(iface, repo.find_by_name('GIMarshallingTests', 'InterfaceIface'))

    def test_struct_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'InterfaceIface')
        self.assertTrue(isinstance(info, GIRepository.StructInfo))
        self.assertTrue(isinstance(info.get_fields(), collections.Iterable))
        self.assertTrue(isinstance(info.get_methods(), collections.Iterable))
        self.assertTrue(isinstance(info.get_size(), int))
        self.assertTrue(isinstance(info.get_alignment(), int))
        self.assertTrue(info.is_gtype_struct())
        self.assertFalse(info.is_foreign())

    def test_enum_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Enum')
        self.assertTrue(isinstance(info, GIRepository.EnumInfo))
        self.assertTrue(isinstance(info.get_values(), collections.Iterable))
        self.assertTrue(isinstance(info.get_methods(), collections.Iterable))
        self.assertFalse(info.is_flags())
        self.assertTrue(info.get_storage_type() > 0)  # might be platform dependent

    def test_union_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Union')
        self.assertTrue(isinstance(info, GIRepository.UnionInfo))
        self.assertTrue(isinstance(info.get_fields(), collections.Iterable))
        self.assertTrue(isinstance(info.get_methods(), collections.Iterable))

    def test_type_info(self):
        func_info = repo.find_by_name('GIMarshallingTests', 'array_fixed_out_struct')
        arg_info, = func_info.get_arguments()
        type_info = arg_info.get_type()

        self.assertTrue(type_info.is_pointer())
        self.assertEqual(type_info.get_tag(), GIRepository.TypeTag.ARRAY)
        self.assertEqual(type_info.get_tag_as_string(), 'array')
        self.assertEqual(type_info.get_param_type(0).get_tag(),
                         GIRepository.TypeTag.INTERFACE)
        self.assertEqual(type_info.get_param_type(0).get_interface(),
                         repo.find_by_name('GIMarshallingTests', 'SimpleStruct'))
        self.assertEqual(type_info.get_interface(), None)
        self.assertEqual(type_info.get_array_length(), -1)
        self.assertEqual(type_info.get_array_fixed_size(), 2)
        self.assertFalse(type_info.is_zero_terminated())
        self.assertEqual(type_info.get_array_type(), GIRepository.ArrayType.C)

    def test_field_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'InterfaceIface')
        field = find_child_info(info, 'get_fields', 'test_int8_in')
        self.assertEqual(field.get_name(), 'test_int8_in')
        self.assertTrue(field.get_flags() & GIRepository.FieldInfoFlags.IS_READABLE)
        self.assertFalse(field.get_flags() & GIRepository.FieldInfoFlags.IS_WRITABLE)
        self.assertEqual(field.get_type().get_tag(), GIRepository.TypeTag.INTERFACE)

        # don't test actual values because that might fail with architecture differences
        self.assertTrue(isinstance(field.get_size(), int))
        self.assertTrue(isinstance(field.get_offset(), int))

    def test_property_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'PropertiesObject')
        prop = find_child_info(info, 'get_properties', 'some-object')

        flags = GObject.ParamFlags.READABLE | GObject.ParamFlags.WRITABLE | GObject.ParamFlags.CONSTRUCT
        self.assertEqual(prop.get_flags(), flags)
        self.assertEqual(prop.get_type().get_tag(), GIRepository.TypeTag.INTERFACE)
        self.assertEqual(prop.get_type().get_interface(),
                         repo.find_by_name('GObject', 'Object'))
        self.assertEqual(prop.get_ownership_transfer(), GIRepository.Transfer.NOTHING)

    def test_callable_info(self):
        func_info = repo.find_by_name('GIMarshallingTests', 'array_fixed_out_struct')
        self.assertTrue(hasattr(func_info, 'invoke'))
        self.assertTrue(isinstance(func_info.get_arguments(), collections.Iterable))
        self.assertEqual(func_info.get_caller_owns(), GIRepository.Transfer.NOTHING)
        self.assertFalse(func_info.may_return_null())
        self.assertEqual(func_info.get_return_type().get_tag(), GIRepository.TypeTag.VOID)
        self.assertRaises(AttributeError, func_info.get_return_attribute, '_not_an_attr')

    @unittest.skipUnless(has_cairo, 'Regress needs cairo')
    def test_signal_info(self):
        repo.require('Regress')
        info = repo.find_by_name('Regress', 'TestObj')
        sig_info = find_child_info(info, 'get_signals', 'test')

        sig_flags = GObject.SignalFlags.RUN_LAST | \
            GObject.SignalFlags.NO_RECURSE | GObject.SignalFlags.NO_HOOKS

        self.assertTrue(sig_info is not None)
        self.assertTrue(isinstance(sig_info, GIRepository.CallableInfo))
        self.assertTrue(isinstance(sig_info, GIRepository.SignalInfo))
        self.assertEqual(sig_info.get_name(), 'test')
        self.assertEqual(sig_info.get_class_closure(), None)
        self.assertFalse(sig_info.true_stops_emit())
        self.assertEqual(sig_info.get_flags(), sig_flags)

    @unittest.skipUnless(has_cairo, 'Regress needs cairo')
    def test_notify_signal_info_with_obj(self):
        repo.require('Regress')
        info = repo.find_by_name('Regress', 'TestObj')
        sig_info = find_child_info(info, 'get_signals', 'sig-with-array-prop')

        sig_flags = GObject.SignalFlags.RUN_LAST

        self.assertTrue(sig_info is not None)
        self.assertTrue(isinstance(sig_info, GIRepository.CallableInfo))
        self.assertTrue(isinstance(sig_info, GIRepository.SignalInfo))
        self.assertEqual(sig_info.get_name(), 'sig-with-array-prop')
        self.assertEqual(sig_info.get_class_closure(), None)
        self.assertFalse(sig_info.true_stops_emit())
        self.assertEqual(sig_info.get_flags(), sig_flags)

    def test_object_constructor(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        method = find_child_info(info, 'get_methods', 'new')

        self.assertTrue(isinstance(method, GIRepository.CallableInfo))
        self.assertTrue(isinstance(method, GIRepository.FunctionInfo))
        self.assertTrue(method in info.get_methods())
        self.assertEqual(method.get_name(), 'new')
        self.assertFalse(method.is_method())
        self.assertTrue(method.is_constructor())
        self.assertEqual(method.get_symbol(), 'gi_marshalling_tests_object_new')

        flags = method.get_flags()
        self.assertFalse(flags & GIRepository.FunctionInfoFlags.IS_METHOD)
        self.assertTrue(flags & GIRepository.FunctionInfoFlags.IS_CONSTRUCTOR)
        self.assertFalse(flags & GIRepository.FunctionInfoFlags.IS_GETTER)
        self.assertFalse(flags & GIRepository.FunctionInfoFlags.IS_SETTER)
        self.assertFalse(flags & GIRepository.FunctionInfoFlags.WRAPS_VFUNC)
        self.assertFalse(flags & GIRepository.FunctionInfoFlags.THROWS)

    def test_method_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        method = find_child_info(info, 'get_methods', 'vfunc_return_value_only')

        self.assertTrue(isinstance(method, GIRepository.CallableInfo))
        self.assertTrue(isinstance(method, GIRepository.FunctionInfo))
        self.assertTrue(method in info.get_methods())
        self.assertEqual(method.get_name(), 'vfunc_return_value_only')
        self.assertFalse(method.is_constructor())
        self.assertEqual(method.get_symbol(), 'gi_marshalling_tests_object_vfunc_return_value_only')
        self.assertTrue(method.is_method())

        flags = method.get_flags()
        self.assertTrue(flags & GIRepository.FunctionInfoFlags.IS_METHOD)
        self.assertFalse(flags & GIRepository.FunctionInfoFlags.IS_CONSTRUCTOR)
        self.assertFalse(flags & GIRepository.FunctionInfoFlags.IS_GETTER)
        self.assertFalse(flags & GIRepository.FunctionInfoFlags.IS_SETTER)
        self.assertFalse(flags & GIRepository.FunctionInfoFlags.WRAPS_VFUNC)
        self.assertFalse(flags & GIRepository.FunctionInfoFlags.THROWS)

    def test_vfunc_info(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        invoker = find_child_info(info, 'get_methods', 'vfunc_return_value_only')
        vfunc = find_child_info(info, 'get_vfuncs', 'vfunc_return_value_only')

        self.assertTrue(isinstance(vfunc, GIRepository.CallableInfo))
        self.assertTrue(isinstance(vfunc, GIRepository.VFuncInfo))
        self.assertEqual(vfunc.get_name(), 'vfunc_return_value_only')
        self.assertEqual(vfunc.get_invoker(), invoker)
        self.assertEqual(invoker, info.find_method('vfunc_return_value_only'))
        self.assertEqual(vfunc.get_flags(), 0)
        self.assertEqual(vfunc.get_offset(), 0xFFFF)  # unknown offset
        self.assertEqual(vfunc.get_signal(), None)

    def test_callable_can_throw_gerror(self):
        info = repo.find_by_name('GIMarshallingTests', 'Object')
        invoker = find_child_info(info, 'get_methods', 'vfunc_meth_with_error')
        vfunc = find_child_info(info, 'get_vfuncs', 'vfunc_meth_with_err')

        self.assertTrue(invoker.can_throw_gerror())
        self.assertTrue(vfunc.can_throw_gerror())

        # Test that args do not include the GError**
        self.assertEqual(len(invoker.get_arguments()), 1)
        self.assertEqual(len(vfunc.get_arguments()), 1)

        # Sanity check method that does not throw.
        invoker_no_throws = find_child_info(info, 'get_methods', 'method_int8_in')
        self.assertFalse(invoker_no_throws.can_throw_gerror())

    def test_flags_double_registration_error(self):
        # a warning is printed for double registration and pygobject will
        # also raise a RuntimeError.
        GIMarshallingTests.NoTypeFlags  # cause flags registration
        info = repo.find_by_name('GIMarshallingTests', 'NoTypeFlags')
        old_mask = GLib.log_set_always_fatal(GLib.LogLevelFlags.LEVEL_ERROR)
        try:
            self.assertRaises(RuntimeError,
                              GIRepository.flags_register_new_gtype_and_add,
                              info)
        finally:
            GLib.log_set_always_fatal(old_mask)

    def test_enum_double_registration_error(self):
        # a warning is printed for double registration and pygobject will
        # also raise a RuntimeError.
        GIMarshallingTests.Enum  # cause enum registration
        info = repo.find_by_name('GIMarshallingTests', 'Enum')
        old_mask = GLib.log_set_always_fatal(GLib.LogLevelFlags.LEVEL_ERROR)
        try:
            self.assertRaises(RuntimeError,
                              GIRepository.enum_register_new_gtype_and_add,
                              info)
        finally:
            GLib.log_set_always_fatal(old_mask)

    def test_enums(self):
        self.assertTrue(hasattr(GIRepository, 'Direction'))
        self.assertTrue(hasattr(GIRepository, 'Transfer'))
        self.assertTrue(hasattr(GIRepository, 'ArrayType'))
        self.assertTrue(hasattr(GIRepository, 'ScopeType'))
        self.assertTrue(hasattr(GIRepository, 'VFuncInfoFlags'))
        self.assertTrue(hasattr(GIRepository, 'FieldInfoFlags'))
        self.assertTrue(hasattr(GIRepository, 'FunctionInfoFlags'))
        self.assertTrue(hasattr(GIRepository, 'TypeTag'))
        self.assertTrue(hasattr(GIRepository, 'InfoType'))

    def test_introspected_argument_info(self):
        self.assertTrue(isinstance(IntrospectedRepository.Argument.__info__,
                                   GIRepository.UnionInfo))

        arg = IntrospectedRepository.Argument()
        self.assertTrue(isinstance(arg.__info__, GIRepository.UnionInfo))

        old_info = IntrospectedRepository.Argument.__info__
        IntrospectedRepository.Argument.__info__ = 'not an info'
        self.assertRaises(TypeError, IntrospectedRepository.Argument)
        IntrospectedRepository.Argument.__info__ = old_info


if __name__ == '__main__':
    unittest.main()

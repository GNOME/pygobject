# -*- Mode: Python -*-

from __future__ import absolute_import

import sys
import gc
import unittest
import warnings

import pytest

from gi.repository import GObject, GLib, Gio
from gi import PyGIDeprecationWarning
from gi.module import get_introspection_module
from gi import _gi

import testhelper


class TestGObjectAPI(unittest.TestCase):

    def test_call_method_uninitialized_instance(self):
        obj = GObject.Object.__new__(GObject.Object)
        with self.assertRaisesRegex(RuntimeError, '.*is not initialized'):
            obj.notify("foo")

    def test_gobject_inheritance(self):
        # GObject.Object is a class hierarchy as follows:
        # overrides.Object -> introspection.Object -> static.GObject
        GIObjectModule = get_introspection_module('GObject')
        self.assertTrue(issubclass(GObject.Object, GIObjectModule.Object))
        self.assertTrue(issubclass(GIObjectModule.Object, _gi.GObject))

        self.assertEqual(_gi.GObject.__gtype__, GObject.TYPE_OBJECT)
        self.assertEqual(GIObjectModule.Object.__gtype__, GObject.TYPE_OBJECT)
        self.assertEqual(GObject.Object.__gtype__, GObject.TYPE_OBJECT)

        # The pytype wrapper should hold the outer most Object class from overrides.
        self.assertEqual(GObject.TYPE_OBJECT.pytype, GObject.Object)

    def test_gobject_unsupported_overrides(self):
        obj = GObject.Object()

        with self.assertRaisesRegex(RuntimeError, 'Data access methods are unsupported.*'):
            obj.get_data()

        with self.assertRaisesRegex(RuntimeError, 'This method is currently unsupported.*'):
            obj.force_floating()

    def test_compat_api(self):
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter('always')
            # GObject formerly exposed a lot of GLib's functions
            self.assertEqual(GObject.markup_escape_text('foo'), 'foo')

            ml = GObject.MainLoop()
            self.assertFalse(ml.is_running())

            context = GObject.main_context_default()
            self.assertTrue(context.pending() in [False, True])

            context = GObject.MainContext()
            self.assertFalse(context.pending())

            self.assertTrue(issubclass(w[0].category, PyGIDeprecationWarning))
            self.assertTrue('GLib.markup_escape_text' in str(w[0]), str(w[0]))

            self.assertLess(GObject.PRIORITY_HIGH, GObject.PRIORITY_DEFAULT)

    def test_min_max_int(self):
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', PyGIDeprecationWarning)

            self.assertEqual(GObject.G_MAXINT16, 2 ** 15 - 1)
            self.assertEqual(GObject.G_MININT16, -2 ** 15)
            self.assertEqual(GObject.G_MAXUINT16, 2 ** 16 - 1)

            self.assertEqual(GObject.G_MAXINT32, 2 ** 31 - 1)
            self.assertEqual(GObject.G_MININT32, -2 ** 31)
            self.assertEqual(GObject.G_MAXUINT32, 2 ** 32 - 1)

            self.assertEqual(GObject.G_MAXINT64, 2 ** 63 - 1)
            self.assertEqual(GObject.G_MININT64, -2 ** 63)
            self.assertEqual(GObject.G_MAXUINT64, 2 ** 64 - 1)


class TestReferenceCounting(unittest.TestCase):
    def test_regular_object(self):
        obj = GObject.GObject()
        self.assertEqual(obj.__grefcount__, 1)

        obj = GObject.new(GObject.GObject)
        self.assertEqual(obj.__grefcount__, 1)

    def test_floating(self):
        obj = testhelper.Floating()
        self.assertEqual(obj.__grefcount__, 1)

        obj = GObject.new(testhelper.Floating)
        self.assertEqual(obj.__grefcount__, 1)

    def test_owned_by_library(self):
        # Upon creation, the refcount of the object should be 2:
        # - someone already has a reference on the new object.
        # - the python wrapper should hold its own reference.
        obj = testhelper.OwnedByLibrary()
        self.assertEqual(obj.__grefcount__, 2)

        # We ask the library to release its reference, so the only
        # remaining ref should be our wrapper's. Once the wrapper
        # will run out of scope, the object will get finalized.
        obj.release()
        self.assertEqual(obj.__grefcount__, 1)

    def test_owned_by_library_out_of_scope(self):
        obj = testhelper.OwnedByLibrary()
        self.assertEqual(obj.__grefcount__, 2)

        # We are manually taking the object out of scope. This means
        # that our wrapper has been freed, and its reference dropped. We
        # cannot check it but the refcount should now be 1 (the ref held
        # by the library is still there, we didn't call release()
        obj = None

        # When we get the object back from the lib, the wrapper is
        # re-created, so our refcount will be 2 once again.
        obj = testhelper.owned_by_library_get_instance_list()[0]
        self.assertEqual(obj.__grefcount__, 2)

        obj.release()
        self.assertEqual(obj.__grefcount__, 1)

    def test_owned_by_library_using_gobject_new(self):
        # Upon creation, the refcount of the object should be 2:
        # - someone already has a reference on the new object.
        # - the python wrapper should hold its own reference.
        obj = GObject.new(testhelper.OwnedByLibrary)
        self.assertEqual(obj.__grefcount__, 2)

        # We ask the library to release its reference, so the only
        # remaining ref should be our wrapper's. Once the wrapper
        # will run out of scope, the object will get finalized.
        obj.release()
        self.assertEqual(obj.__grefcount__, 1)

    def test_owned_by_library_out_of_scope_using_gobject_new(self):
        obj = GObject.new(testhelper.OwnedByLibrary)
        self.assertEqual(obj.__grefcount__, 2)

        # We are manually taking the object out of scope. This means
        # that our wrapper has been freed, and its reference dropped. We
        # cannot check it but the refcount should now be 1 (the ref held
        # by the library is still there, we didn't call release()
        obj = None

        # When we get the object back from the lib, the wrapper is
        # re-created, so our refcount will be 2 once again.
        obj = testhelper.owned_by_library_get_instance_list()[0]
        self.assertEqual(obj.__grefcount__, 2)

        obj.release()
        self.assertEqual(obj.__grefcount__, 1)

    def test_floating_and_sunk(self):
        # Upon creation, the refcount of the object should be 2:
        # - someone already has a reference on the new object.
        # - the python wrapper should hold its own reference.
        obj = testhelper.FloatingAndSunk()
        self.assertEqual(obj.__grefcount__, 2)

        # We ask the library to release its reference, so the only
        # remaining ref should be our wrapper's. Once the wrapper
        # will run out of scope, the object will get finalized.
        obj.release()
        self.assertEqual(obj.__grefcount__, 1)

    def test_floating_and_sunk_out_of_scope(self):
        obj = testhelper.FloatingAndSunk()
        self.assertEqual(obj.__grefcount__, 2)

        # We are manually taking the object out of scope. This means
        # that our wrapper has been freed, and its reference dropped. We
        # cannot check it but the refcount should now be 1 (the ref held
        # by the library is still there, we didn't call release()
        obj = None

        # When we get the object back from the lib, the wrapper is
        # re-created, so our refcount will be 2 once again.
        obj = testhelper.floating_and_sunk_get_instance_list()[0]
        self.assertEqual(obj.__grefcount__, 2)

        obj.release()
        self.assertEqual(obj.__grefcount__, 1)

    def test_floating_and_sunk_using_gobject_new(self):
        # Upon creation, the refcount of the object should be 2:
        # - someone already has a reference on the new object.
        # - the python wrapper should hold its own reference.
        obj = GObject.new(testhelper.FloatingAndSunk)
        self.assertEqual(obj.__grefcount__, 2)

        # We ask the library to release its reference, so the only
        # remaining ref should be our wrapper's. Once the wrapper
        # will run out of scope, the object will get finalized.
        obj.release()
        self.assertEqual(obj.__grefcount__, 1)

    def test_floating_and_sunk_out_of_scope_using_gobject_new(self):
        obj = GObject.new(testhelper.FloatingAndSunk)
        self.assertEqual(obj.__grefcount__, 2)

        # We are manually taking the object out of scope. This means
        # that our wrapper has been freed, and its reference dropped. We
        # cannot check it but the refcount should now be 1 (the ref held
        # by the library is still there, we didn't call release()
        obj = None

        # When we get the object back from the lib, the wrapper is
        # re-created, so our refcount will be 2 once again.
        obj = testhelper.floating_and_sunk_get_instance_list()[0]
        self.assertEqual(obj.__grefcount__, 2)

        obj.release()
        self.assertEqual(obj.__grefcount__, 1)

    def test_uninitialized_object(self):
        class Obj(GObject.GObject):
            def __init__(self):
                x = self.__grefcount__
                super(Obj, self).__init__()
                assert x >= 0  # quiesce pyflakes

        # Accessing __grefcount__ before the object is initialized is wrong.
        # Ensure we get a proper exception instead of a crash.
        self.assertRaises(TypeError, Obj)


class A(GObject.GObject):
    def __init__(self):
        super(A, self).__init__()


class TestPythonReferenceCounting(unittest.TestCase):
    # Newly created instances should alwayshave two references: one for
    # the GC, and one for the bound variable in the local scope.

    def test_new_instance_has_two_refs(self):
        obj = GObject.GObject()
        if hasattr(sys, "getrefcount"):
            self.assertEqual(sys.getrefcount(obj), 2)

    def test_new_instance_has_two_refs_using_gobject_new(self):
        obj = GObject.new(GObject.GObject)
        if hasattr(sys, "getrefcount"):
            self.assertEqual(sys.getrefcount(obj), 2)

    def test_new_subclass_instance_has_two_refs(self):
        obj = A()
        if hasattr(sys, "getrefcount"):
            self.assertEqual(sys.getrefcount(obj), 2)

    def test_new_subclass_instance_has_two_refs_using_gobject_new(self):
        obj = GObject.new(A)
        if hasattr(sys, "getrefcount"):
            self.assertEqual(sys.getrefcount(obj), 2)


class TestContextManagers(unittest.TestCase):
    class ContextTestObject(GObject.GObject):
        prop = GObject.Property(default=0, type=int)

    def on_prop_set(self, obj, prop):
        # Handler which tracks property changed notifications.
        self.tracking.append(obj.get_property(prop.name))

    def setUp(self):
        self.tracking = []
        self.obj = self.ContextTestObject()
        self.handler = self.obj.connect('notify::prop', self.on_prop_set)

    def test_freeze_notify_context(self):
        # Verify prop tracking list
        self.assertEqual(self.tracking, [])
        self.obj.props.prop = 1
        self.assertEqual(self.tracking, [1])
        self.obj.props.prop = 2
        self.assertEqual(self.tracking, [1, 2])
        self.assertEqual(self.obj.__grefcount__, 1)

        if hasattr(sys, "getrefcount"):
            pyref_count = sys.getrefcount(self.obj)

        # Using the context manager the tracking list should not be affected.
        # The GObject reference count should stay the same and the python
        # object ref-count should go up.
        with self.obj.freeze_notify():
            self.assertEqual(self.obj.__grefcount__, 1)
            if hasattr(sys, "getrefcount"):
                self.assertEqual(sys.getrefcount(self.obj), pyref_count + 1)
            self.obj.props.prop = 3
            self.assertEqual(self.obj.props.prop, 3)
            self.assertEqual(self.tracking, [1, 2])

        # After the context manager, the prop should have been modified,
        # the tracking list will be modified, and the python object ref
        # count goes back down.
        gc.collect()
        self.assertEqual(self.obj.props.prop, 3)
        self.assertEqual(self.tracking, [1, 2, 3])
        self.assertEqual(self.obj.__grefcount__, 1)
        if hasattr(sys, "getrefcount"):
            self.assertEqual(sys.getrefcount(self.obj), pyref_count)

    def test_handler_block_context(self):
        # Verify prop tracking list
        self.assertEqual(self.tracking, [])
        self.obj.props.prop = 1
        self.assertEqual(self.tracking, [1])
        self.obj.props.prop = 2
        self.assertEqual(self.tracking, [1, 2])
        self.assertEqual(self.obj.__grefcount__, 1)

        if hasattr(sys, "getrefcount"):
            pyref_count = sys.getrefcount(self.obj)

        # Using the context manager the tracking list should not be affected.
        # The GObject reference count should stay the same and the python
        # object ref-count should go up.
        with self.obj.handler_block(self.handler):
            self.assertEqual(self.obj.__grefcount__, 1)
            if hasattr(sys, "getrefcount"):
                self.assertEqual(sys.getrefcount(self.obj), pyref_count + 1)
            self.obj.props.prop = 3
            self.assertEqual(self.obj.props.prop, 3)
            self.assertEqual(self.tracking, [1, 2])

        # After the context manager, the prop should have been modified
        # the tracking list should have stayed the same and the GObject ref
        # count goes back down.
        gc.collect()
        self.assertEqual(self.obj.props.prop, 3)
        self.assertEqual(self.tracking, [1, 2])
        self.assertEqual(self.obj.__grefcount__, 1)
        if hasattr(sys, "getrefcount"):
            self.assertEqual(sys.getrefcount(self.obj), pyref_count)

    def test_freeze_notify_context_nested(self):
        self.assertEqual(self.tracking, [])
        with self.obj.freeze_notify():
            self.obj.props.prop = 1
            self.assertEqual(self.tracking, [])

            with self.obj.freeze_notify():
                self.obj.props.prop = 2
                self.assertEqual(self.tracking, [])

                with self.obj.freeze_notify():
                    self.obj.props.prop = 3
                    self.assertEqual(self.tracking, [])
                self.assertEqual(self.tracking, [])
            self.assertEqual(self.tracking, [])

        # Finally after last context, the notifications should have collapsed
        # and the last one sent.
        self.assertEqual(self.tracking, [3])

    def test_handler_block_context_nested(self):
        self.assertEqual(self.tracking, [])
        with self.obj.handler_block(self.handler):
            self.obj.props.prop = 1
            self.assertEqual(self.tracking, [])

            with self.obj.handler_block(self.handler):
                self.obj.props.prop = 2
                self.assertEqual(self.tracking, [])

                with self.obj.handler_block(self.handler):
                    self.obj.props.prop = 3
                    self.assertEqual(self.tracking, [])
                self.assertEqual(self.tracking, [])
            self.assertEqual(self.tracking, [])

        # Finally after last context, the notifications should have collapsed
        # and the last one sent.
        self.assertEqual(self.obj.props.prop, 3)
        self.assertEqual(self.tracking, [])

    def test_freeze_notify_normal_usage_ref_counts(self):
        # Ensure ref counts without using methods as context managers
        # maintain the same count.
        self.assertEqual(self.obj.__grefcount__, 1)
        self.obj.freeze_notify()
        self.assertEqual(self.obj.__grefcount__, 1)
        self.obj.thaw_notify()
        self.assertEqual(self.obj.__grefcount__, 1)

    def test_handler_block_normal_usage_ref_counts(self):
        self.assertEqual(self.obj.__grefcount__, 1)
        self.obj.handler_block(self.handler)
        self.assertEqual(self.obj.__grefcount__, 1)
        self.obj.handler_unblock(self.handler)
        self.assertEqual(self.obj.__grefcount__, 1)

    def test_freeze_notify_context_error(self):
        # Test an exception occurring within a freeze context exits the context
        try:
            with self.obj.freeze_notify():
                self.obj.props.prop = 1
                self.assertEqual(self.tracking, [])
                raise ValueError('Simulation')
        except ValueError:
            pass

        # Verify the property set within the context called notify.
        self.assertEqual(self.obj.props.prop, 1)
        self.assertEqual(self.tracking, [1])

        # Verify we are still not in a frozen context.
        self.obj.props.prop = 2
        self.assertEqual(self.tracking, [1, 2])

    def test_handler_block_context_error(self):
        # Test an exception occurring within a handler block exits the context
        try:
            with self.obj.handler_block(self.handler):
                self.obj.props.prop = 1
                self.assertEqual(self.tracking, [])
                raise ValueError('Simulation')
        except ValueError:
            pass

        # Verify the property set within the context didn't call notify.
        self.assertEqual(self.obj.props.prop, 1)
        self.assertEqual(self.tracking, [])

        # Verify we are still not in a handler block context.
        self.obj.props.prop = 2
        self.assertEqual(self.tracking, [2])


@unittest.skipUnless(hasattr(GObject.Binding, 'unbind'),
                     'Requires newer GLib which has g_binding_unbind')
class TestPropertyBindings(unittest.TestCase):
    class TestObject(GObject.GObject):
        int_prop = GObject.Property(default=0, type=int)

    def setUp(self):
        self.source = self.TestObject()
        self.target = self.TestObject()

    def test_default_binding(self):
        binding = self.source.bind_property('int_prop', self.target, 'int_prop',
                                            GObject.BindingFlags.DEFAULT)
        binding = binding  # PyFlakes

        # Test setting value on source gets pushed to target
        self.source.int_prop = 1
        self.assertEqual(self.source.int_prop, 1)
        self.assertEqual(self.target.int_prop, 1)

        # Test setting value on target does not change source
        self.target.props.int_prop = 2
        self.assertEqual(self.source.int_prop, 1)
        self.assertEqual(self.target.int_prop, 2)

    def test_bidirectional_binding(self):
        binding = self.source.bind_property('int_prop', self.target, 'int_prop',
                                            GObject.BindingFlags.BIDIRECTIONAL)
        binding = binding  # PyFlakes

        # Test setting value on source gets pushed to target
        self.source.int_prop = 1
        self.assertEqual(self.source.int_prop, 1)
        self.assertEqual(self.target.int_prop, 1)

        # Test setting value on target also changes source
        self.target.props.int_prop = 2
        self.assertEqual(self.source.int_prop, 2)
        self.assertEqual(self.target.int_prop, 2)

    def test_transform_to_only(self):
        def transform_to(binding, value, user_data=None):
            self.assertEqual(user_data, 'test-data')
            return value * 2

        binding = self.source.bind_property('int_prop', self.target, 'int_prop',
                                            GObject.BindingFlags.DEFAULT,
                                            transform_to, None, 'test-data')
        binding = binding  # PyFlakes

        self.source.int_prop = 1
        self.assertEqual(self.source.int_prop, 1)
        self.assertEqual(self.target.int_prop, 2)

        self.target.props.int_prop = 1
        self.assertEqual(self.source.int_prop, 1)
        self.assertEqual(self.target.int_prop, 1)

    def test_transform_from_only(self):
        def transform_from(binding, value, user_data=None):
            self.assertEqual(user_data, None)
            return value * 2

        binding = self.source.bind_property('int_prop', self.target, 'int_prop',
                                            GObject.BindingFlags.BIDIRECTIONAL,
                                            None, transform_from)
        binding = binding  # PyFlakes

        self.source.int_prop = 1
        self.assertEqual(self.source.int_prop, 1)
        self.assertEqual(self.target.int_prop, 1)

        self.target.props.int_prop = 1
        self.assertEqual(self.source.int_prop, 2)
        self.assertEqual(self.target.int_prop, 1)

    def test_transform_bidirectional(self):
        test_data = object()

        def transform_to(binding, value, user_data=None):
            self.assertEqual(user_data, test_data)
            return value * 2

        def transform_from(binding, value, user_data=None):
            self.assertEqual(user_data, test_data)
            return value // 2

        if hasattr(sys, "getrefcount"):
            test_data_ref_count = sys.getrefcount(test_data)
            transform_to_ref_count = sys.getrefcount(transform_to)
            transform_from_ref_count = sys.getrefcount(transform_from)

        # bidirectional bindings
        binding = self.source.bind_property('int_prop', self.target, 'int_prop',
                                            GObject.BindingFlags.BIDIRECTIONAL,
                                            transform_to, transform_from, test_data)
        binding = binding  # PyFlakes
        if hasattr(sys, "getrefcount"):
            binding_ref_count = sys.getrefcount(binding)
            binding_gref_count = binding.__grefcount__

        self.source.int_prop = 1
        self.assertEqual(self.source.int_prop, 1)
        self.assertEqual(self.target.int_prop, 2)

        self.target.props.int_prop = 4
        self.assertEqual(self.source.int_prop, 2)
        self.assertEqual(self.target.int_prop, 4)

        if hasattr(sys, "getrefcount"):
            self.assertEqual(sys.getrefcount(binding), binding_ref_count)
            self.assertEqual(binding.__grefcount__, binding_gref_count)

        # test_data ref count increases by 2, once for each callback.
        if hasattr(sys, "getrefcount"):
            self.assertEqual(sys.getrefcount(test_data), test_data_ref_count + 2)
            self.assertEqual(sys.getrefcount(transform_to), transform_to_ref_count + 1)
            self.assertEqual(sys.getrefcount(transform_from), transform_from_ref_count + 1)

        # Unbind should clear out the binding and its transforms
        binding.unbind()

        # Setting source or target should not change the other.
        self.target.int_prop = 3
        self.source.int_prop = 5
        self.assertEqual(self.target.int_prop, 3)
        self.assertEqual(self.source.int_prop, 5)

        if hasattr(sys, "getrefcount"):
            self.assertEqual(sys.getrefcount(test_data), test_data_ref_count)
            self.assertEqual(sys.getrefcount(transform_to), transform_to_ref_count)
            self.assertEqual(sys.getrefcount(transform_from), transform_from_ref_count)

    def test_explicit_unbind_clears_connection(self):
        self.assertEqual(self.source.int_prop, 0)
        self.assertEqual(self.target.int_prop, 0)

        # Test deleting binding reference removes binding.
        binding = self.source.bind_property('int_prop', self.target, 'int_prop')
        self.source.int_prop = 1
        self.assertEqual(self.source.int_prop, 1)
        self.assertEqual(self.target.int_prop, 1)

        # unbind should clear out the bindings self reference
        binding.unbind()
        self.assertEqual(binding.__grefcount__, 1)

        self.source.int_prop = 10
        self.assertEqual(self.source.int_prop, 10)
        self.assertEqual(self.target.int_prop, 1)

        glib_version = (GLib.MAJOR_VERSION, GLib.MINOR_VERSION, GLib.MICRO_VERSION)

        # calling unbind() on an already unbound binding
        if glib_version >= (2, 57, 3):
            # Fixed in newer glib:
            # https://gitlab.gnome.org/GNOME/glib/merge_requests/244
            for i in range(10):
                binding.unbind()
        else:
            self.assertRaises(ValueError, binding.unbind)

    def test_reference_counts(self):
        self.assertEqual(self.source.__grefcount__, 1)
        self.assertEqual(self.target.__grefcount__, 1)

        # Binding ref count will be 2 do to the initial ref implicitly held by
        # the act of binding and the ref incurred by using __call__ to generate
        # a wrapper from the weak binding ref within python.
        binding = self.source.bind_property('int_prop', self.target, 'int_prop')
        self.assertEqual(binding.__grefcount__, 2)

        # Creating a binding does not inc refs on source and target (they are weak
        # on the binding object itself)
        self.assertEqual(self.source.__grefcount__, 1)
        self.assertEqual(self.target.__grefcount__, 1)

        # Use GObject.get_property because the "props" accessor leaks.
        # Note property names are canonicalized.
        self.assertEqual(binding.get_property('source'), self.source)
        self.assertEqual(binding.get_property('source_property'), 'int-prop')
        self.assertEqual(binding.get_property('target'), self.target)
        self.assertEqual(binding.get_property('target_property'), 'int-prop')
        self.assertEqual(binding.get_property('flags'), GObject.BindingFlags.DEFAULT)

        # Delete reference to source or target and the binding will remove its own
        # "self reference".
        ref = self.source.weak_ref()
        del self.source
        gc.collect()
        self.assertEqual(ref(), None)
        self.assertEqual(binding.__grefcount__, 1)

        # Finally clear out the last ref held by the python wrapper
        ref = binding.weak_ref()
        del binding
        gc.collect()
        self.assertEqual(ref(), None)


class TestGValue(unittest.TestCase):
    def test_type_constant(self):
        self.assertEqual(GObject.TYPE_VALUE, GObject.Value.__gtype__)
        self.assertEqual(GObject.type_name(GObject.TYPE_VALUE), 'GValue')

    def test_no_type(self):
        value = GObject.Value()
        self.assertEqual(value.g_type, GObject.TYPE_INVALID)
        self.assertRaises(TypeError, value.set_value, 23)
        self.assertEqual(value.get_value(), None)

    def test_int(self):
        value = GObject.Value(GObject.TYPE_UINT)
        self.assertEqual(value.g_type, GObject.TYPE_UINT)
        value.set_value(23)
        self.assertEqual(value.get_value(), 23)
        value.set_value(42.0)
        self.assertEqual(value.get_value(), 42)

    def test_multi_del(self):
        value = GObject.Value(str, 'foo_bar')
        value.__del__()
        value.__del__()
        del value

    def test_string(self):
        value = GObject.Value(str, 'foo_bar')
        self.assertEqual(value.g_type, GObject.TYPE_STRING)
        self.assertEqual(value.get_value(), 'foo_bar')

    def test_float(self):
        # python float is G_TYPE_DOUBLE
        value = GObject.Value(float, 23.4)
        self.assertEqual(value.g_type, GObject.TYPE_DOUBLE)
        value.set_value(1e50)
        self.assertAlmostEqual(value.get_value(), 1e50)

        value = GObject.Value(GObject.TYPE_FLOAT, 23.4)
        self.assertEqual(value.g_type, GObject.TYPE_FLOAT)
        self.assertRaises(TypeError, value.set_value, 'string')
        self.assertRaises(OverflowError, value.set_value, 1e50)

    def test_float_inf_nan(self):
        nan = float('nan')
        for type_ in [GObject.TYPE_FLOAT, GObject.TYPE_DOUBLE]:
            for x in [float('inf'), float('-inf'), nan]:
                value = GObject.Value(type_, x)
                # assertEqual() is False for (nan, nan)
                if x is nan:
                    self.assertEqual(str(value.get_value()), 'nan')
                else:
                    self.assertEqual(value.get_value(), x)

    def test_enum(self):
        value = GObject.Value(GLib.FileError, GLib.FileError.FAILED)
        self.assertEqual(value.get_value(), GLib.FileError.FAILED)

    def test_flags(self):
        value = GObject.Value(GLib.IOFlags, GLib.IOFlags.IS_READABLE)
        self.assertEqual(value.get_value(), GLib.IOFlags.IS_READABLE)

    def test_object(self):
        class TestObject(GObject.Object):
            pass
        obj = TestObject()
        value = GObject.Value(GObject.TYPE_OBJECT, obj)
        self.assertEqual(value.get_value(), obj)

    def test_value_array(self):
        value = GObject.Value(GObject.ValueArray)
        self.assertEqual(value.g_type, GObject.type_from_name('GValueArray'))
        value.set_value([32, 'foo_bar', 0.3])
        self.assertEqual(value.get_value(), [32, 'foo_bar', 0.3])

    def test_value_array_from_gvalue_list(self):
        value = GObject.Value(GObject.ValueArray, [
            GObject.Value(GObject.TYPE_UINT, 0xffffffff),
            GObject.Value(GObject.TYPE_STRING, 'foo_bar')])
        self.assertEqual(value.g_type, GObject.type_from_name('GValueArray'))
        self.assertEqual(value.get_value(), [0xffffffff, 'foo_bar'])
        self.assertEqual(testhelper.value_array_get_nth_type(value, 0), GObject.TYPE_UINT)
        self.assertEqual(testhelper.value_array_get_nth_type(value, 1), GObject.TYPE_STRING)

    def test_value_array_append_gvalue(self):
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)

            arr = GObject.ValueArray.new(0)
            arr.append(GObject.Value(GObject.TYPE_UINT, 0xffffffff))
            arr.append(GObject.Value(GObject.TYPE_STRING, 'foo_bar'))
            self.assertEqual(arr.get_nth(0), 0xffffffff)
            self.assertEqual(arr.get_nth(1), 'foo_bar')
            self.assertEqual(testhelper.value_array_get_nth_type(arr, 0), GObject.TYPE_UINT)
            self.assertEqual(testhelper.value_array_get_nth_type(arr, 1), GObject.TYPE_STRING)

    def test_gerror_boxing(self):
        error = GLib.Error('test message', domain='mydomain', code=42)
        value = GObject.Value(GLib.Error, error)
        self.assertEqual(value.g_type, GObject.type_from_name('GError'))

        unboxed = value.get_value()
        self.assertEqual(unboxed.message, error.message)
        self.assertEqual(unboxed.domain, error.domain)
        self.assertEqual(unboxed.code, error.code)

    def test_gerror_novalue(self):
        GLib.Error('test message', domain='mydomain', code=42)
        value = GObject.Value(GLib.Error)
        self.assertEqual(value.g_type, GObject.type_from_name('GError'))
        self.assertEqual(value.get_value(), None)


def test_list_properties():

    def find_param(l, name):
        for param in l:
            if param.name == name:
                return param
        return

    list_props = GObject.list_properties

    props = list_props(Gio.Action)
    param = find_param(props, "enabled")
    assert param
    assert param.value_type == GObject.TYPE_BOOLEAN
    assert list_props("GAction") == list_props(Gio.Action)
    assert list_props(Gio.Action.__gtype__) == list_props(Gio.Action)

    props = list_props(Gio.SimpleAction)
    assert find_param(props, "enabled")

    def names(l):
        return [p.name for p in l]

    assert (set(names(list_props(Gio.Action))) <=
            set(names(list_props(Gio.SimpleAction))))

    props = list_props(Gio.FileIcon)
    param = find_param(props, "file")
    assert param
    assert param.value_type == Gio.File.__gtype__

    assert list_props("GFileIcon") == list_props(Gio.FileIcon)
    assert list_props(Gio.FileIcon.__gtype__) == list_props(Gio.FileIcon)
    assert list_props(Gio.FileIcon()) == list_props(Gio.FileIcon)

    for obj in [Gio.ActionEntry, Gio.DBusError, 0, object()]:
        with pytest.raises(TypeError):
            list_props(obj)

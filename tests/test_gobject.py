# -*- Mode: Python -*-

import gc
import unittest
import warnings

from gi.repository import GObject
from gi import PyGIDeprecationWarning
import sys
import testhelper


class TestGObjectAPI(unittest.TestCase):
    def test_module(self):
        obj = GObject.GObject()

        self.assertEqual(obj.__module__,
                         'gi._gobject._gobject')

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
        self.assertEqual(sys.getrefcount(obj), 2)

    def test_new_instance_has_two_refs_using_gobject_new(self):
        obj = GObject.new(GObject.GObject)
        self.assertEqual(sys.getrefcount(obj), 2)

    def test_new_subclass_instance_has_two_refs(self):
        obj = A()
        self.assertEqual(sys.getrefcount(obj), 2)

    def test_new_subclass_instance_has_two_refs_using_gobject_new(self):
        obj = GObject.new(A)
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

        # Using the context manager the tracking list should not be affected
        # and the GObject reference count should go up.
        with self.obj.freeze_notify():
            self.assertEqual(self.obj.__grefcount__, 2)
            self.obj.props.prop = 3
            self.assertEqual(self.obj.props.prop, 3)
            self.assertEqual(self.tracking, [1, 2])

        # After the context manager, the prop should have been modified,
        # the tracking list will be modified, and the GObject ref
        # count goes back down.
        self.assertEqual(self.obj.props.prop, 3)
        self.assertEqual(self.tracking, [1, 2, 3])
        self.assertEqual(self.obj.__grefcount__, 1)

    def test_handler_block_context(self):
        # Verify prop tracking list
        self.assertEqual(self.tracking, [])
        self.obj.props.prop = 1
        self.assertEqual(self.tracking, [1])
        self.obj.props.prop = 2
        self.assertEqual(self.tracking, [1, 2])
        self.assertEqual(self.obj.__grefcount__, 1)

        # Using the context manager the tracking list should not be affected
        # and the GObject reference count should go up.
        with self.obj.handler_block(self.handler):
            self.assertEqual(self.obj.__grefcount__, 2)
            self.obj.props.prop = 3
            self.assertEqual(self.obj.props.prop, 3)
            self.assertEqual(self.tracking, [1, 2])

        # After the context manager, the prop should have been modified
        # the tracking list should have stayed the same and the GObject ref
        # count goes back down.
        self.assertEqual(self.obj.props.prop, 3)
        self.assertEqual(self.tracking, [1, 2])
        self.assertEqual(self.obj.__grefcount__, 1)

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
        def transform_to(binding, value, user_data=None):
            self.assertEqual(user_data, 'test-data')
            return value * 2

        def transform_from(binding, value, user_data=None):
            self.assertEqual(user_data, 'test-data')
            return value / 2

        # bidirectional bindings
        binding = self.source.bind_property('int_prop', self.target, 'int_prop',
                                            GObject.BindingFlags.BIDIRECTIONAL,
                                            transform_to, transform_from, 'test-data')
        binding = binding  # PyFlakes

        self.source.int_prop = 1
        self.assertEqual(self.source.int_prop, 1)
        self.assertEqual(self.target.int_prop, 2)

        self.target.props.int_prop = 4
        self.assertEqual(self.source.int_prop, 2)
        self.assertEqual(self.target.int_prop, 4)

    def test_explicit_unbind_clears_connection(self):
        self.assertEqual(self.source.int_prop, 0)
        self.assertEqual(self.target.int_prop, 0)

        # Test deleting binding reference removes binding.
        binding = self.source.bind_property('int_prop', self.target, 'int_prop')
        self.source.int_prop = 1
        self.assertEqual(self.source.int_prop, 1)
        self.assertEqual(self.target.int_prop, 1)

        binding.unbind()
        self.assertEqual(binding(), None)

        self.source.int_prop = 10
        self.assertEqual(self.source.int_prop, 10)
        self.assertEqual(self.target.int_prop, 1)

        # An already unbound BindingWeakRef will raise if unbind is attempted a second time.
        self.assertRaises(ValueError, binding.unbind)

    def test_reference_counts(self):
        self.assertEqual(self.source.__grefcount__, 1)
        self.assertEqual(self.target.__grefcount__, 1)

        # Binding ref count will be 2 do to the initial ref implicitly held by
        # the act of binding and the ref incurred by using __call__ to generate
        # a wrapper from the weak binding ref within python.
        binding = self.source.bind_property('int_prop', self.target, 'int_prop')
        self.assertEqual(binding().__grefcount__, 2)

        # Creating a binding does not inc refs on source and target (they are weak
        # on the binding object itself)
        self.assertEqual(self.source.__grefcount__, 1)
        self.assertEqual(self.target.__grefcount__, 1)

        # Use GObject.get_property because the "props" accessor leaks.
        # Note property names are canonicalized.
        self.assertEqual(binding().get_property('source'), self.source)
        self.assertEqual(binding().get_property('source_property'), 'int-prop')
        self.assertEqual(binding().get_property('target'), self.target)
        self.assertEqual(binding().get_property('target_property'), 'int-prop')
        self.assertEqual(binding().get_property('flags'), GObject.BindingFlags.DEFAULT)

        # Delete reference to source or target and the binding should listen.
        ref = self.source.weak_ref()
        del self.source
        gc.collect()
        self.assertEqual(ref(), None)
        self.assertEqual(binding(), None)

if __name__ == '__main__':
    unittest.main()

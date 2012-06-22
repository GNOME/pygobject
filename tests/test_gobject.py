# -*- Mode: Python -*-

import gc
import unittest

from gi.repository import GObject
import sys
import testhelper


class TestGObjectAPI(unittest.TestCase):
    def testGObjectModule(self):
        obj = GObject.GObject()

        self.assertEqual(obj.__module__,
                         'gi._gobject._gobject')


class TestReferenceCounting(unittest.TestCase):
    def testRegularObject(self):
        obj = GObject.GObject()
        self.assertEqual(obj.__grefcount__, 1)

        obj = GObject.new(GObject.GObject)
        self.assertEqual(obj.__grefcount__, 1)

    def testFloating(self):
        obj = testhelper.Floating()
        self.assertEqual(obj.__grefcount__, 1)

        obj = GObject.new(testhelper.Floating)
        self.assertEqual(obj.__grefcount__, 1)

    def testOwnedByLibrary(self):
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

    def testOwnedByLibraryOutOfScope(self):
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

    def testOwnedByLibraryUsingGObjectNew(self):
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

    def testOwnedByLibraryOutOfScopeUsingGobjectNew(self):
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

    def testFloatingAndSunk(self):
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

    def testFloatingAndSunkOutOfScope(self):
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

    def testFloatingAndSunkUsingGObjectNew(self):
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

    def testFloatingAndSunkOutOfScopeUsingGObjectNew(self):
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

    def testUninitializedObject(self):
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

    def testNewInstanceHasTwoRefs(self):
        obj = GObject.GObject()
        self.assertEqual(sys.getrefcount(obj), 2)

    def testNewInstanceHasTwoRefsUsingGObjectNew(self):
        obj = GObject.new(GObject.GObject)
        self.assertEqual(sys.getrefcount(obj), 2)

    def testNewSubclassInstanceHasTwoRefs(self):
        obj = A()
        self.assertEqual(sys.getrefcount(obj), 2)

    def testNewSubclassInstanceHasTwoRefsUsingGObjectNew(self):
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

    def testFreezeNotifyContext(self):
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

    def testHandlerBlockContext(self):
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

    def testFreezeNotifyContextNested(self):
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

    def testHandlerBlockContextNested(self):
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

    def testFreezeNotifyNormalUsageRefCounts(self):
        # Ensure ref counts without using methods as context managers
        # maintain the same count.
        self.assertEqual(self.obj.__grefcount__, 1)
        self.obj.freeze_notify()
        self.assertEqual(self.obj.__grefcount__, 1)
        self.obj.thaw_notify()
        self.assertEqual(self.obj.__grefcount__, 1)

    def testHandlerBlockNormalUsageRefCounts(self):
        self.assertEqual(self.obj.__grefcount__, 1)
        self.obj.handler_block(self.handler)
        self.assertEqual(self.obj.__grefcount__, 1)
        self.obj.handler_unblock(self.handler)
        self.assertEqual(self.obj.__grefcount__, 1)

    def testFreezeNotifyContextError(self):
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

    def testHandlerBlockContextError(self):
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

    def testDefaultBinding(self):
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

    def testBiDirectionalBinding(self):
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

    def testTransformToOnly(self):
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

    def testTransformFromOnly(self):
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

    def testTransformBidrectional(self):
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

    def testExplicitUnbindClearsConnection(self):
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

    def testReferenceCounts(self):
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

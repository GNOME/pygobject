# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest
import weakref
import gc
import sys
import warnings

from gi.repository import GObject
from gi.repository import GIMarshallingTests

try:
    from gi.repository import Regress
except ImportError:
    Regress = None


class StrongRef(object):
    # A class that behaves like weakref.ref but holds a strong reference.
    # This allows re-use of the VFuncsBase by swapping out the ObjectRef
    # class var with either weakref.ref or StrongRef.

    def __init__(self, obj):
        self.obj = obj

    def __call__(self):
        return self.obj


class VFuncsBase(GIMarshallingTests.Object):
    # Class which generically implements the vfuncs used for reference counting tests
    # in a way that can be easily sub-classed and modified.

    #: Object type used by this class for testing
    #: This can be GObject.Object or GObject.InitiallyUnowned
    Object = GObject.Object

    #: Reference type used by this class for holding refs to in/out objects.
    #: This can be set to weakref.ref or StrongRef
    ObjectRef = weakref.ref

    def __init__(self):
        super(VFuncsBase, self).__init__()

        #: Hold ref of input or output python wrappers
        self.object_ref = None

        #: store grefcount of input object
        self.in_object_grefcount = None
        self.in_object_is_floating = None

    def do_vfunc_return_object_transfer_none(self):
        # Return an object but keep a python reference to it.
        obj = self.Object()
        self.object_ref = self.ObjectRef(obj)
        return obj

    def do_vfunc_return_object_transfer_full(self):
        # Return an object and hand off the reference to the caller.
        obj = self.Object()
        self.object_ref = self.ObjectRef(obj)
        return obj

    def do_vfunc_out_object_transfer_none(self):
        # Same as do_vfunc_return_object_transfer_none but the pygi
        # internals convert the return here into an out arg.
        obj = self.Object()
        self.object_ref = self.ObjectRef(obj)
        return obj

    def do_vfunc_out_object_transfer_full(self):
        # Same as do_vfunc_return_object_transfer_full but the pygi
        # internals convert the return here into an out arg.
        obj = self.Object()
        self.object_ref = self.ObjectRef(obj)
        return obj

    def do_vfunc_in_object_transfer_none(self, obj):
        # 'obj' will have a python wrapper as well as still held
        # by the caller.
        self.object_ref = self.ObjectRef(obj)
        self.in_object_grefcount = obj.__grefcount__
        self.in_object_is_floating = obj.is_floating()

    def do_vfunc_in_object_transfer_full(self, obj):
        # 'obj' will now be owned by the Python GObject wrapper.
        # When obj goes out of scope and is collected, the GObject
        # should also be fully released.
        self.object_ref = self.ObjectRef(obj)
        self.in_object_grefcount = obj.__grefcount__
        self.in_object_is_floating = obj.is_floating()


class TestVFuncsWithObjectArg(unittest.TestCase):
    # Basic set of tests which work on non-floating objects which python does
    # not keep an additional reference of.

    class VFuncs(VFuncsBase):
        # Object for testing non-floating objects without holding any refs.
        Object = GObject.Object
        ObjectRef = weakref.ref

    def test_vfunc_self_arg_ref_count(self):
        # Check to make sure vfunc "self" arguments don't leak.
        vfuncs = self.VFuncs()
        vfuncs_ref = weakref.ref(vfuncs)
        vfuncs.get_ref_info_for_vfunc_return_object_transfer_full()  # Use any vfunc to test this.

        gc.collect()
        if hasattr(sys, "getrefcount"):
            self.assertEqual(sys.getrefcount(vfuncs), 2)
        self.assertEqual(vfuncs.__grefcount__, 1)

        del vfuncs
        gc.collect()
        self.assertTrue(vfuncs_ref() is None)

    def test_vfunc_return_object_transfer_none(self):
        # This tests a problem case where the vfunc returns a GObject owned solely by Python
        # but the argument is marked as transfer-none.
        # In this case pygobject marshaling adds an additional ref and gives a warning
        # of a potential leak. If this occures it is really a bug in the underlying library
        # but pygobject tries to react to this in a reasonable way.
        vfuncs = self.VFuncs()
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_out_object_transfer_none()
            if hasattr(sys, "getrefcount"):
                self.assertTrue(issubclass(warn[0].category, RuntimeWarning))

        # The ref count of the GObject returned to the caller (get_ref_info_for_vfunc_return_object_transfer_none)
        # should be a single floating ref
        if hasattr(sys, "getrefcount"):
            self.assertEqual(ref_count, 1)
        self.assertFalse(is_floating)

        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)

    def test_vfunc_out_object_transfer_none(self):
        # Same as above except uses out arg instead of return
        vfuncs = self.VFuncs()
        with warnings.catch_warnings(record=True) as warn:
            warnings.simplefilter('always')
            ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_out_object_transfer_none()
            if hasattr(sys, "getrefcount"):
                self.assertTrue(issubclass(warn[0].category, RuntimeWarning))

        if hasattr(sys, "getrefcount"):
            self.assertEqual(ref_count, 1)
        self.assertFalse(is_floating)

        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)

    def test_vfunc_return_object_transfer_full(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_return_object_transfer_full()

        # The vfunc caller receives full ownership of a single ref which should not
        # be floating.
        if hasattr(sys, "getrefcount"):
            self.assertEqual(ref_count, 1)
        self.assertFalse(is_floating)

        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)

    def test_vfunc_out_object_transfer_full(self):
        # Same as above except uses out arg instead of return
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_out_object_transfer_full()

        if hasattr(sys, "getrefcount"):
            self.assertEqual(ref_count, 1)
        self.assertFalse(is_floating)

        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)

    def test_vfunc_in_object_transfer_none(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_in_object_transfer_none(self.VFuncs.Object)

        gc.collect()
        self.assertEqual(vfuncs.in_object_grefcount, 2)  # initial + python wrapper
        self.assertFalse(vfuncs.in_object_is_floating)

        if hasattr(sys, "getrefcount"):
            self.assertEqual(ref_count, 1)  # ensure python wrapper released
        self.assertFalse(is_floating)

        gc.collect()
        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)

    def test_vfunc_in_object_transfer_full(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_in_object_transfer_full(self.VFuncs.Object)

        gc.collect()

        # python wrapper should take sole ownership of the gobject
        self.assertEqual(vfuncs.in_object_grefcount, 1)
        self.assertFalse(vfuncs.in_object_is_floating)

        # ensure python wrapper took ownership and released, after vfunc was complete
        if hasattr(sys, "getrefcount"):
            self.assertEqual(ref_count, 0)
        self.assertFalse(is_floating)

        gc.collect()
        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)


class TestVFuncsWithFloatingArg(unittest.TestCase):
    # All tests here work with a floating object by using InitiallyUnowned as the argument

    class VFuncs(VFuncsBase):
        # Object for testing non-floating objects without holding any refs.
        Object = GObject.InitiallyUnowned
        ObjectRef = weakref.ref

    @unittest.skipUnless(hasattr(sys, "getrefcount"), "refcount specific")
    def test_vfunc_return_object_transfer_none_with_floating(self):
        # Python is expected to return a single floating reference without warning.
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_return_object_transfer_none()

        # The ref count of the GObject returned to the caller (get_ref_info_for_vfunc_return_object_transfer_none)
        # should be a single floating ref
        self.assertEqual(ref_count, 1)
        self.assertTrue(is_floating)

        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)

    @unittest.skipUnless(hasattr(sys, "getrefcount"), "refcount specific")
    def test_vfunc_out_object_transfer_none_with_floating(self):
        # Same as above except uses out arg instead of return
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_out_object_transfer_none()

        self.assertEqual(ref_count, 1)
        self.assertTrue(is_floating)

        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)

    def test_vfunc_return_object_transfer_full_with_floating(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_return_object_transfer_full()

        # The vfunc caller receives full ownership of a single ref.
        if hasattr(sys, "getrefcount"):
            self.assertEqual(ref_count, 1)
        self.assertFalse(is_floating)

        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)

    def test_vfunc_out_object_transfer_full_with_floating(self):
        # Same as above except uses out arg instead of return
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_out_object_transfer_full()

        if hasattr(sys, "getrefcount"):
            self.assertEqual(ref_count, 1)
        self.assertFalse(is_floating)

        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)

    def test_vfunc_in_object_transfer_none_with_floating(self):
        vfuncs = self.VFuncs()

        def ref(obj):
            obj._ref()
            # FIXME: Add an item dictionary so that the python wrapper cannot
            # be destroyed because it is unused. Unfortunately, a weakref does
            # not trigger this handling (though it may be possible through
            # object resurrection).
            obj._something = None
            return weakref.ref(obj)

        vfuncs.ObjectRef = ref

        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_in_object_transfer_none(self.VFuncs.Object)

        gc.collect()

        # python wrapper should maintain the object as floating and add an additional ref
        # So, two plus the one we added explicitly. No extra toggle reference to keep the wrapper alive.
        self.assertEqual(vfuncs.in_object_grefcount, 2 + 1)
        self.assertTrue(vfuncs.in_object_is_floating)

        # vfunc caller should only have a single floating ref after the vfunc finishes
        # Add to that the explicit reference we added and then the toggle reference for the dict
        if hasattr(sys, "getrefcount"):
            self.assertEqual(ref_count, 1 + 2)
        self.assertTrue(is_floating)

        # There are two references now, one explicit and one from the wrapper
        self.assertEqual(vfuncs.object_ref().__grefcount__, 2)
        self.assertTrue(vfuncs.object_ref().is_floating())
        # Sinking and unref'ing our "C" one will drop both (as the second is a toggle ref)
        vfuncs.object_ref()._ref_sink()
        vfuncs.object_ref()._unref()

        gc.collect()
        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)

    def test_vfunc_in_object_transfer_full_with_floating(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_in_object_transfer_full(self.VFuncs.Object)

        gc.collect()

        # python wrapper sinks and owns the gobject
        self.assertEqual(vfuncs.in_object_grefcount, 1)
        self.assertFalse(vfuncs.in_object_is_floating)

        # ensure python wrapper took ownership and released
        if hasattr(sys, "getrefcount"):
            self.assertEqual(ref_count, 0)
        self.assertFalse(is_floating)

        gc.collect()
        gc.collect()
        self.assertTrue(vfuncs.object_ref() is None)


class TestVFuncsWithHeldObjectArg(unittest.TestCase):
    # Same tests as TestVFuncsWithObjectArg except we hold
    # onto the python object reference in all cases.

    class VFuncs(VFuncsBase):
        # Object for testing non-floating objects with a held ref.
        Object = GObject.Object
        ObjectRef = StrongRef

    def test_vfunc_return_object_transfer_none_with_held_object(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_return_object_transfer_none()

        # Python holds the single gobject ref in 'vfuncs.object_ref'
        # Because of this, we do not expect a floating ref or a ref increase.
        self.assertEqual(ref_count, 1)
        self.assertFalse(is_floating)

        # The actual grefcount should stay at 1 even after the vfunc return.
        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)
        self.assertFalse(vfuncs.in_object_is_floating)

        held_object_ref = weakref.ref(vfuncs.object_ref)
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)

    def test_vfunc_out_object_transfer_none_with_held_object(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_out_object_transfer_none()

        self.assertEqual(ref_count, 1)
        self.assertFalse(is_floating)

        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)
        self.assertFalse(vfuncs.in_object_is_floating)

        held_object_ref = weakref.ref(vfuncs.object_ref)
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)

    def test_vfunc_return_object_transfer_full_with_held_object(self):
        # The vfunc caller receives full ownership which should not
        # be floating. However, the held python wrapper also has a ref.

        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_return_object_transfer_full()

        # Ref count from the perspective of C after the vfunc is called
        # The vfunc caller receives a new reference which should not
        # be floating. However, the held python wrapper also has a ref.
        self.assertEqual(ref_count, 2)
        self.assertFalse(is_floating)

        # Current ref count
        # The vfunc caller should have decremented its reference.
        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)

        held_object_ref = weakref.ref(vfuncs.object_ref)
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)

    def test_vfunc_out_object_transfer_full_with_held_object(self):
        # Same test as above except uses out arg instead of return
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_out_object_transfer_full()

        # Ref count from the perspective of C after the vfunc is called
        # The vfunc caller receives a new reference which should not
        # be floating. However, the held python wrapper also has a ref.
        self.assertEqual(ref_count, 2)
        self.assertFalse(is_floating)

        # Current ref count
        # The vfunc caller should have decremented its reference.
        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)

        held_object_ref = weakref.ref(vfuncs.object_ref())
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)

    def test_vfunc_in_object_transfer_none_with_held_object(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_in_object_transfer_none(self.VFuncs.Object)

        gc.collect()

        # Ref count inside vfunc from the perspective of Python
        self.assertEqual(vfuncs.in_object_grefcount, 2)  # initial + python wrapper
        self.assertFalse(vfuncs.in_object_is_floating)

        # Ref count from the perspective of C after the vfunc is called
        self.assertEqual(ref_count, 2)  # kept after vfunc + held python wrapper
        self.assertFalse(is_floating)

        # Current ref count after C cleans up its reference
        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)

        held_object_ref = weakref.ref(vfuncs.object_ref())
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)

    def test_vfunc_in_object_transfer_full_with_held_object(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_in_object_transfer_full(self.VFuncs.Object)

        gc.collect()

        # Ref count inside vfunc from the perspective of Python
        self.assertEqual(vfuncs.in_object_grefcount, 1)  # python wrapper takes ownership of the gobject
        self.assertFalse(vfuncs.in_object_is_floating)

        # Ref count from the perspective of C after the vfunc is called
        self.assertEqual(ref_count, 1)
        self.assertFalse(is_floating)

        # Current ref count
        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)

        held_object_ref = weakref.ref(vfuncs.object_ref())
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)


class TestVFuncsWithHeldFloatingArg(unittest.TestCase):
    # Tests for a floating object which we hold a reference to the python wrapper
    # on the VFuncs test class.

    class VFuncs(VFuncsBase):
        # Object for testing floating objects with a held ref.
        Object = GObject.InitiallyUnowned
        ObjectRef = StrongRef

    def test_vfunc_return_object_transfer_none_with_held_floating(self):
        # Python holds onto the wrapper which basically means the floating ref
        # should also be owned by python.

        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_return_object_transfer_none()

        # This is a borrowed ref from what is held in python.
        self.assertEqual(ref_count, 1)
        self.assertFalse(is_floating)

        # The actual grefcount should stay at 1 even after the vfunc return.
        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)

        held_object_ref = weakref.ref(vfuncs.object_ref)
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)

    def test_vfunc_out_object_transfer_none_with_held_floating(self):
        # Same as above

        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_out_object_transfer_none()

        self.assertEqual(ref_count, 1)
        self.assertFalse(is_floating)

        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)

        held_object_ref = weakref.ref(vfuncs.object_ref)
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)

    def test_vfunc_return_object_transfer_full_with_held_floating(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_return_object_transfer_full()

        # Ref count from the perspective of C after the vfunc is called
        self.assertEqual(ref_count, 2)
        self.assertFalse(is_floating)

        # Current ref count
        # vfunc wrapper destroyes ref it was given
        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)

        held_object_ref = weakref.ref(vfuncs.object_ref)
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)

    def test_vfunc_out_object_transfer_full_with_held_floating(self):
        # Same test as above except uses out arg instead of return
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_out_object_transfer_full()

        # Ref count from the perspective of C after the vfunc is called
        self.assertEqual(ref_count, 2)
        self.assertFalse(is_floating)

        # Current ref count
        # vfunc wrapper destroyes ref it was given
        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)

        held_object_ref = weakref.ref(vfuncs.object_ref())
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)

    def test_vfunc_in_floating_transfer_none_with_held_floating(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_in_object_transfer_none(self.VFuncs.Object)
        gc.collect()

        # Ref count inside vfunc from the perspective of Python
        self.assertTrue(vfuncs.in_object_is_floating)
        self.assertEqual(vfuncs.in_object_grefcount, 2)  # python wrapper sinks and owns the gobject

        # Ref count from the perspective of C after the vfunc is called
        self.assertTrue(is_floating)
        self.assertEqual(ref_count, 2)  # floating + held by wrapper

        # Current ref count after C cleans up its reference
        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)

        # Our reference is still floating at this point ...
        self.assertTrue(vfuncs.object_ref().is_floating())
        # ... usually it should have been sunk within C at some point,
        # do it here to avoid a critical warning.
        vfuncs.object_ref()._ref_sink()
        # However, that means that the above comment of "sinks and owns" is wrong.

        held_object_ref = weakref.ref(vfuncs.object_ref())
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)

    def test_vfunc_in_floating_transfer_full_with_held_floating(self):
        vfuncs = self.VFuncs()
        ref_count, is_floating = vfuncs.get_ref_info_for_vfunc_in_object_transfer_full(self.VFuncs.Object)
        gc.collect()

        # Ref count from the perspective of C after the vfunc is called
        self.assertEqual(vfuncs.in_object_grefcount, 1)  # python wrapper sinks and owns the gobject
        self.assertFalse(vfuncs.in_object_is_floating)

        # Ref count from the perspective of C after the vfunc is called
        self.assertEqual(ref_count, 1)  # held by wrapper
        self.assertFalse(is_floating)

        # Current ref count
        self.assertEqual(vfuncs.object_ref().__grefcount__, 1)

        held_object_ref = weakref.ref(vfuncs.object_ref())
        del vfuncs.object_ref
        gc.collect()
        self.assertTrue(held_object_ref() is None)


@unittest.skipIf(Regress is None, 'Regress is required')
class TestArgumentTypeErrors(unittest.TestCase):
    def test_object_argument_type_error(self):
        # ensure TypeError is raised for things which are not GObjects
        obj = Regress.TestObj()
        obj.set_bare(GObject.Object())
        obj.set_bare(None)

        self.assertRaises(TypeError, obj.set_bare, object())
        self.assertRaises(TypeError, obj.set_bare, 42)
        self.assertRaises(TypeError, obj.set_bare, 'not an object')

    def test_instance_argument_error(self):
        # ensure TypeError is raised for non Regress.TestObj instances.
        obj = Regress.TestObj()
        self.assertEqual(Regress.TestObj.instance_method(obj), -1)
        self.assertRaises(TypeError, Regress.TestObj.instance_method, object())
        self.assertRaises(TypeError, Regress.TestObj.instance_method, GObject.Object())
        self.assertRaises(TypeError, Regress.TestObj.instance_method, 42)
        self.assertRaises(TypeError, Regress.TestObj.instance_method, 'not an object')

    def test_instance_argument_base_type_error(self):
        # ensure TypeError is raised when a base type is passed to something
        # expecting a derived type
        obj = Regress.TestSubObj()
        self.assertEqual(Regress.TestSubObj.instance_method(obj), 0)
        self.assertRaises(TypeError, Regress.TestSubObj.instance_method, GObject.Object())
        self.assertRaises(TypeError, Regress.TestSubObj.instance_method, Regress.TestObj())

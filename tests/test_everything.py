# -*- Mode: Python; py-indent-offset: 4 -*-
# coding=utf-8
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest

import sys
sys.path.insert(0, "../")
from sys import getrefcount

import copy
import cairo

from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
from gi.repository import Regress as Everything

if sys.version_info < (3, 0):
    UNICHAR = "\xe2\x99\xa5"
    PY2_UNICODE_UNICHAR = unicode(UNICHAR, 'UTF-8')
else:
    UNICHAR = "♥"


class TestEverything(unittest.TestCase):

    def test_cairo_context(self):
        context = Everything.test_cairo_context_full_return()
        self.assertTrue(isinstance(context, cairo.Context))

        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        context = cairo.Context(surface)
        Everything.test_cairo_context_none_in(context)

    def test_cairo_surface(self):
        surface = Everything.test_cairo_surface_none_return()
        self.assertTrue(isinstance(surface, cairo.ImageSurface))
        self.assertTrue(isinstance(surface, cairo.Surface))
        self.assertEqual(surface.get_format(), cairo.FORMAT_ARGB32)
        self.assertEqual(surface.get_width(), 10)
        self.assertEqual(surface.get_height(), 10)

        surface = Everything.test_cairo_surface_full_return()
        self.assertTrue(isinstance(surface, cairo.ImageSurface))
        self.assertTrue(isinstance(surface, cairo.Surface))
        self.assertEqual(surface.get_format(), cairo.FORMAT_ARGB32)
        self.assertEqual(surface.get_width(), 10)
        self.assertEqual(surface.get_height(), 10)

        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        Everything.test_cairo_surface_none_in(surface)

        surface = Everything.test_cairo_surface_full_out()
        self.assertTrue(isinstance(surface, cairo.ImageSurface))
        self.assertTrue(isinstance(surface, cairo.Surface))
        self.assertEqual(surface.get_format(), cairo.FORMAT_ARGB32)
        self.assertEqual(surface.get_width(), 10)
        self.assertEqual(surface.get_height(), 10)

    def test_unichar(self):
        self.assertEqual("c", Everything.test_unichar("c"))

        if sys.version_info < (3, 0):
            self.assertEqual(UNICHAR, Everything.test_unichar(PY2_UNICODE_UNICHAR))
        self.assertEqual(UNICHAR, Everything.test_unichar(UNICHAR))
        self.assertRaises(TypeError, Everything.test_unichar, "")
        self.assertRaises(TypeError, Everything.test_unichar, "morethanonechar")

    def test_floating(self):
        e = Everything.TestFloating()
        self.assertEqual(e.__grefcount__, 1)

        e = GObject.new(Everything.TestFloating)
        self.assertEqual(e.__grefcount__, 1)

        e = Everything.TestFloating.new()
        self.assertEqual(e.__grefcount__, 1)

    def test_caller_allocates(self):
        struct_a = Everything.TestStructA()
        struct_a.some_int = 10
        struct_a.some_int8 = 21
        struct_a.some_double = 3.14
        struct_a.some_enum = Everything.TestEnum.VALUE3

        struct_a_clone = struct_a.clone()
        self.assertTrue(struct_a != struct_a_clone)
        self.assertEqual(struct_a.some_int, struct_a_clone.some_int)
        self.assertEqual(struct_a.some_int8, struct_a_clone.some_int8)
        self.assertEqual(struct_a.some_double, struct_a_clone.some_double)
        self.assertEqual(struct_a.some_enum, struct_a_clone.some_enum)

        struct_b = Everything.TestStructB()
        struct_b.some_int8 = 8
        struct_b.nested_a.some_int = 20
        struct_b.nested_a.some_int8 = 12
        struct_b.nested_a.some_double = 333.3333
        struct_b.nested_a.some_enum = Everything.TestEnum.VALUE2

        struct_b_clone = struct_b.clone()
        self.assertTrue(struct_b != struct_b_clone)
        self.assertEqual(struct_b.some_int8, struct_b_clone.some_int8)
        self.assertEqual(struct_b.nested_a.some_int, struct_b_clone.nested_a.some_int)
        self.assertEqual(struct_b.nested_a.some_int8, struct_b_clone.nested_a.some_int8)
        self.assertEqual(struct_b.nested_a.some_double, struct_b_clone.nested_a.some_double)
        self.assertEqual(struct_b.nested_a.some_enum, struct_b_clone.nested_a.some_enum)

    def test_wrong_type_of_arguments(self):
        try:
            Everything.test_int8()
        except TypeError:
            (e_type, e) = sys.exc_info()[:2]
            self.assertEqual(e.args, ("test_int8() takes exactly 1 argument (0 given)",))

    def test_gtypes(self):
        gchararray_gtype = GObject.type_from_name('gchararray')
        gtype = Everything.test_gtype(str)
        self.assertEqual(gchararray_gtype, gtype)
        gtype = Everything.test_gtype('gchararray')
        self.assertEqual(gchararray_gtype, gtype)
        gobject_gtype = GObject.GObject.__gtype__
        gtype = Everything.test_gtype(GObject.GObject)
        self.assertEqual(gobject_gtype, gtype)
        gtype = Everything.test_gtype('GObject')
        self.assertEqual(gobject_gtype, gtype)
        self.assertRaises(TypeError, Everything.test_gtype, 'invalidgtype')

        class NotARegisteredClass(object):
            pass

        self.assertRaises(TypeError, Everything.test_gtype, NotARegisteredClass)

        class ARegisteredClass(GObject.GObject):
            __gtype_name__ = 'EverythingTestsARegisteredClass'

        gtype = Everything.test_gtype('EverythingTestsARegisteredClass')
        self.assertEqual(ARegisteredClass.__gtype__, gtype)
        gtype = Everything.test_gtype(ARegisteredClass)
        self.assertEqual(ARegisteredClass.__gtype__, gtype)
        self.assertRaises(TypeError, Everything.test_gtype, 'ARegisteredClass')

    def test_dir(self):
        attr_list = dir(Everything)

        # test that typelib attributes are listed
        self.assertTrue('TestStructA' in attr_list)

        # test that class attributes and methods are listed
        self.assertTrue('__class__' in attr_list)
        self.assertTrue('__dir__' in attr_list)
        self.assertTrue('__repr__' in attr_list)

        # test that instance members are listed
        self.assertTrue('_namespace' in attr_list)
        self.assertTrue('_version' in attr_list)

        # test that there are no duplicates returned
        self.assertEqual(len(attr_list), len(set(attr_list)))

    def test_ptrarray(self):
        # transfer container
        result = Everything.test_garray_container_return()
        self.assertEqual(result, ['regress'])
        result = None

        # transfer full
        result = Everything.test_garray_full_return()
        self.assertEqual(result, ['regress'])
        result = None

    def test_hash_return(self):
        result = Everything.test_ghash_gvalue_return()
        self.assertEqual(result['integer'], 12)
        self.assertEqual(result['boolean'], True)
        self.assertEqual(result['string'], 'some text')
        strings = result['strings']
        self.assertTrue('first' in strings)
        self.assertTrue('second' in strings)
        self.assertTrue('third' in strings)
        result = None

    @unittest.skip('marshalling of GStrv in GValue not working')
    def test_hash_in_out(self):
        result = Everything.test_ghash_gvalue_return()
        # The following line will cause a failure, because we don't support
        # marshalling GStrv in GValue.
        Everything.test_ghash_gvalue_in(result)
        result = None

    def test_struct_gpointer(self):
        l1 = GLib.List()
        self.assertEqual(l1.data, None)
        init_refcount = getrefcount(l1)

        l1.data = 'foo'
        self.assertEqual(l1.data, 'foo')

        l2 = l1
        self.assertEqual(l1.data, l2.data)
        self.assertEqual(getrefcount(l1), init_refcount + 1)

        l3 = copy.copy(l1)
        l3.data = 'bar'
        self.assertEqual(l1.data, 'foo')
        self.assertEqual(l2.data, 'foo')
        self.assertEqual(l3.data, 'bar')
        self.assertEqual(getrefcount(l1), init_refcount + 1)
        self.assertEqual(getrefcount(l3), init_refcount)


class TestNullableArgs(unittest.TestCase):
    def test_in_nullable_hash(self):
        Everything.test_ghash_null_in(None)

    def test_in_nullable_list(self):
        Everything.test_gslist_null_in(None)
        Everything.test_glist_null_in(None)
        Everything.test_gslist_null_in([])
        Everything.test_glist_null_in([])

    def test_in_nullable_array(self):
        Everything.test_array_int_null_in(None)
        Everything.test_array_int_null_in([])

    def test_in_nullable_string(self):
        Everything.test_utf8_null_in(None)

    def test_in_nullable_object(self):
        Everything.func_obj_null_in(None)

    def test_out_nullable_hash(self):
        self.assertEqual(None, Everything.test_ghash_null_out())

    def test_out_nullable_list(self):
        self.assertEqual([], Everything.test_gslist_null_out())
        self.assertEqual([], Everything.test_glist_null_out())

    def test_out_nullable_array(self):
        self.assertEqual([], Everything.test_array_int_null_out())

    def test_out_nullable_string(self):
        self.assertEqual(None, Everything.test_utf8_null_out())

    def test_out_nullable_object(self):
        self.assertEqual(None, Everything.TestObj.null_out())


class TestCallbacks(unittest.TestCase):
    called = False
    main_loop = GObject.MainLoop()

    def test_callback(self):
        TestCallbacks.called = False

        def callback():
            TestCallbacks.called = True

        Everything.test_simple_callback(callback)
        self.assertTrue(TestCallbacks.called)

    def test_callback_exception(self):
        """
        This test ensures that we get errors from callbacks correctly
        and in particular that we do not segv when callbacks fail
        """
        def callback():
            x = 1 / 0
            self.fail('unexpected surviving zero divsion:' + str(x))

        # note that we do NOT expect the ZeroDivisionError to be propagated
        # through from the callback, as it crosses the Python<->C boundary
        # twice. (See GNOME #616279)
        Everything.test_simple_callback(callback)

    def test_double_callback_exception(self):
        """
        This test ensures that we get errors from callbacks correctly
        and in particular that we do not segv when callbacks fail
        """
        def badcallback():
            x = 1 / 0
            self.fail('unexpected surviving zero divsion:' + str(x))

        def callback():
            Everything.test_boolean(True)
            Everything.test_boolean(False)
            Everything.test_simple_callback(badcallback())

        # note that we do NOT expect the ZeroDivisionError to be propagated
        # through from the callback, as it crosses the Python<->C boundary
        # twice. (See GNOME #616279)
        Everything.test_simple_callback(callback)

    def test_return_value_callback(self):
        TestCallbacks.called = False

        def callback():
            TestCallbacks.called = True
            return 44

        self.assertEqual(Everything.test_callback(callback), 44)
        self.assertTrue(TestCallbacks.called)

    def test_callback_async(self):
        TestCallbacks.called = False

        def callback(foo):
            TestCallbacks.called = True
            return foo

        Everything.test_callback_async(callback, 44)
        i = Everything.test_callback_thaw_async()
        self.assertEqual(44, i)
        self.assertTrue(TestCallbacks.called)

    def test_callback_scope_call(self):
        TestCallbacks.called = 0

        def callback():
            TestCallbacks.called += 1
            return 0

        Everything.test_multi_callback(callback)
        self.assertEqual(TestCallbacks.called, 2)

    def test_callback_userdata(self):
        TestCallbacks.called = 0

        def callback(userdata):
            self.assertEqual(userdata, "Test%d" % TestCallbacks.called)
            TestCallbacks.called += 1
            return TestCallbacks.called

        for i in range(100):
            val = Everything.test_callback_user_data(callback, "Test%d" % i)
            self.assertEqual(val, i + 1)

        self.assertEqual(TestCallbacks.called, 100)

    def test_callback_userdata_refcount(self):
        TestCallbacks.called = False

        def callback(userdata):
            TestCallbacks.called = True
            return 1

        ud = "Test User Data"

        start_ref_count = getrefcount(ud)
        for i in range(100):
            Everything.test_callback_destroy_notify(callback, ud)

        Everything.test_callback_thaw_notifications()
        end_ref_count = getrefcount(ud)

        self.assertEqual(start_ref_count, end_ref_count)

    def test_async_ready_callback(self):
        TestCallbacks.called = False
        TestCallbacks.main_loop = GObject.MainLoop()

        def callback(obj, result, user_data):
            TestCallbacks.main_loop.quit()
            TestCallbacks.called = True

        Everything.test_async_ready_callback(callback)

        TestCallbacks.main_loop.run()

        self.assertTrue(TestCallbacks.called)

    def test_callback_destroy_notify(self):
        def callback(user_data):
            TestCallbacks.called = True
            return 42

        TestCallbacks.called = False
        self.assertEqual(Everything.test_callback_destroy_notify(callback, 42), 42)
        self.assertTrue(TestCallbacks.called)
        self.assertEqual(Everything.test_callback_thaw_notifications(), 42)

    def test_callback_in_methods(self):
        object_ = Everything.TestObj()

        def callback():
            TestCallbacks.called = True
            return 42

        TestCallbacks.called = False
        object_.instance_method_callback(callback)
        self.assertTrue(TestCallbacks.called)

        TestCallbacks.called = False
        Everything.TestObj.static_method_callback(callback)
        self.assertTrue(TestCallbacks.called)

        def callbackWithUserData(user_data):
            TestCallbacks.called = True
            return 42

        TestCallbacks.called = False
        Everything.TestObj.new_callback(callbackWithUserData, None)
        self.assertTrue(TestCallbacks.called)

    def test_callback_none(self):
        # make sure this doesn't assert or crash
        Everything.test_simple_callback(None)

    def test_callback_gerror(self):
        def callback(error):
            self.assertEqual(error.message, 'regression test error')
            self.assertTrue('g-io' in error.domain)
            self.assertEqual(error.code, Gio.IOErrorEnum.NOT_SUPPORTED)
            TestCallbacks.called = True

        TestCallbacks.called = False
        Everything.test_gerror_callback(callback)
        self.assertTrue(TestCallbacks.called)

    def test_callback_null_gerror(self):
        def callback(error):
            self.assertEqual(error, None)
            TestCallbacks.called = True

        TestCallbacks.called = False
        Everything.test_null_gerror_callback(callback)
        self.assertTrue(TestCallbacks.called)

    def test_callback_owned_gerror(self):
        def callback(error):
            self.assertEqual(error.message, 'regression test owned error')
            self.assertTrue('g-io' in error.domain)
            self.assertEqual(error.code, Gio.IOErrorEnum.PERMISSION_DENIED)
            TestCallbacks.called = True

        TestCallbacks.called = False
        Everything.test_owned_gerror_callback(callback)
        self.assertTrue(TestCallbacks.called)

    def test_callback_hashtable(self):
        def callback(data):
            self.assertEqual(data, mydict)
            mydict['new'] = 42
            TestCallbacks.called = True

        mydict = {'foo': 1, 'bar': 2}
        TestCallbacks.called = False
        Everything.test_hash_table_callback(mydict, callback)
        self.assertTrue(TestCallbacks.called)
        self.assertEqual(mydict, {'foo': 1, 'bar': 2, 'new': 42})


class TestClosures(unittest.TestCase):
    def test_int_arg(self):
        def callback(num):
            self.called = True
            return num + 1

        self.called = False
        result = Everything.test_closure_one_arg(callback, 42)
        self.assertTrue(self.called)
        self.assertEqual(result, 43)

    # https://bugzilla.gnome.org/show_bug.cgi?id=656554

    @unittest.expectedFailure
    def test_variant(self):
        def callback(variant):
            self.assertEqual(variant.get_type_string(), 'i')
            self.called = True
            return GLib.Variant('i', variant.get_int32() + 1)

        self.called = False
        result = Everything.test_closure_variant(callback, GLib.Variant('i', 42))
        self.assertTrue(self.called)
        self.assertEqual(result.get_type_string(), 'i')
        self.assertEqual(result.get_int32(), 43)


class TestProperties(unittest.TestCase):

    def test_basic(self):
        object_ = Everything.TestObj()

        self.assertEqual(object_.props.int, 0)
        object_.props.int = 42
        self.assertTrue(isinstance(object_.props.int, int))
        self.assertEqual(object_.props.int, 42)

        self.assertEqual(object_.props.float, 0.0)
        object_.props.float = 42.42
        self.assertTrue(isinstance(object_.props.float, float))
        self.assertAlmostEquals(object_.props.float, 42.42, places=5)

        self.assertEqual(object_.props.double, 0.0)
        object_.props.double = 42.42
        self.assertTrue(isinstance(object_.props.double, float))
        self.assertAlmostEquals(object_.props.double, 42.42, places=5)

        self.assertEqual(object_.props.string, None)
        object_.props.string = 'mec'
        self.assertTrue(isinstance(object_.props.string, str))
        self.assertEqual(object_.props.string, 'mec')

        self.assertEqual(object_.props.gtype, GObject.TYPE_INVALID)
        object_.props.gtype = int
        self.assertEqual(object_.props.gtype, GObject.TYPE_INT)

    def test_hash_table(self):
        object_ = Everything.TestObj()
        self.assertEqual(object_.props.hash_table, None)

        object_.props.hash_table = {'mec': 56}
        self.assertTrue(isinstance(object_.props.hash_table, dict))
        self.assertEqual(list(object_.props.hash_table.items())[0], ('mec', 56))

    def test_list(self):
        object_ = Everything.TestObj()
        self.assertEqual(object_.props.list, [])

        object_.props.list = ['1', '2', '3']
        self.assertTrue(isinstance(object_.props.list, list))
        self.assertEqual(object_.props.list, ['1', '2', '3'])

    def test_boxed(self):
        object_ = Everything.TestObj()
        self.assertEqual(object_.props.boxed, None)

        boxed = Everything.TestBoxed()
        boxed.some_int8 = 42
        object_.props.boxed = boxed

        self.assertTrue(isinstance(object_.props.boxed, Everything.TestBoxed))
        self.assertEqual(object_.props.boxed.some_int8, 42)

    def test_gtype(self):
        object_ = Everything.TestObj()
        self.assertEqual(object_.props.gtype, GObject.TYPE_INVALID)
        object_.props.gtype = int
        self.assertEqual(object_.props.gtype, GObject.TYPE_INT)

        object_ = Everything.TestObj(gtype=int)
        self.assertEqual(object_.props.gtype, GObject.TYPE_INT)
        object_.props.gtype = str
        self.assertEqual(object_.props.gtype, GObject.TYPE_STRING)


class TestTortureProfile(unittest.TestCase):
    def test_torture_profile(self):
        import time
        total_time = 0
        print("")
        object_ = Everything.TestObj()
        sys.stdout.write("\ttorture test 1 (10000 iterations): ")

        start_time = time.clock()
        for i in range(10000):
            (y, z, q) = object_.torture_signature_0(5000,
                                                    "Torture Test 1",
                                                    12345)

        end_time = time.clock()
        delta_time = end_time - start_time
        total_time += delta_time
        print("%f secs" % delta_time)

        sys.stdout.write("\ttorture test 2 (10000 iterations): ")

        start_time = time.clock()
        for i in range(10000):
            (y, z, q) = Everything.TestObj().torture_signature_0(
                5000, "Torture Test 2", 12345)

        end_time = time.clock()
        delta_time = end_time - start_time
        total_time += delta_time
        print("%f secs" % delta_time)

        sys.stdout.write("\ttorture test 3 (10000 iterations): ")
        start_time = time.clock()
        for i in range(10000):
            try:
                (y, z, q) = object_.torture_signature_1(
                    5000, "Torture Test 3", 12345)
            except:
                pass
        end_time = time.clock()
        delta_time = end_time - start_time
        total_time += delta_time
        print("%f secs" % delta_time)

        sys.stdout.write("\ttorture test 4 (10000 iterations): ")

        def callback(userdata):
            pass

        userdata = [1, 2, 3, 4]
        start_time = time.clock()
        for i in range(10000):
            (y, z, q) = Everything.test_torture_signature_2(
                5000, callback, userdata, "Torture Test 4", 12345)
        end_time = time.clock()
        delta_time = end_time - start_time
        total_time += delta_time
        print("%f secs" % delta_time)
        print("\t====")
        print("\tTotal: %f sec" % total_time)


class TestAdvancedInterfaces(unittest.TestCase):
    def test_array_objs(self):
        obj1, obj2 = Everything.test_array_fixed_out_objects()
        self.assertTrue(isinstance(obj1, Everything.TestObj))
        self.assertTrue(isinstance(obj2, Everything.TestObj))
        self.assertNotEqual(obj1, obj2)

    def test_obj_skip_return_val(self):
        obj = Everything.TestObj()
        ret = obj.skip_return_val(50, 42.0, 60, 2, 3)
        self.assertEqual(len(ret), 3)
        self.assertEqual(ret[0], 51)
        self.assertEqual(ret[1], 61)
        self.assertEqual(ret[2], 32)

    def test_obj_skip_return_val_no_out(self):
        obj = Everything.TestObj()
        # raises an error for 0, succeeds for any other value
        self.assertRaises(GLib.GError, obj.skip_return_val_no_out, 0)

        ret = obj.skip_return_val_no_out(1)
        self.assertEqual(ret, None)


class TestSignals(unittest.TestCase):
    def test_object_param_signal(self):
        obj = Everything.TestObj()

        def callback(obj, obj_param):
            self.assertEqual(obj_param.props.int, 3)
            self.assertGreater(obj_param.__grefcount__, 1)

        obj.connect('sig-with-obj', callback)
        obj.emit_sig_with_obj()

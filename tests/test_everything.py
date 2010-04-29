# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import unittest

import sys
sys.path.insert(0, "../")

import cairo

from gi.repository import GObject
from gi.repository import Everything

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
        self.assertEquals(surface.get_format(), cairo.FORMAT_ARGB32)
        self.assertEquals(surface.get_width(), 10)
        self.assertEquals(surface.get_height(), 10)

        surface = Everything.test_cairo_surface_full_return()
        self.assertTrue(isinstance(surface, cairo.ImageSurface))
        self.assertTrue(isinstance(surface, cairo.Surface))
        self.assertEquals(surface.get_format(), cairo.FORMAT_ARGB32)
        self.assertEquals(surface.get_width(), 10)
        self.assertEquals(surface.get_height(), 10)

        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 10, 10)
        Everything.test_cairo_surface_none_in(surface)

        surface = Everything.test_cairo_surface_full_out()
        self.assertTrue(isinstance(surface, cairo.ImageSurface))
        self.assertTrue(isinstance(surface, cairo.Surface))
        self.assertEquals(surface.get_format(), cairo.FORMAT_ARGB32)
        self.assertEquals(surface.get_width(), 10)
        self.assertEquals(surface.get_height(), 10)


class TestNullableArgs(unittest.TestCase):
    def test_in_nullable_hash(self):
        Everything.test_ghash_null_in(None)

    def test_in_nullable_list(self):
        Everything.test_gslist_null_in(None)
        Everything.test_glist_null_in(None)

    def test_in_nullable_array(self):
        Everything.test_array_int_null_in(None, -1)

    def test_in_nullable_string(self):
        Everything.test_utf8_null_in(None)

    def test_out_nullable_hash(self):
        self.assertEqual(None, Everything.test_ghash_null_out())

    def test_out_nullable_list(self):
        self.assertEqual(None, Everything.test_gslist_null_out())
        self.assertEqual(None, Everything.test_glist_null_out())

    def test_out_nullable_array(self):
        self.assertEqual((None, 0), Everything.test_array_int_null_out())

    def test_out_nullable_string(self):
        self.assertEqual(None, Everything.test_utf8_null_out())

class TestCallbacks(unittest.TestCase):
    called = False
    main_loop = GObject.MainLoop()

    def testCallback(self):
        TestCallbacks.called = False
        def callback():
            TestCallbacks.called = True
        
        Everything.test_simple_callback(callback)
        self.assertTrue(TestCallbacks.called)

    def testCallbackException(self):
        """
        This test ensures that we get errors from callbacks correctly
        and in particular that we do not segv when callbacks fail
        """
        def callback():
            x = 1 / 0
            
        try:
            Everything.test_simple_callback(callback)
        except ZeroDivisionError:
            pass

    def testDoubleCallbackException(self):
        """
        This test ensures that we get errors from callbacks correctly
        and in particular that we do not segv when callbacks fail
        """
        def badcallback():
            x = 1 / 0

        def callback():
            Everything.test_boolean(True)
            Everything.test_boolean(False)
            Everything.test_simple_callback(badcallback())

        try:
            Everything.test_simple_callback(callback)
        except ZeroDivisionError:
            pass

    def testReturnValueCallback(self):
        TestCallbacks.called = False
        def callback():
            TestCallbacks.called = True
            return 44

        self.assertEquals(Everything.test_callback(callback), 44)
        self.assertTrue(TestCallbacks.called)
    
    def testCallbackAsync(self):
        TestCallbacks.called = False
        def callback(foo):
            TestCallbacks.called = True
            return foo

        Everything.test_callback_async(callback, 44);
        i = Everything.test_callback_thaw_async();
        self.assertEquals(44, i);
        self.assertTrue(TestCallbacks.called)

    def testCallbackScopeCall(self):
        TestCallbacks.called = 0
        def callback():
            TestCallbacks.called += 1
            return 0

        Everything.test_multi_callback(callback)
        self.assertEquals(TestCallbacks.called, 2)

    def testCallbackUserdata(self):
        TestCallbacks.called = 0
        def callback(userdata):
            self.assertEquals(userdata, "Test%d" % TestCallbacks.called)
            TestCallbacks.called += 1
            return TestCallbacks.called
        
        for i in range(100):
            val = Everything.test_callback_user_data(callback, "Test%d" % i)
            self.assertEquals(val, i+1)
            
        self.assertEquals(TestCallbacks.called, 100)

    def testAsyncReadyCallback(self):
        TestCallbacks.called = False
        TestCallbacks.main_loop = GObject.MainLoop()

        def callback(obj, result, user_data):
            TestCallbacks.main_loop.quit()
            TestCallbacks.called = True

        Everything.test_async_ready_callback(callback)

        TestCallbacks.main_loop.run()

        self.assertTrue(TestCallbacks.called)

    def testCallbackDestroyNotify(self):
        def callback(user_data):
            TestCallbacks.called = True
            return 42

        TestCallbacks.called = False
        self.assertEquals(Everything.test_callback_destroy_notify(callback, 42), 42)
        self.assertTrue(TestCallbacks.called)
        self.assertEquals(Everything.test_callback_thaw_notifications(), 42)

    def testCallbackInMethods(self):
        object_ = Everything.TestObj()

        def callback():
            TestCallbacks.called = True

        TestCallbacks.called = False
        object_.instance_method_callback(callback)
        self.assertTrue(TestCallbacks.called)

        TestCallbacks.called = False
        Everything.TestObj.static_method_callback(callback)
        self.assertTrue(TestCallbacks.called)

        def callbackWithUserData(user_data):
            TestCallbacks.called = True

        TestCallbacks.called = False
        obj_ = Everything.TestObj.new_callback(callbackWithUserData, None)
        self.assertTrue(TestCallbacks.called)

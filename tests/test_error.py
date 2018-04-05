# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# test_error.py: Tests for GError wrapper implementation
#
# Copyright (C) 2012 Will Thompson
# Copyright (C) 2013 Martin Pitt
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

from __future__ import absolute_import

import unittest
import pickle

from gi.repository import GLib
from gi.repository import GIMarshallingTests


class TestType(unittest.TestCase):
    def test_attributes(self):
        e = GLib.Error('test message', 'mydomain', 42)
        self.assertEqual(e.message, 'test message')
        self.assertEqual(e.domain, 'mydomain')
        self.assertEqual(e.code, 42)

    def test_new_literal(self):
        mydomain = GLib.quark_from_string('mydomain')
        e = GLib.Error.new_literal(mydomain, 'test message', 42)
        self.assertEqual(e.message, 'test message')
        self.assertEqual(e.domain, 'mydomain')
        self.assertEqual(e.code, 42)

    def test_matches(self):
        mydomain = GLib.quark_from_string('mydomain')
        notmydomain = GLib.quark_from_string('notmydomain')
        e = GLib.Error('test message', 'mydomain', 42)
        self.assertTrue(e.matches(mydomain, 42))
        self.assertFalse(e.matches(notmydomain, 42))
        self.assertFalse(e.matches(mydomain, 40))

    def test_str(self):
        e = GLib.Error('test message', 'mydomain', 42)
        self.assertEqual(str(e),
                         'mydomain: test message (42)')

    def test_repr(self):
        e = GLib.Error('test message', 'mydomain', 42)
        self.assertEqual(repr(e),
                         "GLib.Error('test message', 'mydomain', 42)")

    def test_inheritance(self):
        self.assertTrue(issubclass(GLib.Error, RuntimeError))

    def test_pickle(self):

        def check_pickle(e):
            assert isinstance(e, GLib.Error)
            new_e = pickle.loads(pickle.dumps(e))
            assert type(new_e) is type(e)
            assert repr(e) == repr(new_e)

        e = GLib.Error('test message', 'mydomain', 42)
        check_pickle(e)

        try:
            GLib.file_get_contents("")
        except Exception as e:
            check_pickle(e)


class ObjectWithVFuncException(GIMarshallingTests.Object):
    def do_vfunc_meth_with_err(self, x):
        if x == 42:
            return True

        raise GLib.Error('unexpected value %d' % x, 'mydomain', 42)


class TestMarshalling(unittest.TestCase):
    def test_array_in_crash(self):
        # Previously there was a bug in invoke, in which C arrays were unwrapped
        # from inside GArrays to be passed to the C function. But when a GError was
        # set, invoke would attempt to free the C array as if it were a GArray.
        # This crash is only for C arrays. It does not happen for C functions which
        # take in GArrays. See https://bugzilla.gnome.org/show_bug.cgi?id=642708
        self.assertRaises(GLib.Error, GIMarshallingTests.gerror_array_in, [1, 2, 3])

    def test_out(self):
        # See https://bugzilla.gnome.org/show_bug.cgi?id=666098
        error, debug = GIMarshallingTests.gerror_out()

        self.assertIsInstance(error, GLib.Error)
        self.assertEqual(error.domain, GIMarshallingTests.CONSTANT_GERROR_DOMAIN)
        self.assertEqual(error.code, GIMarshallingTests.CONSTANT_GERROR_CODE)
        self.assertEqual(error.message, GIMarshallingTests.CONSTANT_GERROR_MESSAGE)
        self.assertEqual(debug, GIMarshallingTests.CONSTANT_GERROR_DEBUG_MESSAGE)

    def test_out_transfer_none(self):
        # See https://bugzilla.gnome.org/show_bug.cgi?id=666098
        error, debug = GIMarshallingTests.gerror_out_transfer_none()

        self.assertIsInstance(error, GLib.Error)
        self.assertEqual(error.domain, GIMarshallingTests.CONSTANT_GERROR_DOMAIN)
        self.assertEqual(error.code, GIMarshallingTests.CONSTANT_GERROR_CODE)
        self.assertEqual(error.message, GIMarshallingTests.CONSTANT_GERROR_MESSAGE)
        self.assertEqual(GIMarshallingTests.CONSTANT_GERROR_DEBUG_MESSAGE, debug)

    def test_return(self):
        # See https://bugzilla.gnome.org/show_bug.cgi?id=666098
        error = GIMarshallingTests.gerror_return()

        self.assertIsInstance(error, GLib.Error)
        self.assertEqual(error.domain, GIMarshallingTests.CONSTANT_GERROR_DOMAIN)
        self.assertEqual(error.code, GIMarshallingTests.CONSTANT_GERROR_CODE)
        self.assertEqual(error.message, GIMarshallingTests.CONSTANT_GERROR_MESSAGE)

    def test_exception(self):
        with self.assertRaises(GLib.Error) as context:
            GIMarshallingTests.gerror()

        e = context.exception
        self.assertEqual(e.domain, GIMarshallingTests.CONSTANT_GERROR_DOMAIN)
        self.assertEqual(e.code, GIMarshallingTests.CONSTANT_GERROR_CODE)
        self.assertEqual(e.message, GIMarshallingTests.CONSTANT_GERROR_MESSAGE)

    def test_vfunc_no_exception(self):
        obj = ObjectWithVFuncException()
        self.assertTrue(obj.vfunc_meth_with_error(42))

    def test_vfunc_gerror_exception(self):
        obj = ObjectWithVFuncException()
        with self.assertRaises(GLib.Error) as context:
            obj.vfunc_meth_with_error(-1)

        e = context.exception
        self.assertEqual(e.message, 'unexpected value -1')
        self.assertEqual(e.domain, 'mydomain')
        self.assertEqual(e.code, 42)

    def tests_compare_two_gerrors_in_gvalue(self):
        error = GLib.Error.new_literal(1, "error", 1)
        error1 = GLib.Error.new_literal(1, "error", 1)

        GIMarshallingTests.compare_two_gerrors_in_gvalue(error, error1)

# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2015 Christoph Reiter <reiter.christoph@gmail.com>
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

import gi
from gi.repository import GIMarshallingTests
from gi.repository import Regress


ResultTuple = gi._gi.ResultTuple


class TestResultTuple(unittest.TestCase):

    def test_base(self):
        self.assertTrue(issubclass(ResultTuple, tuple))

    def test_create(self):
        names = [None, "foo", None, "bar"]
        for i in range(10):
            new = ResultTuple._new_type(names)
            self.assertTrue(issubclass(new, ResultTuple))

    def test_repr_dir(self):
        new = ResultTuple._new_type([None, "foo", None, "bar"])
        inst = new([1, 2, 3, "a"])

        self.assertEqual(repr(inst), "(1, foo=2, 3, bar='a')")
        self.assertTrue("foo" in dir(inst))

    def test_repr_dir_empty(self):
        new = ResultTuple._new_type([])
        inst = new()
        self.assertEqual(repr(inst), "()")
        dir(inst)

    def test_getatttr(self):
        new = ResultTuple._new_type([None, "foo", None, "bar"])
        inst = new([1, 2, 3, "a"])

        self.assertTrue(hasattr(inst, "foo"))
        self.assertEqual(inst.foo, inst[1])
        self.assertRaises(AttributeError, getattr, inst, "nope")

    def test_pickle(self):
        new = ResultTuple._new_type([None, "foo", None, "bar"])
        inst = new([1, 2, 3, "a"])

        inst2 = pickle.loads(pickle.dumps(inst))
        self.assertEqual(inst2, inst)
        self.assertTrue(isinstance(inst2, tuple))
        self.assertFalse(isinstance(inst2, new))

    def test_gi(self):
        res = GIMarshallingTests.init_function([])
        self.assertEqual(repr(res), "(True, argv=[])")

        res = GIMarshallingTests.array_return_etc(5, 9)
        self.assertEqual(repr(res), "([5, 0, 1, 9], sum=14)")

        res = GIMarshallingTests.array_out_etc(-5, 9)
        self.assertEqual(repr(res), "(ints=[-5, 0, 1, 9], sum=4)")

        cb = lambda: (1, 2)
        res = GIMarshallingTests.callback_multiple_out_parameters(cb)
        self.assertEqual(repr(res), "(a=1.0, b=2.0)")

    def test_regress(self):
        res = Regress.TestObj().skip_return_val(50, 42.0, 60, 2, 3)
        self.assertEqual(repr(res), "(out_b=51, inout_d=61, out_sum=32)")

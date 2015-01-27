# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import gc
import unittest

import gi
from gi.repository import GLib
from compathelper import _long


class TestGVariant(unittest.TestCase):
    def test_create_simple(self):
        variant = GLib.Variant('i', 42)
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertEqual(variant.get_int32(), 42)

        variant = GLib.Variant('s', '')
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertEqual(variant.get_string(), '')

        variant = GLib.Variant('s', 'hello')
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertEqual(variant.get_string(), 'hello')

    def test_create_variant(self):
        variant = GLib.Variant('v', GLib.Variant('i', 42))
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertTrue(isinstance(variant.get_variant(), GLib.Variant))
        self.assertEqual(variant.get_type_string(), 'v')
        self.assertEqual(variant.get_variant().get_type_string(), 'i')
        self.assertEqual(variant.get_variant().get_int32(), 42)

        variant = GLib.Variant('v', GLib.Variant('v', GLib.Variant('i', 42)))
        self.assertEqual(variant.get_type_string(), 'v')
        self.assertEqual(variant.get_variant().get_type_string(), 'v')
        self.assertEqual(variant.get_variant().get_variant().get_type_string(), 'i')
        self.assertEqual(variant.get_variant().get_variant().get_int32(), 42)

    def test_create_tuple(self):
        variant = GLib.Variant('()', ())
        self.assertEqual(variant.get_type_string(), '()')
        self.assertEqual(variant.n_children(), 0)

        variant = GLib.Variant('(i)', (3,))
        self.assertEqual(variant.get_type_string(), '(i)')
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertEqual(variant.n_children(), 1)
        self.assertTrue(isinstance(variant.get_child_value(0), GLib.Variant))
        self.assertEqual(variant.get_child_value(0).get_int32(), 3)

        variant = GLib.Variant('(ss)', ('mec', 'mac'))
        self.assertEqual(variant.get_type_string(), '(ss)')
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(0), GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(1), GLib.Variant))
        self.assertEqual(variant.get_child_value(0).get_string(), 'mec')
        self.assertEqual(variant.get_child_value(1).get_string(), 'mac')

        # nested tuples
        variant = GLib.Variant('((si)(ub))', (('hello', -1), (42, True)))
        self.assertEqual(variant.get_type_string(), '((si)(ub))')
        self.assertEqual(variant.unpack(), (('hello', -1), (_long(42), True)))

    def test_new_tuple_sink(self):
        # https://bugzilla.gnome.org/show_bug.cgi?id=735166
        variant = GLib.Variant.new_tuple(GLib.Variant.new_tuple())
        del variant
        gc.collect()

    def test_create_dictionary(self):
        variant = GLib.Variant('a{si}', {})
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertEqual(variant.get_type_string(), 'a{si}')
        self.assertEqual(variant.n_children(), 0)

        variant = GLib.Variant('a{si}', {'': 1, 'key1': 2, 'key2': 3})
        self.assertEqual(variant.get_type_string(), 'a{si}')
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(0), GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(1), GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(2), GLib.Variant))
        self.assertEqual(variant.unpack(), {'': 1, 'key1': 2, 'key2': 3})

        # nested dictionaries
        variant = GLib.Variant('a{sa{si}}', {})
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertEqual(variant.get_type_string(), 'a{sa{si}}')
        self.assertEqual(variant.n_children(), 0)

        d = {'': {'': 1, 'keyn1': 2},
             'key1': {'key11': 11, 'key12': 12}}
        variant = GLib.Variant('a{sa{si}}', d)
        self.assertEqual(variant.get_type_string(), 'a{sa{si}}')
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertEqual(variant.unpack(), d)

    def test_create_array(self):
        variant = GLib.Variant('ai', [])
        self.assertEqual(variant.get_type_string(), 'ai')
        self.assertEqual(variant.n_children(), 0)

        variant = GLib.Variant('ai', [1, 2])
        self.assertEqual(variant.get_type_string(), 'ai')
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(0), GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(1), GLib.Variant))
        self.assertEqual(variant.get_child_value(0).get_int32(), 1)
        self.assertEqual(variant.get_child_value(1).get_int32(), 2)

        variant = GLib.Variant('as', [])
        self.assertEqual(variant.get_type_string(), 'as')
        self.assertEqual(variant.n_children(), 0)

        variant = GLib.Variant('as', [''])
        self.assertEqual(variant.get_type_string(), 'as')
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(0), GLib.Variant))
        self.assertEqual(variant.get_child_value(0).get_string(), '')

        variant = GLib.Variant('as', ['hello', 'world'])
        self.assertEqual(variant.get_type_string(), 'as')
        self.assertTrue(isinstance(variant, GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(0), GLib.Variant))
        self.assertTrue(isinstance(variant.get_child_value(1), GLib.Variant))
        self.assertEqual(variant.get_child_value(0).get_string(), 'hello')
        self.assertEqual(variant.get_child_value(1).get_string(), 'world')

        # nested arrays
        variant = GLib.Variant('aai', [])
        self.assertEqual(variant.get_type_string(), 'aai')
        self.assertEqual(variant.n_children(), 0)

        variant = GLib.Variant('aai', [[]])
        self.assertEqual(variant.get_type_string(), 'aai')
        self.assertEqual(variant.n_children(), 1)
        self.assertEqual(variant.get_child_value(0).n_children(), 0)

        variant = GLib.Variant('aai', [[1, 2], [3, 4, 5]])
        self.assertEqual(variant.get_type_string(), 'aai')
        self.assertEqual(variant.unpack(), [[1, 2], [3, 4, 5]])

    def test_create_complex(self):
        variant = GLib.Variant('(as)', ([],))
        self.assertEqual(variant.get_type_string(), '(as)')
        self.assertEqual(variant.n_children(), 1)
        self.assertEqual(variant.get_child_value(0).n_children(), 0)

        variant = GLib.Variant('(as)', ([''],))
        self.assertEqual(variant.get_type_string(), '(as)')
        self.assertEqual(variant.n_children(), 1)
        self.assertEqual(variant.get_child_value(0).n_children(), 1)
        self.assertEqual(variant.get_child_value(0).get_child_value(0).get_string(), '')

        variant = GLib.Variant('(as)', (['hello'],))
        self.assertEqual(variant.get_type_string(), '(as)')
        self.assertEqual(variant.n_children(), 1)
        self.assertEqual(variant.get_child_value(0).n_children(), 1)
        self.assertEqual(variant.get_child_value(0).get_child_value(0).get_string(), 'hello')

        variant = GLib.Variant('a(ii)', [])
        self.assertEqual(variant.get_type_string(), 'a(ii)')
        self.assertEqual(variant.n_children(), 0)

        variant = GLib.Variant('a(ii)', [(5, 6)])
        self.assertEqual(variant.get_type_string(), 'a(ii)')
        self.assertEqual(variant.n_children(), 1)
        self.assertEqual(variant.get_child_value(0).n_children(), 2)
        self.assertEqual(variant.get_child_value(0).get_child_value(0).get_int32(), 5)
        self.assertEqual(variant.get_child_value(0).get_child_value(1).get_int32(), 6)

        variant = GLib.Variant('(a(ii))', ([],))
        self.assertEqual(variant.get_type_string(), '(a(ii))')
        self.assertEqual(variant.n_children(), 1)
        self.assertEqual(variant.get_child_value(0).n_children(), 0)

        variant = GLib.Variant('(a(ii))', ([(5, 6)],))
        self.assertEqual(variant.get_type_string(), '(a(ii))')
        self.assertEqual(variant.n_children(), 1)
        self.assertEqual(variant.get_child_value(0).n_children(), 1)
        self.assertEqual(variant.get_child_value(0).get_child_value(0).n_children(), 2)
        self.assertEqual(variant.get_child_value(0).get_child_value(0).get_child_value(0).get_int32(), 5)
        self.assertEqual(variant.get_child_value(0).get_child_value(0).get_child_value(1).get_int32(), 6)

        obj = {'a1': (1, True), 'a2': (2, False)}
        variant = GLib.Variant('a{s(ib)}', obj)
        self.assertEqual(variant.get_type_string(), 'a{s(ib)}')
        self.assertEqual(variant.unpack(), obj)

        obj = {'a1': (1, GLib.Variant('b', True)), 'a2': (2, GLib.Variant('y', 255))}
        variant = GLib.Variant('a{s(iv)}', obj)
        self.assertEqual(variant.get_type_string(), 'a{s(iv)}')
        self.assertEqual(variant.unpack(), {'a1': (1, True), 'a2': (2, 255)})

        obj = (1, {'a': {'a1': True, 'a2': False},
                   'b': {'b1': False},
                   'c': {}
                  },
               'foo')
        variant = GLib.Variant('(ia{sa{sb}}s)', obj)
        self.assertEqual(variant.get_type_string(), '(ia{sa{sb}}s)')
        self.assertEqual(variant.unpack(), obj)

        obj = {"frequency": GLib.Variant('t', 738000000),
               "hierarchy": GLib.Variant('i', 0),
               "bandwidth": GLib.Variant('x', 8),
               "code-rate-hp": GLib.Variant('d', 2.0 / 3.0),
               "constellation": GLib.Variant('s', "QAM16"),
               "guard-interval": GLib.Variant('u', 4)}
        variant = GLib.Variant('a{sv}', obj)
        self.assertEqual(variant.get_type_string(), 'a{sv}')
        self.assertEqual(variant.unpack(),
                         {"frequency": 738000000,
                          "hierarchy": 0,
                          "bandwidth": 8,
                          "code-rate-hp": 2.0 / 3.0,
                          "constellation": "QAM16",
                          "guard-interval": 4
                         })

    def test_create_errors(self):
        # excess arguments
        self.assertRaises(TypeError, GLib.Variant, 'i', 42, 3)
        self.assertRaises(TypeError, GLib.Variant, '(i)', (42, 3))

        # not enough arguments
        self.assertRaises(TypeError, GLib.Variant, '(ii)', (42,))

        # data type mismatch
        self.assertRaises(TypeError, GLib.Variant, 'i', 'hello')
        self.assertRaises(TypeError, GLib.Variant, 's', 42)
        self.assertRaises(TypeError, GLib.Variant, '(ss)', 'mec', 'mac')
        self.assertRaises(TypeError, GLib.Variant, '(s)', 'hello')

        # unimplemented data type
        self.assertRaises(NotImplementedError, GLib.Variant, 'Q', 1)

        # invalid types
        self.assertRaises(TypeError, GLib.Variant, '(ii', (42, 3))
        self.assertRaises(TypeError, GLib.Variant, '(ii))', (42, 3))
        self.assertRaises(TypeError, GLib.Variant, 'a{si', {})
        self.assertRaises(TypeError, GLib.Variant, 'a{si}}', {})
        self.assertRaises(TypeError, GLib.Variant, 'a{iii}', {})

    def test_unpack(self):
        # simple values
        res = GLib.Variant.new_int32(-42).unpack()
        self.assertEqual(res, -42)

        res = GLib.Variant.new_uint64(34359738368).unpack()
        self.assertEqual(res, 34359738368)

        res = GLib.Variant.new_boolean(True).unpack()
        self.assertEqual(res, True)

        res = GLib.Variant.new_object_path('/foo/Bar').unpack()
        self.assertEqual(res, '/foo/Bar')

        # variant
        res = GLib.Variant('v', GLib.Variant.new_int32(-42)).unpack()
        self.assertEqual(res, -42)

        GLib.Variant('v', GLib.Variant('v', GLib.Variant('i', 42)))
        self.assertEqual(res, -42)

        # tuple
        res = GLib.Variant.new_tuple(GLib.Variant.new_int32(-1),
                                     GLib.Variant.new_string('hello')).unpack()
        self.assertEqual(res, (-1, 'hello'))

        # array
        vb = GLib.VariantBuilder.new(gi._gi.variant_type_from_string('ai'))
        vb.add_value(GLib.Variant.new_int32(-1))
        vb.add_value(GLib.Variant.new_int32(3))
        res = vb.end().unpack()
        self.assertEqual(res, [-1, 3])

        # dictionary
        res = GLib.Variant('a{si}', {'key1': 1, 'key2': 2}).unpack()
        self.assertEqual(res, {'key1': 1, 'key2': 2})

        # maybe
        v = GLib.Variant.new_maybe(GLib.VariantType.new('i'), GLib.Variant('i', 1))
        res = v.unpack()
        self.assertEqual(res, 1)
        v = GLib.Variant.new_maybe(GLib.VariantType.new('i'), None)
        res = v.unpack()
        self.assertEqual(res, None)

    def test_iteration(self):
        # array index access
        vb = GLib.VariantBuilder.new(gi._gi.variant_type_from_string('ai'))
        vb.add_value(GLib.Variant.new_int32(-1))
        vb.add_value(GLib.Variant.new_int32(3))
        v = vb.end()

        self.assertEqual(len(v), 2)
        self.assertEqual(v[0], -1)
        self.assertEqual(v[1], 3)
        self.assertEqual(v[-1], 3)
        self.assertEqual(v[-2], -1)
        self.assertRaises(IndexError, v.__getitem__, 2)
        self.assertRaises(IndexError, v.__getitem__, -3)
        self.assertRaises(ValueError, v.__getitem__, 'a')

        # array iteration
        self.assertEqual([x for x in v], [-1, 3])
        self.assertEqual(list(v), [-1, 3])

        # tuple index access
        v = GLib.Variant.new_tuple(GLib.Variant.new_int32(-1),
                                   GLib.Variant.new_string('hello'))
        self.assertEqual(len(v), 2)
        self.assertEqual(v[0], -1)
        self.assertEqual(v[1], 'hello')
        self.assertEqual(v[-1], 'hello')
        self.assertEqual(v[-2], -1)
        self.assertRaises(IndexError, v.__getitem__, 2)
        self.assertRaises(IndexError, v.__getitem__, -3)
        self.assertRaises(ValueError, v.__getitem__, 'a')

        # tuple iteration
        self.assertEqual([x for x in v], [-1, 'hello'])
        self.assertEqual(tuple(v), (-1, 'hello'))

        # dictionary index access
        vsi = GLib.Variant('a{si}', {'key1': 1, 'key2': 2})
        vis = GLib.Variant('a{is}', {1: 'val1', 5: 'val2'})

        self.assertEqual(len(vsi), 2)
        self.assertEqual(vsi['key1'], 1)
        self.assertEqual(vsi['key2'], 2)
        self.assertRaises(KeyError, vsi.__getitem__, 'unknown')

        self.assertEqual(len(vis), 2)
        self.assertEqual(vis[1], 'val1')
        self.assertEqual(vis[5], 'val2')
        self.assertRaises(KeyError, vsi.__getitem__, 3)

        # dictionary iteration
        self.assertEqual(set(vsi.keys()), set(['key1', 'key2']))
        self.assertEqual(set(vis.keys()), set([1, 5]))

        # string index access
        v = GLib.Variant('s', 'hello')
        self.assertEqual(len(v), 5)
        self.assertEqual(v[0], 'h')
        self.assertEqual(v[4], 'o')
        self.assertEqual(v[-1], 'o')
        self.assertEqual(v[-5], 'h')
        self.assertRaises(IndexError, v.__getitem__, 5)
        self.assertRaises(IndexError, v.__getitem__, -6)

        # string iteration
        self.assertEqual([x for x in v], ['h', 'e', 'l', 'l', 'o'])

    def test_split_signature(self):
        self.assertEqual(GLib.Variant.split_signature('()'), [])

        self.assertEqual(GLib.Variant.split_signature('s'), ['s'])

        self.assertEqual(GLib.Variant.split_signature('as'), ['as'])

        self.assertEqual(GLib.Variant.split_signature('(s)'), ['s'])

        self.assertEqual(GLib.Variant.split_signature('(iso)'), ['i', 's', 'o'])

        self.assertEqual(GLib.Variant.split_signature('(s(ss)i(ii))'),
                         ['s', '(ss)', 'i', '(ii)'])

        self.assertEqual(GLib.Variant.split_signature('(as)'), ['as'])

        self.assertEqual(GLib.Variant.split_signature('(s(ss)iaiaasa(ii))'),
                         ['s', '(ss)', 'i', 'ai', 'aas', 'a(ii)'])

        self.assertEqual(GLib.Variant.split_signature('(a{iv}(ii)((ss)a{s(ss)}))'),
                         ['a{iv}', '(ii)', '((ss)a{s(ss)})'])

    def test_hash(self):
        v1 = GLib.Variant('s', 'somestring')
        v2 = GLib.Variant('s', 'somestring')
        v3 = GLib.Variant('s', 'somestring2')

        self.assertTrue(v2 in set([v1, v3]))
        self.assertTrue(v2 in frozenset([v1, v3]))
        self.assertTrue(v2 in {v1: '1', v3: '2'})

    def test_compare(self):
        # Check if identical GVariant are equal

        def assert_equal(vtype, value):
            self.assertEqual(GLib.Variant(vtype, value), GLib.Variant(vtype, value))

        def assert_not_equal(vtype1, value1, vtype2, value2):
            self.assertNotEqual(GLib.Variant(vtype1, value1), GLib.Variant(vtype2, value2))

        numbers = ['y', 'n', 'q', 'i', 'u', 'x', 't', 'h', 'd']
        for num in numbers:
            assert_equal(num, 42)
            assert_not_equal(num, 42, num, 41)
            assert_not_equal(num, 42, 's', '42')

        assert_equal('s', 'something')
        assert_not_equal('s', 'something', 's', 'somethingelse')
        assert_not_equal('s', 'something', 'i', 1234)

        assert_equal('g', 'dustybinqhogx')
        assert_not_equal('g', 'dustybinqhogx', 'g', 'dustybin')
        assert_not_equal('g', 'dustybinqhogx', 'i', 1234)

        assert_equal('o', '/dev/null')
        assert_not_equal('o', '/dev/null', 'o', '/dev/zero')
        assert_not_equal('o', '/dev/null', 'i', 1234)

        assert_equal('(s)', ('strtuple',))
        assert_not_equal('(s)', ('strtuple',), '(s)', ('strtuple2',))

        assert_equal('a{si}', {'str': 42})
        assert_not_equal('a{si}', {'str': 42}, 'a{si}', {'str': 43})

        assert_equal('v', GLib.Variant('i', 42))
        assert_not_equal('v', GLib.Variant('i', 42), 'v', GLib.Variant('i', 43))

    def test_bool(self):
        # Check if the GVariant bool matches the unpacked Pythonic bool

        def assert_equals_bool(vtype, value):
            self.assertEqual(bool(GLib.Variant(vtype, value)), bool(value))

        # simple values
        assert_equals_bool('b', True)
        assert_equals_bool('b', False)

        numbers = ['y', 'n', 'q', 'i', 'u', 'x', 't', 'h', 'd']
        for number in numbers:
            assert_equals_bool(number, 0)
            assert_equals_bool(number, 1)

        assert_equals_bool('s', '')
        assert_equals_bool('g', '')
        assert_equals_bool('s', 'something')
        assert_equals_bool('o', '/dev/null')
        assert_equals_bool('g', 'dustybinqhogx')

        # arrays
        assert_equals_bool('ab', [True])
        assert_equals_bool('ab', [False])
        for number in numbers:
            assert_equals_bool('a' + number, [])
            assert_equals_bool('a' + number, [0])
        assert_equals_bool('as', [])
        assert_equals_bool('as', [''])
        assert_equals_bool('ao', [])
        assert_equals_bool('ao', ['/'])
        assert_equals_bool('ag', [])
        assert_equals_bool('ag', [''])
        assert_equals_bool('aai', [[]])

        # tuples
        assert_equals_bool('()', ())
        for number in numbers:
            assert_equals_bool('(' + number + ')', (0,))
        assert_equals_bool('(s)', ('',))
        assert_equals_bool('(o)', ('/',))
        assert_equals_bool('(g)', ('',))
        assert_equals_bool('(())', ((),))

        # dictionaries
        assert_equals_bool('a{si}', {})
        assert_equals_bool('a{si}', {'': 0})

        # complex types, always True
        assert_equals_bool('(as)', ([],))
        assert_equals_bool('a{s(i)}', {'': (0,)})

        # variant types, recursive unpacking
        assert_equals_bool('v', GLib.Variant('i', 0))
        assert_equals_bool('v', GLib.Variant('i', 1))

    def test_repr(self):
        # with C constructor
        v = GLib.Variant.new_uint32(42)
        self.assertEqual(repr(v), "GLib.Variant('u', 42)")

        # with override constructor
        v = GLib.Variant('(is)', (1, 'somestring'))
        self.assertEqual(repr(v), "GLib.Variant('(is)', (1, 'somestring'))")

    def test_str(self):
        # with C constructor
        v = GLib.Variant.new_uint32(42)
        self.assertEqual(str(v), 'uint32 42')

        # with override constructor
        v = GLib.Variant('(is)', (1, 'somestring'))
        self.assertEqual(str(v), "(1, 'somestring')")


class TestConstants(unittest.TestCase):

    def test_basic_types_limits(self):
        self.assertTrue(isinstance(GLib.MINFLOAT, float))
        self.assertTrue(isinstance(GLib.MAXLONG, (int, _long)))

# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2010 Tomeu Vizoso <tomeu.vizoso@collabora.co.uk>
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

from ..importer import modules
from .._gi import variant_new_tuple, variant_type_from_string

GLib = modules['GLib']._introspection_module

__all__ = []

class _VariantCreator(object):

    _LEAF_CONSTRUCTORS = {
        'b': GLib.Variant.new_boolean,
        'y': GLib.Variant.new_byte,
        'n': GLib.Variant.new_int16,
        'q': GLib.Variant.new_uint16,
        'i': GLib.Variant.new_int32,
        'u': GLib.Variant.new_uint32,
        'x': GLib.Variant.new_int64,
        't': GLib.Variant.new_uint64,
        'h': GLib.Variant.new_handle,
        'd': GLib.Variant.new_double,
        's': GLib.Variant.new_string,
        'o': GLib.Variant.new_object_path,
        'g': GLib.Variant.new_signature,
        'v': GLib.Variant.new_variant,
    }

    def __init__(self, format_string, args):
        self._format_string = format_string
        self._args = args

    def create(self):
        if self._format_string_is_leaf():
            return self._new_variant_leaf()

        format_char = self._pop_format_char()
        arg = self._pop_arg()

        if format_char == 'm':
            raise NotImplementedError()
        else:
            builder = GLib.VariantBuilder()
            if format_char == '(':
                builder.init(variant_type_from_string('r'))
            elif format_char == '{':
                builder.init(variant_type_from_string('{?*}'))
            else:
                raise NotImplementedError()
            format_char = self._pop_format_char()
            while format_char not in [')', '}']:
                builder.add_value(Variant(format_char, arg))
                format_char = self._pop_format_char()
                if self._args:
                    arg = self._pop_arg()
            return builder.end()

    def _format_string_is_leaf(self):
        format_char = self._format_string[0]
        return not format_char in ['m', '(', '{']

    def _format_string_is_nnp(self):
        format_char = self._format_string[0]
        return format_char in ['a', 's', 'o', 'g', '^', '@', '*', '?', 'r',
                               'v', '&']

    def _new_variant_leaf(self):
        if self._format_string_is_nnp():
            return self._new_variant_nnp()

        format_char = self._pop_format_char()
        arg = self._pop_arg()

        return _VariantCreator._LEAF_CONSTRUCTORS[format_char](arg)

    def _new_variant_nnp(self):
        format_char = self._pop_format_char()
        arg = self._pop_arg()

        if format_char == '&':
            format_char = self._pop_format_char()

        if format_char == 'a':
            builder = GLib.VariantBuilder()
            builder.init(variant_type_from_string('a*'))

            element_format_string = self._pop_leaf_format_string()

            if isinstance(arg, dict):
                for element in arg.items():
                    value = Variant(element_format_string, *element)
                    builder.add_value(value)
            else:
                for element in arg:
                    value = Variant(element_format_string, element)
                    builder.add_value(value)
            return builder.end()
        elif format_char == '^':
            raise NotImplementedError()
        elif format_char == '@':
            raise NotImplementedError()
        elif format_char == '*':
            raise NotImplementedError()
        elif format_char == 'r':
            raise NotImplementedError()
        elif format_char == '?':
            raise NotImplementedError()
        else:
            return _VariantCreator._LEAF_CONSTRUCTORS[format_char](arg)

    def _pop_format_char(self):
        format_char = self._format_string[0]
        self._format_string = self._format_string[1:]
        return format_char

    def _pop_leaf_format_string(self):
        # FIXME: This will break when the leaf is inside a tuple or dict entry
        format_string = self._format_string
        self._format_string = ''
        return format_string

    def _pop_arg(self):
        arg = self._args[0]
        self._args = self._args[1:]
        return arg

class Variant(GLib.Variant):
    def __new__(cls, format_string, *args):
        creator = _VariantCreator(format_string, args)
        return creator.create()

    def __repr__(self):
        return '<GLib.Variant(%s)>' % getattr(self, 'print')(True)

    def unpack(self):
        '''Decompose a GVariant into a native Python object.'''

        LEAF_ACCESSORS = {
            'b': self.get_boolean,
            'y': self.get_byte,
            'n': self.get_int16,
            'q': self.get_uint16,
            'i': self.get_int32,
            'u': self.get_uint32,
            'x': self.get_int64,
            't': self.get_uint64,
            'h': self.get_handle,
            'd': self.get_double,
            's': self.get_string,
            'o': self.get_string, # object path
            'g': self.get_string, # signature
        }

        # simple values
        la = LEAF_ACCESSORS.get(self.get_type_string())
        if la:
            return la()

        # tuple
        if self.get_type_string().startswith('('):
            res = [self.get_child_value(i).unpack() 
                    for i in xrange(self.n_children())]
            return tuple(res)

        # dictionary
        if self.get_type_string().startswith('a{'):
            res = {}
            for i in xrange(self.n_children()):
                v = self.get_child_value(i)
                res[v.get_child_value(0).unpack()] = v.get_child_value(1).unpack()
            return res

        # array
        if self.get_type_string().startswith('a'):
            return [self.get_child_value(i).unpack() 
                    for i in xrange(self.n_children())]

        raise NotImplementedError, 'unsupported GVariant type ' + self.get_type_string()

    #
    # Pythonic iterators
    #
    def __len__(self):
        if self.get_type_string().startswith('a') or self.get_type_string().startswith('('):
            return self.n_children()
        raise TypeError, 'GVariant type %s is not a container' % self.get_type_string()

    def __getitem__(self, key):
        # dict
        if self.get_type_string().startswith('a{'):
            try:
                val = self.lookup_value(key, variant_type_from_string('*'))
                if val is None:
                    raise KeyError, key
                return val.unpack()
            except TypeError:
                # lookup_value() only works for string keys, which is certainly
                # the common case; we have to do painful iteration for other
                # key types
                for i in xrange(self.n_children()):
                    v = self.get_child_value(i)
                    if v.get_child_value(0).unpack() == key:
                        return v.get_child_value(1).unpack()
                raise KeyError, key

        # array/tuple
        if self.get_type_string().startswith('a') or self.get_type_string().startswith('('):
            try:
                key = int(key)
            except ValueError, e:
                raise TypeError, str(e)
            if key < 0:
                key = self.n_children() + key
            if key < 0 or key >= self.n_children():
                raise IndexError, 'list index out of range'
            return self.get_child_value(key).unpack()

        raise TypeError, 'GVariant type %s is not a container' % self.get_type_string()

    def keys(self):
        if not self.get_type_string().startswith('a{'):
            return TypeError, 'GVariant type %s is not a dictionary' % self.get_type_string()

        res = []
        for i in xrange(self.n_children()):
            v = self.get_child_value(i)
            res.append(v.get_child_value(0).unpack())
        return res

@classmethod
def new_tuple(cls, *elements):
    return variant_new_tuple(elements)

def get_string(self):
    value, length = GLib.Variant.get_string(self)
    return value

setattr(Variant, 'new_tuple', new_tuple)
setattr(Variant, 'get_string', get_string)

__all__.append('Variant')


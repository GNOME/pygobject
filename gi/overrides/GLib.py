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

    def _create(self, format, args):
        '''Create a GVariant object from given format and argument list.

        This method recursively calls itself for complex structures (arrays,
        dictionaries, boxed).

        Return a tuple (variant, rest_format, rest_args) with the generated
        GVariant, the remainder of the format string, and the remainder of the
        arguments.

        If args is None, then this won't actually consume any arguments, and
        just parse the format string and generate empty GVariant structures.
        This is required for creating empty dictionaries or arrays.
        '''
        # leaves (simple types)
        constructor = self._LEAF_CONSTRUCTORS.get(format[0])
        if constructor:
            if args is not None:
                if not args:
                    raise TypeError('not enough arguments for GVariant format string')
                v = constructor(args[0])
                return (v, format[1:], args[1:])
            else:
                return (None, format[1:], None)

        if format[0] == '(':
            return self._create_tuple(format, args)

        if format.startswith('a{'):
            return self._create_dict(format, args)

        if format[0] == 'a':
            return self._create_array(format, args)

        raise NotImplementedError('cannot handle GVariant type ' + format)

    def _create_tuple(self, format, args):
        '''Handle the case where the outermost type of format is a tuple.'''

        format = format[1:] # eat the '('
        builder = GLib.VariantBuilder.new(variant_type_from_string('r'))
        if args is not None:
            if not args or type(args[0]) != type(()):
                raise (TypeError, 'expected tuple argument')

            for i in range(len(args[0])):
                if format.startswith(')'):
                    raise (TypeError, 'too many arguments for tuple signature')

                (v, format, _) = self._create(format, args[0][i:])
                builder.add_value(v)
            args = args[1:]
        return (builder.end(), format[1:], args)

    def _create_dict(self, format, args):
        '''Handle the case where the outermost type of format is a dict.'''

        builder = None
        if args is None or not args[0]:
            # empty value: we need to call _create() to parse the subtype,
            # and specify the element type precisely
            rest_format = self._create(format[2:], None)[1]
            rest_format = self._create(rest_format, None)[1]
            if not rest_format.startswith('}'):
                raise ValueError('dictionary type string not closed with }')
            rest_format = rest_format[1:] # eat the }
            element_type = format[:len(format) - len(rest_format)]
            builder = GLib.VariantBuilder.new(variant_type_from_string(element_type))
        else:
            builder = GLib.VariantBuilder.new(variant_type_from_string('a{?*}'))
            for k, v in args[0].items():
                (key_v, rest_format, _) = self._create(format[2:], [k])
                (val_v, rest_format, _) = self._create(rest_format, [v])

                if not rest_format.startswith('}'):
                    raise ValueError('dictionary type string not closed with }')
                rest_format = rest_format[1:] # eat the }

                entry = GLib.VariantBuilder.new(variant_type_from_string('{?*}'))
                entry.add_value(key_v)
                entry.add_value(val_v)
                builder.add_value(entry.end())

        if args is not None:
            args = args[1:]
        return (builder.end(), rest_format, args)

    def _create_array(self, format, args):
        '''Handle the case where the outermost type of format is an array.'''

        builder = None
        if args is None or not args[0]:
            # empty value: we need to call _create() to parse the subtype,
            # and specify the element type precisely
            rest_format = self._create(format[1:], None)[1]
            element_type = format[:len(format) - len(rest_format)]
            builder = GLib.VariantBuilder.new(variant_type_from_string(element_type))
        else:
            builder = GLib.VariantBuilder.new(variant_type_from_string('a*'))
            for i in range(len(args[0])):
                (v, rest_format, _) = self._create(format[1:], args[0][i:])
                builder.add_value(v)
        if args is not None:
            args = args[1:]
        return (builder.end(), rest_format, args)

class Variant(GLib.Variant):
    def __new__(cls, format_string, value):
        '''Create a GVariant from a native Python object.

        format_string is a standard GVariant type signature, value is a Python
        object whose structure has to match the signature.
        
        Examples:
          GLib.Variant('i', 1)
          GLib.Variant('(is)', (1, 'hello'))
          GLib.Variant('(asa{sv})', ([], {'foo': GLib.Variant('b', True), 
                                          'bar': GLib.Variant('i', 2)}))
        '''
        creator = _VariantCreator()
        (v, rest_format, _) = creator._create(format_string, [value])
        if rest_format:
            raise TypeError('invalid remaining format string: "%s"' % rest_format)
        return v

    def __repr__(self):
        return '<GLib.Variant(%s)>' % getattr(self, 'print')(True)

    def __eq__(self, other):
        try:
            return self.equal(other)
        except TypeError:
            return False

    def __ne__(self, other):
        try:
            return not self.equal(other)
        except TypeError:
            return True

    def __hash__(self):
        # We're not using just hash(self.unpack()) because otherwise we'll have
        # hash collisions between the same content in different variant types,
        # which will cause a performance issue in set/dict/etc.
        return hash((self.get_type_string(), self.unpack()))

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
                    for i in range(self.n_children())]
            return tuple(res)

        # dictionary
        if self.get_type_string().startswith('a{'):
            res = {}
            for i in range(self.n_children()):
                v = self.get_child_value(i)
                res[v.get_child_value(0).unpack()] = v.get_child_value(1).unpack()
            return res

        # array
        if self.get_type_string().startswith('a'):
            return [self.get_child_value(i).unpack() 
                    for i in range(self.n_children())]

        # variant (just unbox transparently)
        if self.get_type_string().startswith('v'):
            return self.get_variant().unpack()

        raise NotImplementedError('unsupported GVariant type ' + self.get_type_string())

    @classmethod
    def split_signature(klass, signature):
        '''Return a list of the element signatures of the topmost signature tuple.

        If the signature is not a tuple, it returns one element with the entire
        signature. If the signature is an empty tuple, the result is [].
        
        This is useful for e. g. iterating over method parameters which are
        passed as a single Variant.
        '''
        if signature == '()':
            return []

        if not signature.startswith('('):
            return [signature]

        result = []
        head = ''
        tail = signature[1:-1] # eat the surrounding ( )
        while tail:
            c = tail[0]
            head += c
            tail = tail[1:]

            if c in ('m', 'a'):
                # prefixes, keep collecting
                continue
            if c in ('(', '{'):
                # consume until corresponding )/}
                level = 1
                up = c
                if up == '(':
                    down = ')'
                else:
                    down = '}'
                while level > 0:
                    c = tail[0]
                    head += c
                    tail = tail[1:]
                    if c == up:
                        level += 1
                    elif c == down:
                        level -= 1

            # otherwise we have a simple type
            result.append(head)
            head = ''

        return result

    #
    # Pythonic iterators
    #
    def __len__(self):
        if self.get_type_string() in ['s', 'o', 'g']:
            return len(self.get_string())
        # Array, dict, tuple
        if self.get_type_string().startswith('a') or self.get_type_string().startswith('('):
            return self.n_children()
        raise TypeError('GVariant type %s does not have a length' % self.get_type_string())

    def __getitem__(self, key):
        # dict
        if self.get_type_string().startswith('a{'):
            try:
                val = self.lookup_value(key, variant_type_from_string('*'))
                if val is None:
                    raise KeyError(key)
                return val.unpack()
            except TypeError:
                # lookup_value() only works for string keys, which is certainly
                # the common case; we have to do painful iteration for other
                # key types
                for i in range(self.n_children()):
                    v = self.get_child_value(i)
                    if v.get_child_value(0).unpack() == key:
                        return v.get_child_value(1).unpack()
                raise KeyError(key)

        # array/tuple
        if self.get_type_string().startswith('a') or self.get_type_string().startswith('('):
            key = int(key)
            if key < 0:
                key = self.n_children() + key
            if key < 0 or key >= self.n_children():
                raise IndexError('list index out of range')
            return self.get_child_value(key).unpack()

        # string
        if self.get_type_string() in ['s', 'o', 'g']:
            return self.get_string().__getitem__(key)

        raise TypeError('GVariant type %s is not a container' % self.get_type_string())

    #
    # Pythonic bool operations
    #
    def __nonzero__(self):
        return self.__bool__()

    def __bool__(self):
        if self.get_type_string() in ['y', 'n', 'q', 'i', 'u', 'x', 't', 'h', 'd']:
            return self.unpack() != 0
        if self.get_type_string() in ['b']:
            return self.get_boolean()
        if self.get_type_string() in ['s', 'o', 'g']:
            return len(self.get_string()) != 0
        # Array, dict, tuple
        if self.get_type_string().startswith('a') or self.get_type_string().startswith('('):
            return self.n_children() != 0
        if self.get_type_string() in ['v']:
            # unpack works recursively, hence bool also works recursively
            return bool(self.unpack())
        # Everything else is True
        return True

    def keys(self):
        if not self.get_type_string().startswith('a{'):
            return TypeError, 'GVariant type %s is not a dictionary' % self.get_type_string()

        res = []
        for i in range(self.n_children()):
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


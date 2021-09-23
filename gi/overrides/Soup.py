#!/usr/bin/env python3

# Copyright (C) 2021 Igalia S.L.
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

import sys

from ..module import get_introspection_module
from ..overrides import override

Soup = get_introspection_module('Soup')

__all__ = []

if Soup._version == '3.0' and sys.version_info >= (3, 3):
    import collections.abc

    class MessageHeadersIter(Soup.MessageHeadersIter):

        def __next__(self):
            ret, key, value = self.next()
            if ret is True:
                return key, value
            else:
                raise StopIteration

    @override
    class MessageHeaders(Soup.MessageHeaders):

        def __getitem__(self, key):
            if not isinstance(key, str):
                raise TypeError

            value = self.get_one(key)
            if value is None:
                raise KeyError

            return value

        def __delitem__(self, key):
            if not isinstance(key, str):
                raise TypeError

            self.remove(key)

        def __setitem__(self, key, value):
            if not isinstance(key, str) or not isinstance(value, str):
                raise TypeError

            self.replace(key, value)

        def __contains__(self, item):
            if not isinstance(item, str):
                raise TypeError

            return self.get_one(item) is not None

        def __iter__(self):
            return MessageHeadersIter.init(self)

        def __len__(self):
            return len(self.keys())

        def keys(self):
            return [k for k, _ in self]

        def values(self):
            return [v for _, v in self]

        def items(self):
            return {k: v for k, v in self}

        def get(self, default=None):
            return collections.abc.Mapping.get(self, default)

        def pop(self, key):
            return collections.abc.MutableMapping.pop(self, key)

        def update(self, key, value):
            return collections.abc.MutableMapping.update(self, key, value)

        def setdefault(self, key, default=None):
            return collections.abc.MutableMapping.setdefault(self, key, default)

        def popitem(self):
            return collections.abc.MutableMapping.popitem(self)

    __all__.append('MessageHeaders')
    __all__.append('MessageHeadersIter')
    collections.abc.Iterable.register(MessageHeaders)
    collections.abc.Iterator.register(MessageHeadersIter)
    collections.abc.Mapping.register(MessageHeaders)
    collections.abc.MutableMapping.register(MessageHeaders)
    collections.abc.Container.register(MessageHeaders)
    collections.abc.Sized.register(MessageHeaders)

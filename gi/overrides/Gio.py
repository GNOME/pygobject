# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2010 Ignacio Casal Quinteiro <icq@gnome.org>
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

from ..overrides import override
from ..importer import modules

from gi.repository import GLib

Gio = modules['Gio']._introspection_module

__all__ = []

class FileEnumerator(Gio.FileEnumerator):
    def __iter__(self):
        return self

    def __next__(self):
        file_info = self.next_file(None)

        if file_info is not None:
            return file_info
        else:
            raise StopIteration

    # python 2 compat for the iter protocol
    next = __next__


FileEnumerator = override(FileEnumerator)
__all__.append('FileEnumerator')

class Settings(Gio.Settings):
    '''Provide dictionary-like access to GLib.Settings.'''

    def __init__(self, schema, path=None, backend=None):
        Gio.Settings.__init__(self, schema=schema, backend=backend, path=path)

    def __contains__(self, key):
        return key in self.list_keys()

    def __len__(self):
        return len(self.list_keys())

    def __bool__(self):
        # for "if mysettings" we don't want a dictionary-like test here, just
        # if the object isn't None
        return True

    # alias for Python 2.x object protocol
    __nonzero__ = __bool__

    def __getitem__(self, key):
        # get_value() aborts the program on an unknown key
        if not key in self:
            raise KeyError('unknown key: %r' % (key,))

        return self.get_value(key).unpack()

    def __setitem__(self, key, value):
        # set_value() aborts the program on an unknown key
        if not key in self:
            raise KeyError('unknown key: %r' % (key,))

        # determine type string of this key
        range = self.get_range(key)
        type_ = range.get_child_value(0).get_string()
        v = range.get_child_value(1)
        if type_ == 'type':
            # v is boxed empty array, type of its elements is the allowed value type
            type_str = v.get_child_value(0).get_type_string()
            assert type_str.startswith('a')
            type_str = type_str[1:]
        else:
            raise NotImplementedError('Cannot handle allowed type range class' + str(type_))

        self.set_value(key, GLib.Variant(type_str, value))

    def keys(self):
        return self.list_keys()

Settings = override(Settings)
__all__.append('Settings')

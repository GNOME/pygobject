# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2025 James Henstridge <james@jamesh.id.au>
#
#   _enum.py: base classes for enumeration and flags types.
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

import enum

from . import _gi


class GEnumMeta(enum.EnumMeta):

    def __new__(metacls, name, bases, classdict, /, **kwargs):
        enum_class = super().__new__(metacls, name, bases, classdict, **kwargs)

        # If __gtype__ is not set, this is a new enum or flags defined
        # from Python. Register a new GType for it.
        if '__gtype__' not in enum_class.__dict__:
            type_name = enum_class.__dict__.get('__gtype_name__')
            if issubclass(enum_class, GFlags):
                _gi.flags_register(enum_class, type_name)
            elif issubclass(enum_class, GEnum):
                _gi.enum_register(enum_class, type_name)
            else:
                raise AssertionError(f"unexpected class {enum_class}")

        return enum_class


class GEnum(enum.IntEnum, metaclass=GEnumMeta):
    __module__ = _gi.__name__
    __gtype__ = None


class GFlags(enum.IntFlag, metaclass=GEnumMeta):
    __module__ = _gi.__name__
    __gtype__ = None

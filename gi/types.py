# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
#
#   types.py: base types for introspected items.
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

from new import instancemethod

import gobject

from ._gi import \
	setObjectHasNewConstructor, \
	InterfaceInfo, \
	ObjectInfo, \
	StructInfo, \
	EnumInfo
from .repository import repository


def Function(info):

    def function(*args):
        return info.invoke(*args)
    function.__info__ = info
    function.__name__ = info.getName()
    function.__module__ = info.getNamespace()

    return function


class Field(object):

    def __init__(self, info):
        self.info = info

    def __get__(self, instance, owner):
        return self.info.getValue(instance)

    def __set__(self, instance, value):
        return self.info.setValue(instance, value)


class GObjectIntrospectionMeta(gobject.GObjectMeta):

    def __init__(cls, name, bases, dict_):
        super(GObjectIntrospectionMeta, cls).__init__(name, bases, dict_)

        if hasattr(cls, '__gtype__'):
            setObjectHasNewConstructor(cls.__gtype__)

        # Only set up the wrapper methods and fields in their base classes.
        if cls.__name__ == cls.__info__.getName():
            if isinstance(cls.__info__, InterfaceInfo):
                cls._setup_methods()

            if isinstance(cls.__info__, ObjectInfo):
                cls._setup_fields()
                cls._setup_methods()

            if isinstance(cls.__info__, StructInfo):
                cls._setup_fields()
                cls._setup_methods()

            if isinstance(cls.__info__, EnumInfo):
                cls._setup_values()

    def _setup_methods(cls):
        constructor_infos = []
        method_infos = cls.__info__.getMethods()
        if hasattr(cls.__info__, 'getInterfaces'):
            method_infos += reduce(lambda x, y: x + y, [interface_info.getMethods() for interface_info in cls.__info__.getInterfaces()], ())

        for method_info in method_infos:
            name = method_info.getName()
            function = Function(method_info)
            if method_info.isMethod():
                method = instancemethod(function, None, cls)
            elif method_info.isConstructor():
                method = classmethod(function)
            else:
                method = staticmethod(function)
            setattr(cls, name, method)

            if method_info.isConstructor():
                constructor_infos.append(method_info)

        default_constructor_info = None
        if len(constructor_infos) == 1:
            (default_constructor_info,) = constructor_infos
        else:
            for constructor_info in constructor_infos:
                if constructor_info.getName() == 'new':
                    default_constructor_info = constructor_info
                    break

        if default_constructor_info is not None:
            function = Function(default_constructor_info)
            cls.__new__ = staticmethod(function)
            # Override the initializer because of the constructor's arguments
            cls.__init__ = lambda self, *args, **kwargs: super(cls, self).__init__()

    def _setup_fields(cls):
        for field_info in cls.__info__.getFields():
            name = field_info.getName().replace('-', '_')
            setattr(cls, name, Field(field_info))

    def _setup_values(cls):
        for value_info in cls.__info__.getValues():
            name = value_info.getName().upper()
            setattr(cls, name, value_info.getValue())


class GIStruct(object):

    def __init__(self, buffer=None):
        if buffer is None:
            buffer = self.__info__.newBuffer()
        self.__buffer__ = buffer

    def __eq__(self, other):
        for field_info in self.__info__.getFields():
            name = field_info.getName()
            if getattr(self, name) != getattr(other, name):
                return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)


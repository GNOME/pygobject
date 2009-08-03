# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2007-2009 Johan Dahlin <johan@gnome.org>
#
#   module.py: dynamic module for introspected libraries.
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

import os

import gobject
from gobject import \
    GObject, \
    GInterface, \
    GEnum

from ._gi import \
    Repository, \
    UnresolvedInfo, \
    FunctionInfo, \
    RegisteredTypeInfo, \
    EnumInfo, \
    ObjectInfo, \
    InterfaceInfo, \
    StructInfo
from .types import \
    GObjectIntrospectionMeta, \
    GIStruct, \
    Function

repository = Repository.get_default()


def get_parent_for_object(object_info):
    parent_object_info = object_info.get_parent()

    if not parent_object_info:
        return object

    namespace = parent_object_info.get_namespace()
    name = parent_object_info.get_name()

    # Workaround for GObject.Object and GObject.InitiallyUnowned.
    if namespace == 'GObject' and name == 'Object' or name == 'InitiallyUnowned':
        return GObject

    module = __import__('gi.repository.%s' % namespace, fromlist=[name])
    return getattr(module, name)


class DynamicModule(object):

    def __str__(self):
        path = repository.get_typelib_path(self.__namespace__)
        return "<dynamic module %r from %r>" % (self.__name__,  path)

    def __getattr__(self, name):
        info = repository.find_by_name(self.__namespace__, name)
        if not info:
            raise AttributeError("%r object has no attribute %r" % (
                    self.__class__.__name__, name))

        if isinstance(info, StructInfo):
            # FIXME: This could be wrong for structures that are registered (like GValue or GClosure).
            bases = (GIStruct,)
            name = info.get_name()
            dict_ = {
                '__info__': info,
                '__module__': info.get_namespace()
            }
            value = GObjectIntrospectionMeta(name, bases, dict_)
        elif isinstance(info, EnumInfo):
            type_ = info.get_g_type()
            if type_.is_a(gobject.TYPE_ENUM):
                value = gobject.enum_from_g_type(type_)
            elif type_.is_a(gobject.TYPE_FLAGS):
                value = gobject.flags_from_g_type(type_)
            else:
                raise TypeError, "Must be a subtype of either gobject.TYPE_ENUM, or gobject.TYPE_FLAGS"
            value.__info__ = info
            value.__module__ = info.get_namespace()

            for value_info in info.get_values():
                name = value_info.get_name().upper()
                setattr(value, name, value(value_info.get_value()))

        elif isinstance(info, RegisteredTypeInfo):
            # Check if there is already a Python wrapper.
            gtype = info.get_g_type()
            if gtype.pytype is not None:
                self.__dict__[name] = gtype.pytype
                return

            # Create a wrapper.
            if isinstance(info, ObjectInfo):
                parent = get_parent_for_object(info)
                bases = (parent,)
            elif isinstance(info, InterfaceInfo):
                bases = (GInterface,)
            else:
                raise NotImplementedError(info)

            name = info.get_name()
            dict_ = {
                '__info__': info,
                '__module__': info.get_namespace(),
                '__gtype__': gtype
            }
            value = GObjectIntrospectionMeta(name, bases, dict_)
            gtype.pytype = value
        elif isinstance(info, FunctionInfo):
            value = Function(info)
        else:
            raise NotImplementedError(info)

        self.__dict__[name] = value
        return value

    @property
    def __members__(self):
        r = []
        for type_info in repository.get_infos(self.__namespace__):
            if type_info is None:
                continue
            r.append(type_info.get_name())
        return r


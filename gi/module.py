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

from __future__ import absolute_import

import os
import gobject

from ._gi import \
    Repository, \
    FunctionInfo, \
    RegisteredTypeInfo, \
    EnumInfo, \
    ObjectInfo, \
    InterfaceInfo, \
    ConstantInfo, \
    StructInfo, \
    Struct, \
    Boxed, \
    enum_add, \
    flags_add
from .types import \
    GObjectMeta, \
    StructMeta, \
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
        return gobject.GObject

    module = __import__('gi.repository.%s' % namespace, fromlist=[name])
    return getattr(module, name)

def get_interfaces_for_object(object_info):
    interfaces = []
    for interface_info in object_info.get_interfaces():
        namespace = interface_info.get_namespace()
        name = interface_info.get_name()

        module = __import__('gi.repository.%s' % namespace, fromlist=[name])
        interfaces.append(getattr(module, name))
    return interfaces


class DynamicModule(object):

    def __str__(self):
        path = repository.get_typelib_path(self.__namespace__)
        return "<dynamic module %r from %r>" % (self.__name__,  path)

    def __getattr__(self, name):
        if self.__dict__.has_key(name):
            return self.__dict__[name]

        info = repository.find_by_name(self.__namespace__, name)
        if not info:
            raise AttributeError("%r object has no attribute %r" % (
                    self.__class__.__name__, name))

        if isinstance(info, EnumInfo):
            g_type = info.get_g_type()
            value = g_type.pytype

            if value is None: 
                if g_type.is_a(gobject.TYPE_ENUM):
                    value = enum_add(g_type)
                else: 
                    value = flags_add(g_type)

                value.__info__ = info
                value.__module__ = info.get_namespace()

                for value_info in info.get_values():
                    name = value_info.get_name().upper()
                    setattr(value, name, value(value_info.get_value()))

        elif isinstance(info, RegisteredTypeInfo):
            g_type = info.get_g_type()

            # Check if there is already a Python wrapper.
            if g_type != gobject.TYPE_NONE:
                type_ = g_type.pytype
                if type_ is not None:
                    self.__dict__[name] = type_
                    return type_

            # Create a wrapper.
            if isinstance(info, ObjectInfo):
                parent = get_parent_for_object(info)
                interfaces = tuple(interface for interface in get_interfaces_for_object(info)
                        if not issubclass(parent, interface))
                bases = (parent,) + interfaces
                metaclass = GObjectMeta
            elif isinstance(info, InterfaceInfo):
                bases = (gobject.GInterface,)
                metaclass = GObjectMeta
            elif isinstance(info, StructInfo):
                if g_type.is_a(gobject.TYPE_BOXED):
                    bases = (Boxed,)
                elif g_type.is_a(gobject.TYPE_POINTER) or g_type == gobject.TYPE_NONE:
                    bases = (Struct,)
                else:
                    raise TypeError, "unable to create a wrapper for %s.%s" % (info.get_namespace(), info.get_name())
                metaclass = StructMeta
            else:
                raise NotImplementedError(info)

            name = info.get_name()
            dict_ = {
                '__info__': info,
                '__module__': self.__namespace__,
                '__gtype__': g_type
            }
            value = metaclass(name, bases, dict_)

            # Register the new Python wrapper.
            if g_type != gobject.TYPE_NONE:
                g_type.pytype = value

        elif isinstance(info, FunctionInfo):
            value = Function(info)
        elif isinstance(info, ConstantInfo):
            value = info.get_value()
        else:
            raise NotImplementedError(info)

        self.__dict__[name] = value
        return value

    @property
    def __members__(self):
        r = []
        for type_info in repository.get_infos(self.__namespace__):
            r.append(type_info.get_name())
        return r


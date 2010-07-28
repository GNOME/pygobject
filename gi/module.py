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
    UnionInfo, \
    Struct, \
    Boxed, \
    enum_add, \
    flags_add
from .types import \
    GObjectMeta, \
    StructMeta, \
    Function, \
    Enum

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


class IntrospectionModule(object):

    def __init__(self, namespace, version=None):
        repository.require(namespace, version)
        self._namespace = namespace
        self.version = version
        self.__name__ = 'gi.repository.' + namespace

        repository.require(self._namespace, self.version)

        if self.version is None:
            self.version = repository.get_version(self._namespace)

    def __getattr__(self, name):
        info = repository.find_by_name(self._namespace, name)
        if not info:
            raise AttributeError("%r object has no attribute %r" % (
                    self.__name__, name))

        if isinstance(info, EnumInfo):
            g_type = info.get_g_type()
            wrapper = g_type.pytype

            if wrapper is None:
                if g_type.is_a(gobject.TYPE_ENUM):
                    wrapper = enum_add(g_type)
                elif g_type.is_a(gobject.TYPE_NONE):
                    # An enum with a GType of None is an enum without GType
                    wrapper = Enum
                else:
                    wrapper = flags_add(g_type)

                wrapper.__info__ = info
                wrapper.__module__ = 'gi.repository.' + info.get_namespace()

                for value_info in info.get_values():
                    name = value_info.get_name().upper()
                    setattr(wrapper, name, wrapper(value_info.get_value()))

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
            elif isinstance(info, (StructInfo, UnionInfo)):
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
                '__module__': 'gi.repository.' + self._namespace,
                '__gtype__': g_type
            }
            wrapper = metaclass(name, bases, dict_)

            # Register the new Python wrapper.
            if g_type != gobject.TYPE_NONE:
                g_type.pytype = wrapper

        elif isinstance(info, FunctionInfo):
            wrapper = Function(info)
        elif isinstance(info, ConstantInfo):
            wrapper = info.get_value()
        else:
            raise NotImplementedError(info)

        self.__dict__[name] = wrapper
        return wrapper

    def __repr__(self):
        path = repository.get_typelib_path(self._namespace)
        return "<IntrospectionModule %r from %r>" % (self._namespace, path)


class DynamicGObjectModule(IntrospectionModule):
    """Wrapper for the GObject module

    This class allows us to access both the static PyGObject module and the GI GObject module
    through the same interface.  It is returned when by importing GObject from the gi repository:

    from gi.repository import GObject

    We use this because some PyGI interfaces generated from the GIR require GObject types not wrapped
    by the static bindings.  This also allows access to module attributes in a way that is more
    familiar to GI application developers.  Take signal flags as an example.  The G_SIGNAL_RUN_FIRST
    flag would be accessed as GObject.SIGNAL_RUN_FIRST in the static bindings but in the dynamic bindings
    can be accessed as GObject.SignalFlags.RUN_FIRST.  The latter follows a GI naming convention which
    would be familiar to GI application developers in a number of languages.
    """

    def __init__(self):
        IntrospectionModule.__init__(self, namespace='GObject')

    def __getattr__(self, name):
        # first see if this attr is in the gobject module
        attr = getattr(gobject, name, None)

        # if not in module assume request for an attr exported through GI
        if attr is None:
            attr = super(DynamicGObjectModule, self).__getattr__(name)

        return attr


class DynamicModule(object):
    def __init__(self, namespace):
        self._namespace = namespace
        self.introspection_module = None
        self._version = None
        self._overrides_module = None

    def require_version(self, version):
        if self.introspection_module is not None and \
                self.introspection_module.version != version:
            raise RuntimeError('Module has been already loaded ')
        self._version = version

    def _import(self):
        self.introspection_module = IntrospectionModule(self._namespace,
                                                        self._version)

        overrides_modules = __import__('gi.overrides', fromlist=[self._namespace])
        self._overrides_module = getattr(overrides_modules, self._namespace, None)

    def __getattr__(self, name):
        if self.introspection_module is None:
            self._import()

        if self._overrides_module is not None:
            override_exports = getattr(self._overrides_module, '__all__', ())
            if name in override_exports:
                return getattr(self._overrides_module, name, None)

        return getattr(self.introspection_module, name)

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
from . import _glib, _gobject
try:
    maketrans = ''.maketrans
except AttributeError:
    # fallback for Python 2
    from string import maketrans

import gi
from .overrides import registry

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
    enum_register_new_gtype_and_add, \
    flags_add, \
    flags_register_new_gtype_and_add
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
        return _gobject.GObject

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
        self._version = version
        self.__name__ = 'gi.repository.' + namespace

        repository.require(self._namespace, self._version)
        self.__path__ = repository.get_typelib_path(self._namespace)

        if self._version is None:
            self._version = repository.get_version(self._namespace)

    def __getattr__(self, name):
        info = repository.find_by_name(self._namespace, name)
        if not info:
            raise AttributeError("%r object has no attribute %r" % (
                    self.__name__, name))

        if isinstance(info, EnumInfo):
            g_type = info.get_g_type()
            wrapper = g_type.pytype

            if wrapper is None:
                if info.is_flags():
                    if g_type.is_a(_gobject.TYPE_FLAGS):
                        wrapper = flags_add(g_type)
                    else:
                        assert g_type == _gobject.TYPE_NONE
                        wrapper = flags_register_new_gtype_and_add(info)
                else:
                    if g_type.is_a(_gobject.TYPE_ENUM):
                        wrapper = enum_add(g_type)
                    else:
                        assert g_type == _gobject.TYPE_NONE
                        wrapper = enum_register_new_gtype_and_add(info)

                wrapper.__info__ = info
                wrapper.__module__ = 'gi.repository.' + info.get_namespace()

                # Don't use upper() here to avoid locale specific
                # identifier conversion (e. g. in Turkish 'i'.upper() == 'i')
                # see https://bugzilla.gnome.org/show_bug.cgi?id=649165
                ascii_upper_trans = maketrans(
                        'abcdefgjhijklmnopqrstuvwxyz', 
                        'ABCDEFGJHIJKLMNOPQRSTUVWXYZ')
                for value_info in info.get_values():
                    value_name = value_info.get_name().translate(ascii_upper_trans)
                    setattr(wrapper, value_name, wrapper(value_info.get_value()))

            if g_type != _gobject.TYPE_NONE:
                g_type.pytype = wrapper

        elif isinstance(info, RegisteredTypeInfo):
            g_type = info.get_g_type()

            # Check if there is already a Python wrapper.
            if g_type != _gobject.TYPE_NONE:
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
                bases = (_gobject.GInterface,)
                metaclass = GObjectMeta
            elif isinstance(info, (StructInfo, UnionInfo)):
                if g_type.is_a(_gobject.TYPE_BOXED):
                    bases = (Boxed,)
                elif g_type.is_a(_gobject.TYPE_POINTER) or \
                     g_type == _gobject.TYPE_NONE or \
                     g_type.fundamental == g_type:
                    bases = (Struct,)
                else:
                    raise TypeError("unable to create a wrapper for %s.%s" % (info.get_namespace(), info.get_name()))
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
            if g_type != _gobject.TYPE_NONE:
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

    def __dir__ (self):
        # Python's default dir() is just dir(self.__class__) + self.__dict__.keys()
        result = set(dir(self.__class__))
        result.update(self.__dict__.keys())

        # update *set* because some repository attributes have already been
        # wrapped by __getattr__() and included in self.__dict__
        namespace_infos = repository.get_infos(self._namespace)
        result.update(info.get_name() for info in namespace_infos)

        return list(result)

class DynamicModule(object):
    def __init__(self, namespace):
        self._namespace = namespace
        self._introspection_module = None
        self._overrides_module = None
        self.__path__ = None

    def _load(self):
        version = gi.get_required_version(self._namespace)
        self._introspection_module = IntrospectionModule(self._namespace,
                                                         version)

        overrides_modules = __import__('gi.overrides', fromlist=[self._namespace])
        self._overrides_module = getattr(overrides_modules, self._namespace, None)
        self.__path__ = repository.get_typelib_path(self._namespace)

    def __getattr__(self, name):
        if self._overrides_module is not None:
            override_exports = getattr(self._overrides_module, '__all__', ())
            if name in override_exports:
                return getattr(self._overrides_module, name, None)
        else:
            # check the registry just in case the module hasn't loaded yet
            # TODO: Only gtypes are registered in the registry right now 
            #       but it would be nice to register all overrides and 
            #       get rid of the module imports. We might actually see a 
            #       speedup.
            key = '%s.%s' % (self._namespace, name)
            if key in registry:
                return registry[key]

        return getattr(self._introspection_module, name)

    def __dir__ (self):
        # Python's default dir() is just dir(self.__class__) + self.__dict__.keys()
        result = set(dir(self.__class__))
        result.update(self.__dict__.keys())
        
        result.update(dir(self._introspection_module))
        override_exports = getattr(self._overrides_module, '__all__', ())
        result.update(override_exports)
        return list(result)

    def __repr__(self):
        path = repository.get_typelib_path(self._namespace)
        return "<%s.%s %r from %r>" % (self.__class__.__module__,
                                      self.__class__.__name__,
                                      self._namespace,
                                      path)

class DynamicGObjectModule(DynamicModule):
    """Wrapper for the internal GObject module

    This class allows us to access both the static internal PyGObject module and the GI GObject module
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
        DynamicModule.__init__(self, namespace='GObject')

    def __getattr__(self, name):
        from . import _gobject

        # first see if this attr is in the internal _gobject module
        attr = getattr(_gobject, name, None)

        # if not in module assume request for an attr exported through GI
        if attr is None:
            attr = super(DynamicGObjectModule, self).__getattr__(name)

        return attr


class DynamicGLibModule(DynamicModule):
    """Wrapper for the internal GLib module

    This class allows us to access both the static internal PyGLib module and the GI GLib module
    through the same interface.  It is returned when by importing GLib from the gi repository:

    from gi.repository import GLib

    We use this because some PyGI interfaces generated from the GIR require GLib types not wrapped
    by the static bindings.  This also allows access to module attributes in a way that is more
    familiar to GI application developers.
    """

    def __init__(self):
        DynamicModule.__init__(self, namespace='GLib')

    def __getattr__(self, name):
        # first see if this attr is in the internal _gobject module
        attr = getattr(_glib, name, None)

        # if not in module assume request for an attr exported through GI
        if attr is None:
            attr = super(DynamicGLibModule, self).__getattr__(name)

        return attr

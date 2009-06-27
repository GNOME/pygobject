# -*- Mode: Python; py-indent-offset: 4 -*-
#
# Copyright (C) 2007 Johan Dahlin <johan@gnome.org>
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

import os

import gobject
from gobject import GEnum

from .btypes import Function, BaseBlob, buildType
from .repo import EnumInfo, FunctionInfo, ObjectInfo, UnresolvedInfo, \
                  InterfaceInfo, StructInfo, BoxedInfo
from .repository import repository

class DynamicModule(object):
    def __init__(self, namespace, path):
        self._namespace = namespace
        self._path = path
        repository.register(self, namespace, path)
        self.created()

    @property
    def __file__(self):
        return self._namespace

    @property
    def __name__(self):
        return self._namespace

    @property
    def __path__(self):
        return [os.path.dirname(self.__file__)]

    def __repr__(self):
        return "<dyn-module %r from %r>" % (self._namespace, self._path)

    def __getattr__(self, name):
        type_info = repository.get_by_name(self._namespace, name)
        if not type_info:
            raise AttributeError("%r object has no attribute %r" % (
                    self.__class__.__name__, name))

        value = self._create_attribute(name, type_info)
        self.__dict__[name] = value
        return value

    @property
    def __members__(self):
        r = []
        for type_info in repository.get_infos(self._namespace):
            if type_info is None:
                continue
            r.append(type_info.getName())
        return r



    # Override this in a subclass

    def created(self):
        pass

    # Private API

    def _create_attribute(self, attr, type_info):
        if isinstance(type_info, ObjectInfo):
            return self._create_object(type_info)
        elif isinstance(type_info, EnumInfo):
            return self._create_enum(type_info)
        elif isinstance(type_info, FunctionInfo):
            return self._create_function(type_info)
        elif isinstance(type_info, InterfaceInfo):
            return self._create_interface(type_info)
        elif isinstance(type_info, StructInfo) or \
                isinstance(type_info, BoxedInfo):
            return self._create_boxed(type_info)
        else:
            raise NotImplementedError(type_info)

    def _get_parent_for_object(self, object_info):
        parent_info = object_info.getParent()

        if isinstance(parent_info, UnresolvedInfo):
            namespace = parent_info.getNamespace()
            __import__(namespace)
            parent_info = object_info.getParent()

        if not parent_info:
            parent = object
        else:
            namespace = parent_info.getNamespace()
            module = repository.get_module(namespace)
            name = parent_info.getName()
            try:
                # Hack for gobject.Object
                if module == gobject and name == 'Object':
                    name = 'GObject'
                parent = getattr(module, name)
            except AttributeError:
                return self._get_parent_for_object(parent_info)

        if parent is None:
            parent = object
        return parent

    def _create_object(self, object_info):
        name = object_info.getName()

        namespace = repository.get_c_prefix(object_info.getNamespace())
        full_name = namespace + name
        object_info.getGType()
        gtype = None
        try:
            gtype = gobject.GType.from_name(full_name)
        except RuntimeError:
            pass
        else:
            if gtype.pytype is not None:
                return gtype.pytype
        # Check if the klass is already created, eg
        # present in our namespace, this is necessary since we're
        # not always entering here through the __getattr__ hook.
        klass = self.__dict__.get(name)
        if klass:
            return klass

        parent = self._get_parent_for_object(object_info)
        klass = buildType(object_info, (parent,))
        if gtype is not None:
            klass.__gtype__ = gtype
            gtype.pytype = klass
        self.__dict__[name] = klass

        return klass

    def _create_enum(self, enum_info):
        ns = dict(__name__=enum_info.getName(),
                  __module__=enum_info.getNamespace())
        for value in enum_info.getValues():
            ns[value.getName().upper()] = value.getValue()
        return type(enum_info.getName(), (GEnum,), ns)

    def _create_function(self, function_info):
        return Function(function_info)

    def _create_interface(self, interface_info):
        name = interface_info.getName()

        namespace = repository.get_c_prefix(interface_info.getNamespace())
        full_name = namespace + name
        interface_info.getGType()
        gtype = None
        try:
            gtype = gobject.GType.from_name(full_name)
        except RuntimeError:
            pass
        else:
            if gtype.pytype is not None:
                return gtype.pytype
        # Check if the klass is already created, eg
        # present in our namespace, this is necessary since we're
        # not always entering here through the __getattr__ hook.
        klass = self.__dict__.get(name)
        if klass:
            return klass

        bases = (gobject.GInterface,)
        klass = buildType(interface_info, bases)
        if gtype is not None:
            klass.__gtype__ = gtype
            gtype.pytype = klass
            interface_info.register()
        self.__dict__[name] = klass

        return klass

    def _create_boxed(self, boxed_info):
        name = boxed_info.getName()

        namespace = repository.get_c_prefix(boxed_info.getNamespace())
        full_name = namespace + name

        gtype = None
        try:
            gtype = gobject.GType.from_name(full_name)
        except RuntimeError:
            pass
        else:
            if gtype.pytype is not None:
                return gtype.pytype

        # Check if the klass is already created, eg
        # present in our namespace, this is necessary since we're
        # not always entering here through the __getattr__ hook.
        klass = self.__dict__.get(name)
        if klass:
            return klass

        bases = (BaseBlob,)
        if isinstance(boxed_info, BoxedInfo):
            bases += gobject.Boxed

        klass = buildType(boxed_info, bases)

        if gtype is None:
            gtype = boxed_info.getGType()

        klass.__gtype__ = gtype
        gtype.pytype = klass

        self.__dict__[name] = klass

        return klass


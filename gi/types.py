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

from __future__ import absolute_import

import sys
import gobject

from ._gi import \
    InterfaceInfo, \
    ObjectInfo, \
    StructInfo, \
    set_object_has_new_constructor, \
    register_interface_info


def Function(info):

    def function(*args):
        return info.invoke(*args)
    function.__info__ = info
    function.__name__ = info.get_name()
    function.__module__ = info.get_namespace()

    return function


def Constructor(info):

    def constructor(cls, *args):
        cls_name = info.get_container().get_name()
        if cls.__name__ != cls_name:
            raise TypeError, '%s constructor cannot be used to create instances of a subclass' % cls_name
        return info.invoke(cls, *args)

    constructor.__info__ = info
    constructor.__name__ = info.get_name()
    constructor.__module__ = info.get_namespace()

    return constructor


class MetaClassHelper(object):

    def _setup_methods(cls):
        for method_info in cls.__info__.get_methods():
            name = method_info.get_name()
            function = Function(method_info)
            if method_info.is_method():
                method = function
            elif method_info.is_constructor():
                continue
            else:
                method = staticmethod(function)
            setattr(cls, name, method)

    def _setup_fields(cls):
        for field_info in cls.__info__.get_fields():
            name = field_info.get_name().replace('-', '_')
            setattr(cls, name, property(field_info.get_value, field_info.set_value))

    def _setup_constants(cls):
        for constant_info in cls.__info__.get_constants():
            name = constant_info.get_name()
            value = constant_info.get_value()
            setattr(cls, name, value)


class GObjectMeta(gobject.GObjectMeta, MetaClassHelper):

    def _setup_constructors(cls):
        for method_info in cls.__info__.get_methods():
            if method_info.is_constructor():
                name = method_info.get_name()
                constructor = classmethod(Constructor(method_info))
                setattr(cls, name, constructor)

    def __init__(cls, name, bases, dict_):
        super(GObjectMeta, cls).__init__(name, bases, dict_)

        # Avoid touching anything else than the base class.
        if cls.__info__.get_g_type().pytype is not None:
            return;

        cls._setup_methods()
        cls._setup_constants()

        if (isinstance(cls.__info__, ObjectInfo)):
            cls._setup_fields()
            cls._setup_constructors()
            set_object_has_new_constructor(cls.__info__.get_g_type())
        elif (isinstance(cls.__info__, InterfaceInfo)):
            register_interface_info(cls.__info__.get_g_type())


class StructMeta(type, MetaClassHelper):

    def _setup_constructors(cls):
        constructor_infos = []
        default_constructor_info = None

        for method_info in cls.__info__.get_methods():
            if method_info.is_constructor():
                name = method_info.get_name()
                constructor = classmethod(Function(method_info))

                setattr(cls, name, constructor)

                constructor_infos.append(method_info)
                if name == "new":
                    default_constructor_info = method_info

        if default_constructor_info is None and constructor_infos:
            default_constructor_info = constructor_infos[0]

        if default_constructor_info is not None:
            cls.__new__ = staticmethod(Function(default_constructor_info))

    def __init__(cls, name, bases, dict_):
        super(StructMeta, cls).__init__(name, bases, dict_)

        # Avoid touching anything else than the base class.
        g_type = cls.__info__.get_g_type()
        if g_type != gobject.TYPE_INVALID and g_type.pytype is not None:
            return;

        cls._setup_fields()
        cls._setup_methods()
        cls._setup_constructors()


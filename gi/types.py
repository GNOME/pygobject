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

import gobject
from new import instancemethod

from ._gi import InterfaceInfo, ObjectInfo, StructInfo, set_object_has_new_constructor


def Function(info):

    def function(*args):
        return info.invoke(*args)
    function.__info__ = info
    function.__name__ = info.get_name()
    function.__module__ = info.get_namespace()

    return function


class MetaClassHelper(object):

    def _setup_methods(cls):
        constructor_infos = []
        method_infos = cls.__info__.get_methods()

        for method_info in method_infos:
            name = method_info.get_name()
            function = Function(method_info)
            if method_info.is_method():
                method = instancemethod(function, None, cls)
            elif method_info.is_constructor():
                method = classmethod(function)
            else:
                method = staticmethod(function)
            setattr(cls, name, method)

            if method_info.is_constructor():
                constructor_infos.append(method_info)

        default_constructor_info = None
        if len(constructor_infos) == 1:
            (default_constructor_info,) = constructor_infos
        else:
            for constructor_info in constructor_infos:
                if constructor_info.get_name() == 'new':
                    default_constructor_info = constructor_info
                    break

        if default_constructor_info is not None:
            function = Function(default_constructor_info)
            cls.__new__ = staticmethod(function)
            # Override the initializer because of the constructor's arguments
            cls.__init__ = lambda self, *args, **kwargs: super(cls, self).__init__()

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

    def __init__(cls, name, bases, dict_):
        super(GObjectMeta, cls).__init__(name, bases, dict_)

        # Avoid touching anything else than the base class.
        if cls.__name__ != cls.__info__.get_name():
            return;

        cls._setup_methods()
        cls._setup_constants()

        if (isinstance(cls.__info__, ObjectInfo)):
            cls._setup_fields()
            set_object_has_new_constructor(cls.__info__.get_g_type())


class StructMeta(type, MetaClassHelper):

    def __init__(cls, name, bases, dict_):
        super(StructMeta, cls).__init__(name, bases, dict_)

        # Avoid touching anything else than the base class.
        if cls.__name__ != cls.__info__.get_name():
            return;

        cls._setup_fields()
        cls._setup_methods()


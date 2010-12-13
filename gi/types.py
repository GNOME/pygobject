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
    register_interface_info, \
    hook_up_vfunc_implementation


def Function(info):

    def function(*args):
        return info.invoke(*args)
    function.__info__ = info
    function.__name__ = info.get_name()
    function.__module__ = info.get_namespace()

    return function


def NativeVFunc(info, cls):

    def native_vfunc(*args):
        return info.invoke(*args, gtype=cls.__gtype__)
    native_vfunc.__info__ = info
    native_vfunc.__name__ = info.get_name()
    native_vfunc.__module__ = info.get_namespace()

    return native_vfunc

def Constructor(info):

    def constructor(cls, *args):
        cls_name = info.get_container().get_name()
        if cls.__name__ != cls_name:
            raise TypeError('%s constructor cannot be used to create instances of a subclass' % cls_name)
        return info.invoke(cls, *args)

    constructor.__info__ = info
    constructor.__name__ = info.get_name()
    constructor.__module__ = info.get_namespace()

    return constructor


class MetaClassHelper(object):

    def _setup_constructors(cls):
        for method_info in cls.__info__.get_methods():
            if method_info.is_constructor():
                name = method_info.get_name()
                constructor = classmethod(Constructor(method_info))
                setattr(cls, name, constructor)

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

    def _setup_vfuncs(cls, impl):
        for base in cls.__bases__:
            if not hasattr(base, '__info__') or \
                    not hasattr(base.__info__, 'get_vfuncs'):
                continue
            for vfunc_info in base.__info__.get_vfuncs():
                vfunc_name = 'do_' + vfunc_info.get_name()
                vfunc = getattr(impl, vfunc_name, None)
                if vfunc is not None:
                    # check to see if there are vfuncs with the same name in the bases
                    # we have no way of specifying which one we are supposed to override
                    ambiguous_base = find_vfunc_conflict_in_bases(vfunc_info, cls.__bases__)
                    if ambiguous_base is not None:
                        raise TypeError('Method %s() on class %s.%s is ambiguous '
                                'with methods in base classes %s.%s and %s.%s' %
                                (vfunc_name,
                                 impl.__info__.get_namespace(),
                                 impl.__info__.get_name(),
                                 base.__info__.get_namespace(),
                                 base.__info__.get_name(),
                                 ambiguous_base.__info__.get_namespace(),
                                 ambiguous_base.__info__.get_name()))

                    function = vfunc
                    if sys.version_info < (3, 0):
                        function = vfunc.im_func
 
                    if not is_function_in_classes(function, impl.__bases__):
                        hook_up_vfunc_implementation(vfunc_info, impl.__gtype__,
                                                     vfunc)

            base._setup_vfuncs(impl)

    def _setup_native_vfuncs(cls, impl):
        for base in cls.__bases__ + (cls,):
            if not hasattr(base, '__info__') or \
                    not hasattr(base.__info__, 'get_vfuncs') or \
                    isinstance(base.__info__, InterfaceInfo):
                continue
            for vfunc_info in base.__info__.get_vfuncs():
                name = 'do_%s' % vfunc_info.get_name()
                value = NativeVFunc(vfunc_info, impl)
                setattr(impl, name, value)

            if base != cls:
                base._setup_native_vfuncs(impl)


def find_vfunc_conflict_in_bases(vfunc, bases):
    for klass in bases:
        if not hasattr(klass, '__info__') or \
                not hasattr(klass.__info__, 'get_vfuncs'):
            continue
        vfuncs = klass.__info__.get_vfuncs()
        vfunc_name = vfunc.get_name()
        for v in vfuncs:
            if v.get_name() == vfunc_name and v != vfunc:
                return klass

        aklass = find_vfunc_conflict_in_bases(vfunc, klass.__bases__)
        if aklass is not None:
            return aklass
    return None

def is_function_in_classes(function, classes):
    for klass in classes:
        if function in klass.__dict__.values():
            return True
        elif is_function_in_classes(function, klass.__bases__):
            return True
    return False

class GObjectMeta(gobject.GObjectMeta, MetaClassHelper):

    def __init__(cls, name, bases, dict_):
        super(GObjectMeta, cls).__init__(name, bases, dict_)
        is_gi_defined = False
        if cls.__module__ == 'gi.repository.' + cls.__info__.get_namespace():
            is_gi_defined = True

        is_python_defined = False
        if not is_gi_defined and cls.__module__ != GObjectMeta.__module__:
            is_python_defined = True

        if is_python_defined:
            cls._setup_vfuncs(cls)
        elif is_gi_defined:
            cls._setup_methods()
            cls._setup_constants()
            cls._setup_native_vfuncs(cls)

            if isinstance(cls.__info__, ObjectInfo):
                cls._setup_fields()
                cls._setup_constructors()
                set_object_has_new_constructor(cls.__info__.get_g_type())
            elif isinstance(cls.__info__, InterfaceInfo):
                register_interface_info(cls.__info__.get_g_type())


class StructMeta(type, MetaClassHelper):

    def __init__(cls, name, bases, dict_):
        super(StructMeta, cls).__init__(name, bases, dict_)

        # Avoid touching anything else than the base class.
        g_type = cls.__info__.get_g_type()
        if g_type != gobject.TYPE_INVALID and g_type.pytype is not None:
            return

        cls._setup_fields()
        cls._setup_methods()
        cls._setup_constructors()

class Enum(int):
    __info__ = None
    def __init__(self, value):
        int.__init__(value)

    def __repr__(self):
        value_name = str(self)
        for value_info in self.__info__.get_values():
            if self == value_info.get_value():
                value_name = value_info.get_name().upper()
        return "<enum %s of type %s.%s>" % (value_name,
                                            self.__info__.get_namespace(),
                                            self.__info__.get_name())

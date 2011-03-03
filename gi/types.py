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
    VFuncInfo, \
    set_object_has_new_constructor, \
    register_interface_info, \
    hook_up_vfunc_implementation

if sys.version_info > (3, 0):
    def callable(obj):
        return hasattr(obj, '__call__')

def Function(info):

    def function(*args):
        return info.invoke(*args)
    function.__info__ = info
    function.__name__ = info.get_name()
    function.__module__ = info.get_namespace()

    return function


def NativeVFunc(info, cls):

    def native_vfunc(*args):
        return info.invoke(*args, **dict(gtype=cls.__gtype__))
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

    def _setup_vfuncs(cls):
        for vfunc_name, py_vfunc in cls.__dict__.items():
            if not vfunc_name.startswith("do_") or not callable(py_vfunc):
                continue

            # If a method name starts with "do_" assume it is a vfunc, and search
            # in the base classes for a method with the same name to override.
            # Recursion is not necessary here because getattr() searches all
            # super class attributes as well.
            vfunc_info = None
            for base in cls.__bases__:
                method = getattr(base, vfunc_name, None)
                if method is not None and hasattr(method, '__info__') and \
                        isinstance(method.__info__, VFuncInfo):
                    vfunc_info = method.__info__
                    break

            # If we did not find a matching method name in the bases, we might
            # be overriding an interface virtual method. Since interfaces do not
            # provide implementations, there will be no method attribute installed
            # on the object. Instead we have to search through
            # InterfaceInfo.get_vfuncs(). Note that the infos returned by
            # get_vfuncs() use the C vfunc name (ie. there is no "do_" prefix).
            if vfunc_info is None:
                vfunc_info = find_vfunc_info_in_interface(cls.__bases__, vfunc_name[len("do_"):])

            if vfunc_info is not None:
                assert vfunc_name == ('do_' + vfunc_info.get_name())
                # Check to see if there are vfuncs with the same name in the bases.
                # We have no way of specifying which one we are supposed to override.
                ambiguous_base = find_vfunc_conflict_in_bases(vfunc_info, cls.__bases__)
                if ambiguous_base is not None:
                    base_info = vfunc_info.get_container()
                    raise TypeError('Method %s() on class %s.%s is ambiguous '
                            'with methods in base classes %s.%s and %s.%s' %
                            (vfunc_name,
                             cls.__info__.get_namespace(),
                             cls.__info__.get_name(),
                             base_info.get_namespace(),
                             base_info.get_name(),
                             ambiguous_base.__info__.get_namespace(),
                             ambiguous_base.__info__.get_name()))

                hook_up_vfunc_implementation(vfunc_info, cls.__gtype__,
                                             py_vfunc)

    def _setup_native_vfuncs(cls):
        # Only InterfaceInfo and ObjectInfo have the get_vfuncs() method.
        # We skip InterfaceInfo because interfaces have no implementations for vfuncs.
        # Also check if __info__ in __dict__, not hasattr('__info__', ...)
        # because we do not want to accidentally retrieve __info__ from a base class.
        class_info = cls.__dict__.get('__info__')
        if class_info is None or not isinstance(class_info, ObjectInfo):
            return

        for vfunc_info in class_info.get_vfuncs():
            name = 'do_%s' % vfunc_info.get_name()
            value = NativeVFunc(vfunc_info, cls)
            setattr(cls, name, value)

def find_vfunc_info_in_interface(bases, vfunc_name):
    for base in bases:
        # All wrapped interfaces inherit from GInterface.
        # This can be seen in IntrospectionModule.__getattr__() in module.py.
        # We do not need to search regular classes here, only wrapped interfaces.
        # We also skip GInterface, because it is not wrapped and has no __info__ attr.
        if base is gobject.GInterface or\
                not issubclass(base, gobject.GInterface) or\
                not isinstance(base.__info__, InterfaceInfo):
            continue

        for vfunc in base.__info__.get_vfuncs():
            if vfunc.get_name() == vfunc_name:
                return vfunc

        vfunc = find_vfunc_info_in_interface(base.__bases__, vfunc_name)
        if vfunc is not None:
            return vfunc

    return None

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
            cls._setup_vfuncs()
        elif is_gi_defined:
            cls._setup_methods()
            cls._setup_constants()
            cls._setup_native_vfuncs()

            if isinstance(cls.__info__, ObjectInfo):
                cls._setup_fields()
                cls._setup_constructors()
                set_object_has_new_constructor(cls.__info__.get_g_type())
            elif isinstance(cls.__info__, InterfaceInfo):
                register_interface_info(cls.__info__.get_g_type())

    def mro(cls):
        return mro(cls)


def mro(C):
    """Compute the class precedence list (mro) according to C3

    Based on http://www.python.org/download/releases/2.3/mro/
    Modified to consider that interfaces don't create the diamond problem
    """
    # TODO: If this turns out being too slow, consider using generators
    bases = []
    bases_of_subclasses = [[C]]

    if C.__bases__:
        bases_of_subclasses += list(map(mro, C.__bases__)) + [list(C.__bases__)]

    while bases_of_subclasses:
        for subclass_bases in bases_of_subclasses:
            candidate = subclass_bases[0]
            not_head = [s for s in bases_of_subclasses if candidate in s[1:]]
            if not_head and gobject.GInterface not in candidate.__bases__:
                candidate = None # conflict, reject candidate
            else:
                break

        if candidate is None:
            raise TypeError('Cannot create a consistent method resolution '
                            'order (MRO)')

        bases.append(candidate)

        for subclass_bases in bases_of_subclasses[:]: # remove candidate
            if subclass_bases and subclass_bases[0] == candidate:
                del subclass_bases[0]
                if not subclass_bases:
                    bases_of_subclasses.remove(subclass_bases)

    return bases


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

        for method_info in cls.__info__.get_methods():
            if method_info.is_constructor() and \
                    method_info.get_name() == 'new' and \
                    not method_info.get_arguments():
                cls.__new__ = staticmethod(Constructor(method_info))
                break

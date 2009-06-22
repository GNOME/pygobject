# -*- Mode: Python; py-indent-offset: 4 -*-
#
# Copyright (C) 2005, 2007  Johan Dahlin <johan@gnome.org>
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

import gobject

import new

from . import repo

from .repository import repository

class Callable(object):

    # Types of callables
    INSTANCE_METHOD = 'method'
    STATIC_METHOD = 'static'
    CLASS_METHOD = 'class'
    FUNTION = 'function'

    def __init__(self, info):
        self.info = info
        self.call_type = None

    def __call__(self, *args, **kwargs):
        # TODO: put the kwargs in their right positions
        totalInArgs = args + tuple(kwargs.values())

        retval = self.info.invoke(*totalInArgs)

        if self.info.isConstructor():
            return None

        return retval

class Function(Callable):
    def __init__(self, info):
        Callable.__init__(self, info)
        self.info = info
        self.static = True

    def __repr__(self):
        return "<function %s>" % (self.info.getName(),)


class Method(Callable):

    def __init__(self, info, className, call_type=Callable.INSTANCE_METHOD):
        Callable.__init__(self, info)
        self.object = None
        self.className = className
        self.call_type = call_type
        self.__name__ = info.getName()
        self.__module__ = info.getNamespace()

    def newType(self, retval, type_info=None):
        if type_info == None:
            type_info = self.info.getReturnType()

        info = type_info.getInterface()
        klass = getClass(info)
        obj = klass.__new__(klass)
        obj._object = retval
        return obj

    #def __get__(self, instance, type):
        #if instance is None:
            #return self

        #def wrapper(*args, **kwargs):
            #return self(instance, *args, **kwargs)

        #return wrapper

    def __repr__(self):
        return "<method %s of %s.%s object>" % (
            self.__name__,
            self.__module__,
            self.className)

class FieldDescriptor(object):
    def __init__(self, info):
        self._info = info

    def __get__(self, obj, klass=None):
        return self._info.getValue(obj)

    def __set__(self, obj, value):
        return self._info.setValue(obj, value)

class PyBankMeta(gobject.GObjectMeta):
    def __init__(cls, name, bases, dict_):
        gobject.GObjectMeta.__init__(cls, name, bases, dict_)

        if hasattr(cls, '__gtype__'):
            repo.setObjectHasNewConstructor(cls.__gtype__)

        # Only set up the wrapper methods and fields in their base classes
        if name == cls.__info__.getName():
            needs_constructor = not '__init__' in dict_
            cls._setup_methods(needs_constructor)

            if hasattr(cls.__info__, 'getFields'):
                cls._setup_fields()

    def _setup_methods(cls, needs_constructor):
        info = cls.__info__
        constructors = []
        static_methods = []
        for method in info.getMethods():
            name = method.getName()

            if method.isConstructor():
                constructors.append(method)
            elif method.isMethod():
                func = Method(method, cls.__name__)
                setattr(cls, name, new.instancemethod(func, None, cls))
            else:
                static_methods.append(method)

        if hasattr(info, 'getInterfaces'):
            for interface in info.getInterfaces():
                for method in interface.getMethods():
                    name = method.getName()
                    if method.isMethod():
                        func = Method(method, interface.getName())
                        setattr(cls, name, new.instancemethod(func, None, cls))
                    else:
                        static_methods.append(method)

        winner = None
        if needs_constructor:
            if len(constructors) == 1:
                winner = constructors[0]
            else:
                for constructor in constructors:
                    if constructor.getName() == 'new':
                        winner = constructor
                        break

        if winner is not None:
            func = Method(winner, cls.__name__, call_type=Method.CLASS_METHOD)
            func.__name__ = '__init__'
            func.__orig_name__ = winner.getName()
            cls.__init__ = new.instancemethod(func, None, cls)
            # TODO: do we want the constructor as a static method?
            #constructors.remove(winner)

        static_methods.extend(constructors)
        for static_method in static_methods:
            func = Method(static_method, cls.__name__, call_type=Method.STATIC_METHOD)
            setattr(cls, static_method.getName(), staticmethod(func))

    def _setup_fields(cls):
        info = cls.__info__
        for field in info.getFields():
            name = field.getName().replace('-', '_')
            setattr(cls, name, FieldDescriptor(field))

_classDict = {}

def getClass(info):
    className = info.getName()
    namespaceName = info.getNamespace()
    fullName = namespaceName + '.' + className

    klass = _classDict.get(fullName)
    if klass is None:
        module = repository.get_module(info.getNamespace())
        klass = getattr(module, className)
    return klass

def buildType(info, bases):
    className = info.getName()
    namespaceName = info.getNamespace()
    fullName = namespaceName + '.' + className

    if _classDict.has_key(fullName):
        return _classDict[fullName]

    namespace = {}
    namespace['__info__'] = info
    namespace['__module__'] = namespaceName
    newType = PyBankMeta(className, bases, namespace)

    _classDict[fullName] = newType

    return newType

class BaseBlob(object):
    """Base class for Struct, Boxed and Union.
    """
    def __init__(self, buf=None):
        if buf is None:
            buf = self.__info__.newBuffer()
        self.__buffer__ = buf

    def __eq__(self, other):
        for field in self.__info__.getFields():
            if getattr(self, field.getName()) != getattr(other, field.getName()):
                return False
        return True

    def __ne__(self, other):
        return not self.__eq__(other)


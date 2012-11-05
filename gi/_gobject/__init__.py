# -*- Mode: Python; py-indent-offset: 4 -*-
# pygobject - Python bindings for the GObject library
# Copyright (C) 2006-2012  Johan Dahlin
#
#   gobject/__init__.py: initialisation file for gobject module
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
# USA

# this can go when things are a little further along

import sys

# we can't have pygobject 2 loaded at the same time we load the internal _gobject
if 'gobject' in sys.modules:
    raise ImportError('When using gi.repository you must not import static modules like "gobject". Please change all occurrences of "import gobject" to "from gi.repository import GObject".')

from . import _gobject
from . import propertyhelper
from . import signalhelper

GObject = _gobject.GObject
GType = _gobject.GType
_PyGObject_API = _gobject._PyGObject_API
pygobject_version = _gobject.pygobject_version


class GObjectMeta(type):
    "Metaclass for automatically registering GObject classes"
    def __init__(cls, name, bases, dict_):
        type.__init__(cls, name, bases, dict_)
        propertyhelper.install_properties(cls)
        signalhelper.install_signals(cls)
        cls._type_register(cls.__dict__)

    def _type_register(cls, namespace):
        ## don't register the class if already registered
        if '__gtype__' in namespace:
            return

        # Do not register a new GType for the overrides, as this would sort of
        # defeat the purpose of overrides...
        if cls.__module__.startswith('gi.overrides.'):
            return

        _gobject.type_register(cls, namespace.get('__gtype_name__'))

_gobject._install_metaclass(GObjectMeta)

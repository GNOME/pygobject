# -*- Mode: Python; py-indent-offset: 4 -*-
# pygobject - Python bindings for the GObject library
# Copyright (C) 2006  Johan Dahlin
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

# this can go when things are a little further along
try:
    import ltihooks
    ltihooks # pyflakes
    del ltihooks
except ImportError:
    pass

from _gobject import *

class GObjectMeta(type):
    "Metaclass for automatically registering GObject classes"
    def __init__(cls, name, bases, dict_):
        type.__init__(cls, name, bases, dict_)
        cls._type_register(cls.__dict__)

    def _type_register(cls, namespace):
        ## don't register the class if already registered
        if '__gtype__' in namespace:
            return

        if not ('__gproperties__' in namespace or
                '__gsignals__' in namespace or
                '__gtype_name__' in namespace):
            return

        type_register(cls, namespace.get('__gtype_name__'))

_gobject._install_metaclass(GObjectMeta)

# TYPE_INVALID defined in gobjectmodule.c
TYPE_NONE = type_from_name('void')
TYPE_INTERFACE = type_from_name('GInterface')
TYPE_CHAR = type_from_name('gchar')
TYPE_UCHAR = type_from_name('guchar')
TYPE_BOOLEAN = type_from_name('gboolean')
TYPE_INT = type_from_name('gint')
TYPE_UINT = type_from_name('guint')
TYPE_LONG = type_from_name('glong')
TYPE_ULONG = type_from_name('gulong')
TYPE_INT64 = type_from_name('gint64')
TYPE_UINT64 = type_from_name('guint64')
TYPE_ENUM = type_from_name('GEnum')
TYPE_FLAGS = type_from_name('GFlags')
TYPE_FLOAT = type_from_name('gfloat')
TYPE_DOUBLE = type_from_name('gdouble')
TYPE_STRING = type_from_name('gchararray')
TYPE_POINTER = type_from_name('gpointer')
TYPE_BOXED = type_from_name('GBoxed')
TYPE_PARAM = type_from_name('GParam')
TYPE_OBJECT = type_from_name('GObject')
TYPE_PYOBJECT = type_from_name('PyObject')
TYPE_UNICHAR = TYPE_UINT

del _gobject

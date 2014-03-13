# -*- Mode: Python; py-indent-offset: 4 -*-
# pygobject - Python bindings for the GObject library
# Copyright (C) 2006-2007 Johan Dahlin
#
#   gi/_constants.py: GObject type constants
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
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

from ._gi import _gobject

TYPE_INVALID = _gobject.TYPE_INVALID
TYPE_NONE = _gobject.type_from_name('void')
TYPE_INTERFACE = _gobject.type_from_name('GInterface')
TYPE_CHAR = _gobject.type_from_name('gchar')
TYPE_UCHAR = _gobject.type_from_name('guchar')
TYPE_BOOLEAN = _gobject.type_from_name('gboolean')
TYPE_INT = _gobject.type_from_name('gint')
TYPE_UINT = _gobject.type_from_name('guint')
TYPE_LONG = _gobject.type_from_name('glong')
TYPE_ULONG = _gobject.type_from_name('gulong')
TYPE_INT64 = _gobject.type_from_name('gint64')
TYPE_UINT64 = _gobject.type_from_name('guint64')
TYPE_ENUM = _gobject.type_from_name('GEnum')
TYPE_FLAGS = _gobject.type_from_name('GFlags')
TYPE_FLOAT = _gobject.type_from_name('gfloat')
TYPE_DOUBLE = _gobject.type_from_name('gdouble')
TYPE_STRING = _gobject.type_from_name('gchararray')
TYPE_POINTER = _gobject.type_from_name('gpointer')
TYPE_BOXED = _gobject.type_from_name('GBoxed')
TYPE_PARAM = _gobject.type_from_name('GParam')
TYPE_OBJECT = _gobject.type_from_name('GObject')
TYPE_PYOBJECT = _gobject.type_from_name('PyObject')
TYPE_GTYPE = _gobject.type_from_name('GType')
TYPE_STRV = _gobject.type_from_name('GStrv')
TYPE_VARIANT = _gobject.type_from_name('GVariant')
TYPE_UNICHAR = TYPE_UINT

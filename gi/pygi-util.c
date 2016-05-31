/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   gobjectmodule.c: wrapper for the gobject library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "pygi-util.h"

PyObject *
pyg_integer_richcompare(PyObject *v, PyObject *w, int op)
{
    PyObject *result;
    gboolean t;

    switch (op) {
    case Py_EQ: t = PYGLIB_PyLong_AS_LONG(v) == PYGLIB_PyLong_AS_LONG(w); break;
    case Py_NE: t = PYGLIB_PyLong_AS_LONG(v) != PYGLIB_PyLong_AS_LONG(w); break;
    case Py_LE: t = PYGLIB_PyLong_AS_LONG(v) <= PYGLIB_PyLong_AS_LONG(w); break;
    case Py_GE: t = PYGLIB_PyLong_AS_LONG(v) >= PYGLIB_PyLong_AS_LONG(w); break;
    case Py_LT: t = PYGLIB_PyLong_AS_LONG(v) <  PYGLIB_PyLong_AS_LONG(w); break;
    case Py_GT: t = PYGLIB_PyLong_AS_LONG(v) >  PYGLIB_PyLong_AS_LONG(w); break;
    default: g_assert_not_reached();
    }

    result = t ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}

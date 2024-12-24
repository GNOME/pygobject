/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
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

#include <pythoncapi_compat.h>

#include "pygi-util.h"

gboolean
pygi_guint_from_pyssize (Py_ssize_t pyval, guint *result)
{
    if (pyval < 0) {
        PyErr_SetString (PyExc_ValueError, "< 0");
        return FALSE;
    } else if (G_MAXUINT < PY_SSIZE_T_MAX && pyval > (Py_ssize_t)G_MAXUINT) {
        PyErr_SetString (PyExc_ValueError, "too large");
        return FALSE;
    }
    *result = (guint)pyval;
    return TRUE;
}

PyObject *
pyg_integer_richcompare (PyObject *v, PyObject *w, int op)
{
    PyObject *result;
    gboolean t;

    switch (op) {
    case Py_EQ:
        t = PyLong_AS_LONG (v) == PyLong_AS_LONG (w);
        break;
    case Py_NE:
        t = PyLong_AS_LONG (v) != PyLong_AS_LONG (w);
        break;
    case Py_LE:
        t = PyLong_AS_LONG (v) <= PyLong_AS_LONG (w);
        break;
    case Py_GE:
        t = PyLong_AS_LONG (v) >= PyLong_AS_LONG (w);
        break;
    case Py_LT:
        t = PyLong_AS_LONG (v) < PyLong_AS_LONG (w);
        break;
    case Py_GT:
        t = PyLong_AS_LONG (v) > PyLong_AS_LONG (w);
        break;
    default:
        g_assert_not_reached ();
    }

    result = t ? Py_True : Py_False;
    return Py_NewRef (result);
}

PyObject *
pyg_ptr_richcompare (void *a, void *b, int op)
{
    PyObject *res;

    switch (op) {
    case Py_EQ:
        res = (a == b) ? Py_True : Py_False;
        break;
    case Py_NE:
        res = (a != b) ? Py_True : Py_False;
        break;
    case Py_LT:
        res = (a < b) ? Py_True : Py_False;
        break;
    case Py_LE:
        res = (a <= b) ? Py_True : Py_False;
        break;
    case Py_GT:
        res = (a > b) ? Py_True : Py_False;
        break;
    case Py_GE:
        res = (a >= b) ? Py_True : Py_False;
        break;
    default:
        res = Py_NotImplemented;
        break;
    }

    return Py_NewRef (res);
}

/**
 * pyg_constant_strip_prefix:
 * @name: the constant name.
 * @strip_prefix: the prefix to strip.
 *
 * Advances the pointer @name by strlen(@strip_prefix) characters.  If
 * the resulting name does not start with a letter or underscore, the
 * @name pointer will be rewound.  This is to ensure that the
 * resulting name is a valid identifier.  Hence the returned string is
 * a pointer into the string @name.
 *
 * Returns: the stripped constant name.
 */
const gchar *
pyg_constant_strip_prefix (const gchar *name, const gchar *strip_prefix)
{
    size_t prefix_len, i;

    prefix_len = strlen (strip_prefix);

    /* Check so name starts with strip_prefix, if it doesn't:
     * return the rest of the part which doesn't match
     */
    for (i = 0; i < prefix_len; i++) {
        if (name[i] != strip_prefix[i] && name[i] != '_') {
            return &name[i];
        }
    }

    /* strip off prefix from value name, while keeping it a valid
     * identifier */
    for (i = prefix_len + 1; i > 0; i--) {
        if (g_ascii_isalpha (name[i - 1]) || name[i - 1] == '_') {
            return &name[i - 1];
        }
    }
    return name;
}

/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2014 Simon Feltman <sfeltman@gnome.org>
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

#include "pyglib.h"
#include "pygi-private.h"
#include "pygi-error.h"


static gboolean
_pygi_marshal_from_py_gerror (PyGIInvokeState   *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache      *arg_cache,
                              PyObject          *py_arg,
                              GIArgument        *arg,
                              gpointer          *cleanup_data)
{
    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for GErrors is not implemented");
    return FALSE;
}

static PyObject *
_pygi_marshal_to_py_gerror (PyGIInvokeState   *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache      *arg_cache,
                            GIArgument        *arg)
{
    GError *error = arg->v_pointer;
    PyObject *py_obj = NULL;

    py_obj = pyglib_error_marshal(&error);

    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING && error != NULL) {
        g_error_free (error);
    }

    if (py_obj != NULL) {
        return py_obj;
    } else {
        Py_RETURN_NONE;
    }
}

static gboolean
pygi_arg_gerror_setup_from_info (PyGIArgCache  *arg_cache,
                                 GITypeInfo    *type_info,
                                 GIArgInfo     *arg_info,
                                 GITransfer     transfer,
                                 PyGIDirection  direction)
{
    if (!pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction)) {
        return FALSE;
    }

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_gerror;
        arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_gerror;
        arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
    }

    return TRUE;
}

PyGIArgCache *
pygi_arg_gerror_new_from_info (GITypeInfo   *type_info,
                               GIArgInfo    *arg_info,
                               GITransfer    transfer,
                               PyGIDirection direction)
{
    gboolean res = FALSE;
    PyGIArgCache *arg_cache = NULL;

    arg_cache = _arg_cache_alloc ();
    if (arg_cache == NULL)
        return NULL;

    res = pygi_arg_gerror_setup_from_info (arg_cache,
                                           type_info,
                                           arg_info,
                                           transfer,
                                           direction);
    if (res) {
        return arg_cache;
    } else {
        _pygi_arg_cache_free (arg_cache);
        return NULL;
    }
}

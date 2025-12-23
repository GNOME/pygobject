/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 1998-2003  James Henstridge
 *               2004-2008  Johan Dahlin
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

#include "pygi-error.h"
#include "pygi-cache-private.h"

static gboolean
_pygi_marshal_from_py_gerror (PyGIInvokeState *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache *arg_cache, PyObject *py_arg,
                              GIArgument *arg,
                              PyGIMarshalCleanupData *cleanup_data)
{
    GError *error = NULL;
    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        return TRUE;
    } else if (pygi_error_marshal_from_py (py_arg, &error)) {
        arg->v_pointer = error;
        if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
            pygi_marshal_cleanup_data_init (cleanup_data, error,
                                            (GDestroyNotify)g_error_free);
        }
        return TRUE;
    } else {
        return FALSE;
    }
}


static void
_pygi_marshal_from_py_gerror_cleanup (PyGIInvokeState *state,
                                      PyGIMarshalCleanupData cleanup_data)
{
    pygi_marshal_cleanup_data_destroy (&cleanup_data);
}

static PyObject *
_pygi_marshal_to_py_gerror (PyGIInvokeState *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache *arg_cache, GIArgument *arg,
                            PyGIMarshalCleanupData *cleanup_data)
{
    GError *error = arg->v_pointer;
    PyObject *py_obj = NULL;

    py_obj = pygi_error_marshal_to_py (&error);

    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING && error != NULL) {
        g_error_free (error);
    }

    return py_obj;
}

PyGIArgCache *
pygi_arg_gerror_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                               GITransfer transfer, PyGIDirection direction)
{
    PyGIArgCache *arg_cache;

    arg_cache = pygi_arg_cache_alloc ();

    pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction);

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_gerror;
        arg_cache->from_py_cleanup = _pygi_marshal_from_py_gerror_cleanup;
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_gerror;
        arg_cache->meta_type = PYGI_META_ARG_TYPE_PARENT;
    }

    return arg_cache;
}

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

#include "pygi-basictype.h"
#include "pygi-cache-private.h"


/* pygi_arg_base_setup:
 * arg_cache: argument cache to initialize
 * type_info: source for type related attributes to cache
 * arg_info: (allow-none): source for argument related attributes to cache
 * transfer: transfer mode to store in the argument cache
 * direction: marshaling direction to store in the cache
 *
 * Initializer for PyGIArgCache
 */
void
pygi_arg_base_setup (
    PyGIArgCache *arg_cache, GITypeInfo *type_info,
    GIArgInfo *arg_info, /* may be NULL for return arguments */
    GITransfer transfer, PyGIDirection direction)
{
    arg_cache->direction = direction;
    arg_cache->transfer = transfer;
    arg_cache->py_arg_index = -1;
    arg_cache->c_arg_index = -1;

    if (type_info != NULL) {
        arg_cache->is_pointer = gi_type_info_is_pointer (type_info);
        arg_cache->type_tag = gi_type_info_get_tag (type_info);
        gi_base_info_ref ((GIBaseInfo *)type_info);
        arg_cache->type_info = type_info;
    }

    if (arg_info != NULL) {
        gi_base_info_ref ((GIBaseInfo *)arg_info);
        arg_cache->arg_info = arg_info;
    }
}

gboolean
pygi_marshal_from_py_basic_type_cache_adapter (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, PyObject *py_arg, GIArgument *arg,
    PyGIMarshalCleanupData *cleanup_data)
{
    *arg = pygi_marshal_from_py_basic_type (py_arg, arg_cache->type_tag,
                                            arg_cache->transfer,
                                            &(cleanup_data->data));
    return !PyErr_Occurred ();
}

PyObject *
pygi_marshal_to_py_basic_type_cache_adapter (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, GIArgument *arg,
    PyGIMarshalCleanupData *cleanup_data)
{
    return pygi_marshal_to_py_basic_type (*arg, arg_cache->type_tag,
                                          arg_cache->transfer);
}

static gboolean
marshal_from_py_void (PyGIInvokeState *state,
                      PyGICallableCache *callable_cache,
                      PyGIArgCache *arg_cache, PyObject *py_arg,
                      GIArgument *arg, PyGIMarshalCleanupData *cleanup_data)
{
    g_warn_if_fail (arg_cache->transfer == GI_TRANSFER_NOTHING);

    return pygi_gpointer_from_py (py_arg, &(arg->v_pointer));
}

static PyObject *
marshal_to_py_void (PyGIInvokeState *state, PyGICallableCache *callable_cache,
                    PyGIArgCache *arg_cache, GIArgument *arg,
                    PyGIMarshalCleanupData *cleanup_data)
{
    if (arg_cache->is_pointer) {
        return PyLong_FromVoidPtr (arg->v_pointer);
    }
    Py_RETURN_NONE;
}


static gboolean
pygi_marshal_from_py_utf8_cache_adapter (PyGIInvokeState *state,
                                         PyGICallableCache *callable_cache,
                                         PyGIArgCache *arg_cache,
                                         PyObject *py_arg, GIArgument *arg,
                                         PyGIMarshalCleanupData *cleanup_data)
{
    *arg = pygi_marshal_from_py_basic_type (py_arg, arg_cache->type_tag,
                                            arg_cache->transfer,
                                            &(cleanup_data->data));

    /* We strdup strings so free unless ownership is transferred to C. */
    if (cleanup_data->data != NULL)
        pygi_marshal_cleanup_data_init_full (
            cleanup_data, arg->v_pointer,
            arg_cache->transfer == GI_TRANSFER_NOTHING ? g_free : NULL,
            g_free);

    return !PyErr_Occurred ();
}

static PyObject *
pygi_marshal_to_py_utf8_cache_adapter (PyGIInvokeState *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache *arg_cache,
                                       GIArgument *arg,
                                       PyGIMarshalCleanupData *cleanup_data)
{
    PyObject *object = pygi_marshal_to_py_basic_type (
        *arg, arg_cache->type_tag, arg_cache->transfer);

    /* Python copies the string so we need to free it
       if the interface is transfering ownership,
       whether or not it has been processed yet */
    pygi_marshal_cleanup_data_init_full (
        cleanup_data, arg->v_pointer,
        arg_cache->transfer == GI_TRANSFER_EVERYTHING ? g_free : NULL, g_free);

    return object;
}

PyGIArgCache *
pygi_arg_void_type_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                                  GITransfer transfer, PyGIDirection direction)
{
    PyGIArgCache *arg_cache = pygi_arg_cache_alloc ();

    pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction);

    if (direction & PYGI_DIRECTION_FROM_PYTHON)
        arg_cache->from_py_marshaller = marshal_from_py_void;

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        arg_cache->to_py_marshaller = marshal_to_py_void;

    return arg_cache;
}

PyGIArgCache *
pygi_arg_numeric_type_new_from_info (GITypeInfo *type_info,
                                     GIArgInfo *arg_info, GITransfer transfer,
                                     PyGIDirection direction)
{
    PyGIArgCache *arg_cache = pygi_arg_cache_alloc ();

    pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction);

    if (direction & PYGI_DIRECTION_FROM_PYTHON)
        arg_cache->from_py_marshaller =
            pygi_marshal_from_py_basic_type_cache_adapter;

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        arg_cache->to_py_marshaller =
            pygi_marshal_to_py_basic_type_cache_adapter;

    return arg_cache;
}


PyGIArgCache *
pygi_arg_string_type_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                                    GITransfer transfer,
                                    PyGIDirection direction)
{
    PyGIArgCache *arg_cache = pygi_arg_cache_alloc ();

    pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction);

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        arg_cache->from_py_marshaller =
            pygi_marshal_from_py_utf8_cache_adapter;
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        arg_cache->to_py_marshaller = pygi_marshal_to_py_utf8_cache_adapter;
    }

    return arg_cache;
}

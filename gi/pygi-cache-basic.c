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
    gpointer *cleanup_data)
{
    return pygi_marshal_from_py_basic_type (py_arg, arg, arg_cache->type_tag,
                                            arg_cache->transfer, cleanup_data);
}

PyObject *
pygi_marshal_to_py_basic_type_cache_adapter (PyGIInvokeState *state,
                                             PyGICallableCache *callable_cache,
                                             PyGIArgCache *arg_cache,
                                             GIArgument *arg,
                                             gpointer *cleanup_data)
{
    return pygi_marshal_to_py_basic_type (arg, arg_cache->type_tag,
                                          arg_cache->transfer);
}

static gboolean
marshal_from_py_void (PyGIInvokeState *state,
                      PyGICallableCache *callable_cache,
                      PyGIArgCache *arg_cache, PyObject *py_arg,
                      GIArgument *arg, gpointer *cleanup_data)
{
    g_warn_if_fail (arg_cache->transfer == GI_TRANSFER_NOTHING);

    if (pygi_gpointer_from_py (py_arg, &(arg->v_pointer))) {
        *cleanup_data = arg->v_pointer;
        return TRUE;
    }

    return FALSE;
}

static PyObject *
marshal_to_py_void (PyGIInvokeState *state, PyGICallableCache *callable_cache,
                    PyGIArgCache *arg_cache, GIArgument *arg,
                    gpointer *cleanup_data)
{
    if (arg_cache->is_pointer) {
        return PyLong_FromVoidPtr (arg->v_pointer);
    }
    Py_RETURN_NONE;
}

static void
marshal_cleanup_from_py_utf8 (PyGIInvokeState *state, PyGIArgCache *arg_cache,
                              PyObject *py_arg, gpointer data,
                              gboolean was_processed)
{
    /* We strdup strings so free unless ownership is transferred to C. */
    if (was_processed && arg_cache->transfer == GI_TRANSFER_NOTHING)
        g_free (data);
}

static void
marshal_cleanup_to_py_utf8 (PyGIInvokeState *state, PyGIArgCache *arg_cache,
                            gpointer cleanup_data, gpointer data,
                            gboolean was_processed)
{
    /* Python copies the string so we need to free it
       if the interface is transfering ownership,
       whether or not it has been processed yet */
    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING) g_free (data);
}

PyGIArgCache *
pygi_arg_basic_type_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                                   GITransfer transfer,
                                   PyGIDirection direction)
{
    PyGIArgCache *arg_cache = pygi_arg_cache_alloc ();
    GITypeTag type_tag = gi_type_info_get_tag (type_info);

    pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction);

    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
        if (direction & PYGI_DIRECTION_FROM_PYTHON)
            arg_cache->from_py_marshaller = marshal_from_py_void;

        if (direction & PYGI_DIRECTION_TO_PYTHON)
            arg_cache->to_py_marshaller = marshal_to_py_void;

        break;
    case GI_TYPE_TAG_BOOLEAN:
    case GI_TYPE_TAG_INT8:
    case GI_TYPE_TAG_UINT8:
    case GI_TYPE_TAG_INT16:
    case GI_TYPE_TAG_UINT16:
    case GI_TYPE_TAG_INT32:
    case GI_TYPE_TAG_UINT32:
    case GI_TYPE_TAG_INT64:
    case GI_TYPE_TAG_UINT64:
    case GI_TYPE_TAG_FLOAT:
    case GI_TYPE_TAG_DOUBLE:
    case GI_TYPE_TAG_UNICHAR:
    case GI_TYPE_TAG_GTYPE:
        if (direction & PYGI_DIRECTION_FROM_PYTHON)
            arg_cache->from_py_marshaller =
                pygi_marshal_from_py_basic_type_cache_adapter;

        if (direction & PYGI_DIRECTION_TO_PYTHON)
            arg_cache->to_py_marshaller =
                pygi_marshal_to_py_basic_type_cache_adapter;

        break;
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
        if (direction & PYGI_DIRECTION_FROM_PYTHON) {
            arg_cache->from_py_marshaller =
                pygi_marshal_from_py_basic_type_cache_adapter;
            arg_cache->from_py_cleanup = marshal_cleanup_from_py_utf8;
        }

        if (direction & PYGI_DIRECTION_TO_PYTHON) {
            arg_cache->to_py_marshaller =
                pygi_marshal_to_py_basic_type_cache_adapter;
            arg_cache->to_py_cleanup = marshal_cleanup_to_py_utf8;
        }

        break;
    case GI_TYPE_TAG_ARRAY:
    case GI_TYPE_TAG_INTERFACE:
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
    case GI_TYPE_TAG_GHASH:
    case GI_TYPE_TAG_ERROR:
    default:
        g_assert_not_reached ();
    }

    return arg_cache;
}

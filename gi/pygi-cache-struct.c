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

#include "pygpointer.h"
#include "pygi-struct-marshal.h"
#include "pygi-boxed.h"
#include "pygi-foreign.h"
#include "pygi-struct.h"
#include "pygi-type.h"
#include "pygi-value.h"
#include "pygi-cache-private.h"

static gboolean
arg_type_class_from_py_marshal (PyGIInvokeState *state,
                                PyGICallableCache *callable_cache,
                                PyGIArgCache *arg_cache, PyObject *py_arg,
                                GIArgument *arg, gpointer *cleanup_data)
{
    GType gtype = pyg_type_from_object (py_arg);

    if (G_TYPE_IS_CLASSED (gtype)) {
        arg->v_pointer = g_type_class_ref (gtype);
        *cleanup_data = arg->v_pointer;
        return TRUE;
    } else {
        PyErr_Format (PyExc_TypeError,
                      "Unable to retrieve a GObject type class from \"%s\".",
                      Py_TYPE (py_arg)->tp_name);
        return FALSE;
    }
}

static void
arg_type_class_from_py_cleanup (PyGIInvokeState *state,
                                PyGIArgCache *arg_cache, PyObject *py_arg,
                                gpointer data, gboolean was_processed)
{
    if (was_processed) {
        g_type_class_unref (data);
    }
}

static gboolean
arg_struct_from_py_marshal_adapter (PyGIInvokeState *state,
                                    PyGICallableCache *callable_cache,
                                    PyGIArgCache *arg_cache, PyObject *py_arg,
                                    GIArgument *arg, gpointer *cleanup_data)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    gboolean res = pygi_arg_struct_from_py_marshal (
        py_arg, arg, pygi_arg_cache_get_name (arg_cache),
        GI_REGISTERED_TYPE_INFO (iface_cache->interface_info),
        iface_cache->g_type, iface_cache->py_type, arg_cache->transfer,
        TRUE, /*copy_reference*/
        iface_cache->is_foreign, arg_cache->is_pointer);

    /* Assume struct marshaling is always a pointer and assign cleanup_data
     * here rather than passing it further down the chain.
     */
    *cleanup_data = arg->v_pointer;
    return res;
}

static PyObject *
arg_struct_to_py_marshal_adapter (PyGIInvokeState *state,
                                  PyGICallableCache *callable_cache,
                                  PyGIArgCache *arg_cache, GIArgument *arg,
                                  gpointer *cleanup_data)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    PyObject *ret;

    ret = pygi_arg_struct_to_py_marshal (
        arg, iface_cache->interface_info, iface_cache->g_type,
        iface_cache->py_type, arg_cache->transfer,
        pygi_arg_cache_is_caller_allocates (arg_cache),
        iface_cache->is_foreign);

    *cleanup_data = ret;

    return ret;
}

static void
arg_gclosure_from_py_cleanup (PyGIInvokeState *state, PyGIArgCache *arg_cache,
                              PyObject *py_arg, gpointer cleanup_data,
                              gboolean was_processed)
{
    if (cleanup_data != NULL) {
        g_closure_unref (cleanup_data);
    }
}

static void
arg_foreign_from_py_cleanup (PyGIInvokeState *state, PyGIArgCache *arg_cache,
                             PyObject *py_arg, gpointer data,
                             gboolean was_processed)
{
    if (state->failed && was_processed) {
        pygi_struct_foreign_release (
            GI_BASE_INFO (((PyGIInterfaceCache *)arg_cache)->interface_info),
            data);
    }
}

void
pygi_arg_gvalue_from_py_cleanup (PyGIInvokeState *state,
                                 PyGIArgCache *arg_cache, PyObject *py_arg,
                                 gpointer data, gboolean was_processed)
{
    /* Note py_arg can be NULL for hash table which is a bug. */
    if (was_processed && py_arg != NULL) {
        GType py_object_type =
            pyg_type_from_object_strict ((PyObject *)Py_TYPE (py_arg), FALSE);

        /* When a GValue was not passed, it means the marshalers created a new
         * one to pass in, clean this up.
         */
        if (py_object_type != G_TYPE_VALUE) {
            g_value_unset ((GValue *)data);
            g_slice_free (GValue, data);
        }
    }
}

static void
arg_foreign_to_py_cleanup (PyGIInvokeState *state, PyGIArgCache *arg_cache,
                           gpointer cleanup_data, gpointer data,
                           gboolean was_processed)
{
    if (!was_processed && arg_cache->transfer == GI_TRANSFER_EVERYTHING) {
        pygi_struct_foreign_release (
            GI_BASE_INFO (((PyGIInterfaceCache *)arg_cache)->interface_info),
            data);
    }
}

static void
arg_boxed_to_py_cleanup (PyGIInvokeState *state, PyGIArgCache *arg_cache,
                         gpointer cleanup_data, gpointer data,
                         gboolean was_processed)
{
    if (cleanup_data != NULL && arg_cache->transfer == GI_TRANSFER_NOTHING)
        pygi_boxed_copy_in_place ((PyGIBoxed *)cleanup_data);
}

static void
arg_struct_from_py_setup (PyGIArgCache *arg_cache,
                          GIRegisteredTypeInfo *iface_info,
                          GITransfer transfer)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (gi_struct_info_is_gtype_struct ((GIStructInfo *)iface_info)) {
        arg_cache->from_py_marshaller = arg_type_class_from_py_marshal;
        /* Since we always add a ref in the marshalling, only unref the
         * GTypeClass when we don't transfer ownership. */
        if (transfer == GI_TRANSFER_NOTHING) {
            arg_cache->from_py_cleanup = arg_type_class_from_py_cleanup;
        }

    } else {
        arg_cache->from_py_marshaller = arg_struct_from_py_marshal_adapter;

        if (g_type_is_a (iface_cache->g_type, G_TYPE_CLOSURE)) {
            arg_cache->from_py_cleanup = arg_gclosure_from_py_cleanup;

        } else if (iface_cache->g_type == G_TYPE_VALUE) {
            arg_cache->from_py_cleanup = pygi_arg_gvalue_from_py_cleanup;

        } else if (iface_cache->is_foreign) {
            arg_cache->from_py_cleanup = arg_foreign_from_py_cleanup;
        }
    }
}

static void
arg_struct_to_py_setup (PyGIArgCache *arg_cache,
                        GIRegisteredTypeInfo *iface_info, GITransfer transfer)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (arg_cache->to_py_marshaller == NULL) {
        arg_cache->to_py_marshaller = arg_struct_to_py_marshal_adapter;
    }

    iface_cache->is_foreign =
        gi_struct_info_is_foreign ((GIStructInfo *)iface_info);

    if (iface_cache->is_foreign)
        arg_cache->to_py_cleanup = arg_foreign_to_py_cleanup;
    else if (!g_type_is_a (iface_cache->g_type, G_TYPE_VALUE)
             && iface_cache->py_type
             && g_type_is_a (iface_cache->g_type, G_TYPE_BOXED))
        arg_cache->to_py_cleanup = arg_boxed_to_py_cleanup;
}

PyGIArgCache *
pygi_arg_struct_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                               GITransfer transfer, PyGIDirection direction,
                               GIRegisteredTypeInfo *iface_info)
{
    PyGIArgCache *cache = NULL;
    PyGIInterfaceCache *iface_cache;

    cache = pygi_arg_interface_new_from_info (type_info, arg_info, transfer,
                                              direction, iface_info);
    if (cache == NULL) return NULL;

    iface_cache = (PyGIInterfaceCache *)cache;
    iface_cache->is_foreign =
        (GI_IS_STRUCT_INFO ((GIBaseInfo *)iface_info))
        && (gi_struct_info_is_foreign ((GIStructInfo *)iface_info));

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        arg_struct_from_py_setup (cache, iface_info, transfer);
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        arg_struct_to_py_setup (cache, iface_info, transfer);
    }

    return cache;
}

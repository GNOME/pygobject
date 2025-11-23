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

#include "pygi-type.h"
#include "pygenum.h"
#include "pygflags.h"
#include "pygi-argument.h"
#include "pygi-cache-private.h"

static gboolean
gi_argument_to_c_long (GIArgument *arg_in, long *c_long_out,
                       GITypeTag type_tag)
{
    switch (type_tag) {
    case GI_TYPE_TAG_INT8:
        *c_long_out = arg_in->v_int8;
        return TRUE;
    case GI_TYPE_TAG_UINT8:
        *c_long_out = arg_in->v_uint8;
        return TRUE;
    case GI_TYPE_TAG_INT16:
        *c_long_out = arg_in->v_int16;
        return TRUE;
    case GI_TYPE_TAG_UINT16:
        *c_long_out = arg_in->v_uint16;
        return TRUE;
    case GI_TYPE_TAG_INT32:
        *c_long_out = arg_in->v_int32;
        return TRUE;
    case GI_TYPE_TAG_UINT32:
    case GI_TYPE_TAG_UNICHAR:
        *c_long_out = arg_in->v_uint32;
        return TRUE;
    case GI_TYPE_TAG_INT64:
        if (arg_in->v_int64 > G_MAXLONG || arg_in->v_int64 < G_MINLONG) {
            PyErr_Format (PyExc_TypeError, "Unable to marshal %s to C long",
                          gi_type_tag_to_string (type_tag));
            return FALSE;
        }
        *c_long_out = (glong)arg_in->v_int64;
        return TRUE;
    case GI_TYPE_TAG_UINT64:
        if (arg_in->v_uint64 > G_MAXLONG) {
            PyErr_Format (PyExc_TypeError, "Unable to marshal %s to C long",
                          gi_type_tag_to_string (type_tag));
            return FALSE;
        }
        *c_long_out = (glong)arg_in->v_uint64;
        return TRUE;
    case GI_TYPE_TAG_VOID:
    case GI_TYPE_TAG_BOOLEAN:
    case GI_TYPE_TAG_FLOAT:
    case GI_TYPE_TAG_DOUBLE:
    case GI_TYPE_TAG_GTYPE:
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_ARRAY:
    case GI_TYPE_TAG_INTERFACE:
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
    case GI_TYPE_TAG_GHASH:
    case GI_TYPE_TAG_ERROR:
        PyErr_Format (PyExc_TypeError, "Unable to marshal %s to C long",
                      gi_type_tag_to_string (type_tag));
        return FALSE;
    default:
        g_assert_not_reached ();
    }
}

static gboolean
_pygi_marshal_from_py_interface_enum (PyGIInvokeState *state,
                                      PyGICallableCache *callable_cache,
                                      PyGIArgCache *arg_cache,
                                      PyObject *py_arg, GIArgument *arg,
                                      gpointer *cleanup_data)
{
    *arg = pygi_argument_interface_from_py (py_arg, arg_cache->type_info,
                                            arg_cache->transfer);
    return !PyErr_Occurred ();
}

static gboolean
_pygi_marshal_from_py_interface_flags (PyGIInvokeState *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache *arg_cache,
                                       PyObject *py_arg, GIArgument *arg,
                                       gpointer *cleanup_data)
{
    *arg = pygi_argument_interface_from_py (py_arg, arg_cache->type_info,
                                            arg_cache->transfer);
    return !PyErr_Occurred ();
}

static PyObject *
_pygi_marshal_to_py_interface_enum (PyGIInvokeState *state,
                                    PyGICallableCache *callable_cache,
                                    PyGIArgCache *arg_cache, GIArgument *arg,
                                    gpointer *cleanup_data)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIBaseInfo *interface;
    long c_long;

    interface = gi_type_info_get_interface (arg_cache->type_info);
    g_assert (GI_IS_ENUM_INFO (interface) && !GI_IS_FLAGS_INFO (interface));

    if (!gi_argument_to_c_long (
            arg, &c_long,
            gi_enum_info_get_storage_type ((GIEnumInfo *)interface))) {
        return NULL;
    }
    gi_base_info_unref (interface);

    return pyg_enum_val_new (iface_cache->py_type, c_long);
}

static PyObject *
_pygi_marshal_to_py_interface_flags (PyGIInvokeState *state,
                                     PyGICallableCache *callable_cache,
                                     PyGIArgCache *arg_cache, GIArgument *arg,
                                     gpointer *cleanup_data)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIBaseInfo *interface;
    long c_long;

    interface = gi_type_info_get_interface (arg_cache->type_info);
    g_assert (GI_IS_FLAGS_INFO (interface));

    if (!gi_argument_to_c_long (
            arg, &c_long,
            gi_enum_info_get_storage_type ((GIEnumInfo *)interface))) {
        gi_base_info_unref (interface);
        return NULL;
    }
    gi_base_info_unref (interface);

    return pyg_flags_val_new (iface_cache->py_type, (guint)c_long);
}

PyGIArgCache *
pygi_arg_enum_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                             GITransfer transfer, PyGIDirection direction,
                             GIEnumInfo *iface_info)
{
    PyGIArgCache *arg_cache = NULL;

    arg_cache = pygi_arg_interface_new_from_info (
        type_info, arg_info, transfer, direction,
        GI_REGISTERED_TYPE_INFO (iface_info));
    if (arg_cache == NULL) return NULL;

    if (direction & PYGI_DIRECTION_FROM_PYTHON)
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_interface_enum;

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_interface_enum;

    return arg_cache;
}


PyGIArgCache *
pygi_arg_flags_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                              GITransfer transfer, PyGIDirection direction,
                              GIFlagsInfo *iface_info)
{
    PyGIArgCache *arg_cache = NULL;

    arg_cache = pygi_arg_interface_new_from_info (
        type_info, arg_info, transfer, direction,
        GI_REGISTERED_TYPE_INFO (iface_info));
    if (arg_cache == NULL) return NULL;

    if (direction & PYGI_DIRECTION_FROM_PYTHON)
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_interface_flags;

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_interface_flags;

    return arg_cache;
}

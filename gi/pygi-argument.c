/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   pygi-argument.c: GIArgument - PyObject conversion functions.
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

#include <string.h>
#include <time.h>

#include "pygenum.h"
#include "pygflags.h"

#include "pygi-type.h"

#include "pygi-argument.h"
#include "pygi-basictype.h"
#include "pygi-boxed.h"
#include "pygi-error.h"
#include "pygi-foreign.h"
#include "pygi-info.h"
#include "pygi-util.h"
#include "pygi-value.h"
#include "pygi-cache-private.h"

GIArgument
pygi_argument_from_py (GITypeInfo *type_info, PyObject *object,
                       GITransfer transfer,
                       PyGIArgumentFromPyCleanupData *arg_cleanup)
{
    GIArgument arg = PYGI_ARG_INIT;

    PyGIArgCache *cache = pygi_arg_cache_new (
        type_info, /*arg_info=*/NULL, transfer, PYGI_DIRECTION_FROM_PYTHON,
        /*callable_cache=*/NULL, 0, 0);

    if (!cache->from_py_marshaller (&arg_cleanup->state,
                                    /*callable_cache=*/NULL, cache, object,
                                    &arg, &arg_cleanup->cleanup_data)) {
        if (!PyErr_Occurred ())
            PyErr_Format (PyExc_TypeError,
                          "Unable to convert argument %R to its C equivalent",
                          object);
    }

    arg_cleanup->cache = cache;
    arg_cleanup->object = object;

    return arg;
}

void
pygi_argument_from_py_cleanup (PyGIArgumentFromPyCleanupData *arg_cleanup)
{
    PyGIArgCache *cache = (PyGIArgCache *)arg_cleanup->cache;

    if (cache) {
        if (cache->from_py_cleanup != NULL
            //&& arg_cleanup->cleanup_data.destroy != NULL
            && arg_cleanup->cleanup_data.data != NULL)
            cache->from_py_cleanup (&arg_cleanup->state, cache,
                                    arg_cleanup->object,
                                    arg_cleanup->cleanup_data, TRUE);
        pygi_arg_cache_free (cache);
        memset (arg_cleanup, 0, sizeof (PyGIArgumentFromPyCleanupData));
    }
}

PyObject *
pygi_argument_to_py (GITypeInfo *type_info, GIArgument arg,
                     GITransfer transfer)
{
    PyGIInvokeState state = { 0 };
    MarshalCleanupData cleanup_data = { 0 };
    PyObject *object;

    PyGIArgCache *cache = pygi_arg_cache_new (
        type_info, /*arg_info=*/NULL, transfer, PYGI_DIRECTION_TO_PYTHON,
        /*callable_cache=*/NULL, 0, 0);

    object = cache->to_py_marshaller (&state, /*callable_cache=*/NULL, cache,
                                      &arg, &cleanup_data);

    if (cache->to_py_cleanup
        //&& cleanup_data.destroy != NULL
        && cleanup_data.data != NULL)
        cache->to_py_cleanup (&state, cache, cleanup_data, arg.v_pointer,
                              TRUE);

    pygi_arg_cache_free (cache);

    return object;
}

PyObject *
pygi_argument_to_py_with_array_length (GITypeInfo *type_info, GIArgument arg,
                                       GITransfer transfer, gsize array_length)
{
    PyGIInvokeState state = { 0 };
    PyGIInvokeArgState arg_state = { 0 };
    MarshalCleanupData cleanup_data = { 0 };
    PyObject *object;

    PyGIArgCache *cache = pygi_arg_garray_new_from_info (
        type_info, /*arg_info=*/NULL, transfer, PYGI_DIRECTION_TO_PYTHON,
        /*callable_cache=*/NULL, 0, NULL);
    arg_state.arg_value = (GIArgument){ .v_size = array_length };
    state.args = &arg_state;
    ((PyGIArgGArray *)cache)->has_len_arg = TRUE;

    object = cache->to_py_marshaller (&state, /*callable_cache=*/NULL, cache,
                                      &arg, &cleanup_data);

    if (cache->to_py_cleanup && arg.v_pointer)
        cache->to_py_cleanup (&state, cache, cleanup_data, arg.v_pointer,
                              TRUE);

    pygi_arg_cache_free (cache);

    return object;
}

gboolean
pygi_argument_to_gsize (GIArgument arg, GITypeTag type_tag, gsize *gsize_out)
{
    switch (type_tag) {
    case GI_TYPE_TAG_INT8:
        *gsize_out = arg.v_int8;
        return TRUE;
    case GI_TYPE_TAG_UINT8:
        *gsize_out = arg.v_uint8;
        return TRUE;
    case GI_TYPE_TAG_INT16:
        *gsize_out = arg.v_int16;
        return TRUE;
    case GI_TYPE_TAG_UINT16:
        *gsize_out = arg.v_uint16;
        return TRUE;
    case GI_TYPE_TAG_INT32:
        *gsize_out = arg.v_int32;
        return TRUE;
    case GI_TYPE_TAG_UINT32:
    case GI_TYPE_TAG_UNICHAR:
        *gsize_out = arg.v_uint32;
        return TRUE;
    case GI_TYPE_TAG_INT64:
        if (arg.v_uint64 > G_MAXSIZE) {
            PyErr_Format (PyExc_TypeError, "Unable to marshal %s to gsize",
                          gi_type_tag_to_string (type_tag));
            return FALSE;
        }
        *gsize_out = (gsize)arg.v_int64;
        return TRUE;
    case GI_TYPE_TAG_UINT64:
        if (arg.v_uint64 > G_MAXSIZE) {
            PyErr_Format (PyExc_TypeError, "Unable to marshal %s to gsize",
                          gi_type_tag_to_string (type_tag));
            return FALSE;
        }
        *gsize_out = (gsize)arg.v_uint64;
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
        PyErr_Format (PyExc_TypeError, "Unable to marshal %s to gsize",
                      gi_type_tag_to_string (type_tag));
        return FALSE;
    default:
        g_assert_not_reached ();
    }
}

static GITypeTag
_pygi_get_storage_type (GITypeInfo *type_info)
{
    GITypeTag type_tag = gi_type_info_get_tag (type_info);

    if (type_tag == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *interface = gi_type_info_get_interface (type_info);
        if (GI_IS_ENUM_INFO (interface))
            type_tag = gi_enum_info_get_storage_type ((GIEnumInfo *)interface);
        /* FIXME: we might have something to do for other types */
        gi_base_info_unref (interface);
    }
    return type_tag;
}

void
_pygi_hash_pointer_to_arg_in_place (GIArgument *arg, GITypeInfo *type_info)
{
    GITypeTag type_tag = _pygi_get_storage_type (type_info);

    switch (type_tag) {
    case GI_TYPE_TAG_INT8:
        arg->v_int8 = (gint8)GPOINTER_TO_INT (arg->v_pointer);
        break;
    case GI_TYPE_TAG_INT16:
        arg->v_int16 = (gint16)GPOINTER_TO_INT (arg->v_pointer);
        break;
    case GI_TYPE_TAG_INT32:
        arg->v_int32 = (gint32)GPOINTER_TO_INT (arg->v_pointer);
        break;
    case GI_TYPE_TAG_INT64:
        arg->v_int64 = (gint64)GPOINTER_TO_INT (arg->v_pointer);
        break;
    case GI_TYPE_TAG_UINT8:
        arg->v_uint8 = (guint8)GPOINTER_TO_UINT (arg->v_pointer);
        break;
    case GI_TYPE_TAG_UINT16:
        arg->v_uint16 = (guint16)GPOINTER_TO_UINT (arg->v_pointer);
        break;
    case GI_TYPE_TAG_UINT32:
    case GI_TYPE_TAG_UNICHAR:
        arg->v_uint32 = (guint32)GPOINTER_TO_UINT (arg->v_pointer);
        break;
    case GI_TYPE_TAG_UINT64:
        arg->v_uint64 = (guint64)GPOINTER_TO_UINT (arg->v_pointer);
        break;
    case GI_TYPE_TAG_GTYPE:
        arg->v_size = GPOINTER_TO_SIZE (arg->v_pointer);
        break;
    case GI_TYPE_TAG_VOID:
    case GI_TYPE_TAG_BOOLEAN:
    case GI_TYPE_TAG_FLOAT:
    case GI_TYPE_TAG_DOUBLE:
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_INTERFACE:
    case GI_TYPE_TAG_ARRAY:
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
    case GI_TYPE_TAG_GHASH:
    case GI_TYPE_TAG_ERROR:
        break;
    default:
        g_assert_not_reached ();
    }
}

gpointer
_pygi_arg_to_hash_pointer (const GIArgument arg, GITypeInfo *type_info)
{
    GITypeTag type_tag = _pygi_get_storage_type (type_info);

    switch (type_tag) {
    case GI_TYPE_TAG_INT8:
        return GINT_TO_POINTER (arg.v_int8);
    case GI_TYPE_TAG_UINT8:
        return GUINT_TO_POINTER (arg.v_uint8);
    case GI_TYPE_TAG_INT16:
        return GINT_TO_POINTER (arg.v_int16);
    case GI_TYPE_TAG_UINT16:
        return GUINT_TO_POINTER (arg.v_uint16);
    case GI_TYPE_TAG_INT32:
        return GINT_TO_POINTER (arg.v_int32);
    case GI_TYPE_TAG_UINT32:
    case GI_TYPE_TAG_UNICHAR:
        return GUINT_TO_POINTER (arg.v_uint32);
    case GI_TYPE_TAG_INT64:
        return GINT_TO_POINTER (arg.v_int64);
    case GI_TYPE_TAG_UINT64:
        return GUINT_TO_POINTER (arg.v_uint64);
    case GI_TYPE_TAG_GTYPE:
        return GSIZE_TO_POINTER (arg.v_size);
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_INTERFACE:
    case GI_TYPE_TAG_ARRAY:
        return arg.v_pointer;
    case GI_TYPE_TAG_VOID:
    case GI_TYPE_TAG_BOOLEAN:
    case GI_TYPE_TAG_FLOAT:
    case GI_TYPE_TAG_DOUBLE:
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
    case GI_TYPE_TAG_GHASH:
    case GI_TYPE_TAG_ERROR:
        g_critical ("Unsupported type %s", gi_type_tag_to_string (type_tag));
        return arg.v_pointer;
    default:
        g_assert_not_reached ();
    }
}

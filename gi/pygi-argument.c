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
#include "pygi-object.h"
#include "pygi-struct-marshal.h"
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
            && arg_cleanup->cleanup_data != NULL)
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
    gpointer cleanup_data = NULL;
    PyObject *object;

    PyGIArgCache *cache = pygi_arg_cache_new (
        type_info, /*arg_info=*/NULL, transfer, PYGI_DIRECTION_TO_PYTHON,
        /*callable_cache=*/NULL, 0, 0);

    object = cache->to_py_marshaller (&state, /*callable_cache=*/NULL, cache,
                                      &arg, &cleanup_data);

    if (cache->to_py_cleanup && arg.v_pointer)
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
    gpointer cleanup_data = NULL;
    PyObject *object;

    // PyGIArgCache *cache = pygi_arg_cache_new (
    //     type_info, /*arg_info=*/NULL, transfer, PYGI_DIRECTION_TO_PYTHON,
    //     /*callable_cache=*/NULL, 0, 0);
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

static GIArgument
pygi_argument_array_from_object (PyObject *object, GITypeInfo *type_info,
                                 GITransfer transfer)
{
    GIArgument arg = PYGI_ARG_INIT;
    Py_ssize_t py_length;
    guint length, i;
    gboolean is_zero_terminated;
    GITypeInfo *item_type_info;
    gsize item_size;
    GArray *array;
    GITransfer item_transfer;

    if (Py_IsNone (object)) return arg;


    /* Note, strings are sequences, but we cannot accept them here */
    if (!PySequence_Check (object) || PyUnicode_Check (object)) {
        PyErr_SetString (PyExc_TypeError, "expected sequence");
        return arg;
    }

    py_length = PySequence_Length (object);
    if (py_length < 0) return arg;

    if (!pygi_guint_from_pyssize (py_length, &length)) return arg;

    is_zero_terminated = gi_type_info_is_zero_terminated (type_info);
    item_type_info = gi_type_info_get_param_type (type_info, 0);

    /* we handle arrays that are really strings specially, see below */
    if (gi_type_info_get_tag (item_type_info) == GI_TYPE_TAG_UINT8)
        item_size = 1;
    else
        item_size = sizeof (GIArgument);

    array = g_array_sized_new (is_zero_terminated, FALSE, (guint)item_size,
                               length);
    if (array == NULL) {
        gi_base_info_unref ((GIBaseInfo *)item_type_info);
        PyErr_NoMemory ();
        return arg;
    }

    if (gi_type_info_get_tag (item_type_info) == GI_TYPE_TAG_UINT8
        && PyBytes_Check (object)) {
        memcpy (array->data, PyBytes_AsString (object), length);
        array->len = length;
    } else {
        item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING
                                                          : transfer;

        for (i = 0; i < length; i++) {
            PyObject *py_item;
            GIArgument item;

            py_item = PySequence_GetItem (object, i);
            if (py_item == NULL) {
                goto array_item_error;
            }

            item = _pygi_argument_from_object (py_item, item_type_info,
                                               item_transfer);

            Py_DECREF (py_item);

            if (PyErr_Occurred ()) {
                goto array_item_error;
            }

            g_array_insert_val (array, i, item);
        }
    }

    arg.v_pointer = array;

    gi_base_info_unref ((GIBaseInfo *)item_type_info);
    return arg;

array_item_error:
    gi_base_info_unref ((GIBaseInfo *)item_type_info);

    /* Free everything we have converted so far. */
    _pygi_argument_release ((GIArgument *)&array, type_info,
                            GI_TRANSFER_NOTHING, GI_DIRECTION_IN);

    _PyGI_ERROR_PREFIX ("Item %u: ", i);
    return arg;
}

static GIArgument
pygi_argument_interface_from_object (PyObject *object, GITypeInfo *type_info,
                                     GITransfer transfer)
{
    GIArgument arg = PYGI_ARG_INIT;
    GIBaseInfo *info;

    info = gi_type_info_get_interface (type_info);

    switch (pygi_interface_type_tag (info)) {
    case PYGI_INTERFACE_TYPE_TAG_CALLBACK:
        PyErr_SetString (PyExc_TypeError,
                         "Cannot translate Python object to callback type");
        break;
    case PYGI_INTERFACE_TYPE_TAG_STRUCT:
    case PYGI_INTERFACE_TYPE_TAG_UNION: {
        GType g_type;
        PyObject *py_type;
        gboolean is_foreign =
            (GI_IS_STRUCT_INFO (info))
            && (gi_struct_info_is_foreign ((GIStructInfo *)info));

        g_type =
            gi_registered_type_info_get_g_type ((GIRegisteredTypeInfo *)info);
        py_type = pygi_type_import_by_gi_info ((GIBaseInfo *)info);

        /* Note for G_TYPE_VALUE g_type:
         * This will currently leak the GValue that is allocated and
         * stashed in arg.v_pointer. Out argument marshaling for caller
         * allocated GValues already pass in memory for the GValue.
         * Further re-factoring is needed to fix this leak.
         * See: https://bugzilla.gnome.org/show_bug.cgi?id=693405
         */
        pygi_arg_struct_from_py_marshal (
            object, &arg, NULL, /*arg_name*/
            GI_REGISTERED_TYPE_INFO (info), g_type, py_type, transfer,
            FALSE,              /*copy_reference*/
            is_foreign, gi_type_info_is_pointer (type_info));

        Py_DECREF (py_type);
        break;
    }
    case PYGI_INTERFACE_TYPE_TAG_FLAGS: {
        GType g_type;

        g_type =
            gi_registered_type_info_get_g_type ((GIRegisteredTypeInfo *)info);
        if (pyg_flags_get_value (g_type, object, &arg.v_uint) < 0) return arg;
        break;
    }
    case PYGI_INTERFACE_TYPE_TAG_ENUM: {
        GType g_type;

        g_type =
            gi_registered_type_info_get_g_type ((GIRegisteredTypeInfo *)info);
        if (pyg_enum_get_value (g_type, object, &arg.v_int) < 0) return arg;
        break;
    }
    case PYGI_INTERFACE_TYPE_TAG_OBJECT:
    case PYGI_INTERFACE_TYPE_TAG_INTERFACE:
        /* An error within this call will result in a NULL arg */
        pygi_marshal_from_py_object (object, &arg, transfer);
        break;
    default:
        g_assert_not_reached ();
    }
    gi_base_info_unref (info);
    return arg;
}

static GIArgument
pygi_argument_list_from_object (PyObject *object, GITypeInfo *type_info,
                                GITransfer transfer)
{
    GIArgument arg = PYGI_ARG_INIT;
    GITypeTag type_tag = gi_type_info_get_tag (type_info);
    Py_ssize_t length;
    GITypeInfo *item_type_info;
    GSList *list = NULL;
    GITransfer item_transfer;
    Py_ssize_t i;

    if (Py_IsNone (object)) return arg;

    length = PySequence_Length (object);
    if (length < 0) return arg;

    item_type_info = gi_type_info_get_param_type (type_info, 0);
    g_assert (item_type_info != NULL);

    item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING
                                                      : transfer;

    for (i = length - 1; i >= 0; i--) {
        PyObject *py_item;
        GIArgument item;

        py_item = PySequence_GetItem (object, i);
        if (py_item == NULL) {
            goto list_item_error;
        }

        item = _pygi_argument_from_object (py_item, item_type_info,
                                           item_transfer);

        Py_DECREF (py_item);

        if (PyErr_Occurred ()) {
            goto list_item_error;
        }

        if (type_tag == GI_TYPE_TAG_GLIST) {
            list = (GSList *)g_list_prepend ((GList *)list, item.v_pointer);
        } else {
            list = g_slist_prepend (list, item.v_pointer);
        }
    }

    arg.v_pointer = list;

    gi_base_info_unref ((GIBaseInfo *)item_type_info);

    return arg;

list_item_error:
    gi_base_info_unref ((GIBaseInfo *)item_type_info);
    /* Free everything we have converted so far. */
    _pygi_argument_release ((GIArgument *)&list, type_info,
                            GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
    _PyGI_ERROR_PREFIX ("Item %zd: ", i);
    return arg;
}

static GIArgument
pygi_argument_hash_table_from_object (PyObject *object, GITypeInfo *type_info,
                                      GITransfer transfer)
{
    GIArgument arg = PYGI_ARG_INIT;
    Py_ssize_t length;
    PyObject *keys;
    PyObject *values;
    GITypeInfo *key_type_info;
    GITypeInfo *value_type_info;
    GITypeTag key_type_tag;
    GHashFunc hash_func;
    GEqualFunc equal_func;
    GHashTable *hash_table;
    GITransfer item_transfer;
    Py_ssize_t i;

    if (Py_IsNone (object)) return arg;

    length = PyMapping_Length (object);
    if (length < 0) return arg;

    keys = PyMapping_Keys (object);
    if (keys == NULL) return arg;

    values = PyMapping_Values (object);
    if (values == NULL) {
        Py_DECREF (keys);
        return arg;
    }

    key_type_info = gi_type_info_get_param_type (type_info, 0);
    g_assert (key_type_info != NULL);

    value_type_info = gi_type_info_get_param_type (type_info, 1);
    g_assert (value_type_info != NULL);

    key_type_tag = gi_type_info_get_tag (key_type_info);

    if (key_type_tag == GI_TYPE_TAG_UTF8
        || key_type_tag == GI_TYPE_TAG_FILENAME) {
        hash_func = g_str_hash;
        equal_func = g_str_equal;
    } else {
        hash_func = NULL;
        equal_func = NULL;
    }

    hash_table = g_hash_table_new (hash_func, equal_func);
    if (hash_table == NULL) {
        PyErr_NoMemory ();
        goto hash_table_release;
    }

    item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING
                                                      : transfer;

    for (i = 0; i < length; i++) {
        PyObject *py_key;
        PyObject *py_value;
        GIArgument key;
        GIArgument value;

        py_key = PyList_GET_ITEM (keys, i);
        py_value = PyList_GET_ITEM (values, i);

        key =
            _pygi_argument_from_object (py_key, key_type_info, item_transfer);
        if (PyErr_Occurred ()) {
            goto hash_table_item_error;
        }

        value = _pygi_argument_from_object (py_value, value_type_info,
                                            item_transfer);
        if (PyErr_Occurred ()) {
            _pygi_argument_release (&key, type_info, GI_TRANSFER_NOTHING,
                                    GI_DIRECTION_IN);
            goto hash_table_item_error;
        }

        g_hash_table_insert (
            hash_table, key.v_pointer,
            _pygi_arg_to_hash_pointer (value, value_type_info));
    }

    arg.v_pointer = hash_table;

hash_table_release:
    gi_base_info_unref ((GIBaseInfo *)key_type_info);
    gi_base_info_unref ((GIBaseInfo *)value_type_info);
    Py_DECREF (keys);
    Py_DECREF (values);
    return arg;

hash_table_item_error:
    /* Free everything we have converted so far. */
    _pygi_argument_release ((GIArgument *)&hash_table, type_info,
                            GI_TRANSFER_NOTHING, GI_DIRECTION_IN);

    _PyGI_ERROR_PREFIX ("Item %zd: ", i);
    return arg;
}

GIArgument
_pygi_argument_from_object (PyObject *object, GITypeInfo *type_info,
                            GITransfer transfer)
{
    GIArgument arg = PYGI_ARG_INIT;
    GITypeTag type_tag;
    gpointer cleanup_data = NULL;

    type_tag = gi_type_info_get_tag (type_info);

    switch (type_tag) {
    case GI_TYPE_TAG_ARRAY:
        arg = pygi_argument_array_from_object (object, type_info, transfer);
        break;
    case GI_TYPE_TAG_INTERFACE:
        arg =
            pygi_argument_interface_from_object (object, type_info, transfer);
        break;
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
        arg = pygi_argument_list_from_object (object, type_info, transfer);
        break;
    case GI_TYPE_TAG_GHASH:
        arg =
            pygi_argument_hash_table_from_object (object, type_info, transfer);
        break;
    case GI_TYPE_TAG_ERROR:
        PyErr_SetString (PyExc_NotImplementedError,
                         "error marshalling is not supported yet");
        /* TODO */
        break;
    case GI_TYPE_TAG_VOID:
    case GI_TYPE_TAG_INT8:
    case GI_TYPE_TAG_UINT8:
    case GI_TYPE_TAG_INT16:
    case GI_TYPE_TAG_UINT16:
    case GI_TYPE_TAG_INT32:
    case GI_TYPE_TAG_UINT32:
    case GI_TYPE_TAG_GTYPE:
    case GI_TYPE_TAG_BOOLEAN:
    case GI_TYPE_TAG_INT64:
    case GI_TYPE_TAG_UINT64:
    case GI_TYPE_TAG_FLOAT:
    case GI_TYPE_TAG_DOUBLE:
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_UNICHAR:
        /* Ignores cleanup data for now. */
        arg = pygi_marshal_from_py_basic_type (object, type_tag, transfer,
                                               &cleanup_data);
        break;
    default:
        g_assert_not_reached ();
    }

    return arg;
}

void
_pygi_argument_release (GIArgument *arg, GITypeInfo *type_info,
                        GITransfer transfer, GIDirection direction)
{
    GITypeTag type_tag;
    gboolean is_out =
        (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT);

    type_tag = gi_type_info_get_tag (type_info);

    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
        /* Don't do anything, it's transparent to the C side */
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
    case GI_TYPE_TAG_GTYPE:
    case GI_TYPE_TAG_UNICHAR:
        break;
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_UTF8:
        /* With allow-none support the string could be NULL */
        if ((arg->v_string != NULL
             && (direction == GI_DIRECTION_IN
                 && transfer == GI_TRANSFER_NOTHING))
            || (direction == GI_DIRECTION_OUT
                && transfer == GI_TRANSFER_EVERYTHING)) {
            g_free (arg->v_string);
        }
        break;
    case GI_TYPE_TAG_ARRAY: {
        GArray *array;
        gsize i;

        if (arg->v_pointer == NULL) {
            return;
        }

        array = arg->v_pointer;

        // TODO: simplify this to switch array types and call g_array_unref.

        if ((direction == GI_DIRECTION_IN
             && transfer != GI_TRANSFER_EVERYTHING)
            || (direction == GI_DIRECTION_OUT
                && transfer == GI_TRANSFER_EVERYTHING)) {
            GITypeInfo *item_type_info;
            GITransfer item_transfer;

            item_type_info = gi_type_info_get_param_type (type_info, 0);

            item_transfer = direction == GI_DIRECTION_IN
                                ? GI_TRANSFER_NOTHING
                                : GI_TRANSFER_EVERYTHING;

            /* Free the items */
            for (i = 0; i < array->len; i++) {
                GIArgument item;
                memcpy (&item,
                        array->data + (g_array_get_element_size (array) * i),
                        sizeof (GIArgument));
                _pygi_argument_release (&item, item_type_info, item_transfer,
                                        direction);
            }

            gi_base_info_unref ((GIBaseInfo *)item_type_info);
        }

        if ((direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
            || (direction == GI_DIRECTION_OUT
                && transfer != GI_TRANSFER_NOTHING)) {
            g_array_free (array, TRUE);
        }

        break;
    }
    case GI_TYPE_TAG_INTERFACE: {
        GIBaseInfo *info;

        info = gi_type_info_get_interface (type_info);

        switch (pygi_interface_type_tag (info)) {
        case PYGI_INTERFACE_TYPE_TAG_CALLBACK:
            /* TODO */
            break;
        case PYGI_INTERFACE_TYPE_TAG_STRUCT:
        case PYGI_INTERFACE_TYPE_TAG_UNION: {
            GType type;

            if (arg->v_pointer == NULL) {
                return;
            }

            type = gi_registered_type_info_get_g_type (
                (GIRegisteredTypeInfo *)info);

            if (g_type_is_a (type, G_TYPE_VALUE)) {
                GValue *value;

                value = arg->v_pointer;

                if ((direction == GI_DIRECTION_IN
                     && transfer != GI_TRANSFER_EVERYTHING)
                    || (direction == GI_DIRECTION_OUT
                        && transfer == GI_TRANSFER_EVERYTHING)) {
                    g_value_unset (value);
                }

                if ((direction == GI_DIRECTION_IN
                     && transfer == GI_TRANSFER_NOTHING)
                    || (direction == GI_DIRECTION_OUT
                        && transfer != GI_TRANSFER_NOTHING)) {
                    g_slice_free (GValue, value);
                }
            } else if (g_type_is_a (type, G_TYPE_CLOSURE)) {
                if (direction == GI_DIRECTION_IN
                    && transfer == GI_TRANSFER_NOTHING) {
                    g_closure_unref (arg->v_pointer);
                }
            } else if (GI_IS_STRUCT_INFO (info)
                       && gi_struct_info_is_foreign ((GIStructInfo *)info)) {
                if (direction == GI_DIRECTION_OUT
                    && transfer == GI_TRANSFER_EVERYTHING) {
                    pygi_struct_foreign_release (GI_BASE_INFO (info),
                                                 arg->v_pointer);
                }
            } else if (g_type_is_a (type, G_TYPE_BOXED)) {
            } else if (g_type_is_a (type, G_TYPE_POINTER)
                       || type == G_TYPE_NONE) {
                g_warn_if_fail (!gi_type_info_is_pointer (type_info)
                                || transfer == GI_TRANSFER_NOTHING);
            }
        }
        case PYGI_INTERFACE_TYPE_TAG_FLAGS:
        case PYGI_INTERFACE_TYPE_TAG_ENUM:
            /* nothing */
            break;
        case PYGI_INTERFACE_TYPE_TAG_OBJECT:
        case PYGI_INTERFACE_TYPE_TAG_INTERFACE:
            if (arg->v_pointer == NULL) {
                return;
            }
            if (is_out && transfer == GI_TRANSFER_EVERYTHING) {
                g_object_unref (arg->v_pointer);
            }
            break;
        default:
            g_assert_not_reached ();
        }

        gi_base_info_unref (info);
        break;
    }
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST: {
        GSList *list;

        if (arg->v_pointer == NULL) {
            return;
        }

        list = arg->v_pointer;

        // TODO: simplify to g_(s)list_free_full

        if ((direction == GI_DIRECTION_IN
             && transfer != GI_TRANSFER_EVERYTHING)
            || (direction == GI_DIRECTION_OUT
                && transfer == GI_TRANSFER_EVERYTHING)) {
            GITypeInfo *item_type_info;
            GITransfer item_transfer;
            GSList *item;

            item_type_info = gi_type_info_get_param_type (type_info, 0);
            g_assert (item_type_info != NULL);

            item_transfer = direction == GI_DIRECTION_IN
                                ? GI_TRANSFER_NOTHING
                                : GI_TRANSFER_EVERYTHING;

            /* Free the items */
            for (item = list; item != NULL; item = g_slist_next (item)) {
                _pygi_argument_release ((GIArgument *)&item->data,
                                        item_type_info, item_transfer,
                                        direction);
            }

            gi_base_info_unref ((GIBaseInfo *)item_type_info);
        }

        if ((direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
            || (direction == GI_DIRECTION_OUT
                && transfer != GI_TRANSFER_NOTHING)) {
            if (type_tag == GI_TYPE_TAG_GLIST) {
                g_list_free ((GList *)list);
            } else {
                /* type_tag == GI_TYPE_TAG_GSLIST */
                g_slist_free (list);
            }
        }

        break;
    }
    case GI_TYPE_TAG_GHASH: {
        GHashTable *hash_table;

        if (arg->v_pointer == NULL) {
            return;
        }

        hash_table = arg->v_pointer;

        // TODO: simplify to g_hash_table_unref.

        if (direction == GI_DIRECTION_IN
            && transfer != GI_TRANSFER_EVERYTHING) {
            /* We created the table without a destroy function, so keys and
                 * values need to be released. */
            GITypeInfo *key_type_info;
            GITypeInfo *value_type_info;
            GITransfer item_transfer;
            GHashTableIter hash_table_iter;
            gpointer key;
            gpointer value;

            key_type_info = gi_type_info_get_param_type (type_info, 0);
            g_assert (key_type_info != NULL);

            value_type_info = gi_type_info_get_param_type (type_info, 1);
            g_assert (value_type_info != NULL);

            if (direction == GI_DIRECTION_IN) {
                item_transfer = GI_TRANSFER_NOTHING;
            } else {
                item_transfer = GI_TRANSFER_EVERYTHING;
            }

            g_hash_table_iter_init (&hash_table_iter, hash_table);
            while (g_hash_table_iter_next (&hash_table_iter, &key, &value)) {
                _pygi_argument_release ((GIArgument *)&key, key_type_info,
                                        item_transfer, direction);
                _pygi_argument_release ((GIArgument *)&value, value_type_info,
                                        item_transfer, direction);
            }

            gi_base_info_unref ((GIBaseInfo *)key_type_info);
            gi_base_info_unref ((GIBaseInfo *)value_type_info);
        } else if (direction == GI_DIRECTION_OUT
                   && transfer == GI_TRANSFER_CONTAINER) {
            /* Be careful to avoid keys and values being freed if the
                 * callee gave a destroy function. */
            g_hash_table_steal_all (hash_table);
        }

        if ((direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
            || (direction == GI_DIRECTION_OUT
                && transfer != GI_TRANSFER_NOTHING)) {
            g_hash_table_unref (hash_table);
        }

        break;
    }
    case GI_TYPE_TAG_ERROR: {
        GError *error;

        if (arg->v_pointer == NULL) {
            return;
        }

        error = *(GError **)arg->v_pointer;

        if (error != NULL) {
            g_error_free (error);
        }

        g_slice_free (GError *, arg->v_pointer);
        break;
    }
    default:
        g_assert_not_reached ();
    }
}

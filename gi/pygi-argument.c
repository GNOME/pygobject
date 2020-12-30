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

gboolean
pygi_argument_to_gssize (GIArgument *arg_in, GITypeTag type_tag,
                         gssize *gssize_out)
{
    switch (type_tag) {
    case GI_TYPE_TAG_INT8:
        *gssize_out = arg_in->v_int8;
        return TRUE;
    case GI_TYPE_TAG_UINT8:
        *gssize_out = arg_in->v_uint8;
        return TRUE;
    case GI_TYPE_TAG_INT16:
        *gssize_out = arg_in->v_int16;
        return TRUE;
    case GI_TYPE_TAG_UINT16:
        *gssize_out = arg_in->v_uint16;
        return TRUE;
    case GI_TYPE_TAG_INT32:
        *gssize_out = arg_in->v_int32;
        return TRUE;
    case GI_TYPE_TAG_UINT32:
        *gssize_out = arg_in->v_uint32;
        return TRUE;
    case GI_TYPE_TAG_INT64:
        if (arg_in->v_int64 > G_MAXSSIZE || arg_in->v_int64 < G_MINSSIZE) {
            PyErr_Format (PyExc_TypeError, "Unable to marshal %s to gssize",
                          gi_type_tag_to_string (type_tag));
            return FALSE;
        }
        *gssize_out = (gssize)arg_in->v_int64;
        return TRUE;
    case GI_TYPE_TAG_UINT64:
        if (arg_in->v_uint64 > G_MAXSSIZE) {
            PyErr_Format (PyExc_TypeError, "Unable to marshal %s to gssize",
                          gi_type_tag_to_string (type_tag));
            return FALSE;
        }
        *gssize_out = (gssize)arg_in->v_uint64;
        return TRUE;
    default:
        PyErr_Format (PyExc_TypeError, "Unable to marshal %s to gssize",
                      gi_type_tag_to_string (type_tag));
        return FALSE;
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
        else
            /* FIXME: we might have something to do for other types */
            gi_base_info_unref (interface);
    }
    return type_tag;
}

void
_pygi_hash_pointer_to_arg (GIArgument *arg, GITypeInfo *type_info)
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
    case GI_TYPE_TAG_UINT8:
        arg->v_uint8 = (guint8)GPOINTER_TO_UINT (arg->v_pointer);
        break;
    case GI_TYPE_TAG_UINT16:
        arg->v_uint16 = (guint16)GPOINTER_TO_UINT (arg->v_pointer);
        break;
    case GI_TYPE_TAG_UINT32:
        arg->v_uint32 = (guint32)GPOINTER_TO_UINT (arg->v_pointer);
        break;
    case GI_TYPE_TAG_GTYPE:
        arg->v_size = GPOINTER_TO_SIZE (arg->v_pointer);
        break;
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_INTERFACE:
    case GI_TYPE_TAG_ARRAY:
        break;
    default:
        g_critical ("Unsupported type %s", gi_type_tag_to_string (type_tag));
    }
}

gpointer
_pygi_arg_to_hash_pointer (const GIArgument *arg, GITypeInfo *type_info)
{
    GITypeTag type_tag = _pygi_get_storage_type (type_info);

    switch (type_tag) {
    case GI_TYPE_TAG_INT8:
        return GINT_TO_POINTER (arg->v_int8);
    case GI_TYPE_TAG_UINT8:
        return GINT_TO_POINTER (arg->v_uint8);
    case GI_TYPE_TAG_INT16:
        return GINT_TO_POINTER (arg->v_int16);
    case GI_TYPE_TAG_UINT16:
        return GINT_TO_POINTER (arg->v_uint16);
    case GI_TYPE_TAG_INT32:
        return GINT_TO_POINTER (arg->v_int32);
    case GI_TYPE_TAG_UINT32:
        return GINT_TO_POINTER (arg->v_uint32);
    case GI_TYPE_TAG_GTYPE:
        return GSIZE_TO_POINTER (arg->v_size);
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_INTERFACE:
    case GI_TYPE_TAG_ARRAY:
        return arg->v_pointer;
    default:
        g_critical ("Unsupported type %s", gi_type_tag_to_string (type_tag));
        return arg->v_pointer;
    }
}

static void
_free_value_slice (GValue *value)
{
    g_value_unset (value);
    g_slice_free (GValue, value);
}

static void
_free_error_slice (GError **error)
{
    g_clear_error (error);
    g_slice_free (GError *, error);
}

static GDestroyNotify
_pygi_type_info_get_free_func (GITypeInfo *type_info, GITransfer transfer,
                               gboolean is_none)
{
    GITypeTag type_tag = gi_type_info_get_tag (type_info);

    if (is_none) return NULL;

    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
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
        return g_free;
        break;
    case GI_TYPE_TAG_ARRAY:
        switch (gi_type_info_get_array_type (type_info)) {
        case GI_ARRAY_TYPE_C:
            g_warning ("Cannot make a free_func for a C array");
            break;
        case GI_ARRAY_TYPE_ARRAY:
            return (GDestroyNotify)g_array_unref;
            break;
        case GI_ARRAY_TYPE_PTR_ARRAY:
            return (GDestroyNotify)g_ptr_array_unref;
            break;
        case GI_ARRAY_TYPE_BYTE_ARRAY:
            return (GDestroyNotify)g_byte_array_unref;
            break;
        default:
            g_critical ("_pygi_type_info_get_free_func:array - not reachable");
            break;
        }

        break;
    case GI_TYPE_TAG_INTERFACE: {
        GIBaseInfo *info;
        GDestroyNotify free_func = NULL;

        info = gi_type_info_get_interface (type_info);

        if (GI_IS_CALLBACK_INFO (info)) {
            /* TODO */
        } else if (GI_IS_STRUCT_INFO (info) || GI_IS_UNION_INFO (info)) {
            GType type = gi_registered_type_info_get_g_type (
                (GIRegisteredTypeInfo *)info);

            if (g_type_is_a (type, G_TYPE_VALUE)) {
                free_func = (GDestroyNotify)_free_value_slice;
            } else if (g_type_is_a (type, G_TYPE_CLOSURE)) {
                free_func = (GDestroyNotify)g_closure_unref;
            } else if (GI_IS_STRUCT_INFO (info)) {
                if (gi_struct_info_is_foreign ((GIStructInfo *)info)) {
                    g_warning ("Cannot make a free func for a foreign struct");
                }
            } else if (g_type_is_a (type, G_TYPE_BOXED)) {
                g_warning ("Cannot make a free func for a boxed type");
            } else if (g_type_is_a (type, G_TYPE_POINTER)
                       || type == G_TYPE_NONE) {
                if (gi_type_info_is_pointer (type_info)) {
                    g_warning ("Cannot make a free func for a pointer type");
                }
            }
        } else if (GI_IS_ENUM_INFO (info) || GI_IS_FLAGS_INFO (info)) {
            /* no-op */
        } else if (GI_IS_INTERFACE_INFO (info) || GI_IS_OBJECT_INFO (info)) {
            free_func = g_object_unref;
        } else {
            g_assert_not_reached ();
        }

        gi_base_info_unref (info);
        return free_func;
    }
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST: {
        GITypeInfo *item_type_info =
            gi_type_info_get_param_type (type_info, 0);
        GITransfer item_transfer = transfer == GI_TRANSFER_EVERYTHING
                                       ? GI_TRANSFER_EVERYTHING
                                       : GI_TRANSFER_NOTHING;
        GDestroyNotify item_free = _pygi_type_info_get_free_func (
            item_type_info, item_transfer, FALSE);

        gi_base_info_unref ((GIBaseInfo *)item_type_info);

        if (item_free != NULL) {
            g_warning ("Cannot make free_func to free items in GList");
        }

        return type_tag == GI_TYPE_TAG_GSLIST ? (GDestroyNotify)g_slist_free
                                              : (GDestroyNotify)g_list_free;
    }
    case GI_TYPE_TAG_GHASH: {
        GITypeInfo *key_type_info = gi_type_info_get_param_type (type_info, 0);
        GITypeInfo *value_type_info =
            gi_type_info_get_param_type (type_info, 1);
        GITransfer item_transfer = transfer == GI_TRANSFER_EVERYTHING
                                       ? GI_TRANSFER_EVERYTHING
                                       : GI_TRANSFER_NOTHING;
        GDestroyNotify key_free_func = _pygi_type_info_get_free_func (
            key_type_info, item_transfer, FALSE);
        GDestroyNotify value_free_func = _pygi_type_info_get_free_func (
            value_type_info, item_transfer, FALSE);

        gi_base_info_unref ((GIBaseInfo *)key_type_info);
        gi_base_info_unref ((GIBaseInfo *)value_type_info);

        if (key_free_func != NULL) {
            g_warning ("Cannot make free_func to free keys in GHashTable");
        }

        if (value_free_func != NULL) {
            g_warning ("Cannot make free_func to free values in GHashTable");
        }

        return (GDestroyNotify)g_hash_table_unref;
    }
    case GI_TYPE_TAG_ERROR:
        return (GDestroyNotify)_free_error_slice;
    default:
        break;
    }

    return NULL;
}

/**
 * _pygi_argument_array_length_marshal:
 * @length_arg_index: Index of length argument in the callables args list.
 * @user_data1: (type Array(GValue)): Array of GValue arguments to retrieve length
 * @user_data2: (type GICallableInfo): Callable info to get the argument from.
 *
 * Generic marshalling policy for array length arguments in callables.
 *
 * Returns: The length of the array or -1 on failure.
 */
gssize
_pygi_argument_array_length_marshal (gsize length_arg_index, void *user_data1,
                                     void *user_data2)
{
    GIArgInfo length_arg_info;
    GITypeInfo length_type_info;
    GIArgument length_arg;
    gssize array_len = -1;
    GValue *values = (GValue *)user_data1;
    GICallableInfo *callable_info = (GICallableInfo *)user_data2;

    gi_callable_info_load_arg (callable_info, (gint)length_arg_index,
                               &length_arg_info);
    gi_arg_info_load_type_info (&length_arg_info, &length_type_info);

    length_arg = _pygi_argument_from_g_value (&(values[length_arg_index]),
                                              &length_type_info);
    if (!pygi_argument_to_gssize (&length_arg,
                                  gi_type_info_get_tag (&length_type_info),
                                  &array_len)) {
        return -1;
    }

    return array_len;
}

static size_t
_pygi_measure_c_zero_terminated_array_length (GIArgument *arg,
                                              size_t item_size)
{
    size_t length = 0;

    if (item_size == sizeof (gpointer))
        length = g_strv_length ((gchar **)arg->v_pointer);
    else if (item_size == 1)
        length = strlen ((gchar *)arg->v_pointer);
    else if (item_size == sizeof (int))
        for (length = 0; *(((int *)arg->v_pointer) + length); length++);
    else if (item_size == sizeof (short))
        for (length = 0; *(((short *)arg->v_pointer) + length); length++);
    else
        g_assert_not_reached ();

    return length;
}

static gint
_pygi_determine_c_array_length (GIArgument *arg, GITypeInfo *type_info,
                                size_t item_size,
                                PyGIArgArrayLengthPolicy array_length_policy,
                                void *user_data1, void *user_data2,
                                size_t *return_length)
{
    gboolean is_zero_terminated = gi_type_info_is_zero_terminated (type_info);
    size_t length = 0;

    if (is_zero_terminated) {
        /* Array can be arbitrarily long. Best to store the size in a size_t */
        *return_length =
            _pygi_measure_c_zero_terminated_array_length (arg, item_size);
        return 0;
    } else {
        gboolean has_length;
        has_length = gi_type_info_get_array_fixed_size (type_info, &length);
        if (!has_length) {
            guint length_arg_pos;
            gboolean has_array_length;
            gssize length_by_policy;

            if (G_UNLIKELY (array_length_policy == NULL)) {
                return -1;
            }

            has_array_length = gi_type_info_get_array_length_index (
                type_info, &length_arg_pos);
            g_assert (has_array_length);

            length_by_policy =
                array_length_policy (length_arg_pos, user_data1, user_data2);
            if (length < 0) {
                return -1;
            }
            length = (size_t)length_by_policy;
        }
    }

    /* Getting the length through GI or PyGIArgArrayLengthPolicy
     * will be at most the size of a gint */
    *return_length = length;
    return 0;
}

/**
 * _pygi_argument_to_array
 * @arg: The argument to convert
 * @array_length_policy: Closure for marshalling the array length argument when needed.
 * @user_data1: Generic user data passed to the array_length_policy.
 * @user_data2: Generic user data passed to the array_length_policy.
 * @type_info: The type info for @arg
 * @out_free_array: A return location for a gboolean that indicates whether
 *                  or not the wrapped GArray should be freed
 *
 * Make sure an array type argument is wrapped in a GArray.
 *
 * Note: This method can *not* be folded into _pygi_argument_to_object() because
 * arrays are special in the sense that they might require access to @args in
 * order to get the length.
 *
 * Returns: A GArray wrapping @arg. If @out_free_array has been set to TRUE then
 *          free the array with g_array_free() without freeing the data members.
 *          Otherwise don't free the array.
 */
GArray *
_pygi_argument_to_array (GIArgument *arg,
                         PyGIArgArrayLengthPolicy array_length_policy,
                         void *user_data1, void *user_data2,
                         GITypeInfo *type_info, gboolean *out_free_array)
{
    GITypeInfo *item_type_info;
    gboolean is_zero_terminated;
    gsize item_size;
    size_t length;
    GArray *g_array;

    g_return_val_if_fail (
        gi_type_info_get_tag (type_info) == GI_TYPE_TAG_ARRAY, NULL);

    if (arg->v_pointer == NULL) {
        return NULL;
    }

    switch (gi_type_info_get_array_type (type_info)) {
    case GI_ARRAY_TYPE_C:
        is_zero_terminated = gi_type_info_is_zero_terminated (type_info);
        item_type_info = gi_type_info_get_param_type (type_info, 0);

        item_size = _pygi_gi_type_info_size (item_type_info);

        gi_base_info_unref ((GIBaseInfo *)item_type_info);

        if (_pygi_determine_c_array_length (arg, type_info, item_size,
                                            array_length_policy, user_data1,
                                            user_data2, &length)
            < 0) {
            g_critical ("Unable to determine array length for %p",
                        arg->v_pointer);
            g_array =
                g_array_new (is_zero_terminated, FALSE, (guint)item_size);
            *out_free_array = TRUE;
            return g_array;
        }

        g_array = g_array_new (is_zero_terminated, FALSE, (guint)item_size);

        g_free (g_array->data);
        g_array->data = arg->v_pointer;
        g_array->len = (guint)length;
        *out_free_array = TRUE;
        break;
    case GI_ARRAY_TYPE_ARRAY:
    case GI_ARRAY_TYPE_BYTE_ARRAY:
        /* Note: GByteArray is really just a GArray */
        g_array = arg->v_pointer;
        *out_free_array = FALSE;
        break;
    case GI_ARRAY_TYPE_PTR_ARRAY: {
        GPtrArray *ptr_array = (GPtrArray *)arg->v_pointer;
        /* Similar pattern to GI_ARRAY_TYPE_C - we create a new
         * array but free the segment and replace it with
         * what was in the ptr_array */
        g_array = g_array_new (FALSE, FALSE, sizeof (gpointer));
        g_free (g_array->data);

        g_array->data = (char *)ptr_array->pdata;
        g_array->len = ptr_array->len;
        *out_free_array = TRUE;
        break;
    }
    default:
        g_critical ("Unexpected array type %u",
                    gi_type_info_get_array_type (type_info));
        g_array = NULL;
        break;
    }

    return g_array;
}

typedef void (*ArrayInsertFunc) (gpointer array, guint index, GIArgument item,
                                 guint item_size);

static gint
_pygi_fill_array_from_object (gpointer array, GITypeInfo *item_type_info,
                              GITransfer transfer, guint length,
                              PyObject *object, ArrayInsertFunc insert_func)
{
    size_t item_size = _pygi_gi_type_info_size (item_type_info);
    guint i = 0;

    for (; i < length; ++i) {
        PyObject *py_item;
        GIArgument item;

        py_item = PySequence_GetItem (object, i);
        if (py_item == NULL) {
            _PyGI_ERROR_PREFIX ("Item %u: ", i);
            return -1;
        }

        item = _pygi_argument_from_object (py_item, item_type_info, transfer);

        Py_DECREF (py_item);

        if (PyErr_Occurred ()) {
            _PyGI_ERROR_PREFIX ("Item %u: ", i);
            return -1;
        }

        (*insert_func) (array, i, item, item_size);
    }

    return 0;
}

static inline void
_pygi_insert_garray_element (gpointer array, guint index, GIArgument item,
                             guint item_size)
{
    g_array_insert_vals ((GArray *)array, index, &item, 1);
}

static gint
_pygi_set_g_array_argument (GIArgument *arg, GITypeInfo *type_info,
                            GITypeInfo *item_type_info, GITransfer transfer,
                            gboolean is_zero_terminated, guint length,
                            PyObject *object)
{
    gint ret_val = -1;
    size_t item_size = _pygi_gi_type_info_size (item_type_info);

    GArray *array = g_array_sized_new (is_zero_terminated, FALSE,
                                       (guint)item_size, length);
    if (array == NULL) {
        PyErr_NoMemory ();
        goto out;
    }

    /* we handle arrays that are really strings specially */
    if (gi_type_info_get_tag (item_type_info) == GI_TYPE_TAG_UINT8
        && PyBytes_Check (object)) {
        memcpy (array->data, PyBytes_AsString (object), length);
        array->len = length;
    } else {
        GITransfer item_transfer =
            transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

        if (_pygi_fill_array_from_object (array, item_type_info, item_transfer,
                                          length, object,
                                          _pygi_insert_garray_element)
            < 0) {
            /* Free everything we have converted so far. */
            _pygi_argument_release ((GIArgument *)array, type_info,
                                    GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
            array = NULL;
            goto out;
        }
    }

    ret_val = 0;
    arg->v_pointer = g_array_ref (array);

out:
    if (array) g_array_unref (array);

    return ret_val;
}

static inline void
_pygi_insert_c_array_element (gpointer array, guint index, GIArgument item,
                              guint item_size)
{
    memcpy (((gchar *)array) + (index * item_size), &item, item_size);
}

static gint
_pygi_set_c_array_argument (GIArgument *arg, GITypeInfo *type_info,
                            GITypeInfo *item_type_info, GITransfer transfer,
                            gboolean is_zero_terminated, guint length,
                            PyObject *object)
{
    gint ret_val = -1;
    size_t item_size = _pygi_gi_type_info_size (item_type_info);
    size_t real_length = length + (is_zero_terminated ? 1 : 0);

    gchar *array = (gchar *)g_malloc0 (item_size * real_length);
    if (array == NULL) {
        PyErr_NoMemory ();
        goto out;
    }

    /* we handle arrays that are really strings specially */
    if (gi_type_info_get_tag (item_type_info) == GI_TYPE_TAG_UINT8
        && PyBytes_Check (object)) {
        /* XXX: Hopefully zero-terminated! */
        memcpy (array, PyBytes_AsString (object), length);
    } else {
        GITransfer item_transfer =
            transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

        if (_pygi_fill_array_from_object (array, item_type_info, item_transfer,
                                          length, object,
                                          _pygi_insert_c_array_element)
            < 0) {
            /* Free everything we have converted so far. */
            _pygi_argument_release ((GIArgument *)&array, type_info,
                                    GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
            array = NULL;
            goto out;
        }
    }

    ret_val = 0;
    arg->v_pointer = array;

    /* No need to free the array - the only place where
     * we could fail was in _pygi_fill_array_from_object
     * and there we call _pygi_argument_release
     * which frees the array and the elements */
out:
    return ret_val;
}

static inline void
_pygi_insert_g_ptr_array_element (gpointer array, guint index, GIArgument item,
                                  guint item_size)
{
    g_ptr_array_insert ((GPtrArray *)array, index, item.v_pointer);
}

static gint
_pygi_set_g_ptr_array_argument (GIArgument *arg, GITypeInfo *type_info,
                                GITypeInfo *item_type_info,
                                GITransfer transfer,
                                gboolean is_zero_terminated, guint length,
                                PyObject *object)
{
    size_t item_size = _pygi_gi_type_info_size (item_type_info);
    GITransfer item_transfer;
    gint ret_val = -1;
    GDestroyNotify item_free_func = NULL;

    if (item_size != sizeof (gpointer)) {
        PyErr_SetString (
            PyExc_NotImplementedError,
            "item_size from python object array != sizeof (gpointer), "
            "that will not work for GPtrArray");
        return -1;
    }

    item_free_func =
        _pygi_type_info_get_free_func (item_type_info, transfer, FALSE);

    /* We create a new pointer array with size 0. It is OK to call
     * g_ptr_array_insert as that will automatically grow the array */
    GPtrArray *array = g_ptr_array_new_with_free_func (item_free_func);
    item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING
                                                      : transfer;

    if (_pygi_fill_array_from_object (array, item_type_info, item_transfer,
                                      length, object,
                                      _pygi_insert_g_ptr_array_element)
        < 0) {
        /* Free everything we have converted so far. */
        _pygi_argument_release ((GIArgument *)&array, type_info,
                                GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
        array = NULL;
        goto out;
    }

    ret_val = 0;
    arg->v_pointer = g_ptr_array_ref (array);

out:
    if (array) g_ptr_array_unref (array);

    return ret_val;
}

static inline void
_pygi_insert_g_byte_array_element (gpointer array, guint index,
                                   GIArgument item, guint item_size)
{
    guint8 *item_c = (guint8 *)&item.v_uint8;
    GByteArray *byte_array = array;

    byte_array->data[index] = *item_c;
    byte_array->len = (byte_array->len <= index) ? index + 1 : byte_array->len;
}

static gint
_pygi_set_g_byte_array_argument (GIArgument *arg, GITypeInfo *type_info,
                                 GITypeInfo *item_type_info,
                                 GITransfer transfer,
                                 gboolean is_zero_terminated, guint length,
                                 PyObject *object)
{
    size_t item_size = _pygi_gi_type_info_size (item_type_info);
    GITransfer item_transfer;
    gint ret_val = -01;

    if (item_size != sizeof (gchar)) {
        PyErr_SetString (
            PyExc_NotImplementedError,
            "item_size from python object array != sizeof (char), "
            "that will not work for GByteArray");
        return -1;
    }

    GByteArray *array = g_byte_array_sized_new (length);
    if (array == NULL) {
        PyErr_NoMemory ();
        goto out;
    }

    item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING
                                                      : transfer;

    if (_pygi_fill_array_from_object (array, item_type_info, item_transfer,
                                      length, object,
                                      _pygi_insert_g_byte_array_element)
        < 0) {
        /* Free everything we have converted so far. */
        _pygi_argument_release ((GIArgument *)&array, type_info,
                                GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
        array = NULL;
        goto out;
    }

    ret_val = 0;
    arg->v_pointer = g_byte_array_ref (array);

out:
    if (array) g_byte_array_unref (array);

    return ret_val;
}

typedef gint (*SetArrayArgumentFunc) (GIArgument *arg, GITypeInfo *type_info,
                                      GITypeInfo *item_type_info,
                                      GITransfer transfer,
                                      gboolean is_zero_terminated,
                                      guint length, PyObject *object);


GIArgument
_pygi_argument_from_object (PyObject *object, GITypeInfo *type_info,
                            GITransfer transfer)
{
    GIArgument arg;
    GITypeTag type_tag;
    gpointer cleanup_data = NULL;

    memset (&arg, 0, sizeof (GIArgument));
    type_tag = gi_type_info_get_tag (type_info);

    switch (type_tag) {
    case GI_TYPE_TAG_ARRAY: {
        Py_ssize_t py_length;
        guint length;
        gboolean is_zero_terminated;
        GITypeInfo *item_type_info;
        SetArrayArgumentFunc set_array_argument = NULL;

        if (Py_IsNone (object)) {
            arg.v_pointer = NULL;
            break;
        }

        /* Note, strings are sequences, but we cannot accept them here */
        if (!PySequence_Check (object) || PyUnicode_Check (object)) {
            PyErr_SetString (PyExc_TypeError, "expected sequence");
            break;
        }

        py_length = PySequence_Length (object);
        if (py_length < 0) break;

        if (!pygi_guint_from_pyssize (py_length, &length)) break;

        is_zero_terminated = gi_type_info_is_zero_terminated (type_info);
        item_type_info = gi_type_info_get_param_type (type_info, 0);

        switch (gi_type_info_get_array_type (type_info)) {
        case GI_ARRAY_TYPE_C:
            set_array_argument = _pygi_set_c_array_argument;
            break;
        case GI_ARRAY_TYPE_ARRAY:
            set_array_argument = _pygi_set_g_array_argument;
            break;
        case GI_ARRAY_TYPE_PTR_ARRAY:
            set_array_argument = _pygi_set_g_ptr_array_argument;
            break;
        case GI_ARRAY_TYPE_BYTE_ARRAY:
            set_array_argument = _pygi_set_g_byte_array_argument;
            break;
        default:
            g_assert_not_reached ();
        }

        /* Might fail, if so, exception will be set, but we
         * can cleanup as normal here */
        (*set_array_argument) (&arg, type_info, item_type_info, transfer,
                               is_zero_terminated, length, object);

        gi_base_info_unref ((GIBaseInfo *)item_type_info);
        break;
    }
    case GI_TYPE_TAG_INTERFACE: {
        GIBaseInfo *info;

        info = gi_type_info_get_interface (type_info);

        if (GI_IS_CALLBACK_INFO (info)) {
            PyErr_SetString (
                PyExc_TypeError,
                "Cannot translate Python object to callback type");
        } else if (GI_IS_STRUCT_INFO (info) || GI_IS_UNION_INFO (info)) {
            GType g_type;
            PyObject *py_type;
            gboolean is_foreign =
                (GI_IS_STRUCT_INFO (info))
                && (gi_struct_info_is_foreign ((GIStructInfo *)info));

            g_type = gi_registered_type_info_get_g_type (
                (GIRegisteredTypeInfo *)info);
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
        } else if (GI_IS_FLAGS_INFO (info)) {
            /* Check flags before enums: flags are a subtype of enum. */
            GType g_type;

            g_type = gi_registered_type_info_get_g_type (
                (GIRegisteredTypeInfo *)info);
            if (pyg_flags_get_value (g_type, object, &arg.v_uint) < 0) break;
        } else if (GI_IS_ENUM_INFO (info)) {
            GType g_type;

            g_type = gi_registered_type_info_get_g_type (
                (GIRegisteredTypeInfo *)info);
            if (pyg_enum_get_value (g_type, object, &arg.v_int) < 0) break;
        } else if (GI_IS_INTERFACE_INFO (info) || GI_IS_OBJECT_INFO (info)) {
            /* An error within this call will result in a NULL arg */
            pygi_marshal_from_py_object (object, &arg, transfer);
        } else {
            g_assert_not_reached ();
        }
        gi_base_info_unref (info);
        break;
    }
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST: {
        Py_ssize_t length;
        GITypeInfo *item_type_info;
        GSList *list = NULL;
        GITransfer item_transfer;
        Py_ssize_t i;

        if (Py_IsNone (object)) {
            arg.v_pointer = NULL;
            break;
        }

        length = PySequence_Length (object);
        if (length < 0) {
            break;
        }

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
                list =
                    (GSList *)g_list_prepend ((GList *)list, item.v_pointer);
            } else {
                list = g_slist_prepend (list, item.v_pointer);
            }

            continue;

list_item_error:
            /* Free everything we have converted so far. */
            _pygi_argument_release ((GIArgument *)&list, type_info,
                                    GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
            list = NULL;

            _PyGI_ERROR_PREFIX ("Item %zd: ", i);
            break;
        }

        arg.v_pointer = list;

        gi_base_info_unref ((GIBaseInfo *)item_type_info);

        break;
    }
    case GI_TYPE_TAG_GHASH: {
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


        if (Py_IsNone (object)) {
            arg.v_pointer = NULL;
            break;
        }

        length = PyMapping_Length (object);
        if (length < 0) {
            break;
        }

        keys = PyMapping_Keys (object);
        if (keys == NULL) {
            break;
        }

        values = PyMapping_Values (object);
        if (values == NULL) {
            Py_DECREF (keys);
            break;
        }

        key_type_info = gi_type_info_get_param_type (type_info, 0);
        g_assert (key_type_info != NULL);

        value_type_info = gi_type_info_get_param_type (type_info, 1);
        g_assert (value_type_info != NULL);

        key_type_tag = gi_type_info_get_tag (key_type_info);

        switch (key_type_tag) {
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
            hash_func = g_str_hash;
            equal_func = g_str_equal;
            break;
        default:
            hash_func = NULL;
            equal_func = NULL;
        }

        item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING
                                                          : transfer;

        hash_table =
            g_hash_table_new_full (hash_func, equal_func,
                                   _pygi_type_info_get_free_func (
                                       key_type_info, item_transfer, FALSE),
                                   _pygi_type_info_get_free_func (
                                       value_type_info, item_transfer, FALSE));
        if (hash_table == NULL) {
            PyErr_NoMemory ();
            goto hash_table_release;
        }

        for (i = 0; i < length; i++) {
            PyObject *py_key;
            PyObject *py_value;
            GIArgument key;
            GIArgument value;

            py_key = PyList_GET_ITEM (keys, i);
            py_value = PyList_GET_ITEM (values, i);

            key = _pygi_argument_from_object (py_key, key_type_info,
                                              item_transfer);
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
                _pygi_arg_to_hash_pointer (&value, value_type_info));
            continue;

hash_table_item_error:
            /* Free everything we have converted so far. */
            _pygi_argument_release ((GIArgument *)&hash_table, type_info,
                                    GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
            hash_table = NULL;

            _PyGI_ERROR_PREFIX ("Item %zd: ", i);
            break;
        }

        arg.v_pointer = hash_table;

hash_table_release:
        gi_base_info_unref ((GIBaseInfo *)key_type_info);
        gi_base_info_unref ((GIBaseInfo *)value_type_info);
        Py_DECREF (keys);
        Py_DECREF (values);
        break;
    }
    case GI_TYPE_TAG_ERROR:
        PyErr_SetString (PyExc_NotImplementedError,
                         "error marshalling is not supported yet");
        /* TODO */
        break;
    default:
        /* Ignores cleanup data for now. */
        pygi_marshal_from_py_basic_type (object, &arg, type_tag, transfer,
                                         &cleanup_data);
        break;
    }

    return arg;
}

/**
 * _pygi_argument_to_object:
 * @arg: The argument to convert to an object.
 * @type_info: Type info for @arg
 * @transfer:
 *
 * If the argument is of type array, it must be encoded in a GArray, by calling
 * _pygi_argument_to_array(). This logic can not be folded into this method
 * as determining array lengths may require access to method call arguments.
 *
 * Returns: A PyObject representing @arg
 */
PyObject *
_pygi_argument_to_object (GIArgument *arg, GITypeInfo *type_info,
                          GITransfer transfer)
{
    GITypeTag type_tag;
    PyObject *object = NULL;

    type_tag = gi_type_info_get_tag (type_info);

    switch (type_tag) {
    case GI_TYPE_TAG_VOID: {
        if (gi_type_info_is_pointer (type_info)) {
            g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
            object = PyLong_FromVoidPtr (arg->v_pointer);
        }
        break;
    }
    case GI_TYPE_TAG_ARRAY: {
        /* Arrays are assumed to be packed in a GArray */
        GArray *array;
        GITypeInfo *item_type_info;
        GITypeTag item_type_tag;
        GITransfer item_transfer;
        gsize i, item_size;

        if (arg->v_pointer == NULL) return PyList_New (0);

        item_type_info = gi_type_info_get_param_type (type_info, 0);
        g_assert (item_type_info != NULL);

        item_type_tag = gi_type_info_get_tag (item_type_info);
        item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING
                                                          : transfer;

        array = arg->v_pointer;
        item_size = g_array_get_element_size (array);

        if (G_UNLIKELY (item_size > sizeof (GIArgument))) {
            g_critical (
                "Stack overflow protection. "
                "Can't copy array element into GIArgument.");
            return PyList_New (0);
        }

        if (item_type_tag == GI_TYPE_TAG_UINT8) {
            /* Return as a byte array */
            object = PyBytes_FromStringAndSize (array->data, array->len);
        } else {
            object = PyList_New (array->len);
            if (object == NULL) {
                g_critical ("Failure to allocate array for %u items",
                            array->len);
                gi_base_info_unref ((GIBaseInfo *)item_type_info);
                break;
            }

            for (i = 0; i < array->len; i++) {
                GIArgument item = { 0 };
                PyObject *py_item;

                memcpy (&item, array->data + i * item_size, item_size);

                py_item = _pygi_argument_to_object (&item, item_type_info,
                                                    item_transfer);
                if (py_item == NULL) {
                    Py_CLEAR (object);
                    _PyGI_ERROR_PREFIX ("Item %zu: ", i);
                    break;
                }

                PyList_SET_ITEM (object, i, py_item);
            }
        }

        gi_base_info_unref ((GIBaseInfo *)item_type_info);
        break;
    }
    case GI_TYPE_TAG_INTERFACE: {
        GIBaseInfo *info;

        info = gi_type_info_get_interface (type_info);

        if (GI_IS_CALLBACK_INFO (info)) {
            PyErr_SetString (
                PyExc_TypeError,
                "Cannot translate callback type to Python object");
        } else if (GI_IS_STRUCT_INFO (info) || GI_IS_UNION_INFO (info)) {
            PyObject *py_type;
            GType g_type = gi_registered_type_info_get_g_type (
                (GIRegisteredTypeInfo *)info);
            gboolean is_foreign =
                (GI_IS_STRUCT_INFO (info))
                && (gi_struct_info_is_foreign ((GIStructInfo *)info));

            /* Special case variant and none to force loading from py module. */
            if (g_type == G_TYPE_VARIANT || g_type == G_TYPE_NONE) {
                py_type = pygi_type_import_by_gi_info (info);
            } else {
                py_type = pygi_type_get_from_g_type (g_type);
            }

            object = pygi_arg_struct_to_py_marshal (
                arg, GI_REGISTERED_TYPE_INFO (info), g_type, py_type, transfer,
                FALSE, /*is_allocated*/
                is_foreign);

            if (object
                && PyObject_IsInstance (object, (PyObject *)&PyGIBoxed_Type)
                && transfer == GI_TRANSFER_NOTHING)
                pygi_boxed_copy_in_place ((PyGIBoxed *)object);

            Py_XDECREF (py_type);
        } else if (GI_IS_ENUM_INFO (info)) {
            PyObject *py_type;

            py_type = pygi_type_import_by_gi_info (info);
            if (!py_type) return NULL;

            if (GI_IS_FLAGS_INFO (info)) {
                object = pyg_flags_val_new (py_type, arg->v_uint);
            } else {
                object = pyg_enum_val_new (py_type, arg->v_int);
            }
        } else if (GI_IS_INTERFACE_INFO (info) || GI_IS_OBJECT_INFO (info)) {
            object = pygi_arg_object_to_py_called_from_c (arg, transfer);
        } else {
            g_assert_not_reached ();
        }

        gi_base_info_unref (info);
        break;
    }
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST: {
        GSList *list;
        gsize length;
        GITypeInfo *item_type_info;
        GITransfer item_transfer;
        gsize i;

        list = arg->v_pointer;
        length = g_slist_length (list);

        object = PyList_New (length);
        if (object == NULL) {
            break;
        }

        item_type_info = gi_type_info_get_param_type (type_info, 0);
        g_assert (item_type_info != NULL);

        item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING
                                                          : transfer;

        for (i = 0; list != NULL; list = g_slist_next (list), i++) {
            GIArgument item;
            PyObject *py_item;

            item.v_pointer = list->data;

            py_item = _pygi_argument_to_object (&item, item_type_info,
                                                item_transfer);
            if (py_item == NULL) {
                Py_CLEAR (object);
                _PyGI_ERROR_PREFIX ("Item %zu: ", i);
                break;
            }

            PyList_SET_ITEM (object, i, py_item);
        }

        gi_base_info_unref ((GIBaseInfo *)item_type_info);
        break;
    }
    case GI_TYPE_TAG_GHASH: {
        GITypeInfo *key_type_info;
        GITypeInfo *value_type_info;
        GITransfer item_transfer;
        GHashTableIter hash_table_iter;
        GIArgument key;
        GIArgument value;

        if (arg->v_pointer == NULL) {
            object = Py_None;
            Py_INCREF (object);
            break;
        }

        object = PyDict_New ();
        if (object == NULL) {
            break;
        }

        key_type_info = gi_type_info_get_param_type (type_info, 0);
        g_assert (key_type_info != NULL);
        g_assert (gi_type_info_get_tag (key_type_info) != GI_TYPE_TAG_VOID);

        value_type_info = gi_type_info_get_param_type (type_info, 1);
        g_assert (value_type_info != NULL);
        g_assert (gi_type_info_get_tag (value_type_info) != GI_TYPE_TAG_VOID);

        item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING
                                                          : transfer;

        g_hash_table_iter_init (&hash_table_iter,
                                (GHashTable *)arg->v_pointer);
        while (g_hash_table_iter_next (&hash_table_iter, &key.v_pointer,
                                       &value.v_pointer)) {
            PyObject *py_key;
            PyObject *py_value;
            int retval;

            py_key =
                _pygi_argument_to_object (&key, key_type_info, item_transfer);
            if (py_key == NULL) {
                break;
            }

            _pygi_hash_pointer_to_arg (&value, value_type_info);
            py_value = _pygi_argument_to_object (&value, value_type_info,
                                                 item_transfer);
            if (py_value == NULL) {
                Py_DECREF (py_key);
                break;
            }

            retval = PyDict_SetItem (object, py_key, py_value);

            Py_DECREF (py_key);
            Py_DECREF (py_value);

            if (retval < 0) {
                Py_CLEAR (object);
                break;
            }
        }

        gi_base_info_unref ((GIBaseInfo *)key_type_info);
        gi_base_info_unref ((GIBaseInfo *)value_type_info);
        break;
    }
    case GI_TYPE_TAG_ERROR: {
        GError *error = (GError *)arg->v_pointer;
        if (error != NULL && transfer == GI_TRANSFER_NOTHING) {
            /* If we have not been transferred the ownership we must copy
                 * the error, because pygi_error_check() is going to free it.
                 */
            error = g_error_copy (error);
        }

        if (pygi_error_check (&error)) {
            PyObject *err_type;
            PyObject *err_value;
            PyObject *err_trace;
            PyErr_Fetch (&err_type, &err_value, &err_trace);
            Py_XDECREF (err_type);
            Py_XDECREF (err_trace);
            object = err_value;
        } else {
            object = Py_None;
            Py_INCREF (object);
            break;
        }
        break;
    }
    default: {
        object = pygi_marshal_to_py_basic_type (arg, type_tag, transfer);
    }
    }

    return object;
}

static void
_pygi_array_release_elements (gchar *array, size_t n_items, guint item_size,
                              GITypeInfo *item_type_info,
                              GITransfer item_transfer, GIDirection direction)
{
    size_t i = 0;

    if (G_UNLIKELY (item_size > sizeof (GIArgument))) {
        g_error ("Stack overflow protection: %u > sizeof (GIArgument)",
                 item_size);
    }

    for (; i < n_items; ++i) {
        GIArgument item;
        memcpy (&item, array + (item_size * i), item_size);
        _pygi_argument_release (&item, item_type_info, item_transfer,
                                direction);
    }
}

static gchar *
_pygi_get_underlying_array (GIArgument *arg, GITypeInfo *type_info,
                            size_t *n_elements)
{
    GIArrayType array_type = gi_type_info_get_array_type (type_info);

    g_assert (n_elements != NULL);

    switch (array_type) {
    case GI_ARRAY_TYPE_C: {
        size_t length;
        GITypeInfo *item_type_info =
            gi_type_info_get_param_type (type_info, 0);
        size_t item_size = _pygi_gi_type_info_size (item_type_info);

        gi_base_info_unref ((GIBaseInfo *)item_type_info);

        if (_pygi_determine_c_array_length (arg, type_info, item_size, NULL,
                                            NULL, NULL, &length)
            < 0) {
            g_critical (
                "Unable to determine array length of %p, this will leak",
                arg->v_pointer);

            *n_elements = 0;
            return (gchar *)arg->v_pointer;
        }

        *n_elements = length;
        return (gchar *)arg->v_pointer;
    }
    case GI_ARRAY_TYPE_ARRAY:
        *n_elements = ((GArray *)arg->v_pointer)->len;
        return (gchar *)((GArray *)arg->v_pointer)->data;
    case GI_ARRAY_TYPE_PTR_ARRAY:
        *n_elements = ((GPtrArray *)arg->v_pointer)->len;
        return (gchar *)((GPtrArray *)arg->v_pointer)->pdata;
    case GI_ARRAY_TYPE_BYTE_ARRAY:
        *n_elements = ((GByteArray *)arg->v_pointer)->len;
        return (gchar *)((GByteArray *)arg->v_pointer)->data;
    default:
        g_critical ("_pygi_get_underlying_array: Should not be reached");
        break;
    }

    return NULL;
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
        size_t n_elements = 0;
        gchar *array = NULL;


        if (arg->v_pointer == NULL) {
            return;
        }

        array = _pygi_get_underlying_array (arg, type_info, &n_elements);


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
            _pygi_array_release_elements (
                array, n_elements, _pygi_gi_type_info_size (item_type_info),
                item_type_info, item_transfer, direction);


            gi_base_info_unref ((GIBaseInfo *)item_type_info);
        }

        if ((direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
            || (direction == GI_DIRECTION_OUT
                && transfer != GI_TRANSFER_NOTHING)) {
            switch (gi_type_info_get_array_type (type_info)) {
            case GI_ARRAY_TYPE_C:
                g_free (arg->v_pointer);
                break;
            case GI_ARRAY_TYPE_ARRAY:
                g_array_unref (arg->v_pointer);
                break;
            case GI_ARRAY_TYPE_PTR_ARRAY:
                g_ptr_array_unref (arg->v_pointer);
                break;
            case GI_ARRAY_TYPE_BYTE_ARRAY:
                g_byte_array_unref (arg->v_pointer);
                break;
            default:
                g_critical ("_pygi_argument_release:array - not reachable");
                break;
            }
        }

        break;
    }
    case GI_TYPE_TAG_INTERFACE: {
        GIBaseInfo *info;

        info = gi_type_info_get_interface (type_info);

        if (GI_IS_CALLBACK_INFO (info)) {
            /* TODO */
        } else if (GI_IS_STRUCT_INFO (info) || GI_IS_UNION_INFO (info)) {
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
        } else if (GI_IS_ENUM_INFO (info)) {
            /* nothing */
        } else if (GI_IS_INTERFACE_INFO (info) || GI_IS_OBJECT_INFO (info)) {
            if (arg->v_pointer == NULL) {
                return;
            }
            if (is_out && transfer == GI_TRANSFER_EVERYTHING) {
                g_object_unref (arg->v_pointer);
            }
        } else {
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

        if (direction == GI_DIRECTION_OUT
            && transfer == GI_TRANSFER_CONTAINER) {
            /* Be careful to avoid keys and values being freed if the
                 * callee gave a destroy function. */
            g_hash_table_steal_all (hash_table);
        } else {
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
        break;
    }
}

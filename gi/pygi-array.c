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

#include <glib.h>
#include <pythoncapi_compat.h>

#include "pygi-array.h"
#include "pygi-basictype.h"
#include "pygi-info.h"
#include "pygi-marshal-cleanup.h"
#include "pygi-util.h"

/* Needed for _pygi_marshal_cleanup_from_py_interface_struct_gvalue hack */
#include "pygi-struct-marshal.h"

/*
 * GArray to Python
 */

static gboolean
gi_argument_from_py_ssize_t (GIArgument *arg_out, Py_ssize_t size_in,
                             GITypeTag type_tag)
{
    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
    case GI_TYPE_TAG_BOOLEAN:
        goto unhandled_type;

    case GI_TYPE_TAG_INT8:
        if (size_in >= G_MININT8 && size_in <= G_MAXINT8) {
            arg_out->v_int8 = (gint8)size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_UINT8:
        if (size_in >= 0 && size_in <= G_MAXUINT8) {
            arg_out->v_uint8 = (guint8)size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_INT16:
        if (size_in >= G_MININT16 && size_in <= G_MAXINT16) {
            arg_out->v_int16 = (gint16)size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_UINT16:
        if (size_in >= 0 && size_in <= G_MAXUINT16) {
            arg_out->v_uint16 = (guint16)size_in;
            return TRUE;
        } else {
            goto overflow;
        }

        /* Ranges assume two's complement */
    case GI_TYPE_TAG_INT32:
        if (size_in >= G_MININT32 && size_in <= G_MAXINT32) {
            arg_out->v_int32 = (gint32)size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_UINT32:
        if (size_in >= 0 && (gsize)size_in <= G_MAXUINT32) {
            arg_out->v_uint32 = (guint32)size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_INT64:
        arg_out->v_int64 = size_in;
        return TRUE;

    case GI_TYPE_TAG_UINT64:
        if (size_in >= 0) {
            arg_out->v_uint64 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

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
    case GI_TYPE_TAG_UNICHAR:
    default:
        goto unhandled_type;
    }

overflow:
    PyErr_Format (PyExc_OverflowError,
                  "Unable to marshal C Py_ssize_t %zd to %s", size_in,
                  gi_type_tag_to_string (type_tag));
    return FALSE;

unhandled_type:
    PyErr_Format (PyExc_TypeError, "Unable to marshal C Py_ssize_t %zd to %s",
                  size_in, gi_type_tag_to_string (type_tag));
    return FALSE;
}

static gboolean
gi_argument_to_gsize (GIArgument *arg_in, gsize *gsize_out, GITypeTag type_tag)
{
    switch (type_tag) {
    case GI_TYPE_TAG_INT8:
        *gsize_out = arg_in->v_int8;
        return TRUE;
    case GI_TYPE_TAG_UINT8:
        *gsize_out = arg_in->v_uint8;
        return TRUE;
    case GI_TYPE_TAG_INT16:
        *gsize_out = arg_in->v_int16;
        return TRUE;
    case GI_TYPE_TAG_UINT16:
        *gsize_out = arg_in->v_uint16;
        return TRUE;
    case GI_TYPE_TAG_INT32:
        *gsize_out = arg_in->v_int32;
        return TRUE;
    case GI_TYPE_TAG_UINT32:
        *gsize_out = arg_in->v_uint32;
        return TRUE;
    case GI_TYPE_TAG_INT64:
        if (arg_in->v_uint64 > G_MAXSIZE) {
            PyErr_Format (PyExc_TypeError, "Unable to marshal %s to gsize",
                          gi_type_tag_to_string (type_tag));
            return FALSE;
        }
        *gsize_out = (gsize)arg_in->v_int64;
        return TRUE;
    case GI_TYPE_TAG_UINT64:
        if (arg_in->v_uint64 > G_MAXSIZE) {
            PyErr_Format (PyExc_TypeError, "Unable to marshal %s to gsize",
                          gi_type_tag_to_string (type_tag));
            return FALSE;
        }
        *gsize_out = (gsize)arg_in->v_uint64;
        return TRUE;
    default:
        PyErr_Format (PyExc_TypeError, "Unable to marshal %s to gsize",
                      gi_type_tag_to_string (type_tag));
        return FALSE;
    }
}

static gboolean
_pygi_marshal_from_py_array (PyGIInvokeState *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache *arg_cache, PyObject *py_arg,
                             GIArgument *arg, gpointer *cleanup_data)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
    guint i = 0;
    gsize success_count = 0;
    Py_ssize_t py_length;
    size_t fixed_size;
    guint length;
    guint item_size;
    gboolean is_ptr_array;
    GArray *array_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
    PyGIArgGArray *array_cache = (PyGIArgGArray *)arg_cache;
    GITransfer cleanup_transfer = arg_cache->transfer;
    gboolean is_zero_terminated =
        gi_type_info_is_zero_terminated (arg_cache->type_info);
    GIArrayType array_type;

    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!PySequence_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be sequence, not %s",
                      Py_TYPE (py_arg)->tp_name);
        return FALSE;
    }

    py_length = PySequence_Length (py_arg);
    if (py_length < 0) return FALSE;

    if (!pygi_guint_from_pyssize (py_length, &length)) return FALSE;

    if (gi_type_info_get_array_fixed_size (arg_cache->type_info, &fixed_size)
        && (guint)fixed_size != length) {
        PyErr_Format (PyExc_ValueError, "Must contain %zd items, not %u",
                      fixed_size, length);

        return FALSE;
    }

    item_size = (guint)array_cache->item_size;
    is_ptr_array = (gi_type_info_get_array_type (arg_cache->type_info)
                    == GI_ARRAY_TYPE_PTR_ARRAY);
    if (is_ptr_array) {
        array_ = (GArray *)g_ptr_array_sized_new (length);
    } else {
        array_ =
            g_array_sized_new (is_zero_terminated, TRUE, item_size, length);
    }

    if (array_ == NULL) {
        PyErr_NoMemory ();
        return FALSE;
    }

    array_type = gi_type_info_get_array_type (arg_cache->type_info);

    if (sequence_cache->item_cache->type_tag == GI_TYPE_TAG_UINT8
        && PyBytes_Check (py_arg)) {
        gchar *data = PyBytes_AsString (py_arg);

        /* Avoid making a copy if the data
         * is not transferred to the C function
         * and cannot not be modified by it.
         */
        if (array_type == GI_ARRAY_TYPE_C
            && arg_cache->transfer == GI_TRANSFER_NOTHING
            && !is_zero_terminated) {
            g_free (array_->data);
            array_->data = data;
            cleanup_transfer = GI_TRANSFER_EVERYTHING;
        } else {
            memcpy (array_->data, data, length);
        }
        array_->len = length;
        if (is_zero_terminated) {
            /* If array_ has been created with zero_termination, space for the
             * terminator is properly allocated, so we're not off-by-one here. */
            array_->data[length] = '\0';
        }
        goto array_success;
    }

    from_py_marshaller = sequence_cache->item_cache->from_py_marshaller;
    for (i = 0, success_count = 0; i < length; i++) {
        GIArgument item = { 0 };
        gpointer item_cleanup_data = NULL;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL) goto err;

        if (!from_py_marshaller (state, callable_cache,
                                 sequence_cache->item_cache, py_item, &item,
                                 &item_cleanup_data)) {
            Py_DECREF (py_item);
            goto err;
        }
        Py_DECREF (py_item);

        if (item_cleanup_data != NULL && item_cleanup_data != item.v_pointer) {
            /* We only support one level of data discrepancy between an items
             * data and its cleanup data. This is because we only track a single
             * extra cleanup data pointer per-argument and cannot track the entire
             * array of items differing data and cleanup_data.
             * For example, this would fail if trying to marshal an array of
             * callback closures marked with SCOPE call type where the cleanup data
             * is different from the items v_pointer, likewise an array of arrays.
             */
            PyErr_SetString (
                PyExc_RuntimeError,
                "Cannot cleanup item data for array due to "
                "the items data its cleanup data being different.");
            goto err;
        }

        /* FIXME: it is much more efficent to have seperate marshaller
         *        for ptr arrays than doing the evaluation
         *        and casting each loop iteration
         */
        if (is_ptr_array) {
            g_ptr_array_add ((GPtrArray *)array_, item.v_pointer);
        } else if (sequence_cache->item_cache->is_pointer) {
            /* if the item is a pointer, simply copy the pointer */
            g_assert (item_size == sizeof (item.v_pointer));
            g_array_insert_val (array_, i, item);
        } else if (sequence_cache->item_cache->type_tag
                   == GI_TYPE_TAG_INTERFACE) {
            /* Special case handling of flat arrays of gvalue/boxed/struct */
            PyGIInterfaceCache *item_iface_cache =
                (PyGIInterfaceCache *)sequence_cache->item_cache;
            GIBaseInfo *base_info =
                (GIBaseInfo *)item_iface_cache->interface_info;

            if (GI_IS_STRUCT_INFO (base_info)
                || GI_IS_UNION_INFO (base_info)) {
                PyGIArgCache *item_arg_cache =
                    (PyGIArgCache *)item_iface_cache;
                PyGIMarshalCleanupFunc from_py_cleanup =
                    item_arg_cache->from_py_cleanup;

                if (g_type_is_a (item_iface_cache->g_type, G_TYPE_VALUE)) {
                    /* Special case GValue flat arrays to properly init and copy the contents. */
                    GValue *dest =
                        (GValue *)(void *)(array_->data + (i * item_size));
                    if (item.v_pointer != NULL) {
                        memset (dest, 0, item_size);
                        g_value_init (dest,
                                      G_VALUE_TYPE ((GValue *)item.v_pointer));
                        g_value_copy ((GValue *)item.v_pointer, dest);
                    }
                    /* Manually increment the length because we are manually setting the memory. */
                    array_->len++;

                } else {
                    /* Handles flat arrays of boxed or struct types. */
                    g_array_insert_vals (array_, i, item.v_pointer, 1);
                }

                /* Cleanup any memory left by the per-item marshaler because
                 * _pygi_marshal_cleanup_from_py_array will not know about this
                 * due to "item" being a temporarily marshaled value done on the stack.
                 */
                if (from_py_cleanup)
                    from_py_cleanup (state, item_arg_cache, py_item,
                                     item_cleanup_data, TRUE);
            } else {
                g_array_insert_val (array_, i, item);
            }
        } else {
            /* default value copy of a simple type */
            g_array_insert_val (array_, i, item);
        }

        success_count++;
    }
    goto array_success;

err:
    if (sequence_cache->item_cache->from_py_cleanup != NULL) {
        gsize j;
        PyGIMarshalCleanupFunc cleanup_func =
            sequence_cache->item_cache->from_py_cleanup;

        /* Only attempt per item cleanup on pointer items */
        if (sequence_cache->item_cache->is_pointer) {
            for (j = 0; j < success_count; j++) {
                PyObject *py_seq_item = PySequence_GetItem (py_arg, j);
                cleanup_func (state, sequence_cache->item_cache, py_seq_item,
                              is_ptr_array
                                  ? g_ptr_array_index ((GPtrArray *)array_, j)
                                  : g_array_index (array_, gpointer, j),
                              TRUE);
                Py_DECREF (py_seq_item);
            }
        }
    }

    if (is_ptr_array)
        g_ptr_array_free ((GPtrArray *)array_, TRUE);
    else
        g_array_free (array_, TRUE);
    _PyGI_ERROR_PREFIX ("Item %u: ", i);
    return FALSE;

array_success:
    if (array_cache->has_len_arg) {
        /* we have an child arg to handle */
        PyGIArgCache *child_cache = _pygi_callable_cache_get_arg (
            callable_cache, array_cache->len_arg_index);

        if (!gi_argument_from_py_ssize_t (
                &state->args[child_cache->c_arg_index].arg_value, length,
                child_cache->type_tag)) {
            goto err;
        }
    }

    if (array_type == GI_ARRAY_TYPE_C) {
        /* In the case of GI_ARRAY_C, we give the data directly as the argument
         * but keep the array_ wrapper as cleanup data so we don't have to find
         * it's length again.
         */
        arg->v_pointer = array_->data;

        if (cleanup_transfer == GI_TRANSFER_EVERYTHING) {
            g_array_free (array_, FALSE);
            *cleanup_data = NULL;
        } else {
            *cleanup_data = array_;
        }
    } else {
        arg->v_pointer = array_;

        if (cleanup_transfer == GI_TRANSFER_NOTHING) {
            /* Free everything in cleanup. */
            *cleanup_data = array_;
        } else if (cleanup_transfer == GI_TRANSFER_CONTAINER) {
            /* Make a shallow copy so we can free the elements later in cleanup
             * because it is possible invoke will free the list before our cleanup. */
            *cleanup_data =
                is_ptr_array ? (gpointer)g_ptr_array_ref ((GPtrArray *)array_)
                             : (gpointer)g_array_ref (array_);
        } else { /* GI_TRANSFER_EVERYTHING */
            /* No cleanup, everything is given to the callee. */
            *cleanup_data = NULL;
        }
    }

    return TRUE;
}

static void
_pygi_marshal_cleanup_from_py_array (PyGIInvokeState *state,
                                     PyGIArgCache *arg_cache, PyObject *py_arg,
                                     gpointer data, gboolean was_processed)
{
    if (was_processed) {
        GArray *array_ = NULL;
        GPtrArray *ptr_array_ = NULL;
        PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
        PyGIArgGArray *array_cache = (PyGIArgGArray *)arg_cache;
        GIArrayType array_type =
            gi_type_info_get_array_type (arg_cache->type_info);

        if (array_type == GI_ARRAY_TYPE_PTR_ARRAY) {
            ptr_array_ = (GPtrArray *)data;
        } else {
            array_ = (GArray *)data;
        }

        /* clean up items first */
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            gsize i;
            guint len;
            PyGIMarshalCleanupFunc cleanup_func =
                sequence_cache->item_cache->from_py_cleanup;

            g_assert (array_ || ptr_array_);
            len = (array_ != NULL) ? array_->len : ptr_array_->len;

            for (i = 0; i < len; i++) {
                gpointer item;
                PyObject *py_item = NULL;

                /* case 1: GPtrArray */
                if (ptr_array_ != NULL)
                    item = g_ptr_array_index (ptr_array_, i);
                /* case 2: C array or GArray with object pointers */
                else if (sequence_cache->item_cache->is_pointer)
                    item = g_array_index (array_, gpointer, i);
                /* case 3: C array or GArray with simple types or structs */
                else {
                    item = array_->data + i * array_cache->item_size;
                    /* special-case hack: GValue array items do not get slice
                     * allocated in _pygi_marshal_from_py_array(), so we must
                     * not try to deallocate it as a slice and thus
                     * short-circuit cleanup_func. */
                    if (cleanup_func == pygi_arg_gvalue_from_py_cleanup) {
                        g_value_unset ((GValue *)item);
                        continue;
                    }
                }

                py_item = PySequence_GetItem (py_arg, i);
                cleanup_func (state, sequence_cache->item_cache, py_item, item,
                              TRUE);
                Py_XDECREF (py_item);
            }
        }

        /* Only free the array when we didn't transfer ownership */
        if (array_type == GI_ARRAY_TYPE_C) {
            /* always free the GArray wrapper created in from_py marshaling and
             * passed back as cleanup_data
             */
            g_array_free (array_, arg_cache->transfer == GI_TRANSFER_NOTHING);
        } else {
            if (array_ != NULL)
                g_array_unref (array_);
            else
                g_ptr_array_unref (ptr_array_);
        }
    }
}

/*
 * GArray from Python
 */
static PyObject *
_pygi_marshal_to_py_array (PyGIInvokeState *state,
                           PyGICallableCache *callable_cache,
                           PyGIArgCache *arg_cache, GIArgument *arg,
                           gpointer *cleanup_data)
{
    GArray *array_;
    PyObject *py_obj = NULL;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;
    PyGIArgGArray *array_cache = (PyGIArgGArray *)arg_cache;
    guint processed_items = 0;
    GIArrayType array_type =
        gi_type_info_get_array_type (arg_cache->type_info);

    /* GArrays make it easier to iterate over arrays
      * with different element sizes but requires that
      * we allocate a GArray if the argument was a C array
      */
    if (array_type == GI_ARRAY_TYPE_C) {
        gsize len = 0;
        if (gi_type_info_get_array_fixed_size (arg_cache->type_info, &len)) {
            g_assert (arg->v_pointer != NULL);
        } else if (gi_type_info_is_zero_terminated (arg_cache->type_info)) {
            if (arg->v_pointer == NULL) {
                len = 0;
            } else if (array_cache->item_size == 1) {
                len = strlen (arg->v_pointer);
            } else if (array_cache->item_size == sizeof (gpointer)) {
                len = g_strv_length ((gchar **)arg->v_pointer);
            } else if (array_cache->item_size == sizeof (int)) {
                for (len = 0; *(((int *)arg->v_pointer) + len); len++);
            } else if (array_cache->item_size == sizeof (short)) {
                for (len = 0; *(((short *)arg->v_pointer) + len); len++);
            } else {
                g_assert_not_reached ();
            }
        } else if (array_cache->has_len_arg) {
            GIArgument *len_arg =
                &state->args[array_cache->len_arg_index].arg_value;
            PyGIArgCache *sub_cache = _pygi_callable_cache_get_arg (
                callable_cache, array_cache->len_arg_index);

            if (!gi_argument_to_gsize (len_arg, &len, sub_cache->type_tag)) {
                return NULL;
            }
        }

        array_ = g_array_new (FALSE, FALSE, (guint)array_cache->item_size);
        if (array_ == NULL) {
            PyErr_NoMemory ();

            if (arg_cache->transfer == GI_TRANSFER_EVERYTHING
                && arg->v_pointer != NULL)
                g_free (arg->v_pointer);

            return NULL;
        }

        if (array_->data != NULL) g_free (array_->data);
        array_->data = arg->v_pointer;
        array_->len = (guint)len;
    } else {
        array_ = arg->v_pointer;
    }

    if (seq_cache->item_cache->type_tag == GI_TYPE_TAG_UINT8) {
        if (arg->v_pointer == NULL) {
            py_obj = PyBytes_FromString ("");
        } else {
            py_obj = PyBytes_FromStringAndSize (array_->data, array_->len);
        }
    } else {
        if (arg->v_pointer == NULL) {
            py_obj = PyList_New (0);
        } else {
            guint i;

            gsize item_size;
            PyGIMarshalToPyFunc item_to_py_marshaller;
            PyGIArgCache *item_arg_cache;
            GPtrArray *item_cleanups;

            py_obj = PyList_New (array_->len);
            if (py_obj == NULL) goto err;

            item_cleanups = g_ptr_array_sized_new (array_->len);
            *cleanup_data = item_cleanups;

            item_arg_cache = seq_cache->item_cache;
            item_to_py_marshaller = item_arg_cache->to_py_marshaller;

            item_size = g_array_get_element_size (array_);

            for (i = 0; i < array_->len; i++) {
                GIArgument item_arg = { 0 };
                PyObject *py_item;
                gpointer item_cleanup_data = NULL;

                /* If we are receiving an array of pointers, simply assign the pointer
                 * and move on, letting the per-item marshaler deal with the
                 * various transfer modes and ref counts (e.g. g_variant_ref_sink).
                 */
                if (array_type == GI_ARRAY_TYPE_PTR_ARRAY) {
                    item_arg.v_pointer =
                        g_ptr_array_index ((GPtrArray *)array_, i);

                } else if (item_arg_cache->is_pointer) {
                    item_arg.v_pointer = g_array_index (array_, gpointer, i);

                } else if (item_arg_cache->type_tag == GI_TYPE_TAG_INTERFACE) {
                    PyGIInterfaceCache *iface_cache =
                        (PyGIInterfaceCache *)item_arg_cache;

                    /* FIXME: This probably doesn't work with boxed types or gvalues.
                     * See fx. _pygi_marshal_from_py_array() */
                    if (GI_IS_STRUCT_INFO (iface_cache->interface_info)) {
                        if (arg_cache->transfer == GI_TRANSFER_EVERYTHING
                            && !g_type_is_a (iface_cache->g_type,
                                             G_TYPE_BOXED)) {
                            /* array elements are structs */
                            gpointer *_struct = g_malloc (item_size);
                            memcpy (_struct, array_->data + i * item_size,
                                    item_size);
                            item_arg.v_pointer = _struct;
                        } else {
                            item_arg.v_pointer = array_->data + i * item_size;
                        }
                    } else if (GI_IS_ENUM_INFO (iface_cache->interface_info)) {
                        memcpy (&item_arg, array_->data + i * item_size,
                                item_size);
                    } else {
                        item_arg.v_pointer =
                            g_array_index (array_, gpointer, i);
                    }
                } else {
                    memcpy (&item_arg, array_->data + i * item_size,
                            item_size);
                }

                py_item = item_to_py_marshaller (state, callable_cache,
                                                 item_arg_cache, &item_arg,
                                                 &item_cleanup_data);

                g_ptr_array_index (item_cleanups, i) = item_cleanup_data;

                if (py_item == NULL) {
                    Py_CLEAR (py_obj);

                    g_ptr_array_unref (item_cleanups);

                    goto err;
                }
                PyList_SET_ITEM (py_obj, i, py_item);
                processed_items++;
            }
        }
    }

    if (array_type == GI_ARRAY_TYPE_C) g_array_free (array_, FALSE);

    return py_obj;

err:
    if (array_type == GI_ARRAY_TYPE_C) {
        g_array_free (array_, arg_cache->transfer == GI_TRANSFER_EVERYTHING);
    } else {
        /* clean up unprocessed items */
        if (seq_cache->item_cache->to_py_cleanup != NULL) {
            guint j;
            PyGIMarshalToPyCleanupFunc cleanup_func =
                seq_cache->item_cache->to_py_cleanup;
            for (j = processed_items; j < array_->len; j++) {
                cleanup_func (state, seq_cache->item_cache, NULL,
                              g_array_index (array_, gpointer, j), FALSE);
            }
        }

        if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
            g_array_free (array_, TRUE);
    }

    return NULL;
}

static GArray *
_wrap_c_array (PyGIInvokeState *state, PyGIArgGArray *array_cache,
               gpointer data)
{
    size_t len = 0;

    if (gi_type_info_get_array_fixed_size (
            ((PyGIArgCache *)array_cache)->type_info, &len)) {
        /* len is set. */
    } else if (gi_type_info_is_zero_terminated (
                   ((PyGIArgCache *)array_cache)->type_info)) {
        if (array_cache->item_size == sizeof (gpointer))
            len = g_strv_length ((gchar **)data);
        else if (array_cache->item_size == 1)
            len = strlen ((gchar *)data);
        else if (array_cache->item_size == sizeof (int))
            for (len = 0; *(((int *)data) + len); len++);
        else if (array_cache->item_size == sizeof (short))
            for (len = 0; *(((short *)data) + len); len++);
        else
            g_assert_not_reached ();
    } else if (array_cache->has_len_arg) {
        GIArgument *len_arg =
            &state->args[array_cache->len_arg_index].arg_value;
        len = len_arg->v_long;
    }

    return g_array_new_take (data, (guint)len, FALSE,
                             (guint)array_cache->item_size);
}

static void
_pygi_marshal_cleanup_to_py_array (PyGIInvokeState *state,
                                   PyGIArgCache *arg_cache,
                                   gpointer cleanup_data, gpointer data,
                                   gboolean was_processed)
{
    GArray *array_ = NULL;
    GPtrArray *ptr_array_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
    PyGIArgGArray *array_cache = (PyGIArgGArray *)arg_cache;
    gboolean free_array = FALSE;
    gboolean free_array_full = TRUE;
    GIArrayType array_type =
        gi_type_info_get_array_type (arg_cache->type_info);

    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING
        || arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        free_array = TRUE;
    }

    /* If this isn't a garray create one to help process variable sized
       array elements */
    if (array_type == GI_ARRAY_TYPE_C) {
        array_ = _wrap_c_array (state, array_cache, data);

        if (array_ == NULL) return;

        free_array = TRUE;
        free_array_full = arg_cache->transfer != GI_TRANSFER_NOTHING;
    } else if (array_type == GI_ARRAY_TYPE_PTR_ARRAY) {
        ptr_array_ = (GPtrArray *)data;
    } else {
        array_ = (GArray *)data;
    }

    if (sequence_cache->item_cache->to_py_cleanup != NULL) {
        GPtrArray *item_cleanups = (GPtrArray *)cleanup_data;
        gsize i;
        guint len;
        PyGIMarshalToPyCleanupFunc cleanup_func =
            sequence_cache->item_cache->to_py_cleanup;

        g_assert (array_ || ptr_array_);
        len = (array_ != NULL) ? array_->len : ptr_array_->len;

        for (i = 0; i < len; i++) {
            cleanup_func (state, sequence_cache->item_cache,
                          g_ptr_array_index (item_cleanups, i),
                          (array_ != NULL)
                              ? g_array_index (array_, gpointer, i)
                              : g_ptr_array_index (ptr_array_, i),
                          was_processed);
        }
    }

    if (cleanup_data) g_ptr_array_unref ((GPtrArray *)cleanup_data);

    if (free_array) {
        if (array_ != NULL)
            g_array_free (array_, free_array_full);
        else
            g_ptr_array_free (ptr_array_, free_array_full);
    }
}

static void
_array_cache_free_func (PyGIArgGArray *cache)
{
    if (cache != NULL) {
        pygi_arg_cache_free (((PyGISequenceCache *)cache)->item_cache);
        g_slice_free (PyGIArgGArray, cache);
    }
}

static void
pygi_arg_garray_len_arg_setup (PyGIArgGArray *seq_cache, GITypeInfo *type_info,
                               PyGICallableCache *callable_cache,
                               PyGIDirection direction, gssize arg_index,
                               gssize *py_arg_index)
{
    /* attempt len_arg_index setup for the first time */
    if (!seq_cache->has_len_arg) {
        seq_cache->has_len_arg = gi_type_info_get_array_length_index (
            type_info, &seq_cache->len_arg_index);

        /* offset by self arg for methods and vfuncs */
        if (seq_cache->has_len_arg && callable_cache != NULL) {
            seq_cache->len_arg_index += callable_cache->args_offset;
        }
    }

    if (seq_cache->has_len_arg) {
        PyGIArgCache *child_cache = NULL;

        child_cache = _pygi_callable_cache_get_arg (callable_cache,
                                                    seq_cache->len_arg_index);
        if (child_cache == NULL) {
            child_cache = pygi_arg_cache_alloc ();
            _pygi_callable_cache_set_arg (
                callable_cache, seq_cache->len_arg_index, child_cache);
        } else {
            /* If the "length" arg cache already exists (the length comes before
             * the array in the argument list), remove it from the to_py_args list
             * because it does not belong in "to python" return tuple. The length
             * will implicitly be a part of the returned Python list.
             */
            if (direction & PYGI_DIRECTION_TO_PYTHON) {
                callable_cache->to_py_args =
                    g_slist_remove (callable_cache->to_py_args, child_cache);
            }

            /* This is a case where the arg cache already exists and has been
             * setup by another array argument sharing the same length argument.
             * See: gi_marshalling_tests_multi_array_key_value_in
             */
            if (child_cache->meta_type == PYGI_META_ARG_TYPE_CHILD) return;
        }

        /* There is a length argument for this array, so increment the number
         * of "to python" child arguments when applicable.
         */
        if (direction & PYGI_DIRECTION_TO_PYTHON)
            callable_cache->n_to_py_child_args++;

        child_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
        child_cache->direction = direction;
        child_cache->to_py_marshaller =
            pygi_marshal_to_py_basic_type_cache_adapter;
        child_cache->from_py_marshaller =
            pygi_marshal_from_py_basic_type_cache_adapter;
        child_cache->py_arg_index = -1;

        /* ugly edge case code:
         *
         * When the length comes before the array parameter we need to update
         * indexes of arguments after the index argument.
         */
        if (((gssize)seq_cache->len_arg_index) < arg_index
            && direction & PYGI_DIRECTION_FROM_PYTHON) {
            guint i;
            (*py_arg_index) -= 1;
            callable_cache->n_py_args -= 1;

            for (i = (guint)seq_cache->len_arg_index + 1;
                 (gsize)i < _pygi_callable_cache_args_len (callable_cache);
                 i++) {
                PyGIArgCache *update_cache =
                    _pygi_callable_cache_get_arg (callable_cache, i);
                if (update_cache == NULL) break;

                update_cache->py_arg_index -= 1;
            }
        }
    }
}

static gboolean
pygi_arg_garray_setup (
    PyGIArgGArray *sc, GITypeInfo *type_info,
    GIArgInfo *arg_info, /* may be NULL for return arguments */
    GITransfer transfer, PyGIDirection direction,
    PyGICallableCache *callable_cache)
{
    GITypeInfo *item_type_info;
    PyGIArgCache *arg_cache = (PyGIArgCache *)sc;

    if (!pygi_arg_sequence_setup ((PyGISequenceCache *)sc, type_info, arg_info,
                                  transfer, direction, callable_cache)) {
        return FALSE;
    }

    ((PyGIArgCache *)sc)->destroy_notify =
        (GDestroyNotify)_array_cache_free_func;
    sc->has_len_arg = FALSE; /* setup by pygi_arg_garray_len_arg_setup */

    item_type_info = gi_type_info_get_param_type (type_info, 0);
    sc->item_size = _pygi_gi_type_info_size (item_type_info);
    gi_base_info_unref ((GIBaseInfo *)item_type_info);

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_array;
        arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_array;
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_array;
        arg_cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_array;
    }

    return TRUE;
}

PyGIArgCache *
pygi_arg_garray_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                               GITransfer transfer, PyGIDirection direction,
                               PyGICallableCache *callable_cache,
                               gssize arg_index, gssize *py_arg_index)
{
    PyGIArgGArray *array_cache = g_slice_new0 (PyGIArgGArray);
    if (array_cache == NULL) return NULL;

    if (!pygi_arg_garray_setup (array_cache, type_info, arg_info, transfer,
                                direction, callable_cache)) {
        pygi_arg_cache_free ((PyGIArgCache *)array_cache);
        return NULL;
    }

    pygi_arg_garray_len_arg_setup (array_cache, type_info, callable_cache,
                                   direction, arg_index, py_arg_index);

    return (PyGIArgCache *)array_cache;
}

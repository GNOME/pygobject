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
#include <Python.h>
#include <pyglib-python-compat.h>

#include "pygi-array.h"
#include "pygi-private.h"
#include "pygi-marshal-cleanup.h"
#include "pygi-basictype.h"

/* Needed for _pygi_marshal_cleanup_from_py_interface_struct_gvalue hack */
#include "pygi-struct-marshal.h"

/*
 * GArray to Python
 */

static gboolean
gi_argument_from_py_ssize_t (GIArgument   *arg_out,
                             Py_ssize_t    size_in,
                             GITypeTag     type_tag)
{
    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
    case GI_TYPE_TAG_BOOLEAN:
        goto unhandled_type;

    case GI_TYPE_TAG_INT8:
        if (size_in >= G_MININT8 && size_in <= G_MAXINT8) {
            arg_out->v_int8 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_UINT8:
        if (size_in >= 0 && size_in <= G_MAXUINT8) {
            arg_out->v_uint8 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_INT16:
        if (size_in >= G_MININT16 && size_in <= G_MAXINT16) {
            arg_out->v_int16 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_UINT16:
        if (size_in >= 0 && size_in <= G_MAXUINT16) {
            arg_out->v_uint16 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

        /* Ranges assume two's complement */
    case GI_TYPE_TAG_INT32:
        if (size_in >= G_MININT32 && size_in <= G_MAXINT32) {
            arg_out->v_int32 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_UINT32:
        if (size_in >= 0 && size_in <= G_MAXUINT32) {
            arg_out->v_uint32 = size_in;
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
                  "Unable to marshal C Py_ssize_t %zd to %s",
                  size_in,
                  g_type_tag_to_string (type_tag));
    return FALSE;

 unhandled_type:
    PyErr_Format (PyExc_TypeError,
                  "Unable to marshal C Py_ssize_t %zd to %s",
                  size_in,
                  g_type_tag_to_string (type_tag));
    return FALSE;
}

static gboolean
gi_argument_to_gsize (GIArgument *arg_in,
                      gsize      *gsize_out,
                      GITypeTag   type_tag)
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
          *gsize_out = arg_in->v_int64;
          return TRUE;
      case GI_TYPE_TAG_UINT64:
          *gsize_out = arg_in->v_uint64;
          return TRUE;
      default:
          PyErr_Format (PyExc_TypeError,
                        "Unable to marshal %s to gsize",
                        g_type_tag_to_string (type_tag));
          return FALSE;
    }
}

static gboolean
_pygi_marshal_from_py_array (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg,
                             gpointer          *cleanup_data)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
    int i = 0;
    int success_count = 0;
    Py_ssize_t length;
    gssize item_size;
    gboolean is_ptr_array;
    GArray *array_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
    PyGIArgGArray *array_cache = (PyGIArgGArray *)arg_cache;
    GITransfer cleanup_transfer = arg_cache->transfer;


    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!PySequence_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be sequence, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    length = PySequence_Length (py_arg);
    if (length < 0)
        return FALSE;

    if (array_cache->fixed_size >= 0 &&
            array_cache->fixed_size != length) {
        PyErr_Format (PyExc_ValueError, "Must contain %zd items, not %zd",
                      array_cache->fixed_size, length);

        return FALSE;
    }

    item_size = array_cache->item_size;
    is_ptr_array = (array_cache->array_type == GI_ARRAY_TYPE_PTR_ARRAY);
    if (is_ptr_array) {
        array_ = (GArray *)g_ptr_array_sized_new (length);
    } else {
        array_ = g_array_sized_new (array_cache->is_zero_terminated,
                                    TRUE,
                                    item_size,
                                    length);
    }

    if (array_ == NULL) {
        PyErr_NoMemory ();
        return FALSE;
    }

    if (sequence_cache->item_cache->type_tag == GI_TYPE_TAG_UINT8 &&
        PYGLIB_PyBytes_Check (py_arg)) {
        gchar *data = PYGLIB_PyBytes_AsString (py_arg);

        /* Avoid making a copy if the data
         * is not transferred to the C function
         * and cannot not be modified by it.
         */
        if (array_cache->array_type == GI_ARRAY_TYPE_C &&
            arg_cache->transfer == GI_TRANSFER_NOTHING &&
            !array_cache->is_zero_terminated) {
            g_free (array_->data);
            array_->data = data;
            cleanup_transfer = GI_TRANSFER_EVERYTHING;
        } else {
            memcpy (array_->data, data, length);
        }
        array_->len = length;
        if (array_cache->is_zero_terminated) {
            /* If array_ has been created with zero_termination, space for the
             * terminator is properly allocated, so we're not off-by-one here. */
            array_->data[length] = '\0';
        }
        goto array_success;
    }

    from_py_marshaller = sequence_cache->item_cache->from_py_marshaller;
    for (i = 0, success_count = 0; i < length; i++) {
        GIArgument item = {0};
        gpointer item_cleanup_data = NULL;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL)
            goto err;

        if (!from_py_marshaller ( state,
                                  callable_cache,
                                  sequence_cache->item_cache,
                                  py_item,
                                 &item,
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
            PyErr_SetString(PyExc_RuntimeError, "Cannot cleanup item data for array due to "
                                                "the items data its cleanup data being different.");
            goto err;
        }

        /* FIXME: it is much more efficent to have seperate marshaller
         *        for ptr arrays than doing the evaluation
         *        and casting each loop iteration
         */
        if (is_ptr_array) {
            g_ptr_array_add((GPtrArray *)array_, item.v_pointer);
        } else if (sequence_cache->item_cache->is_pointer) {
            /* if the item is a pointer, simply copy the pointer */
            g_assert (item_size == sizeof (item.v_pointer));
            g_array_insert_val (array_, i, item);
        } else if (sequence_cache->item_cache->type_tag == GI_TYPE_TAG_INTERFACE) {
            /* Special case handling of flat arrays of gvalue/boxed/struct */
            PyGIInterfaceCache *item_iface_cache = (PyGIInterfaceCache *) sequence_cache->item_cache;
            GIBaseInfo *base_info = (GIBaseInfo *) item_iface_cache->interface_info;
            GIInfoType info_type = g_base_info_get_type (base_info);

            switch (info_type) {
                case GI_INFO_TYPE_UNION:
                case GI_INFO_TYPE_STRUCT:
                {
                    PyGIArgCache *item_arg_cache = (PyGIArgCache *)item_iface_cache;
                    PyGIMarshalCleanupFunc from_py_cleanup = item_arg_cache->from_py_cleanup;

                    if (g_type_is_a (item_iface_cache->g_type, G_TYPE_VALUE)) {
                        /* Special case GValue flat arrays to properly init and copy the contents. */
                        GValue* dest = (GValue*) (array_->data + (i * item_size));
                        if (item.v_pointer != NULL) {
                            memset (dest, 0, item_size);
                            g_value_init (dest, G_VALUE_TYPE ((GValue*) item.v_pointer));
                            g_value_copy ((GValue*) item.v_pointer, dest);
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
                        from_py_cleanup (state, item_arg_cache, py_item, item_cleanup_data, TRUE);

                    break;
                }
                default:
                    g_array_insert_val (array_, i, item);
            }
        } else {
            /* default value copy of a simple type */
            g_array_insert_val (array_, i, item);
        }

        success_count++;
        continue;
err:
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            gsize j;
            PyGIMarshalCleanupFunc cleanup_func =
                sequence_cache->item_cache->from_py_cleanup;

            /* Only attempt per item cleanup on pointer items */
            if (sequence_cache->item_cache->is_pointer) {
                for(j = 0; j < success_count; j++) {
                    PyObject *py_item = PySequence_GetItem (py_arg, j);
                    cleanup_func (state,
                                  sequence_cache->item_cache,
                                  py_item,
                                  is_ptr_array ?
                                          g_ptr_array_index ((GPtrArray *)array_, j) :
                                          g_array_index (array_, gpointer, j),
                                  TRUE);
                    Py_DECREF (py_item);
                }
            }
        }

        if (is_ptr_array)
            g_ptr_array_free ( ( GPtrArray *)array_, TRUE);
        else
            g_array_free (array_, TRUE);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

array_success:
    if (array_cache->len_arg_index >= 0) {
        /* we have an child arg to handle */
        PyGIArgCache *child_cache =
            _pygi_callable_cache_get_arg (callable_cache, array_cache->len_arg_index);

        if (!gi_argument_from_py_ssize_t (&state->arg_values[child_cache->c_arg_index],
                                          length,
                                          child_cache->type_tag)) {
            goto err;
        }
    }

    if (array_cache->array_type == GI_ARRAY_TYPE_C) {
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
            *cleanup_data = is_ptr_array ?
                    (gpointer)g_ptr_array_ref ((GPtrArray *)array_) :
                    (gpointer)g_array_ref (array_);
        } else { /* GI_TRANSFER_EVERYTHING */
            /* No cleanup, everything is given to the callee. */
            *cleanup_data = NULL;
        }
    }

    return TRUE;
}

static void
_pygi_marshal_cleanup_from_py_array (PyGIInvokeState *state,
                                     PyGIArgCache    *arg_cache,
                                     PyObject        *py_arg,
                                     gpointer         data,
                                     gboolean         was_processed)
{
    if (was_processed) {
        GArray *array_ = NULL;
        GPtrArray *ptr_array_ = NULL;
        PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
        PyGIArgGArray *array_cache = (PyGIArgGArray *)arg_cache;

        if (array_cache->array_type == GI_ARRAY_TYPE_PTR_ARRAY) {
            ptr_array_ = (GPtrArray *) data;
        } else {
            array_ = (GArray *) data;
        }

        /* clean up items first */
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            gsize i;
            guint len = (array_ != NULL) ? array_->len : ptr_array_->len;
            PyGIMarshalCleanupFunc cleanup_func =
                sequence_cache->item_cache->from_py_cleanup;

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
                        g_value_unset ((GValue*) item);
                        continue;
                    }
                }

                py_item = PySequence_GetItem (py_arg, i);
                cleanup_func (state, sequence_cache->item_cache, py_item, item, TRUE);
                Py_XDECREF (py_item);
            }
        }

        /* Only free the array when we didn't transfer ownership */
        if (array_cache->array_type == GI_ARRAY_TYPE_C) {
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
_pygi_marshal_to_py_array (PyGIInvokeState   *state,
                           PyGICallableCache *callable_cache,
                           PyGIArgCache      *arg_cache,
                           GIArgument        *arg)
{
    GArray *array_;
    PyObject *py_obj = NULL;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;
    PyGIArgGArray *array_cache = (PyGIArgGArray *)arg_cache;
    gsize processed_items = 0;

     /* GArrays make it easier to iterate over arrays
      * with different element sizes but requires that
      * we allocate a GArray if the argument was a C array
      */
    if (array_cache->array_type == GI_ARRAY_TYPE_C) {
        gsize len;
        if (array_cache->fixed_size >= 0) {
            g_assert(arg->v_pointer != NULL);
            len = array_cache->fixed_size;
        } else if (array_cache->is_zero_terminated) {
            if (arg->v_pointer == NULL) {
                len = 0;
            } else if (seq_cache->item_cache->type_tag == GI_TYPE_TAG_UINT8) {
                len = strlen (arg->v_pointer);
            } else {
                len = g_strv_length ((gchar **)arg->v_pointer);
            }
        } else {
            GIArgument *len_arg = &state->arg_values[array_cache->len_arg_index];
            PyGIArgCache *arg_cache = _pygi_callable_cache_get_arg (callable_cache,
                                                                    array_cache->len_arg_index);

            if (!gi_argument_to_gsize (len_arg, &len, arg_cache->type_tag)) {
                return NULL;
            }
        }

        array_ = g_array_new (FALSE,
                              FALSE,
                              array_cache->item_size);
        if (array_ == NULL) {
            PyErr_NoMemory ();

            if (arg_cache->transfer == GI_TRANSFER_EVERYTHING && arg->v_pointer != NULL)
                g_free (arg->v_pointer);

            return NULL;
        }

        if (array_->data != NULL)
            g_free (array_->data);
        array_->data = arg->v_pointer;
        array_->len = len;
    } else {
        array_ = arg->v_pointer;
    }

    if (seq_cache->item_cache->type_tag == GI_TYPE_TAG_UINT8) {
        if (arg->v_pointer == NULL) {
            py_obj = PYGLIB_PyBytes_FromString ("");
        } else {
            py_obj = PYGLIB_PyBytes_FromStringAndSize (array_->data, array_->len);
        }
    } else {
        if (arg->v_pointer == NULL) {
            py_obj = PyList_New (0);
        } else {
            int i;

            gsize item_size;
            PyGIMarshalToPyFunc item_to_py_marshaller;
            PyGIArgCache *item_arg_cache;

            py_obj = PyList_New (array_->len);
            if (py_obj == NULL)
                goto err;


            item_arg_cache = seq_cache->item_cache;
            item_to_py_marshaller = item_arg_cache->to_py_marshaller;

            item_size = g_array_get_element_size (array_);

            for (i = 0; i < array_->len; i++) {
                GIArgument item_arg = {0};
                PyObject *py_item;

                /* If we are receiving an array of pointers, simply assign the pointer
                 * and move on, letting the per-item marshaler deal with the
                 * various transfer modes and ref counts (e.g. g_variant_ref_sink).
                 */
                if (array_cache->array_type == GI_ARRAY_TYPE_PTR_ARRAY) {
                    item_arg.v_pointer = g_ptr_array_index ( ( GPtrArray *)array_, i);

                } else if (item_arg_cache->is_pointer) {
                    item_arg.v_pointer = g_array_index (array_, gpointer, i);

                } else if (item_arg_cache->type_tag == GI_TYPE_TAG_INTERFACE) {
                    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *) item_arg_cache;

                    /* FIXME: This probably doesn't work with boxed types or gvalues.
                     * See fx. _pygi_marshal_from_py_array() */
                    switch (g_base_info_get_type (iface_cache->interface_info)) {
                        case GI_INFO_TYPE_STRUCT:
                            if (arg_cache->transfer == GI_TRANSFER_EVERYTHING &&
                                       !g_type_is_a (iface_cache->g_type, G_TYPE_BOXED)) {
                                /* array elements are structs */
                                gpointer *_struct = g_malloc (item_size);
                                memcpy (_struct, array_->data + i * item_size,
                                        item_size);
                                item_arg.v_pointer = _struct;
                            } else {
                                item_arg.v_pointer = array_->data + i * item_size;
                            }
                            break;
                        default:
                            item_arg.v_pointer = g_array_index (array_, gpointer, i);
                            break;
                    }
                } else {
                    memcpy (&item_arg, array_->data + i * item_size, item_size);
                }

                py_item = item_to_py_marshaller ( state,
                                                callable_cache,
                                                item_arg_cache,
                                                &item_arg);

                if (py_item == NULL) {
                    Py_CLEAR (py_obj);

                    if (array_cache->array_type == GI_ARRAY_TYPE_C)
                        g_array_unref (array_);

                    goto err;
                }
                PyList_SET_ITEM (py_obj, i, py_item);
                processed_items++;
            }
        }
    }

    if (array_cache->array_type == GI_ARRAY_TYPE_C)
        g_array_free (array_, FALSE);

    return py_obj;

err:
    if (array_cache->array_type == GI_ARRAY_TYPE_C) {
        g_array_free (array_, arg_cache->transfer == GI_TRANSFER_EVERYTHING);
    } else {
        /* clean up unprocessed items */
        if (seq_cache->item_cache->to_py_cleanup != NULL) {
            int j;
            PyGIMarshalCleanupFunc cleanup_func = seq_cache->item_cache->to_py_cleanup;
            for (j = processed_items; j < array_->len; j++) {
                cleanup_func (state,
                              seq_cache->item_cache,
                              NULL,
                              g_array_index (array_, gpointer, j),
                              FALSE);
            }
        }

        if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
            g_array_free (array_, TRUE);
    }

    return NULL;
}

static GArray*
_wrap_c_array (PyGIInvokeState   *state,
               PyGIArgGArray     *array_cache,
               gpointer           data)
{
    GArray *array_;
    gsize   len = 0;

    if (array_cache->fixed_size >= 0) {
        len = array_cache->fixed_size;
    } else if (array_cache->is_zero_terminated) {
        len = g_strv_length ((gchar **)data);
    } else if (array_cache->len_arg_index >= 0) {
        GIArgument *len_arg = &state->arg_values[array_cache->len_arg_index];
        len = len_arg->v_long;
    }

    array_ = g_array_new (FALSE,
                          FALSE,
                          array_cache->item_size);

    if (array_ == NULL)
        return NULL;

    g_free (array_->data);
    array_->data = data;
    array_->len = len;

    return array_;
}

static void
_pygi_marshal_cleanup_to_py_array (PyGIInvokeState *state,
                                   PyGIArgCache    *arg_cache,
                                   PyObject        *dummy,
                                   gpointer         data,
                                   gboolean         was_processed)
{
    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING ||
        arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        GArray *array_ = NULL;
        GPtrArray *ptr_array_ = NULL;
        PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
        PyGIArgGArray *array_cache = (PyGIArgGArray *)arg_cache;

        /* If this isn't a garray create one to help process variable sized
           array elements */
        if (array_cache->array_type == GI_ARRAY_TYPE_C) {
            array_ = _wrap_c_array (state, array_cache, data);

            if (array_ == NULL)
                return;

        } else if (array_cache->array_type == GI_ARRAY_TYPE_PTR_ARRAY) {
            ptr_array_ = (GPtrArray *) data;
        } else {
            array_ = (GArray *) data;
        }

        if (sequence_cache->item_cache->to_py_cleanup != NULL) {
            gsize i;
            guint len = (array_ != NULL) ? array_->len : ptr_array_->len;

            PyGIMarshalCleanupFunc cleanup_func = sequence_cache->item_cache->to_py_cleanup;
            for (i = 0; i < len; i++) {
                cleanup_func (state,
                              sequence_cache->item_cache,
                              NULL,
                              (array_ != NULL) ? g_array_index (array_, gpointer, i) : g_ptr_array_index (ptr_array_, i),
                              was_processed);
            }
        }

        if (array_ != NULL)
            g_array_free (array_, TRUE);
        else
            g_ptr_array_free (ptr_array_, TRUE);
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

PyGIArgCache*
pygi_arg_garray_len_arg_setup (PyGIArgCache *arg_cache,
                               GITypeInfo *type_info,
                               PyGICallableCache *callable_cache,
                               PyGIDirection direction,
                               gssize arg_index,
                               gssize *py_arg_index)
{
    PyGIArgGArray *seq_cache = (PyGIArgGArray *)arg_cache;

    /* attempt len_arg_index setup for the first time */
    if (seq_cache->len_arg_index < 0) {
        seq_cache->len_arg_index = g_type_info_get_array_length (type_info);

        /* offset by self arg for methods and vfuncs */
        if (seq_cache->len_arg_index >= 0 && callable_cache != NULL) {
            seq_cache->len_arg_index += callable_cache->args_offset;
        }
    }

    if (seq_cache->len_arg_index >= 0) {
        PyGIArgCache *child_cache = NULL;

        child_cache = _pygi_callable_cache_get_arg (callable_cache,
                                                    seq_cache->len_arg_index);
        if (child_cache == NULL) {
            child_cache = pygi_arg_cache_alloc ();
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
            if (child_cache->meta_type == PYGI_META_ARG_TYPE_CHILD)
                return child_cache;
        }

        /* There is a length argument for this array, so increment the number
         * of "to python" child arguments when applicable.
         */
        if (direction & PYGI_DIRECTION_TO_PYTHON)
             callable_cache->n_to_py_child_args++;

        child_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
        child_cache->direction = direction;
        child_cache->to_py_marshaller = _pygi_marshal_to_py_basic_type_cache_adapter;
        child_cache->from_py_marshaller = _pygi_marshal_from_py_basic_type_cache_adapter;
        child_cache->py_arg_index = -1;

        /* ugly edge case code:
         *
         * When the length comes before the array parameter we need to update
         * indexes of arguments after the index argument.
         */
        if (seq_cache->len_arg_index < arg_index && direction & PYGI_DIRECTION_FROM_PYTHON) {
            gssize i;
            (*py_arg_index) -= 1;
            callable_cache->n_py_args -= 1;

            for (i = seq_cache->len_arg_index + 1;
                   i < _pygi_callable_cache_args_len (callable_cache); i++) {
                PyGIArgCache *update_cache = _pygi_callable_cache_get_arg (callable_cache, i);
                if (update_cache == NULL)
                    break;

                update_cache->py_arg_index -= 1;
            }
        }

        _pygi_callable_cache_set_arg (callable_cache, seq_cache->len_arg_index, child_cache);
        return child_cache;
    }

    return NULL;
}

static gboolean
pygi_arg_garray_setup (PyGIArgGArray     *sc,
                       GITypeInfo        *type_info,
                       GIArgInfo         *arg_info,    /* may be NULL for return arguments */
                       GITransfer         transfer,
                       PyGIDirection      direction,
                       PyGICallableCache *callable_cache)
{
    GITypeInfo *item_type_info;
    PyGIArgCache *arg_cache = (PyGIArgCache *)sc;

    if (!pygi_arg_sequence_setup ((PyGISequenceCache *)sc,
                                  type_info,
                                  arg_info,
                                  transfer,
                                  direction,
                                  callable_cache)) {
        return FALSE;
    }

    ((PyGIArgCache *)sc)->destroy_notify = (GDestroyNotify)_array_cache_free_func;
    sc->array_type = g_type_info_get_array_type (type_info);
    sc->is_zero_terminated = g_type_info_is_zero_terminated (type_info);
    sc->fixed_size = g_type_info_get_array_fixed_size (type_info);
    sc->len_arg_index = -1;  /* setup by pygi_arg_garray_len_arg_setup */

    item_type_info = g_type_info_get_param_type (type_info, 0);
    sc->item_size = _pygi_g_type_info_size (item_type_info);
    g_base_info_unref ( (GIBaseInfo *)item_type_info);

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
pygi_arg_garray_new_from_info (GITypeInfo *type_info,
                               GIArgInfo *arg_info,
                               GITransfer transfer,
                               PyGIDirection direction,
                               PyGICallableCache *callable_cache)
{
    PyGIArgGArray *array_cache = g_slice_new0 (PyGIArgGArray);
    if (array_cache == NULL)
        return NULL;

    if (!pygi_arg_garray_setup (array_cache,
                                type_info,
                                arg_info,
                                transfer,
                                direction,
                                callable_cache)) {
        pygi_arg_cache_free ( (PyGIArgCache *)array_cache);
        return NULL;
    }

    return (PyGIArgCache *)array_cache;
}

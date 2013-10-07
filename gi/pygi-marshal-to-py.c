/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>,  Red Hat, Inc.
 *
 *   pygi-marshal-from-py.c: functions for converting C types to PyObject
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#include "pygi-private.h"

#include <string.h>
#include <time.h>

#include <pyglib.h>
#include <pygobject.h>
#include <pyglib-python-compat.h>

#include "pygi-cache.h"
#include "pygi-marshal-cleanup.h"
#include "pygi-marshal-to-py.h"
#include "pygi-argument.h"

static gboolean
gi_argument_to_c_long (GIArgument *arg_in,
                       long *c_long_out,
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
          *c_long_out = arg_in->v_uint32;
          return TRUE;
      case GI_TYPE_TAG_INT64:
          *c_long_out = arg_in->v_int64;
          return TRUE;
      case GI_TYPE_TAG_UINT64:
          *c_long_out = arg_in->v_uint64;
          return TRUE;
      default:
          PyErr_Format (PyExc_TypeError,
                        "Unable to marshal %s to C long",
                        g_type_tag_to_string (type_tag));
          return FALSE;
    }
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

PyObject *
_pygi_marshal_to_py_void (PyGIInvokeState   *state,
                          PyGICallableCache *callable_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    if (arg_cache->is_pointer) {
        return PyLong_FromVoidPtr (arg->v_pointer);
    }
    Py_RETURN_NONE;
}

static PyObject *
_pygi_marshal_to_py_unichar (GIArgument *arg)
{
    PyObject *py_obj = NULL;

    /* Preserve the bidirectional mapping between 0 and "" */
    if (arg->v_uint32 == 0) {
        py_obj = PYGLIB_PyUnicode_FromString ("");
    } else if (g_unichar_validate (arg->v_uint32)) {
        gchar utf8[6];
        gint bytes;

        bytes = g_unichar_to_utf8 (arg->v_uint32, utf8);
        py_obj = PYGLIB_PyUnicode_FromStringAndSize ((char*)utf8, bytes);
    } else {
        /* TODO: Convert the error to an exception. */
        PyErr_Format (PyExc_TypeError,
                      "Invalid unicode codepoint %" G_GUINT32_FORMAT,
                      arg->v_uint32);
    }

    return py_obj;
}

static PyObject *
_pygi_marshal_to_py_utf8 (GIArgument *arg)
{
    PyObject *py_obj = NULL;
    if (arg->v_string == NULL) {
        Py_RETURN_NONE;
     }

    py_obj = PYGLIB_PyUnicode_FromString (arg->v_string);
    return py_obj;
}

static PyObject *
_pygi_marshal_to_py_filename (GIArgument *arg)
{
    gchar *string = NULL;
    PyObject *py_obj = NULL;
    GError *error = NULL;

    if (arg->v_string == NULL) {
        Py_RETURN_NONE;
    }

    string = g_filename_to_utf8 (arg->v_string, -1, NULL, NULL, &error);
    if (string == NULL) {
        PyErr_SetString (PyExc_Exception, error->message);
        /* TODO: Convert the error to an exception. */
        return NULL;
    }

    py_obj = PYGLIB_PyUnicode_FromString (string);
    g_free (string);

    return py_obj;
}


/**
 * _pygi_marshal_to_py_basic_type:
 * @arg: The argument to convert to an object.
 * @type_tag: Type tag for @arg
 * @transfer: Transfer annotation
 *
 * Convert the given argument to a Python object. This function
 * is restricted to simple types that only require the GITypeTag
 * and GITransfer. For a more complete conversion routine, use:
 * _pygi_argument_to_object.
 *
 * Returns: A PyObject representing @arg or NULL if it cannot convert
 *          the argument.
 */
PyObject *
_pygi_marshal_to_py_basic_type (GIArgument  *arg,
                                 GITypeTag type_tag,
                                 GITransfer transfer)
{
    switch (type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
            return PyBool_FromLong (arg->v_boolean);

        case GI_TYPE_TAG_INT8:
            return PYGLIB_PyLong_FromLong (arg->v_int8);

        case GI_TYPE_TAG_UINT8:
            return PYGLIB_PyLong_FromLong (arg->v_uint8);

        case GI_TYPE_TAG_INT16:
            return PYGLIB_PyLong_FromLong (arg->v_int16);

        case GI_TYPE_TAG_UINT16:
            return PYGLIB_PyLong_FromLong (arg->v_uint16);

        case GI_TYPE_TAG_INT32:
            return PYGLIB_PyLong_FromLong (arg->v_int32);

        case GI_TYPE_TAG_UINT32:
            return PyLong_FromLongLong (arg->v_uint32);

        case GI_TYPE_TAG_INT64:
            return PyLong_FromLongLong (arg->v_int64);

        case GI_TYPE_TAG_UINT64:
            return PyLong_FromUnsignedLongLong (arg->v_uint64);

        case GI_TYPE_TAG_FLOAT:
            return PyFloat_FromDouble (arg->v_float);

        case GI_TYPE_TAG_DOUBLE:
            return PyFloat_FromDouble (arg->v_double);

        case GI_TYPE_TAG_GTYPE:
            return pyg_type_wrapper_new ( (GType) arg->v_long);

        case GI_TYPE_TAG_UNICHAR:
            return _pygi_marshal_to_py_unichar (arg);

        case GI_TYPE_TAG_UTF8:
            return _pygi_marshal_to_py_utf8 (arg);

        case GI_TYPE_TAG_FILENAME:
            return _pygi_marshal_to_py_filename (arg);

        default:
            return NULL;
    }
    return NULL;
}

PyObject *
_pygi_marshal_to_py_basic_type_cache_adapter (PyGIInvokeState   *state,
                                              PyGICallableCache *callable_cache,
                                              PyGIArgCache      *arg_cache,
                                              GIArgument        *arg)
{
    return _pygi_marshal_to_py_basic_type (arg,
                                            arg_cache->type_tag,
                                            arg_cache->transfer);
}

PyObject *
_pygi_marshal_to_py_array (PyGIInvokeState   *state,
                           PyGICallableCache *callable_cache,
                           PyGIArgCache      *arg_cache,
                           GIArgument        *arg)
{
    GArray *array_;
    PyObject *py_obj = NULL;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;
    gsize processed_items = 0;

     /* GArrays make it easier to iterate over arrays
      * with different element sizes but requires that
      * we allocate a GArray if the argument was a C array
      */
    if (seq_cache->array_type == GI_ARRAY_TYPE_C) {
        gsize len;
        if (seq_cache->fixed_size >= 0) {
            g_assert(arg->v_pointer != NULL);
            len = seq_cache->fixed_size;
        } else if (seq_cache->is_zero_terminated) {
            if (arg->v_pointer == NULL) {
                len = 0;
            } else if (seq_cache->item_cache->type_tag == GI_TYPE_TAG_UINT8) {
                len = strlen (arg->v_pointer);
            } else {
                len = g_strv_length ((gchar **)arg->v_pointer);
            }
        } else {
            GIArgument *len_arg = state->args[seq_cache->len_arg_index];
            PyGIArgCache *arg_cache = _pygi_callable_cache_get_arg (callable_cache,
                                                                    seq_cache->len_arg_index);

            if (!gi_argument_to_gsize (len_arg, &len, arg_cache->type_tag)) {
                return NULL;
            }
        }

        array_ = g_array_new (FALSE,
                              FALSE,
                              seq_cache->item_size);
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
                if (seq_cache->array_type == GI_ARRAY_TYPE_PTR_ARRAY) {
                    item_arg.v_pointer = g_ptr_array_index ( ( GPtrArray *)array_, i);

                } else if (item_arg_cache->is_pointer) {
                    item_arg.v_pointer = g_array_index (array_, gpointer, i);

                } else if (item_arg_cache->type_tag == GI_TYPE_TAG_INTERFACE) {
                    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *) item_arg_cache;

                    // FIXME: This probably doesn't work with boxed types or gvalues. See fx. _pygi_marshal_from_py_array()
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

                    if (seq_cache->array_type == GI_ARRAY_TYPE_C)
                        g_array_unref (array_);

                    goto err;
                }
                PyList_SET_ITEM (py_obj, i, py_item);
                processed_items++;
            }
        }
    }

    if (seq_cache->array_type == GI_ARRAY_TYPE_C)
        g_array_free (array_, FALSE);

    return py_obj;

err:
    if (seq_cache->array_type == GI_ARRAY_TYPE_C) {
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

PyObject *
_pygi_marshal_to_py_glist (PyGIInvokeState   *state,
                           PyGICallableCache *callable_cache,
                           PyGIArgCache      *arg_cache,
                           GIArgument        *arg)
{
    GList *list_;
    gsize length;
    gsize i;

    PyGIMarshalToPyFunc item_to_py_marshaller;
    PyGIArgCache *item_arg_cache;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;

    PyObject *py_obj = NULL;

    list_ = arg->v_pointer;
    length = g_list_length (list_);

    py_obj = PyList_New (length);
    if (py_obj == NULL)
        return NULL;

    item_arg_cache = seq_cache->item_cache;
    item_to_py_marshaller = item_arg_cache->to_py_marshaller;

    for (i = 0; list_ != NULL; list_ = g_list_next (list_), i++) {
        GIArgument item_arg;
        PyObject *py_item;

        item_arg.v_pointer = list_->data;
        _pygi_hash_pointer_to_arg (&item_arg, item_arg_cache->type_tag);
        py_item = item_to_py_marshaller (state,
                                         callable_cache,
                                         item_arg_cache,
                                         &item_arg);

        if (py_item == NULL) {
            Py_CLEAR (py_obj);
            _PyGI_ERROR_PREFIX ("Item %zu: ", i);
            return NULL;
        }

        PyList_SET_ITEM (py_obj, i, py_item);
    }

    return py_obj;
}

PyObject *
_pygi_marshal_to_py_gslist (PyGIInvokeState   *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache      *arg_cache,
                            GIArgument        *arg)
{
    GSList *list_;
    gsize length;
    gsize i;

    PyGIMarshalToPyFunc item_to_py_marshaller;
    PyGIArgCache *item_arg_cache;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;

    PyObject *py_obj = NULL;

    list_ = arg->v_pointer;
    length = g_slist_length (list_);

    py_obj = PyList_New (length);
    if (py_obj == NULL)
        return NULL;

    item_arg_cache = seq_cache->item_cache;
    item_to_py_marshaller = item_arg_cache->to_py_marshaller;

    for (i = 0; list_ != NULL; list_ = g_slist_next (list_), i++) {
        GIArgument item_arg;
        PyObject *py_item;

        item_arg.v_pointer = list_->data;
        _pygi_hash_pointer_to_arg (&item_arg, item_arg_cache->type_tag);
        py_item = item_to_py_marshaller (state,
                                        callable_cache,
                                        item_arg_cache,
                                        &item_arg);

        if (py_item == NULL) {
            Py_CLEAR (py_obj);
            _PyGI_ERROR_PREFIX ("Item %zu: ", i);
            return NULL;
        }

        PyList_SET_ITEM (py_obj, i, py_item);
    }

    return py_obj;
}

PyObject *
_pygi_marshal_to_py_ghash (PyGIInvokeState   *state,
                           PyGICallableCache *callable_cache,
                           PyGIArgCache      *arg_cache,
                           GIArgument        *arg)
{
    GHashTable *hash_;
    GHashTableIter hash_table_iter;

    PyGIMarshalToPyFunc key_to_py_marshaller;
    PyGIMarshalToPyFunc value_to_py_marshaller;

    PyGIArgCache *key_arg_cache;
    PyGIArgCache *value_arg_cache;
    PyGIHashCache *hash_cache = (PyGIHashCache *)arg_cache;

    GIArgument key_arg;
    GIArgument value_arg;

    PyObject *py_obj = NULL;

    hash_ = arg->v_pointer;

    if (hash_ == NULL) {
        py_obj = Py_None;
        Py_INCREF (py_obj);
        return py_obj;
    }

    py_obj = PyDict_New ();
    if (py_obj == NULL)
        return NULL;

    key_arg_cache = hash_cache->key_cache;
    key_to_py_marshaller = key_arg_cache->to_py_marshaller;

    value_arg_cache = hash_cache->value_cache;
    value_to_py_marshaller = value_arg_cache->to_py_marshaller;

    g_hash_table_iter_init (&hash_table_iter, hash_);
    while (g_hash_table_iter_next (&hash_table_iter,
                                   &key_arg.v_pointer,
                                   &value_arg.v_pointer)) {
        PyObject *py_key;
        PyObject *py_value;
        int retval;


        _pygi_hash_pointer_to_arg (&key_arg, hash_cache->key_cache->type_tag);
        py_key = key_to_py_marshaller ( state,
                                      callable_cache,
                                      key_arg_cache,
                                     &key_arg);

        if (py_key == NULL) {
            Py_CLEAR (py_obj);
            return NULL;
        }

        _pygi_hash_pointer_to_arg (&value_arg, hash_cache->value_cache->type_tag);
        py_value = value_to_py_marshaller ( state,
                                          callable_cache,
                                          value_arg_cache,
                                         &value_arg);

        if (py_value == NULL) {
            Py_CLEAR (py_obj);
            Py_DECREF(py_key);
            return NULL;
        }

        retval = PyDict_SetItem (py_obj, py_key, py_value);

        Py_DECREF (py_key);
        Py_DECREF (py_value);

        if (retval < 0) {
            Py_CLEAR (py_obj);
            return NULL;
        }
    }

    return py_obj;
}

PyObject *
_pygi_marshal_to_py_gerror (PyGIInvokeState   *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache      *arg_cache,
                            GIArgument        *arg)
{
    GError *error = arg->v_pointer;
    PyObject *py_obj = NULL;

    py_obj = pyglib_error_marshal(&error);

    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING && error != NULL) {
        g_error_free (error);
    }

    if (py_obj != NULL) {
        return py_obj;
    } else {
        Py_RETURN_NONE;
    }
}

PyObject *
_pygi_marshal_to_py_interface_callback (PyGIInvokeState   *state,
                                        PyGICallableCache *callable_cache,
                                        PyGIArgCache      *arg_cache,
                                        GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling a callback to PyObject is not supported");
    return py_obj;
}

PyObject *
_pygi_marshal_to_py_interface_enum (PyGIInvokeState   *state,
                                    PyGICallableCache *callable_cache,
                                    PyGIArgCache      *arg_cache,
                                    GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIBaseInfo *interface;
    long c_long;

    interface = g_type_info_get_interface (arg_cache->type_info);
    g_assert (g_base_info_get_type (interface) == GI_INFO_TYPE_ENUM);

    if (!gi_argument_to_c_long(arg, &c_long,
                               g_enum_info_get_storage_type ((GIEnumInfo *)interface))) {
        return NULL;
    }

    if (iface_cache->g_type == G_TYPE_NONE) {
        py_obj = PyObject_CallFunction (iface_cache->py_type, "l", c_long);
    } else {
        py_obj = pyg_enum_from_gtype (iface_cache->g_type, c_long);
    }
    g_base_info_unref (interface);
    return py_obj;
}

PyObject *
_pygi_marshal_to_py_interface_flags (PyGIInvokeState   *state,
                                     PyGICallableCache *callable_cache,
                                     PyGIArgCache      *arg_cache,
                                     GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIBaseInfo *interface;
    long c_long;

    interface = g_type_info_get_interface (arg_cache->type_info);
    g_assert (g_base_info_get_type (interface) == GI_INFO_TYPE_FLAGS);

    if (!gi_argument_to_c_long(arg, &c_long,
                               g_enum_info_get_storage_type ((GIEnumInfo *)interface))) {
        g_base_info_unref (interface);
        return NULL;
    }

    g_base_info_unref (interface);
    if (iface_cache->g_type == G_TYPE_NONE) {
        /* An enum with a GType of None is an enum without GType */

        PyObject *py_type = _pygi_type_import_by_gi_info (iface_cache->interface_info);
        PyObject *py_args = NULL;

        if (!py_type)
            return NULL;

        py_args = PyTuple_New (1);
        if (PyTuple_SetItem (py_args, 0, PyLong_FromLong (c_long)) != 0) {
            Py_DECREF (py_args);
            Py_DECREF (py_type);
            return NULL;
        }

        py_obj = PyObject_CallFunction (py_type, "l", c_long);

        Py_DECREF (py_args);
        Py_DECREF (py_type);
    } else {
        py_obj = pyg_flags_from_gtype (iface_cache->g_type, c_long);
    }

    return py_obj;
}

PyObject *
_pygi_marshal_to_py_interface_struct_cache_adapter (PyGIInvokeState   *state,
                                                    PyGICallableCache *callable_cache,
                                                    PyGIArgCache      *arg_cache,
                                                    GIArgument        *arg)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    return _pygi_marshal_to_py_interface_struct (arg,
                                                 iface_cache->interface_info,
                                                 iface_cache->g_type,
                                                 iface_cache->py_type,
                                                 arg_cache->transfer,
                                                 arg_cache->is_caller_allocates,
                                                 iface_cache->is_foreign);
}

PyObject *
_pygi_marshal_to_py_interface_interface (PyGIInvokeState   *state,
                                         PyGICallableCache *callable_cache,
                                         PyGIArgCache      *arg_cache,
                                         GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for this type is not implemented yet");
    return py_obj;
}

PyObject *
_pygi_marshal_to_py_interface_boxed (PyGIInvokeState   *state,
                                     PyGICallableCache *callable_cache,
                                     PyGIArgCache      *arg_cache,
                                     GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for this type is not implemented yet");
    return py_obj;
}

PyObject *
_pygi_marshal_to_py_interface_object_cache_adapter (PyGIInvokeState   *state,
                                                    PyGICallableCache *callable_cache,
                                                    PyGIArgCache      *arg_cache,
                                                    GIArgument        *arg)
{
    return _pygi_marshal_to_py_object(arg, arg_cache->transfer);
}

PyObject *
_pygi_marshal_to_py_interface_union  (PyGIInvokeState   *state,
                                      PyGICallableCache *callable_cache,
                                      PyGIArgCache      *arg_cache,
                                      GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for this type is not implemented yet");
    return py_obj;
}

PyObject *
_pygi_marshal_to_py_object (GIArgument *arg, GITransfer transfer) {
    PyObject *pyobj;

    if (arg->v_pointer == NULL) {
        pyobj = Py_None;
        Py_INCREF (pyobj);

    } else if (G_IS_PARAM_SPEC(arg->v_pointer)) {
        pyobj = pyg_param_spec_new (arg->v_pointer);
        if (transfer == GI_TRANSFER_EVERYTHING)
            g_param_spec_unref (arg->v_pointer);

    } else {
         pyobj = pygobject_new_full (arg->v_pointer,
                                     /*steal=*/ transfer == GI_TRANSFER_EVERYTHING,
                                     /*type=*/  NULL);
    }

    return pyobj;
}

PyObject *
_pygi_marshal_to_py_interface_struct (GIArgument *arg,
                                      GIInterfaceInfo *interface_info,
                                      GType g_type,
                                      PyObject *py_type,
                                      GITransfer transfer,
                                      gboolean is_allocated,
                                      gboolean is_foreign)
{
    PyObject *py_obj = NULL;

    if (arg->v_pointer == NULL) {
        Py_RETURN_NONE;
    }

    if (g_type_is_a (g_type, G_TYPE_VALUE)) {
        py_obj = pyg_value_as_pyobject (arg->v_pointer, FALSE);
    } else if (is_foreign) {
        py_obj = pygi_struct_foreign_convert_from_g_argument (interface_info,
                                                              arg->v_pointer);
    } else if (g_type_is_a (g_type, G_TYPE_BOXED)) {
        if (py_type) {
            py_obj = _pygi_boxed_new ((PyTypeObject *) py_type,
                                      arg->v_pointer,
                                      transfer == GI_TRANSFER_EVERYTHING || is_allocated,
                                      is_allocated ?
                                              g_struct_info_get_size(interface_info) : 0);
        }
    } else if (g_type_is_a (g_type, G_TYPE_POINTER)) {
        if (py_type == NULL ||
                !PyType_IsSubtype ((PyTypeObject *) py_type, &PyGIStruct_Type)) {
            g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
            py_obj = pyg_pointer_new (g_type, arg->v_pointer);
        } else {
            py_obj = _pygi_struct_new ( (PyTypeObject *) py_type,
                                       arg->v_pointer,
                                       transfer == GI_TRANSFER_EVERYTHING);
        }
    } else if (g_type_is_a (g_type, G_TYPE_VARIANT)) {
        /* Note: sink the variant (add a ref) only if we are not transfered ownership.
         * GLib.Variant overrides __del__ which will then call "g_variant_unref" for
         * cleanup in either case. */
        if (py_type) {
            if (transfer == GI_TRANSFER_NOTHING) {
                g_variant_ref_sink (arg->v_pointer);
            }
            py_obj = _pygi_struct_new ((PyTypeObject *) py_type,
                                       arg->v_pointer,
                                       FALSE);
        }
    } else if (g_type == G_TYPE_NONE) {
        if (py_type) {
            py_obj = _pygi_struct_new ((PyTypeObject *) py_type,
                                       arg->v_pointer,
                                       transfer == GI_TRANSFER_EVERYTHING);
        }
    } else {
        PyErr_Format (PyExc_NotImplementedError,
                      "structure type '%s' is not supported yet",
                      g_type_name (g_type));
    }

    return py_obj;
}

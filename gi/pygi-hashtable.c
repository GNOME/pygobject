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

#include "pygi-hashtable.h"
#include "pygi-argument.h"
#include "pygi-private.h"

typedef struct _PyGIHashCache
{
    PyGIArgCache arg_cache;
    PyGIArgCache *key_cache;
    PyGIArgCache *value_cache;
} PyGIHashCache;


static void
_hash_cache_free_func (PyGIHashCache *cache)
{
    if (cache != NULL) {
        pygi_arg_cache_free (cache->key_cache);
        pygi_arg_cache_free (cache->value_cache);
        g_slice_free (PyGIHashCache, cache);
    }
}

static gboolean
_pygi_marshal_from_py_ghash (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg,
                             gpointer          *cleanup_data)
{
    PyGIMarshalFromPyFunc key_from_py_marshaller;
    PyGIMarshalFromPyFunc value_from_py_marshaller;

    int i;
    Py_ssize_t length;
    PyObject *py_keys, *py_values;

    GHashFunc hash_func;
    GEqualFunc equal_func;

    GHashTable *hash_ = NULL;
    PyGIHashCache *hash_cache = (PyGIHashCache *)arg_cache;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    py_keys = PyMapping_Keys (py_arg);
    if (py_keys == NULL) {
        PyErr_Format (PyExc_TypeError, "Must be mapping, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    length = PyMapping_Length (py_arg);
    if (length < 0) {
        Py_DECREF (py_keys);
        return FALSE;
    }

    py_values = PyMapping_Values (py_arg);
    if (py_values == NULL) {
        Py_DECREF (py_keys);
        return FALSE;
    }

    key_from_py_marshaller = hash_cache->key_cache->from_py_marshaller;
    value_from_py_marshaller = hash_cache->value_cache->from_py_marshaller;

    switch (hash_cache->key_cache->type_tag) {
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
            hash_func = g_str_hash;
            equal_func = g_str_equal;
            break;
        default:
            hash_func = NULL;
            equal_func = NULL;
    }

    hash_ = g_hash_table_new (hash_func, equal_func);
    if (hash_ == NULL) {
        PyErr_NoMemory ();
        Py_DECREF (py_keys);
        Py_DECREF (py_values);
        return FALSE;
    }

    for (i = 0; i < length; i++) {
        GIArgument key, value;
        gpointer key_cleanup_data = NULL;
        gpointer value_cleanup_data = NULL;
        PyObject *py_key = PyList_GET_ITEM (py_keys, i);
        PyObject *py_value = PyList_GET_ITEM (py_values, i);
        if (py_key == NULL || py_value == NULL)
            goto err;

        if (!key_from_py_marshaller ( state,
                                      callable_cache,
                                      hash_cache->key_cache,
                                      py_key,
                                     &key,
                                     &key_cleanup_data))
            goto err;

        if (!value_from_py_marshaller ( state,
                                        callable_cache,
                                        hash_cache->value_cache,
                                        py_value,
                                       &value,
                                       &value_cleanup_data))
            goto err;

        g_hash_table_insert (hash_,
                             _pygi_arg_to_hash_pointer (&key, hash_cache->key_cache->type_tag),
                             _pygi_arg_to_hash_pointer (&value, hash_cache->value_cache->type_tag));
        continue;
err:
        /* FIXME: cleanup hash keys and values */
        Py_XDECREF (py_key);
        Py_XDECREF (py_value);
        Py_DECREF (py_keys);
        Py_DECREF (py_values);
        g_hash_table_unref (hash_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = hash_;

    if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
        /* Free everything in cleanup. */
        *cleanup_data = arg->v_pointer;
    } else if (arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        /* Make a shallow copy so we can free the elements later in cleanup
         * because it is possible invoke will free the list before our cleanup. */
        *cleanup_data = g_hash_table_ref (arg->v_pointer);
    } else { /* GI_TRANSFER_EVERYTHING */
        /* No cleanup, everything is given to the callee.
         * Note that the keys and values will leak for transfer everything because
         * we do not use g_hash_table_new_full and set key/value_destroy_func. */
        *cleanup_data = NULL;
    }

    return TRUE;
}

static void
_pygi_marshal_cleanup_from_py_ghash  (PyGIInvokeState *state,
                                      PyGIArgCache    *arg_cache,
                                      PyObject        *py_arg,
                                      gpointer         data,
                                      gboolean         was_processed)
{
    if (data == NULL)
        return;

    if (was_processed) {
        GHashTable *hash_;
        PyGIHashCache *hash_cache = (PyGIHashCache *)arg_cache;

        hash_ = (GHashTable *)data;

        /* clean up keys and values first */
        if (hash_cache->key_cache->from_py_cleanup != NULL ||
                hash_cache->value_cache->from_py_cleanup != NULL) {
            GHashTableIter hiter;
            gpointer key;
            gpointer value;

            PyGIMarshalCleanupFunc key_cleanup_func =
                hash_cache->key_cache->from_py_cleanup;
            PyGIMarshalCleanupFunc value_cleanup_func =
                hash_cache->value_cache->from_py_cleanup;

            g_hash_table_iter_init (&hiter, hash_);
            while (g_hash_table_iter_next (&hiter, &key, &value)) {
                if (key != NULL && key_cleanup_func != NULL)
                    key_cleanup_func (state,
                                      hash_cache->key_cache,
                                      NULL,
                                      key,
                                      TRUE);
                if (value != NULL && value_cleanup_func != NULL)
                    value_cleanup_func (state,
                                        hash_cache->value_cache,
                                        NULL,
                                        value,
                                        TRUE);
            }
        }

        g_hash_table_unref (hash_);
    }
}

static PyObject *
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

static void
_pygi_marshal_cleanup_to_py_ghash (PyGIInvokeState *state,
                                   PyGIArgCache    *arg_cache,
                                   PyObject        *dummy,
                                   gpointer         data,
                                   gboolean         was_processed)
{
    if (data == NULL)
        return;

    /* assume hashtable has boxed key and value */
    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING || arg_cache->transfer == GI_TRANSFER_CONTAINER)
        g_hash_table_unref ( (GHashTable *)data);
}

static void
_arg_cache_from_py_ghash_setup (PyGIArgCache *arg_cache)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_ghash;
    arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_ghash;
}

static void
_arg_cache_to_py_ghash_setup (PyGIArgCache *arg_cache)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_ghash;
    arg_cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_ghash;
}

static gboolean
pygi_arg_hash_table_setup_from_info (PyGIHashCache      *hc,
                                     GITypeInfo         *type_info,
                                     GIArgInfo          *arg_info,
                                     GITransfer          transfer,
                                     PyGIDirection       direction,
                                     PyGICallableCache  *callable_cache)
{
    GITypeInfo *key_type_info;
    GITypeInfo *value_type_info;
    GITransfer item_transfer;

    if (!pygi_arg_base_setup ((PyGIArgCache *)hc, type_info, arg_info, transfer, direction))
        return FALSE;

    ( (PyGIArgCache *)hc)->destroy_notify = (GDestroyNotify)_hash_cache_free_func;
    key_type_info = g_type_info_get_param_type (type_info, 0);
    value_type_info = g_type_info_get_param_type (type_info, 1);

    item_transfer =
        transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

    hc->key_cache = pygi_arg_cache_new (key_type_info,
                                        NULL,
                                        item_transfer,
                                        direction,
                                        callable_cache,
                                        0, 0);

    if (hc->key_cache == NULL) {
        return FALSE;
    }

    hc->value_cache = pygi_arg_cache_new (value_type_info,
                                          NULL,
                                          item_transfer,
                                          direction,
                                          callable_cache,
                                          0, 0);

    if (hc->value_cache == NULL) {
        return FALSE;
    }

    g_base_info_unref( (GIBaseInfo *)key_type_info);
    g_base_info_unref( (GIBaseInfo *)value_type_info);

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        _arg_cache_from_py_ghash_setup ((PyGIArgCache *)hc);
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        _arg_cache_to_py_ghash_setup ((PyGIArgCache *)hc);
    }

    return TRUE;
}

PyGIArgCache *
pygi_arg_hash_table_new_from_info (GITypeInfo         *type_info,
                                   GIArgInfo          *arg_info,
                                   GITransfer          transfer,
                                   PyGIDirection       direction,
                                   PyGICallableCache  *callable_cache)
{
    gboolean res = FALSE;
    PyGIHashCache *hc = NULL;

    hc = g_slice_new0 (PyGIHashCache);
    if (hc == NULL)
        return NULL;

    res = pygi_arg_hash_table_setup_from_info (hc,
                                               type_info,
                                               arg_info,
                                               transfer,
                                               direction,
                                               callable_cache);
    if (res) {
        return (PyGIArgCache *)hc;
    } else {
        pygi_arg_cache_free ((PyGIArgCache *)hc);
        return NULL;
    }
}

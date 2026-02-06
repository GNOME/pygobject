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

#include "pygi-argument.h"
#include "pygi-util.h"
#include "pygi-cache-private.h"

typedef PyGISequenceCache PyGIArgGList;

/*
 * GList and GSList from Python
 */
static gboolean
_pygi_marshal_from_py_glist (PyGIInvokeState *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache *arg_cache, PyObject *py_arg,
                             GIArgument *arg,
                             PyGIMarshalCleanupData *cleanup_data)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
    Py_ssize_t i, py_length;
    guint length;
    GList *list_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
    GArray *item_cleanups = NULL;
    PyGIMarshalCleanupData list_cleanup_data = { 0 };

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

    if (arg_cache->transfer != GI_TRANSFER_EVERYTHING) {
        item_cleanups = g_array_sized_new (
            FALSE, TRUE, sizeof (PyGIMarshalCleanupData), length + 1);
        pygi_marshal_cleanup_data_init_full (
            cleanup_data, item_cleanups,
            (GDestroyNotify)pygi_marshal_cleanup_data_destroy_array,
            (GDestroyNotify)pygi_marshal_cleanup_data_destroy_array_failed);
    }

    from_py_marshaller = sequence_cache->item_cache->from_py_marshaller;
    for (i = 0; i < py_length; i++) {
        GIArgument item = PYGI_ARG_INIT;
        PyGIMarshalCleanupData item_cleanup_data = { 0 };
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL) goto err;

        if (!from_py_marshaller (state, callable_cache,
                                 sequence_cache->item_cache, py_item, &item,
                                 &item_cleanup_data))
            goto err;

        if (item_cleanups)
            g_array_append_val (item_cleanups, item_cleanup_data);

        Py_DECREF (py_item);
        list_ = g_list_prepend (
            list_, _pygi_arg_to_hash_pointer (
                       item, sequence_cache->item_cache->type_info));
        continue;
err:
        /* FIXME: clean up list
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            PyGIMarshalFromPyCleanupFunc cleanup = sequence_cache->item_cache->from_py_cleanup;
        }
        */
        Py_XDECREF (py_item);
        g_list_free (list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = g_list_reverse (list_);

    pygi_marshal_cleanup_data_init_full (
        &list_cleanup_data, arg->v_pointer,
        arg_cache->transfer == GI_TRANSFER_NOTHING
            ? (GDestroyNotify)g_list_free
            : NULL,
        (GDestroyNotify)g_list_free);
    g_array_append_val (item_cleanups, list_cleanup_data);

    return TRUE;
}


static gboolean
_pygi_marshal_from_py_gslist (PyGIInvokeState *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache *arg_cache, PyObject *py_arg,
                              GIArgument *arg,
                              PyGIMarshalCleanupData *cleanup_data)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
    Py_ssize_t i, py_length;
    guint length;
    GSList *list_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
    GArray *item_cleanups = NULL;
    PyGIMarshalCleanupData list_cleanup_data = { 0 };

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

    if (arg_cache->transfer != GI_TRANSFER_EVERYTHING) {
        item_cleanups = g_array_sized_new (
            FALSE, TRUE, sizeof (PyGIMarshalCleanupData), length + 1);
        pygi_marshal_cleanup_data_init_full (
            cleanup_data, item_cleanups,
            (GDestroyNotify)pygi_marshal_cleanup_data_destroy_array,
            (GDestroyNotify)pygi_marshal_cleanup_data_destroy_array_failed);
    }

    from_py_marshaller = sequence_cache->item_cache->from_py_marshaller;
    for (i = 0; i < py_length; i++) {
        GIArgument item = { 0 };
        PyGIMarshalCleanupData item_cleanup_data = { 0 };
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL) goto err;

        if (!from_py_marshaller (state, callable_cache,
                                 sequence_cache->item_cache, py_item, &item,
                                 &item_cleanup_data))
            goto err;

        if (item_cleanups)
            g_array_append_val (item_cleanups, item_cleanup_data);

        Py_DECREF (py_item);
        list_ = g_slist_prepend (
            list_, _pygi_arg_to_hash_pointer (
                       item, sequence_cache->item_cache->type_info));
        continue;
err:
        /* FIXME: Clean up list
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            PyGIMarshalFromPyCleanupFunc cleanup = sequence_cache->item_cache->from_py_cleanup;
        }
        */

        Py_XDECREF (py_item);
        g_slist_free (list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = g_slist_reverse (list_);

    pygi_marshal_cleanup_data_init_full (
        &list_cleanup_data, arg->v_pointer,
        arg_cache->transfer == GI_TRANSFER_NOTHING
            ? (GDestroyNotify)g_slist_free
            : NULL,
        (GDestroyNotify)g_slist_free);
    g_array_append_val (item_cleanups, list_cleanup_data);

    return TRUE;
}

/*
 * GList and GSList to Python
 */
static PyObject *
_pygi_marshal_to_py_glist (PyGIInvokeState *state,
                           PyGICallableCache *callable_cache,
                           PyGIArgCache *arg_cache, GIArgument *arg,
                           PyGIMarshalCleanupData *cleanup_data)
{
    GList *list_;
    guint length;
    guint i;
    GArray *item_cleanups;

    PyGIMarshalToPyFunc item_to_py_marshaller;
    PyGIArgCache *item_arg_cache;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;
    PyGIMarshalCleanupData list_cleanup_data = { 0 };

    PyObject *py_obj = NULL;

    list_ = arg->v_pointer;
    length = g_list_length (list_);

    py_obj = PyList_New (length);
    if (py_obj == NULL) return NULL;

    // Last item is for the list itself
    item_cleanups = g_array_sized_new (
        FALSE, TRUE, sizeof (PyGIMarshalCleanupData), length + 1);

    item_arg_cache = seq_cache->item_cache;
    item_to_py_marshaller = item_arg_cache->to_py_marshaller;

    for (i = 0; list_ != NULL; list_ = g_list_next (list_), i++) {
        GIArgument item_arg;
        PyObject *py_item;
        PyGIMarshalCleanupData item_cleanup_data = { 0 };

        item_arg.v_pointer = list_->data;
        _pygi_hash_pointer_to_arg_in_place (&item_arg,
                                            item_arg_cache->type_info);
        py_item = item_to_py_marshaller (state, callable_cache, item_arg_cache,
                                         &item_arg, &item_cleanup_data);
        g_array_append_val (item_cleanups, item_cleanup_data);

        if (py_item == NULL) {
            Py_CLEAR (py_obj);
            _PyGI_ERROR_PREFIX ("Item %u: ", i);
            g_array_unref (item_cleanups);
            return NULL;
        }

        PyList_SET_ITEM (py_obj, i, py_item);
    }

    pygi_marshal_cleanup_data_init_full (
        &list_cleanup_data, arg->v_pointer,
        arg_cache->transfer != GI_TRANSFER_NOTHING
            ? (GDestroyNotify)g_list_free
            : NULL,
        (GDestroyNotify)g_list_free);
    g_array_append_val (item_cleanups, list_cleanup_data);

    pygi_marshal_cleanup_data_init_full (
        cleanup_data, item_cleanups,
        (GDestroyNotify)pygi_marshal_cleanup_data_destroy_array,
        (GDestroyNotify)pygi_marshal_cleanup_data_destroy_array_failed);


    return py_obj;
}

static PyObject *
_pygi_marshal_to_py_gslist (PyGIInvokeState *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache *arg_cache, GIArgument *arg,
                            PyGIMarshalCleanupData *cleanup_data)
{
    GSList *list_;
    guint length;
    guint i;
    GArray *item_cleanups;

    PyGIMarshalToPyFunc item_to_py_marshaller;
    PyGIArgCache *item_arg_cache;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;
    PyGIMarshalCleanupData list_cleanup_data = { 0 };

    PyObject *py_obj = NULL;

    list_ = arg->v_pointer;
    length = g_slist_length (list_);

    py_obj = PyList_New (length);
    if (py_obj == NULL) return NULL;

    // Last item is for the list itself
    item_cleanups = g_array_sized_new (
        FALSE, TRUE, sizeof (PyGIMarshalCleanupData), length + 1);

    item_arg_cache = seq_cache->item_cache;
    item_to_py_marshaller = item_arg_cache->to_py_marshaller;

    for (i = 0; list_ != NULL; list_ = g_slist_next (list_), i++) {
        GIArgument item_arg;
        PyObject *py_item;
        PyGIMarshalCleanupData item_cleanup_data = { 0 };

        item_arg.v_pointer = list_->data;
        _pygi_hash_pointer_to_arg_in_place (&item_arg,
                                            item_arg_cache->type_info);
        py_item = item_to_py_marshaller (state, callable_cache, item_arg_cache,
                                         &item_arg, &item_cleanup_data);
        g_array_append_val (item_cleanups, item_cleanup_data);

        if (py_item == NULL) {
            Py_CLEAR (py_obj);
            _PyGI_ERROR_PREFIX ("Item %u: ", i);
            g_array_unref (item_cleanups);
            return NULL;
        }

        PyList_SET_ITEM (py_obj, i, py_item);
    }

    pygi_marshal_cleanup_data_init_full (
        &list_cleanup_data, arg->v_pointer,
        arg_cache->transfer != GI_TRANSFER_NOTHING
            ? (GDestroyNotify)g_slist_free
            : NULL,
        (GDestroyNotify)g_slist_free);
    g_array_append_val (item_cleanups, list_cleanup_data);

    pygi_marshal_cleanup_data_init_full (
        cleanup_data, item_cleanups,
        (GDestroyNotify)pygi_marshal_cleanup_data_destroy_array,
        (GDestroyNotify)pygi_marshal_cleanup_data_destroy_array_failed);


    return py_obj;
}

/*
 * GList/GSList Interface
 */

PyGIArgCache *
pygi_arg_glist_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                              GITransfer transfer, PyGIDirection direction,
                              PyGICallableCache *callable_cache)
{
    PyGIArgCache *arg_cache = (PyGIArgCache *)g_slice_new0 (PyGIArgGList);
    if (arg_cache == NULL) return NULL;

    if (!pygi_arg_sequence_setup ((PyGISequenceCache *)arg_cache, type_info,
                                  arg_info, transfer, direction,
                                  callable_cache)) {
        pygi_arg_cache_free (arg_cache);
        return NULL;
    }

    if (direction & PYGI_DIRECTION_FROM_PYTHON)
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_glist;

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_glist;

    return arg_cache;
}

PyGIArgCache *
pygi_arg_gslist_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                               GITransfer transfer, PyGIDirection direction,
                               PyGICallableCache *callable_cache)
{
    PyGIArgCache *arg_cache = (PyGIArgCache *)g_slice_new0 (PyGIArgGList);
    if (arg_cache == NULL) return NULL;

    if (!pygi_arg_sequence_setup ((PyGISequenceCache *)arg_cache, type_info,
                                  arg_info, transfer, direction,
                                  callable_cache)) {
        pygi_arg_cache_free (arg_cache);
        return NULL;
    }

    if (direction & PYGI_DIRECTION_FROM_PYTHON)
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_gslist;

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_gslist;

    return arg_cache;
}

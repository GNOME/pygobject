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
                             GIArgument *arg, MarshalCleanupData *cleanup_data)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
    int i;
    Py_ssize_t length;
    GList *list_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
    GArray *item_cleanups;

    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!PySequence_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be sequence, not %s",
                      Py_TYPE (py_arg)->tp_name);
        return FALSE;
    }

    length = PySequence_Length (py_arg);
    if (length < 0) return FALSE;

    item_cleanups = g_array_sized_new (
        FALSE, TRUE, sizeof (MarshalCleanupData), length + 1);
    cleanup_data->data = item_cleanups;
    cleanup_data->destroy = (GDestroyNotify)g_array_unref;

    from_py_marshaller = sequence_cache->item_cache->from_py_marshaller;
    for (i = 0; i < length; i++) {
        GIArgument item = PYGI_ARG_INIT;
        MarshalCleanupData item_cleanup_data = { NULL, NULL };
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL) goto err;

        if (!from_py_marshaller (state, callable_cache,
                                 sequence_cache->item_cache, py_item, &item,
                                 &item_cleanup_data))
            goto err;

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

    switch (arg_cache->transfer) {
    case GI_TRANSFER_NOTHING: {
        /* Free everything in cleanup. */
        MarshalCleanupData list_cleanup_data = {
            .data = arg->v_pointer,
            .destroy = (GDestroyNotify)g_list_free
        };
        g_array_append_val (item_cleanups, list_cleanup_data);
        break;
    }
    case GI_TRANSFER_CONTAINER:
        /* Only the elements need to be deleted. */
        break;
    case GI_TRANSFER_EVERYTHING:
        /* No cleanup, everything is given to the callee. */
        g_array_unref (item_cleanups);
        cleanup_data->data = NULL;
        cleanup_data->destroy = NULL;
        break;
    default:
        g_assert_not_reached ();
    }
    return TRUE;
}


static gboolean
_pygi_marshal_from_py_gslist (PyGIInvokeState *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache *arg_cache, PyObject *py_arg,
                              GIArgument *arg,
                              MarshalCleanupData *cleanup_data)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
    int i;
    Py_ssize_t length;
    GSList *list_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
    GArray *item_cleanups;

    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!PySequence_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be sequence, not %s",
                      Py_TYPE (py_arg)->tp_name);
        return FALSE;
    }

    length = PySequence_Length (py_arg);
    if (length < 0) return FALSE;

    item_cleanups = g_array_sized_new (
        FALSE, TRUE, sizeof (MarshalCleanupData), length + 1);
    cleanup_data->data = item_cleanups;
    cleanup_data->destroy = (GDestroyNotify)g_array_unref;

    from_py_marshaller = sequence_cache->item_cache->from_py_marshaller;
    for (i = 0; i < length; i++) {
        GIArgument item = { 0 };
        MarshalCleanupData item_cleanup_data = { NULL, NULL };
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL) goto err;

        if (!from_py_marshaller (state, callable_cache,
                                 sequence_cache->item_cache, py_item, &item,
                                 &item_cleanup_data))
            goto err;

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

    switch (arg_cache->transfer) {
    case GI_TRANSFER_NOTHING: {
        /* Free everything in cleanup. */
        MarshalCleanupData list_cleanup_data = {
            .data = arg->v_pointer,
            .destroy = (GDestroyNotify)g_slist_free
        };
        g_array_append_val (item_cleanups, list_cleanup_data);
        break;
    }
    case GI_TRANSFER_CONTAINER:
        /* Only the elements need to be deleted. */
        break;
    case GI_TRANSFER_EVERYTHING:
        /* No cleanup, everything is given to the callee. */
        g_array_unref (item_cleanups);
        cleanup_data->data = NULL;
        cleanup_data->destroy = NULL;
        break;
    default:
        g_assert_not_reached ();
    }

    return TRUE;
}

static void
_pygi_marshal_cleanup_from_py_glist (PyGIInvokeState *state,
                                     PyGIArgCache *arg_cache, PyObject *py_arg,
                                     MarshalCleanupData cleanup_data,
                                     gboolean was_processed)
{
    if (was_processed) {
        GArray *item_cleanups = (GArray *)cleanup_data.data;
        guint i;

        for (i = 0; i < item_cleanups->len; i++) {
            MarshalCleanupData *item_cleanup_data =
                &g_array_index (item_cleanups, MarshalCleanupData, i);
            if (item_cleanup_data->destroy && item_cleanup_data->data)
                item_cleanup_data->destroy (item_cleanup_data->data);
        }

        cleanup_data.destroy (cleanup_data.data);
    }
}


/*
 * GList and GSList to Python
 */
static PyObject *
_pygi_marshal_to_py_glist (PyGIInvokeState *state,
                           PyGICallableCache *callable_cache,
                           PyGIArgCache *arg_cache, GIArgument *arg,
                           MarshalCleanupData *cleanup_data)
{
    GList *list_;
    guint length;
    guint i;
    GArray *item_cleanups;

    PyGIMarshalToPyFunc item_to_py_marshaller;
    PyGIArgCache *item_arg_cache;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;

    PyObject *py_obj = NULL;

    list_ = arg->v_pointer;
    length = g_list_length (list_);

    py_obj = PyList_New (length);
    if (py_obj == NULL) return NULL;

    // Last item is for the list itself
    item_cleanups = g_array_sized_new (
        FALSE, TRUE, sizeof (MarshalCleanupData), length + 1);

    item_arg_cache = seq_cache->item_cache;
    item_to_py_marshaller = item_arg_cache->to_py_marshaller;

    for (i = 0; list_ != NULL; list_ = g_list_next (list_), i++) {
        GIArgument item_arg;
        PyObject *py_item;
        MarshalCleanupData item_cleanup_data = { NULL, NULL };

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

    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING
        || arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        MarshalCleanupData list_cleanup_data = {
            .data = arg->v_pointer,
            .destroy = (GDestroyNotify)g_list_free
        };
        g_array_append_val (item_cleanups, list_cleanup_data);
    }
    cleanup_data->data = item_cleanups;
    cleanup_data->destroy = (GDestroyNotify)g_array_unref;

    return py_obj;
}

static PyObject *
_pygi_marshal_to_py_gslist (PyGIInvokeState *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache *arg_cache, GIArgument *arg,
                            MarshalCleanupData *cleanup_data)
{
    GSList *list_;
    guint length;
    guint i;
    GArray *item_cleanups;

    PyGIMarshalToPyFunc item_to_py_marshaller;
    PyGIArgCache *item_arg_cache;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;

    PyObject *py_obj = NULL;

    list_ = arg->v_pointer;
    length = g_slist_length (list_);

    py_obj = PyList_New (length);
    if (py_obj == NULL) return NULL;

    // Last item is for the list itself
    item_cleanups = g_array_sized_new (
        FALSE, TRUE, sizeof (MarshalCleanupData), length + 1);

    item_arg_cache = seq_cache->item_cache;
    item_to_py_marshaller = item_arg_cache->to_py_marshaller;

    for (i = 0; list_ != NULL; list_ = g_slist_next (list_), i++) {
        GIArgument item_arg;
        PyObject *py_item;
        MarshalCleanupData item_cleanup_data = { NULL, NULL };

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

    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING
        || arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        MarshalCleanupData list_cleanup_data = {
            .data = arg->v_pointer,
            .destroy = (GDestroyNotify)g_slist_free
        };
        g_array_append_val (item_cleanups, list_cleanup_data);
    }
    cleanup_data->data = item_cleanups;
    cleanup_data->destroy = (GDestroyNotify)g_array_unref;

    return py_obj;
}

static void
_pygi_marshal_cleanup_to_py_glist (PyGIInvokeState *state,
                                   PyGIArgCache *arg_cache,
                                   MarshalCleanupData cleanup_data,
                                   gpointer data, gboolean was_processed)
{
    GArray *item_cleanups = (GArray *)cleanup_data.data;
    guint i;

    for (i = 0; i < item_cleanups->len; i++) {
        MarshalCleanupData *item_cleanup_data =
            &g_array_index (item_cleanups, MarshalCleanupData, i);
        if (item_cleanup_data->destroy && item_cleanup_data->data)
            item_cleanup_data->destroy (item_cleanup_data->data);
    }

    cleanup_data.destroy (cleanup_data.data);
}

static void
_arg_cache_from_py_glist_setup (PyGIArgCache *arg_cache, GITransfer transfer)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_glist;
    arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_glist;
}

static void
_arg_cache_to_py_glist_setup (PyGIArgCache *arg_cache, GITransfer transfer)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_glist;
    arg_cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_glist;
}

static void
_arg_cache_from_py_gslist_setup (PyGIArgCache *arg_cache, GITransfer transfer)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_gslist;
    arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_glist;
}

static void
_arg_cache_to_py_gslist_setup (PyGIArgCache *arg_cache, GITransfer transfer)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_gslist;
    arg_cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_glist;
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
        _arg_cache_from_py_glist_setup (arg_cache, transfer);

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        _arg_cache_to_py_glist_setup (arg_cache, transfer);

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
        _arg_cache_from_py_gslist_setup (arg_cache, transfer);

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        _arg_cache_to_py_gslist_setup (arg_cache, transfer);

    return arg_cache;
}

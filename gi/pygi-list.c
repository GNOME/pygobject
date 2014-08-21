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

#include "pygi-list.h"
#include "pygi-argument.h"
#include "pygi-private.h"

typedef PyGISequenceCache PyGIArgGList;

/*
 * GList and GSList from Python
 */
static gboolean
_pygi_marshal_from_py_glist (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg,
                             gpointer          *cleanup_data)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
    int i;
    Py_ssize_t length;
    GList *list_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;


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

    from_py_marshaller = sequence_cache->item_cache->from_py_marshaller;
    for (i = 0; i < length; i++) {
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
                                 &item_cleanup_data))
            goto err;

        Py_DECREF (py_item);
        list_ = g_list_prepend (list_, _pygi_arg_to_hash_pointer (&item, sequence_cache->item_cache->type_tag));
        continue;
err:
        /* FIXME: clean up list
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            PyGIMarshalCleanupFunc cleanup = sequence_cache->item_cache->from_py_cleanup;
        }
        */
        Py_XDECREF (py_item);
        g_list_free (list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = g_list_reverse (list_);

    if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
        /* Free everything in cleanup. */
        *cleanup_data = arg->v_pointer;
    } else if (arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        /* Make a shallow copy so we can free the elements later in cleanup
         * because it is possible invoke will free the list before our cleanup. */
        *cleanup_data = g_list_copy (arg->v_pointer);
    } else { /* GI_TRANSFER_EVERYTHING */
        /* No cleanup, everything is given to the callee. */
        *cleanup_data = NULL;
    }
    return TRUE;
}


static gboolean
_pygi_marshal_from_py_gslist (PyGIInvokeState   *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache      *arg_cache,
                              PyObject          *py_arg,
                              GIArgument        *arg,
                              gpointer          *cleanup_data)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
    int i;
    Py_ssize_t length;
    GSList *list_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;

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

    from_py_marshaller = sequence_cache->item_cache->from_py_marshaller;
    for (i = 0; i < length; i++) {
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
                            &item_cleanup_data))
            goto err;

        Py_DECREF (py_item);
        list_ = g_slist_prepend (list_, _pygi_arg_to_hash_pointer (&item, sequence_cache->item_cache->type_tag));
        continue;
err:
        /* FIXME: Clean up list
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            PyGIMarshalCleanupFunc cleanup = sequence_cache->item_cache->from_py_cleanup;
        }
        */

        Py_XDECREF (py_item);
        g_slist_free (list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = g_slist_reverse (list_);

    if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
        /* Free everything in cleanup. */
        *cleanup_data = arg->v_pointer;
    } else if (arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        /* Make a shallow copy so we can free the elements later in cleanup
         * because it is possible invoke will free the list before our cleanup. */
        *cleanup_data = g_slist_copy (arg->v_pointer);
    } else { /* GI_TRANSFER_EVERYTHING */
        /* No cleanup, everything is given to the callee. */
        *cleanup_data = NULL;
    }

    return TRUE;
}

static void
_pygi_marshal_cleanup_from_py_glist  (PyGIInvokeState *state,
                                      PyGIArgCache    *arg_cache,
                                      PyObject        *py_arg,
                                      gpointer         data,
                                      gboolean         was_processed)
{
    if (was_processed) {
        GSList *list_;
        PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;

        list_ = (GSList *)data;

        /* clean up items first */
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            PyGIMarshalCleanupFunc cleanup_func =
                sequence_cache->item_cache->from_py_cleanup;
            GSList *node = list_;
            gsize i = 0;
            while (node != NULL) {
                PyObject *py_item = PySequence_GetItem (py_arg, i);
                cleanup_func (state,
                              sequence_cache->item_cache,
                              py_item,
                              node->data,
                              TRUE);
                Py_XDECREF (py_item);
                node = node->next;
                i++;
            }
        }

        if (arg_cache->type_tag == GI_TYPE_TAG_GLIST) {
            g_list_free ( (GList *)list_);
        } else if (arg_cache->type_tag == GI_TYPE_TAG_GSLIST) {
            g_slist_free (list_);
        } else {
            g_assert_not_reached();
        }
    }
}


/*
 * GList and GSList to Python
 */
static PyObject *
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

static PyObject *
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

static void
_pygi_marshal_cleanup_to_py_glist (PyGIInvokeState *state,
                                   PyGIArgCache    *arg_cache,
                                   PyObject        *dummy,
                                   gpointer         data,
                                   gboolean         was_processed)
{
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING ||
            arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        GSList *list_ = (GSList *)data;

        if (sequence_cache->item_cache->to_py_cleanup != NULL) {
            PyGIMarshalCleanupFunc cleanup_func =
                sequence_cache->item_cache->to_py_cleanup;
            GSList *node = list_;

            while (node != NULL) {
                cleanup_func (state,
                              sequence_cache->item_cache,
                              NULL,
                              node->data,
                              was_processed);
                node = node->next;
            }
        }

        if (arg_cache->type_tag == GI_TYPE_TAG_GLIST) {
            g_list_free ( (GList *)list_);
        } else if (arg_cache->type_tag == GI_TYPE_TAG_GSLIST) {
            g_slist_free (list_);
        } else {
            g_assert_not_reached();
        }
    }
}

static void
_arg_cache_from_py_glist_setup (PyGIArgCache *arg_cache,
                                GITransfer transfer)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_glist;
    arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_glist;
}

static void
_arg_cache_to_py_glist_setup (PyGIArgCache *arg_cache,
                              GITransfer transfer)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_glist;
    arg_cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_glist;
}

static void
_arg_cache_from_py_gslist_setup (PyGIArgCache *arg_cache,
                                 GITransfer transfer)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_gslist;
    arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_glist;
}

static void
_arg_cache_to_py_gslist_setup (PyGIArgCache *arg_cache,
                                 GITransfer transfer)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_gslist;
    arg_cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_glist;
}


/*
 * GList/GSList Interface
 */

static gboolean
pygi_arg_glist_setup_from_info (PyGIArgCache      *arg_cache,
                                GITypeInfo        *type_info,
                                GIArgInfo         *arg_info,
                                GITransfer         transfer,
                                PyGIDirection      direction,
                                PyGICallableCache *callable_cache)
{
    GITypeTag type_tag = g_type_info_get_tag (type_info);

    if (!pygi_arg_sequence_setup ((PyGISequenceCache *)arg_cache,
                                  type_info,
                                  arg_info,
                                  transfer,
                                  direction,
                                  callable_cache))
        return FALSE;

    switch (type_tag) {
        case GI_TYPE_TAG_GLIST:
            {
                if (direction & PYGI_DIRECTION_FROM_PYTHON)
                    _arg_cache_from_py_glist_setup (arg_cache, transfer);

                if (direction & PYGI_DIRECTION_TO_PYTHON)
                    _arg_cache_to_py_glist_setup (arg_cache, transfer);
                break;
            }
        case GI_TYPE_TAG_GSLIST:
            {
                if (direction & PYGI_DIRECTION_FROM_PYTHON)
                    _arg_cache_from_py_gslist_setup (arg_cache, transfer);

                if (direction & PYGI_DIRECTION_TO_PYTHON)
                    _arg_cache_to_py_gslist_setup (arg_cache, transfer);

                break;
             }
       default:
           g_assert_not_reached ();
    }

    return TRUE;
}

PyGIArgCache *
pygi_arg_glist_new_from_info (GITypeInfo        *type_info,
                              GIArgInfo         *arg_info,
                              GITransfer         transfer,
                              PyGIDirection      direction,
                              PyGICallableCache *callable_cache)
{
    gboolean res = FALSE;

    PyGIArgCache *arg_cache = (PyGIArgCache *) g_slice_new0 (PyGIArgGList);
    if (arg_cache == NULL)
        return NULL;

    res = pygi_arg_glist_setup_from_info (arg_cache,
                                          type_info,
                                          arg_info,
                                          transfer,
                                          direction,
                                          callable_cache);
    if (res) {
        return arg_cache;
    } else {
        pygi_arg_cache_free (arg_cache);
        return NULL;
    }
}

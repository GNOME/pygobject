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
_pygi_marshal_from_py_list (PyGIInvokeState *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache *arg_cache, PyObject *object,
                            GIArgument *arg, gpointer *cleanup_data)
{
    *arg = pygi_argument_list_from_py (object, arg_cache->type_info,
                                       arg_cache->transfer);

    switch (arg_cache->transfer) {
    case GI_TRANSFER_NOTHING:
        /* Free everything in cleanup. */
        *cleanup_data = arg->v_pointer;
        break;
    case GI_TRANSFER_CONTAINER:
        /* Make a shallow copy so we can free the elements later in cleanup
         * because it is possible invoke will free the list before our cleanup. */
        if (arg_cache->type_tag == GI_TYPE_TAG_GLIST) {
            *cleanup_data = g_list_copy (arg->v_pointer);
        } else {
            *cleanup_data = g_slist_copy (arg->v_pointer);
        }
        break;
    case GI_TRANSFER_EVERYTHING:
        /* No cleanup, everything is given to the callee. */
        *cleanup_data = NULL;
        break;
    default:
        g_assert_not_reached ();
    }

    return !PyErr_Occurred ();
}

static void
_pygi_marshal_cleanup_from_py_glist (PyGIInvokeState *state,
                                     PyGIArgCache *arg_cache, PyObject *object,
                                     gpointer data, gboolean was_processed)
{
    if (was_processed) {
        GSList *list_;
        PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;

        list_ = (GSList *)data;

        /* clean up items first */
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            PyGIMarshalFromPyCleanupFunc cleanup_func =
                sequence_cache->item_cache->from_py_cleanup;
            GSList *node = list_;
            gsize i = 0;
            while (node != NULL) {
                PyObject *py_item = PySequence_GetItem (object, i);
                cleanup_func (state, sequence_cache->item_cache, py_item,
                              node->data, TRUE);
                Py_XDECREF (py_item);
                node = node->next;
                i++;
            }
        }

        if (arg_cache->type_tag == GI_TYPE_TAG_GLIST) {
            g_list_free ((GList *)list_);
        } else if (arg_cache->type_tag == GI_TYPE_TAG_GSLIST) {
            g_slist_free (list_);
        } else {
            g_assert_not_reached ();
        }
    }
}


/*
 * GList and GSList to Python
 */
static PyObject *
_pygi_marshal_to_py_list (PyGIInvokeState *state,
                          PyGICallableCache *callable_cache,
                          PyGIArgCache *arg_cache, GIArgument *arg,
                          gpointer *cleanup_data)
{
    return pygi_argument_list_to_py (*arg, arg_cache->type_info,
                                     arg_cache->transfer);
}

static void
_pygi_marshal_cleanup_to_py_glist (PyGIInvokeState *state,
                                   PyGIArgCache *arg_cache,
                                   gpointer cleanup_data, gpointer data,
                                   gboolean was_processed)
{
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;
    GSList *list_ = (GSList *)data;

    if (sequence_cache->item_cache->to_py_cleanup != NULL) {
        PyGIMarshalToPyCleanupFunc cleanup_func =
            sequence_cache->item_cache->to_py_cleanup;
        GSList *node = list_;
        guint i = 0;

        while (node != NULL) {
            cleanup_func (state, sequence_cache->item_cache, NULL, node->data,
                          was_processed);
            node = node->next;
            i++;
        }
    }

    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING
        || arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        if (arg_cache->type_tag == GI_TYPE_TAG_GLIST) {
            g_list_free ((GList *)list_);
        } else if (arg_cache->type_tag == GI_TYPE_TAG_GSLIST) {
            g_slist_free (list_);
        } else {
            g_assert_not_reached ();
        }
    }
}

static void
_arg_cache_from_py_glist_setup (PyGIArgCache *arg_cache, GITransfer transfer)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_list;
    arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_glist;
}

static void
_arg_cache_to_py_glist_setup (PyGIArgCache *arg_cache, GITransfer transfer)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_list;
    arg_cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_glist;
}

static void
_arg_cache_from_py_gslist_setup (PyGIArgCache *arg_cache, GITransfer transfer)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_list;
    arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_glist;
}

static void
_arg_cache_to_py_gslist_setup (PyGIArgCache *arg_cache, GITransfer transfer)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_list;
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

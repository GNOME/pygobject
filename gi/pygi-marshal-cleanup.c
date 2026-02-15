/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>, Red Hat, Inc.
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

#include "pygi-marshal-cleanup.h"
#include "pygi-foreign.h"
#include "pygi-cache-private.h"

static inline void
_cleanup_caller_allocates (PyGIInvokeState *state, PyGIArgCache *cache,
                           gpointer data, gboolean was_processed)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)cache;

    if (data == NULL) return;

    /* check GValue first because GValue is also a boxed sub-type */
    if (g_type_is_a (iface_cache->g_type, G_TYPE_VALUE)) {
        if (was_processed) g_value_unset (data);
        g_slice_free (GValue, data);
    } else if (g_type_is_a (iface_cache->g_type, G_TYPE_BOXED)) {
        gsize size;
        if (was_processed) return; /* will be cleaned up at deallocation */
        size = gi_struct_info_get_size (
            GI_STRUCT_INFO (iface_cache->interface_info));
        g_slice_free1 (size, data);
    } else if (iface_cache->is_foreign) {
        if (was_processed) return; /* will be cleaned up at deallocation */
        pygi_struct_foreign_release (
            GI_BASE_INFO (iface_cache->interface_info), data);
    } else {
        if (was_processed) return; /* will be cleaned up at deallocation */
        g_free (data);
    }
}

/**
 * Cleanup during invoke can happen in multiple
 * stages, each of which can be the result of a
 * successful compleation of that stage or an error
 * occured which requires partial cleanup.
 *
 * For the most part, either the C interface being
 * invoked or the python object which wraps the
 * parameters, handle their lifecycles but in some
 * cases, where we have intermediate objects,
 * or when we fail processing a parameter, we need
 * to handle the clean up manually.
 *
 * There are two argument processing stages.
 * They are the in stage, where we process python
 * parameters into their C counterparts, and the out
 * stage, where we process out C parameters back
 * into python objects. The in stage also sets up
 * temporary out structures for caller allocated
 * parameters which need to be cleaned up either on
 * in stage failure or at the completion of the out
 * stage (either success or failure)
 *
 * The in stage must call one of these cleanup functions:
 *    - pygi_marshal_cleanup_args_from_py
 *       (continue to out stage)
 *    - pygi_marshal_cleanup_args_from_py_parameter_fail
 *       (final, exit from invoke)
 *
 * The out stage must call one of these cleanup functions which are all final:
 *    - pygi_marshal_cleanup_args_to_py
 *
 **/
void
pygi_marshal_cleanup_args_from_py (PyGIInvokeState *state,
                                   PyGICallableCache *cache, gboolean success)
{
    PyObject *error_type, *error_value, *error_traceback;
    gboolean have_error = !!PyErr_Occurred ();

    if (have_error) PyErr_Fetch (&error_type, &error_value, &error_traceback);

    /* clean up the return if available */
    if (cache->return_cache != NULL) {
        if (success)
            pygi_marshal_cleanup_data_destroy (
                &state->from_py_return_arg_cleanup_data);
        else
            pygi_marshal_cleanup_data_destroy_failed (
                &state->from_py_return_arg_cleanup_data);
    }

    for (guint i = 0; i < _pygi_callable_cache_args_len (cache); i++) {
        PyGIArgCache *arg_cache = _pygi_callable_cache_get_arg (cache, i);
        PyGIMarshalCleanupData *cleanup_data =
            &state->args[i].from_py_arg_cleanup_data;

        /* Only cleanup using args_cleanup_data when available.
         * It is the responsibility of the various "from_py" marshalers to return
         * cleanup_data which is then passed into their respective cleanup function.
         * PyGIInvokeState.args_cleanup_data stores this data (via _invoke_marshal_in_args)
         * for the duration of the invoke up until this point.
         */
        if (arg_cache->py_arg_index >= 0
            && arg_cache->direction & PYGI_DIRECTION_FROM_PYTHON) {
            if (success)
                pygi_marshal_cleanup_data_destroy (cleanup_data);
            else
                pygi_marshal_cleanup_data_destroy_failed (cleanup_data);
        }
    }

    if (have_error) PyErr_Restore (error_type, error_value, error_traceback);
}

void
pygi_marshal_cleanup_args_to_py (PyGIInvokeState *state,
                                 PyGICallableCache *cache, gboolean success)
{
    GSList *cache_item;
    PyObject *error_type, *error_value, *error_traceback;
    gboolean have_error = !!PyErr_Occurred ();

    if (have_error) PyErr_Fetch (&error_type, &error_value, &error_traceback);

    /* clean up the return if available */
    if (cache->return_cache != NULL) {
        if (success)
            pygi_marshal_cleanup_data_destroy (
                &state->to_py_return_arg_cleanup_data);
        else
            pygi_marshal_cleanup_data_destroy_failed (
                &state->to_py_return_arg_cleanup_data);
    }

    /* Now clean up args */
    cache_item = cache->to_py_args;
    while (cache_item) {
        PyGIArgCache *arg_cache = (PyGIArgCache *)cache_item->data;
        PyGIMarshalCleanupData *cleanup_data =
            &state->args[arg_cache->c_arg_index].to_py_arg_cleanup_data;

        if (cleanup_data->data != NULL
            && (cleanup_data->destroy != NULL
                || cleanup_data->destroy_failed != NULL)) {
            if (success)
                pygi_marshal_cleanup_data_destroy (cleanup_data);
            else
                pygi_marshal_cleanup_data_destroy_failed (cleanup_data);
        } else if (pygi_arg_cache_is_caller_allocates (arg_cache)) {
            gpointer data =
                state->args[arg_cache->c_arg_index].arg_value.v_pointer;
            _cleanup_caller_allocates (state, arg_cache, data, TRUE);
        }

        cache_item = cache_item->next;
    }

    if (have_error) PyErr_Restore (error_type, error_value, error_traceback);
}

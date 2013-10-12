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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */
 
 #include "pygi-marshal-cleanup.h"
 #include <glib.h>
static inline void
_cleanup_caller_allocates (PyGIInvokeState    *state,
                           PyGIArgCache       *cache,
                           PyObject           *py_obj,
                           gpointer            data,
                           gboolean            was_processed)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)cache;

    /* check GValue first because GValue is also a boxed sub-type */
    if (g_type_is_a (iface_cache->g_type, G_TYPE_VALUE)) {
        if (was_processed)
            g_value_unset (data);
        g_slice_free (GValue, data);
    } else if (g_type_is_a (iface_cache->g_type, G_TYPE_BOXED)) {
        gsize size;
        if (was_processed)
            return; /* will be cleaned up at deallocation */
        size = g_struct_info_get_size (iface_cache->interface_info);
        g_slice_free1 (size, data);
    } else if (iface_cache->is_foreign) {
        if (was_processed)
            return; /* will be cleaned up at deallocation */
        pygi_struct_foreign_release ((GIBaseInfo *)iface_cache->interface_info,
                                     data);
    } else {
        if (was_processed)
            return; /* will be cleaned up at deallocation */
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
 *    - pygi_marshal_cleanup_args_from_py_marshal_success
 *       (continue to out stage)
 *    - pygi_marshal_cleanup_args_from_py_parameter_fail
 *       (final, exit from invoke)
 *
 * The out stage must call one of these cleanup functions which are all final:
 *    - pygi_marshal_cleanup_args_to_py_marshal_success
 *    - pygi_marshal_cleanup_args_return_fail
 *    - pygi_marshal_cleanup_args_to_py_parameter_fail
 *
 **/
void
pygi_marshal_cleanup_args_from_py_marshal_success (PyGIInvokeState   *state,
                                                   PyGICallableCache *cache)
{
    gssize i;

    for (i = 0; i < _pygi_callable_cache_args_len (cache); i++) {
        PyGIArgCache *arg_cache = _pygi_callable_cache_get_arg (cache, i);
        PyGIMarshalCleanupFunc cleanup_func = arg_cache->from_py_cleanup;
        PyObject *py_arg = PyTuple_GET_ITEM (state->py_in_args,
                                             arg_cache->py_arg_index);
        gpointer cleanup_data = state->args_cleanup_data[i];

        /* Only cleanup using args_cleanup_data when available.
         * It is the responsibility of the various "from_py" marshalers to return
         * cleanup_data which is then passed into their respective cleanup function.
         * PyGIInvokeState.args_cleanup_data stores this data (via _invoke_marshal_in_args)
         * for the duration of the invoke up until this point.
         */
        if (cleanup_func && cleanup_data != NULL &&
                arg_cache->direction & PYGI_DIRECTION_FROM_PYTHON) {
            cleanup_func (state, arg_cache, py_arg, cleanup_data, TRUE);
            state->args_cleanup_data[i] = NULL;
        }
    }
}

void
pygi_marshal_cleanup_args_to_py_marshal_success (PyGIInvokeState   *state,
                                                 PyGICallableCache *cache)
{
    GSList *cache_item;
    /* clean up the return if available */
    if (cache->return_cache != NULL) {
        PyGIMarshalCleanupFunc cleanup_func = cache->return_cache->to_py_cleanup;
        if (cleanup_func && state->return_arg.v_pointer != NULL)
            cleanup_func (state,
                          cache->return_cache,
                          NULL,
                          state->return_arg.v_pointer,
                          TRUE);
    }

    /* Now clean up args */
    cache_item = cache->to_py_args;
    while (cache_item) {
        PyGIArgCache *arg_cache = (PyGIArgCache *) cache_item->data;
        PyGIMarshalCleanupFunc cleanup_func = arg_cache->to_py_cleanup;
        gpointer data = state->args[arg_cache->c_arg_index]->v_pointer;

        if (cleanup_func != NULL && data != NULL)
            cleanup_func (state,
                          arg_cache,
                          NULL,
                          data,
                          TRUE);
        else if (arg_cache->is_caller_allocates && data != NULL) {
            _cleanup_caller_allocates (state,
                                       arg_cache,
                                       NULL,
                                       data,
                                       TRUE);
        }

        cache_item = cache_item->next;
    }
}

void
pygi_marshal_cleanup_args_from_py_parameter_fail (PyGIInvokeState   *state,
                                                  PyGICallableCache *cache,
                                                  gssize failed_arg_index)
{
    gssize i;

    state->failed = TRUE;

    for (i = 0; i < _pygi_callable_cache_args_len (cache)  && i <= failed_arg_index; i++) {
        PyGIArgCache *arg_cache = _pygi_callable_cache_get_arg (cache, i);
        PyGIMarshalCleanupFunc cleanup_func = arg_cache->from_py_cleanup;
        gpointer data = state->args[i]->v_pointer;
        PyObject *py_arg = PyTuple_GET_ITEM (state->py_in_args,
                                             arg_cache->py_arg_index);

        if (cleanup_func &&
                arg_cache->direction == PYGI_DIRECTION_FROM_PYTHON &&
                    data != NULL) {
            cleanup_func (state,
                          arg_cache,
                          py_arg,
                          data,
                          i < failed_arg_index);

        } else if (arg_cache->is_caller_allocates && data != NULL) {
            _cleanup_caller_allocates (state,
                                       arg_cache,
                                       py_arg,
                                       data,
                                       FALSE);
        }
    }
}

void
pygi_marshal_cleanup_args_return_fail (PyGIInvokeState   *state,
                                       PyGICallableCache *cache)
{
    state->failed = TRUE;
}

void
pygi_marshal_cleanup_args_to_py_parameter_fail (PyGIInvokeState   *state,
                                              PyGICallableCache *cache,
                                              gssize failed_to_py_arg_index)
{
    state->failed = TRUE;
}

void
_pygi_marshal_cleanup_from_py_interface_object (PyGIInvokeState *state,
                                                PyGIArgCache    *arg_cache,
                                                PyObject        *py_arg,
                                                gpointer         data,
                                                gboolean         was_processed)
{
    /* If we processed the parameter but fail before invoking the method,
       we need to remove the ref we added */
    if (was_processed && state->failed && data != NULL &&
            arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        g_object_unref (G_OBJECT(data));
}

void
_pygi_marshal_cleanup_to_py_interface_object (PyGIInvokeState *state,
                                              PyGIArgCache    *arg_cache,
                                              PyObject        *dummy,
                                              gpointer         data,
                                              gboolean         was_processed)
{
    /* If we error out and the object is not marshalled into a PyGObject
       we must take care of removing the ref */
    if (!was_processed && arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        g_object_unref (G_OBJECT(data));
}

void 
_pygi_marshal_cleanup_from_py_interface_struct_gvalue (PyGIInvokeState *state,
                                                       PyGIArgCache    *arg_cache,
                                                       PyObject        *py_arg,
                                                       gpointer         data,
                                                       gboolean         was_processed)
{
    /* Note py_arg can be NULL for hash table which is a bug. */
    if (was_processed && py_arg != NULL) {
        GType py_object_type =
            pyg_type_from_object_strict ( (PyObject *) py_arg->ob_type, FALSE);

        /* When a GValue was not passed, it means the marshalers created a new
         * one to pass in, clean this up.
         */
        if (py_object_type != G_TYPE_VALUE) {
            g_value_unset ((GValue *) data);
            g_slice_free (GValue, data);
        }
    }
}

void
_pygi_marshal_cleanup_from_py_interface_struct_foreign (PyGIInvokeState *state,
                                                        PyGIArgCache    *arg_cache,
                                                        PyObject        *py_arg,
                                                        gpointer         data,
                                                        gboolean         was_processed)
{
    if (state->failed && was_processed)
        pygi_struct_foreign_release (
            ( (PyGIInterfaceCache *)arg_cache)->interface_info,
            data);
}

void
_pygi_marshal_cleanup_to_py_interface_struct_foreign (PyGIInvokeState *state,
                                                      PyGIArgCache    *arg_cache,
                                                      PyObject        *dummy,
                                                      gpointer         data,
                                                      gboolean         was_processed)
{
    if (!was_processed && arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        pygi_struct_foreign_release ( 
            ( (PyGIInterfaceCache *)arg_cache)->interface_info,
            data);
}

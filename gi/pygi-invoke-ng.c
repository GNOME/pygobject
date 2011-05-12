/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 * Copyright (C) 2011 John (J5) Palimier <johnp@redhat.com>
 *
 *   pygi-invoke.c: main invocation function
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

#include <pyglib.h>
#include "pygi-invoke.h"


static inline gboolean
_invoke_callable (PyGIInvokeState *state,
                  PyGICallableCache *cache,
                  GICallableInfo *callable_info)
{
    GError *error;
    gint retval;

    error = NULL;

    pyg_begin_allow_threads;

    /* FIXME: use this for now but we can streamline the calls */
    if (cache->is_vfunc)
        retval = g_vfunc_info_invoke ( callable_info,
                                       state->implementor_gtype,
                                       state->in_args,
                                       cache->n_in_args,
                                       state->out_args,
                                       cache->n_out_args,
                                      &state->return_arg,
                                      &error);
    else
        retval = g_function_info_invoke ( callable_info,
                                          state->in_args,
                                          cache->n_in_args,
                                          state->out_args,
                                          cache->n_out_args,
                                         &state->return_arg,
                                         &error);
    pyg_end_allow_threads;

    if (!retval) {
        g_assert (error != NULL);
        pyglib_error_check (&error);

        /* It is unclear if the error occured before or after the C
         * function was invoked so for now assume success
         * We eventually should marshal directly to FFI so we no longer
         * have to use the reference implementation
         */
        pygi_marshal_cleanup_args_in_marshal_success (state, cache);

        return FALSE;
    }

    if (state->error != NULL) {
        if (pyglib_error_check (&(state->error))) {
            state->stage = PYGI_INVOKE_STAGE_NATIVE_INVOKE_FAILED;
            /* even though we errored out, the call itself was successful,
               so we assume the call processed all of the parameters */
            pygi_marshal_cleanup_args_in_marshal_success (state, cache);
            return FALSE;
        }
    }

    return TRUE;
}

static inline gboolean
_invoke_state_init_from_callable_cache (PyGIInvokeState *state,
                                        PyGICallableCache *cache,
                                        PyObject *py_args,
                                        PyObject *kwargs)
{
    state->stage = PYGI_INVOKE_STAGE_MARSHAL_IN_START;
    state->py_in_args = py_args;
    state->n_py_in_args = PySequence_Length (py_args);

    /* We don't use the class parameter sent in by  the structure
     * so we remove it from the py_args tuple but we keep it 
     * around just in case we want to call actual gobject constructors
     * in the future instead of calling g_object_new
     */
    if  (cache->is_constructor) {
        state->constructor_class = PyTuple_GetItem (py_args, 0);

        if (state->constructor_class == NULL) {
            PyErr_Clear ();
            PyErr_Format (PyExc_TypeError,
                          "Constructors require the class to be passed in as an argument, "
                          "No arguments passed to the %s constructor.",
                          cache->name);
            return FALSE;
        }

        Py_INCREF (state->constructor_class);

        /* we could optimize this by using offsets instead of modifying the tuple but it makes the
         * code more error prone and confusing so don't do that unless profiling shows
         * significant gain
         */
        state->py_in_args = PyTuple_GetSlice (py_args, 1, state->n_py_in_args);
        state->n_py_in_args--;
    } else {
        Py_INCREF (state->py_in_args);
    }
    state->implementor_gtype = 0;
    if (cache->is_vfunc) {
        PyObject *py_gtype;
        py_gtype = PyDict_GetItemString (kwargs, "gtype");
        if (py_gtype == NULL) {
            PyErr_SetString (PyExc_TypeError,
                             "need the GType of the implementor class");
            return FALSE;
        }

        state->implementor_gtype = pyg_type_from_object (py_gtype);

        if (state->implementor_gtype == 0)
            return FALSE;
    }

    state->args = g_slice_alloc0 (cache->n_args * sizeof (GIArgument *));
    if (state->args == NULL && cache->n_args != 0) {
        PyErr_NoMemory();
        return FALSE;
    }

    state->in_args = g_slice_alloc0 (cache->n_in_args * sizeof(GIArgument));
    if (state->in_args == NULL && cache->n_in_args != 0) {
        PyErr_NoMemory ();
        return FALSE;
    }

    state->out_values = g_slice_alloc0 (cache->n_out_args * sizeof(GIArgument));
    if (state->out_values == NULL && cache->n_out_args != 0) {
        PyErr_NoMemory ();
        return FALSE;
    }

    state->out_args = g_slice_alloc0 (cache->n_out_args * sizeof(GIArgument));
    if (state->out_args == NULL && cache->n_out_args != 0) {
        PyErr_NoMemory ();
        return FALSE;
    }

    state->error = NULL;

    return TRUE;
}

static inline void
_invoke_state_clear (PyGIInvokeState *state, PyGICallableCache *cache)
{
    g_slice_free1 (cache->n_args * sizeof(GIArgument *), state->args);
    g_slice_free1 (cache->n_in_args * sizeof(GIArgument), state->in_args);
    g_slice_free1 (cache->n_out_args * sizeof(GIArgument), state->out_args);
    g_slice_free1 (cache->n_out_args * sizeof(GIArgument), state->out_values);

    Py_XDECREF (state->py_in_args);
}

static inline gboolean
_invoke_marshal_in_args (PyGIInvokeState *state, PyGICallableCache *cache)
{
    int i, in_count, out_count;
    in_count = 0;
    out_count = 0;

    if (state->n_py_in_args > cache->n_py_args) {
        PyErr_Format (PyExc_TypeError,
                      "%s() takes exactly %zd argument(s) (%zd given)",
                      cache->name,
                      cache->n_py_args,
                      state->n_py_in_args);
        return FALSE;
    }

    for (i = 0; i < cache->n_args; i++) {
        GIArgument *c_arg;
        PyGIArgCache *arg_cache = cache->args_cache[i];
        PyObject *py_arg = NULL;

        state->current_arg = i;
        state->stage = PYGI_INVOKE_STAGE_MARSHAL_IN_START;
        switch (arg_cache->direction) {
            case GI_DIRECTION_IN:
                state->args[i] = &(state->in_args[in_count]);
                in_count++;

                if (arg_cache->aux_type > 0)
                    continue;

                if (arg_cache->py_arg_index >= state->n_py_in_args) {
                    PyErr_Format (PyExc_TypeError,
                                  "%s() takes exactly %zd argument(s) (%zd given)",
                                   cache->name,
                                   cache->n_py_args,
                                   state->n_py_in_args);

                    /* clean up all of the args we have already marshalled,
                     * since invoke will not be called
                     */
                    pygi_marshal_cleanup_args_in_parameter_fail (state,
                                                                 cache,
                                                                 i - 1);
                    return FALSE;
                }

                py_arg =
                    PyTuple_GET_ITEM (state->py_in_args,
                                      arg_cache->py_arg_index);

                break;
            case GI_DIRECTION_INOUT:
                /* this will be filled in if it is an aux value */
                if (state->in_args[in_count].v_pointer != NULL)
                    state->out_values[out_count] = state->in_args[in_count];

                state->in_args[in_count].v_pointer = &state->out_values[out_count];
                in_count++;

                if (arg_cache->aux_type != PYGI_AUX_TYPE_IGNORE) {
                    if (arg_cache->py_arg_index >= state->n_py_in_args) {
                        PyErr_Format (PyExc_TypeError,
                                      "%s() takes exactly %zd argument(s) (%zd given)",
                                       cache->name,
                                       cache->n_py_args,
                                       state->n_py_in_args);
                        pygi_marshal_cleanup_args_in_parameter_fail (state,
                                                                     cache,
                                                                     i - 1);
                        return FALSE;
                    }

                    py_arg =
                        PyTuple_GET_ITEM (state->py_in_args,
                                          arg_cache->py_arg_index);
                }
            case GI_DIRECTION_OUT:
                if (arg_cache->is_caller_allocates) {
                    PyGIInterfaceCache *iface_cache =
                        (PyGIInterfaceCache *)arg_cache;

                    g_assert (arg_cache->type_tag == GI_TYPE_TAG_INTERFACE);

                    state->out_args[out_count].v_pointer = NULL;
                    state->args[i] = &state->out_args[out_count];
                    if (iface_cache->g_type == G_TYPE_BOXED) {
                        state->args[i]->v_pointer =
                            _pygi_boxed_alloc (iface_cache->interface_info, NULL);
                    } else if (iface_cache->is_foreign) {
                        PyObject *foreign_struct =
                            pygi_struct_foreign_convert_from_g_argument (
                                iface_cache->interface_info,
                                NULL);

                        pygi_struct_foreign_convert_to_g_argument (
                            foreign_struct,
                            iface_cache->interface_info,
                            GI_TRANSFER_EVERYTHING,
                            state->args[i]);
                    } else {
                        gssize size =
                            g_struct_info_get_size(
                                (GIStructInfo *)iface_cache->interface_info);
                        state->args[i]->v_pointer = g_malloc0 (size);
                    }

                } else {
                    state->out_args[out_count].v_pointer = &state->out_values[out_count];
                    state->args[i] = &state->out_values[out_count];
                }
                out_count++;
                break;
        }

        c_arg = state->args[i];
        if (arg_cache->in_marshaller != NULL) {
            if (!arg_cache->allow_none && py_arg == Py_None) {
                PyErr_Format (PyExc_TypeError,
                              "Argument %i does not allow None as a value",
                              i);

                 pygi_marshal_cleanup_args_in_parameter_fail (state,
                                                              cache,
                                                              i - 1);
                 return FALSE;
            }
            gboolean success = arg_cache->in_marshaller (state,
                                                         cache,
                                                         arg_cache,
                                                         py_arg,
                                                         c_arg);
            if (!success) {
                pygi_marshal_cleanup_args_in_parameter_fail (state,
                                                              cache,
                                                              i - 1);
                return FALSE;
            }

            state->stage = PYGI_INVOKE_STAGE_MARSHAL_IN_IDLE;
        }

    }

    return TRUE;
}

static inline PyObject *
_invoke_marshal_out_args (PyGIInvokeState *state, PyGICallableCache *cache)
{
    PyObject *py_out = NULL;
    PyObject *py_return = NULL;
    int total_out_args = cache->n_out_args;
    gboolean has_return = FALSE;

    state->current_arg = 0;

    if (cache->return_cache) {
        state->stage = PYGI_INVOKE_STAGE_MARSHAL_RETURN_START;
        if (cache->is_constructor) {
            if (state->return_arg.v_pointer == NULL) {
                PyErr_SetString (PyExc_TypeError, "constructor returned NULL");
                pygi_marshal_cleanup_args_return_fail (state,
                                                       cache);
                return NULL;
            }
        }

        py_return = cache->return_cache->out_marshaller ( state,
                                                          cache,
                                                          cache->return_cache,
                                                         &state->return_arg);
        if (py_return == NULL) {
            pygi_marshal_cleanup_args_return_fail (state,
                                                   cache);
            return NULL;
        }

        state->stage = PYGI_INVOKE_STAGE_MARSHAL_RETURN_DONE;

        if (cache->return_cache->type_tag != GI_TYPE_TAG_VOID) {
            total_out_args++;
            has_return = TRUE;
        }
    }

    total_out_args -= cache->n_out_aux_args;

    if (cache->n_out_args - cache->n_out_aux_args  == 0) {
        py_out = py_return;
    } else if (total_out_args == 1) {
        /* if we get here there is one out arg an no return */
        state->stage = PYGI_INVOKE_STAGE_MARSHAL_OUT_START;
        PyGIArgCache *arg_cache = (PyGIArgCache *)cache->out_args->data;
        py_out = arg_cache->out_marshaller (state,
                                            cache,
                                            arg_cache,
                                            state->args[arg_cache->c_arg_index]);
        if (py_out == NULL) {
            pygi_marshal_cleanup_args_out_parameter_fail (state,
                                                          cache,
                                                          0);
            return NULL;
        }

        state->stage = PYGI_INVOKE_STAGE_MARSHAL_OUT_IDLE;
    } else {
        int py_arg_index = 0;
        GSList *cache_item = cache->out_args;
        /* return a tuple */
        py_out = PyTuple_New (total_out_args);
        if (has_return) {
            PyTuple_SET_ITEM (py_out, py_arg_index, py_return);
            py_arg_index++;
        }

        for(; py_arg_index < total_out_args; py_arg_index++) {
            PyGIArgCache *arg_cache = (PyGIArgCache *)cache_item->data;
            state->stage = PYGI_INVOKE_STAGE_MARSHAL_OUT_START;
            PyObject *py_obj = arg_cache->out_marshaller (state,
                                                          cache,
                                                          arg_cache,
                                                          state->args[arg_cache->c_arg_index]);

            if (py_obj == NULL) {
                if (has_return)
                    py_arg_index--;
 
                pygi_marshal_cleanup_args_out_parameter_fail (state,
                                                              cache,
                                                              py_arg_index);
                Py_DECREF (py_out);
                return NULL;
            }

            state->current_arg++;
            state->stage = PYGI_INVOKE_STAGE_MARSHAL_OUT_IDLE;

            PyTuple_SET_ITEM (py_out, py_arg_index, py_obj);
            cache_item = cache_item->next;
        }
    }
    return py_out;
}

PyObject *
_wrap_g_callable_info_invoke (PyGIBaseInfo *self,
                              PyObject *py_args,
                              PyObject *kwargs)
{
    PyGIInvokeState state = { 0, };
    PyObject *ret = NULL;

    if (self->cache == NULL) {
        self->cache = _pygi_callable_cache_new (self->info);
        if (self->cache == NULL)
            return NULL;
    }

    _invoke_state_init_from_callable_cache (&state, self->cache, py_args, kwargs);
    if (!_invoke_marshal_in_args (&state, self->cache))
        goto err;

    if (!_invoke_callable (&state, self->cache, self->info))
        goto err;

    pygi_marshal_cleanup_args_in_marshal_success (&state, self->cache);

    ret = _invoke_marshal_out_args (&state, self->cache);
    if (ret)
        pygi_marshal_cleanup_args_out_marshal_success (&state, self->cache);
err:
    _invoke_state_clear (&state, self->cache);
    return ret;
}

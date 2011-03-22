/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
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

#include "pygi-invoke.h"


static inline gboolean
_invoke_function (PyGIInvokeState *state,
                  PyGIFunctionCache *cache,
                  GIFunctionInfo *function_info)
{
    GError *error;
    gint retval;

    error = NULL;

    pyg_begin_allow_threads;

    /* FIXME: use this for now but we can streamline the call */
    retval = g_function_info_invoke ( (GIFunctionInfo *) function_info,
                                      state->in_args,
                                      cache->n_in_args,
                                      state->out_args,
                                      cache->n_out_args,
                                      &state->return_arg,
                                      &error);
    pyg_end_allow_threads;

    if (!retval) {
        g_assert (error != NULL);
        /* TODO: raise the right error, out of the error domain. */
        PyErr_SetString (PyExc_RuntimeError, error->message);
        g_error_free (error);

        /* TODO: release input arguments. */

        return FALSE;
    }

    if (state->error != NULL) {
        /* TODO: raise the right error, out of the error domain, if applicable. */
        PyErr_SetString (PyExc_Exception, state->error->message);

        /* TODO: release input arguments. */

        return FALSE;
    }

    return TRUE;
}

static inline gboolean
_invoke_state_init_from_function_cache(PyGIInvokeState *state,
                                       PyGIFunctionCache *cache,
                                       PyObject *py_args)
{
    state->py_in_args = py_args;
    state->n_py_in_args = PySequence_Length(py_args);
    state->args = g_slice_alloc0(cache->n_args * sizeof(GIArgument *));
    if (state->args == NULL && cache->n_args != 0) {
        PyErr_NoMemory();
        return FALSE;
    }

    state->in_args = g_slice_alloc0(cache->n_in_args * sizeof(GIArgument));
    if (state->in_args == NULL && cache->n_in_args != 0) {
        PyErr_NoMemory();
        return FALSE;
    }

    state->out_values = g_slice_alloc0(cache->n_out_args * sizeof(GIArgument));
    if (state->out_values == NULL && cache->n_out_args != 0) {
        PyErr_NoMemory();
        return FALSE;
    }

    state->out_args = g_slice_alloc0(cache->n_out_args * sizeof(GIArgument));
    if (state->out_args == NULL && cache->n_out_args != 0) {
        PyErr_NoMemory();
        return FALSE;
    }

    state->error = NULL;

    return TRUE;
}

static inline void
_invoke_state_clear(PyGIInvokeState *state, PyGIFunctionCache *cache)
{
    g_slice_free1(cache->n_args * sizeof(GIArgument *), state->args);
    g_slice_free1(cache->n_in_args * sizeof(GIArgument), state->in_args);
    g_slice_free1(cache->n_out_args * sizeof(GIArgument), state->out_args);
    g_slice_free1(cache->n_out_args * sizeof(GIArgument), state->out_values);
}

static inline gboolean
_invoke_marshal_in_args(PyGIInvokeState *state, PyGIFunctionCache *cache)
{
    int i, in_count, out_count;
    in_count = 0;
    out_count = 0;

    if (state->n_py_in_args > cache->n_py_args) {
        PyErr_Format(PyExc_TypeError,
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

        switch (arg_cache->direction) {
            case GI_DIRECTION_IN:
                state->args[i] = &(state->in_args[in_count]);
                in_count++;

                if (arg_cache->aux_type > 0)
                    continue;

                if (arg_cache->py_arg_index >= state->n_py_in_args) {
                    PyErr_Format(PyExc_TypeError,
                                 "%s() takes exactly %zd argument(s) (%zd given)",
                                  cache->name,
                                  cache->n_py_args,
                                  state->n_py_in_args);
                    return FALSE;
                }

                py_arg =
                    PyTuple_GET_ITEM(state->py_in_args,
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
                        PyErr_Format(PyExc_TypeError,
                                     "%s() takes exactly %zd argument(s) (%zd given)",
                                      cache->name,
                                      cache->n_py_args,
                                      state->n_py_in_args);
                        return FALSE;
                    }

                    py_arg =
                        PyTuple_GET_ITEM(state->py_in_args,
                                         arg_cache->py_arg_index);
                }
            case GI_DIRECTION_OUT:
                if (arg_cache->is_caller_allocates) {
                    PyGIInterfaceCache *iface_cache =
                        (PyGIInterfaceCache *)arg_cache;

                    g_assert(arg_cache->type_tag == GI_TYPE_TAG_INTERFACE);

                    state->out_args[out_count].v_pointer = NULL;
                    state->args[i] = &state->out_args[out_count];
                    if (iface_cache->g_type == G_TYPE_BOXED) {
                        state->args[i]->v_pointer =
                            _pygi_boxed_alloc(iface_cache->interface_info, NULL);
                    } else if (iface_cache->is_foreign) {
                        PyObject *foreign_struct =
                            pygi_struct_foreign_convert_from_g_argument(
                                iface_cache->interface_info,
                                NULL);

                        pygi_struct_foreign_convert_to_g_argument(
                            foreign_struct,
                            iface_cache->interface_info,
                            GI_TRANSFER_EVERYTHING,
                            state->args[i]);
                    } else {
                        gssize size =
                            g_struct_info_get_size(
                                (GIStructInfo *)iface_cache->interface_info);
                        state->args[i]->v_pointer = g_malloc0(size);
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
                PyErr_Format(PyExc_TypeError,
                             "Argument %i does not allow None as a value",
                             i);

                return FALSE;
            }
            gboolean success = arg_cache->in_marshaller(state,
                                                        cache,
                                                        arg_cache,
                                                        py_arg,
                                                        c_arg);
            if (!success)
                return FALSE;
        }

    }

    return TRUE;
}

static inline PyObject *
_invoke_marshal_out_args(PyGIInvokeState *state, PyGIFunctionCache *cache)
{
    PyObject *py_out = NULL;
    PyObject *py_return = NULL;
    int total_out_args = cache->n_out_args;
    gboolean has_return = FALSE;

    if (cache->return_cache) {
        if (cache->is_constructor) {
            if (state->return_arg.v_pointer == NULL) {
                PyErr_SetString (PyExc_TypeError, "constructor returned NULL");
                return NULL;
            }
        }

        py_return = cache->return_cache->out_marshaller(state,
                                                        cache,
                                                        cache->return_cache,
                                                        &state->return_arg);
        if (py_return == NULL)
            return NULL;

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
        PyGIArgCache *arg_cache = (PyGIArgCache *)cache->out_args->data;
        py_out = arg_cache->out_marshaller(state,
                                           cache,
                                           arg_cache,
                                           state->args[arg_cache->c_arg_index]);
    } else {
        int out_cache_index = 0;
        int py_arg_index = 0;
        GSList *cache_item = cache->out_args;
        /* return a tuple */
        py_out = PyTuple_New(total_out_args);
        if (has_return) {
            PyTuple_SET_ITEM(py_out, py_arg_index, py_return);
            py_arg_index++;
        }

        for(; py_arg_index < total_out_args; py_arg_index++) {
            PyGIArgCache *arg_cache = (PyGIArgCache *)cache_item->data;
            PyObject *py_obj = arg_cache->out_marshaller(state,
                                                         cache,
                                                         arg_cache,
                                                         state->args[arg_cache->c_arg_index]);

            if (py_obj == NULL)
                return NULL;

            PyTuple_SET_ITEM(py_out, py_arg_index, py_obj);
            cache_item = cache_item->next;
        }
    }
    return py_out;
}

PyObject *
_wrap_g_function_info_invoke (PyGIBaseInfo *self, PyObject *py_args)
{
    PyGIInvokeState state = { 0, };
    PyObject *ret;

    if (self->cache == NULL) {
        self->cache = _pygi_function_cache_new(self->info);
        if (self->cache == NULL)
            return NULL;
    }

    _invoke_state_init_from_function_cache(&state, self->cache, py_args);
    if (!_invoke_marshal_in_args (&state, self->cache))
        goto err;

    if (!_invoke_function(&state, self->cache, self->info))
        goto err;

    ret = _invoke_marshal_out_args (&state, self->cache);
    _invoke_state_clear (&state, self->cache);
    return ret;

err:
    _invoke_state_clear (&state, self->cache);
    return NULL;
}


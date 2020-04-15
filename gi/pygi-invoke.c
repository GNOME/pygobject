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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "pygi-invoke.h"
#include "pygi-marshal-cleanup.h"
#include "pygi-error.h"
#include "pygi-resulttuple.h"
#include "pygi-foreign.h"
#include "pygi-boxed.h"

extern PyObject *_PyGIDefaultArgPlaceholder;

static gboolean
_check_for_unexpected_kwargs (PyGICallableCache *cache,
                              GHashTable  *arg_name_hash,
                              PyObject    *py_kwargs)
{
    PyObject *dict_key, *dict_value;
    Py_ssize_t dict_iter_pos = 0;

    while (PyDict_Next (py_kwargs, &dict_iter_pos, &dict_key, &dict_value)) {
        PyObject *key;

        {
            key = PyUnicode_AsUTF8String (dict_key);
            if (key == NULL) {
                return FALSE;
            }
        }

        /* Use extended lookup because it returns whether or not the key actually
         * exists in the hash table. g_hash_table_lookup returns NULL for keys not
         * found which maps to index 0 for our hash lookup.
         */
        if (!g_hash_table_lookup_extended (arg_name_hash, PyBytes_AsString(key), NULL, NULL)) {
            char *full_name = pygi_callable_cache_get_full_name (cache);
            PyErr_Format (PyExc_TypeError,
                          "%.200s() got an unexpected keyword argument '%.400s'",
                          full_name,
                          PyBytes_AsString (key));
            Py_DECREF (key);
            g_free (full_name);
            return FALSE;
        }

        Py_DECREF (key);
    }
    return TRUE;
}

/**
 * _py_args_combine_and_check_length:
 * @cache: PyGICallableCache
 * @py_args: the tuple of positional arguments.
 * @py_kwargs: the dict of keyword arguments to be merged with py_args.
 *
 * Returns: New value reference to the combined py_args and py_kwargs.
 */
static PyObject *
_py_args_combine_and_check_length (PyGICallableCache *cache,
                                   PyObject    *py_args,
                                   PyObject    *py_kwargs)
{
    PyObject *combined_py_args = NULL;
    Py_ssize_t n_py_args, n_py_kwargs, i;
    gssize n_expected_args = cache->n_py_args;
    GSList *l;

    n_py_args = PyTuple_GET_SIZE (py_args);
    if (py_kwargs == NULL)
        n_py_kwargs = 0;
    else
        n_py_kwargs = PyDict_Size (py_kwargs);

    /* Fast path, we already have the exact number of args and not kwargs. */
    if (n_py_kwargs == 0 && n_py_args == n_expected_args && cache->user_data_varargs_index < 0) {
        Py_INCREF (py_args);
        return py_args;
    }

    if (cache->user_data_varargs_index < 0 && n_expected_args < n_py_args) {
        char *full_name = pygi_callable_cache_get_full_name (cache);
        PyErr_Format (PyExc_TypeError,
                      "%.200s() takes exactly %zd %sargument%s (%zd given)",
                      full_name,
                      n_expected_args,
                      n_py_kwargs > 0 ? "non-keyword " : "",
                      n_expected_args == 1 ? "" : "s",
                      n_py_args);
        g_free (full_name);
        return NULL;
    }

    if (cache->user_data_varargs_index >= 0 && n_py_kwargs > 0 && n_expected_args < n_py_args) {
        char *full_name = pygi_callable_cache_get_full_name (cache);
        PyErr_Format (PyExc_TypeError,
                      "%.200s() cannot use variable user data arguments with keyword arguments",
                      full_name);
        g_free (full_name);
        return NULL;
    }

    if (n_py_kwargs > 0 && !_check_for_unexpected_kwargs (cache,
                                                          cache->arg_name_hash,
                                                          py_kwargs)) {
        return NULL;
    }

    /* will hold arguments from both py_args and py_kwargs
     * when they are combined into a single tuple */
    combined_py_args = PyTuple_New (n_expected_args);

    for (i = 0, l = cache->arg_name_list; i < n_expected_args && l; i++, l = l->next) {
        PyObject *py_arg_item = NULL;
        PyObject *kw_arg_item = NULL;
        const gchar *arg_name = l->data;
        int arg_cache_index = -1;
        gboolean is_varargs_user_data = FALSE;

        if (arg_name != NULL)
            arg_cache_index = GPOINTER_TO_INT (g_hash_table_lookup (cache->arg_name_hash, arg_name));

        is_varargs_user_data = cache->user_data_varargs_index >= 0 &&
                                arg_cache_index == cache->user_data_varargs_index;

        if (n_py_kwargs > 0 && arg_name != NULL) {
            /* NULL means this argument has no keyword name */
            /* ex. the first argument to a method or constructor */
            kw_arg_item = PyDict_GetItemString (py_kwargs, arg_name);
        }

        /* use a bounded retrieval of the original input */
        if (i < n_py_args)
            py_arg_item = PyTuple_GET_ITEM (py_args, i);

        if (kw_arg_item == NULL && py_arg_item != NULL) {
            if (is_varargs_user_data) {
                /* For tail end user_data varargs, pull a slice off and we are done. */
                PyObject *user_data = PyTuple_GetSlice (py_args, i, PY_SSIZE_T_MAX);
                PyTuple_SET_ITEM (combined_py_args, i, user_data);
                return combined_py_args;
            } else {
                Py_INCREF (py_arg_item);
                PyTuple_SET_ITEM (combined_py_args, i, py_arg_item);
            }
        } else if (kw_arg_item != NULL && py_arg_item == NULL) {
            if (is_varargs_user_data) {
                /* Special case where user_data is passed as a keyword argument (user_data=foo)
                 * Wrap the value in a tuple to represent variable args for marshaling later on.
                 */
                PyObject *user_data = Py_BuildValue("(O)", kw_arg_item, NULL);
                PyTuple_SET_ITEM (combined_py_args, i, user_data);
            } else {
                Py_INCREF (kw_arg_item);
                PyTuple_SET_ITEM (combined_py_args, i, kw_arg_item);
            }

        } else if (kw_arg_item == NULL && py_arg_item == NULL) {
            if (is_varargs_user_data) {
                /* For varargs user_data, pass an empty tuple when nothing is given. */
                PyTuple_SET_ITEM (combined_py_args, i, PyTuple_New (0));
            } else if (arg_cache_index >= 0 && _pygi_callable_cache_get_arg (cache, arg_cache_index)->has_default) {
                /* If the argument supports a default, use a place holder in the
                 * argument tuple, this will be checked later during marshaling.
                 */
                Py_INCREF (_PyGIDefaultArgPlaceholder);
                PyTuple_SET_ITEM (combined_py_args, i, _PyGIDefaultArgPlaceholder);
            } else {
                char *full_name = pygi_callable_cache_get_full_name (cache);
                PyErr_Format (PyExc_TypeError,
                              "%.200s() takes exactly %zd %sargument%s (%zd given)",
                              full_name,
                              n_expected_args,
                              n_py_kwargs > 0 ? "non-keyword " : "",
                              n_expected_args == 1 ? "" : "s",
                              n_py_args);
                g_free (full_name);

                Py_DECREF (combined_py_args);
                return NULL;
            }
        } else if (kw_arg_item != NULL && py_arg_item != NULL) {
            char *full_name = pygi_callable_cache_get_full_name (cache);
            PyErr_Format (PyExc_TypeError,
                          "%.200s() got multiple values for keyword argument '%.200s'",
                          full_name,
                          arg_name);

            Py_DECREF (combined_py_args);
            g_free (full_name);
            return NULL;
        }
    }

    return combined_py_args;
}

/* To reduce calls to g_slice_*() we (1) allocate all the memory depended on
 * the argument count in one go and (2) keep one version per argument count
 * around for faster reuse.
 */

#define PyGI_INVOKE_ARG_STATE_SIZE(n)   (n * (sizeof (PyGIInvokeArgState) + sizeof (GIArgument *)))
#define PyGI_INVOKE_ARG_STATE_N_MAX     10
static gpointer free_arg_state[PyGI_INVOKE_ARG_STATE_N_MAX];

/**
 * _pygi_invoke_arg_state_init:
 * Sets PyGIInvokeState.args and PyGIInvokeState.ffi_args.
 * On error returns FALSE and sets an exception.
 */
gboolean
_pygi_invoke_arg_state_init (PyGIInvokeState *state) {

    gpointer mem;

    if (state->n_args < PyGI_INVOKE_ARG_STATE_N_MAX && (mem = free_arg_state[state->n_args]) != NULL) {
        free_arg_state[state->n_args] = NULL;
        memset (mem, 0, PyGI_INVOKE_ARG_STATE_SIZE (state->n_args));
    } else {
        mem = g_slice_alloc0 (PyGI_INVOKE_ARG_STATE_SIZE (state->n_args));
    }

    if (mem == NULL && state->n_args != 0) {
        PyErr_NoMemory();
        return FALSE;
    }

    if (mem != NULL) {
        state->args = mem;
        state->ffi_args = (gpointer)((gchar *)mem + state->n_args * sizeof (PyGIInvokeArgState));
    }

    return TRUE;
}

/**
 * _pygi_invoke_arg_state_free:
 * Frees PyGIInvokeState.args and PyGIInvokeState.ffi_args
 */
void
_pygi_invoke_arg_state_free(PyGIInvokeState *state) {
    if (state->n_args < PyGI_INVOKE_ARG_STATE_N_MAX && free_arg_state[state->n_args] == NULL) {
        free_arg_state[state->n_args] = state->args;
        return;
    }

    g_slice_free1 (PyGI_INVOKE_ARG_STATE_SIZE (state->n_args), state->args);
}

static gboolean
_invoke_state_init_from_cache (PyGIInvokeState *state,
                               PyGIFunctionCache *function_cache,
                               PyObject *py_args,
                               PyObject *kwargs)
{
    PyGICallableCache *cache = (PyGICallableCache *) function_cache;

    state->n_args = _pygi_callable_cache_args_len (cache);

    if (cache->throws) {
        state->n_args++;
    }

    /* Copy the function pointer to the state for the normal case. For vfuncs,
     * this has already been filled out based on the implementor's GType.
     */
    if (state->function_ptr == NULL)
        state->function_ptr = function_cache->invoker.native_address;

    state->py_in_args = _py_args_combine_and_check_length (cache,
                                                           py_args,
                                                           kwargs);

    if (state->py_in_args == NULL) {
        return FALSE;
    }
    state->n_py_in_args = PyTuple_Size (state->py_in_args);

    if (!_pygi_invoke_arg_state_init (state)) {
        return FALSE;
    }

    state->error = NULL;

    if (cache->throws) {
        gssize error_index = state->n_args - 1;
        /* The ffi argument for GError needs to be a triple pointer. */
        state->args[error_index].arg_pointer.v_pointer = &state->error;
        state->ffi_args[error_index] = &(state->args[error_index].arg_pointer);
    }

    return TRUE;
}

static void
_invoke_state_clear (PyGIInvokeState *state, PyGIFunctionCache *function_cache)
{
    _pygi_invoke_arg_state_free (state);
    Py_XDECREF (state->py_in_args);
}

static gboolean
_caller_alloc (PyGIArgCache *arg_cache, GIArgument *arg)
{
    if (arg_cache->type_tag == GI_TYPE_TAG_INTERFACE) {
        PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

        arg->v_pointer = NULL;
        if (g_type_is_a (iface_cache->g_type, G_TYPE_BOXED)) {
            arg->v_pointer =
                pygi_boxed_alloc (iface_cache->interface_info, NULL);
        } else if (iface_cache->g_type == G_TYPE_VALUE) {
            arg->v_pointer = g_slice_new0 (GValue);
        } else if (iface_cache->is_foreign) {
            PyObject *foreign_struct =
                pygi_struct_foreign_convert_from_g_argument (
                    iface_cache->interface_info,
                    GI_TRANSFER_NOTHING,
                    NULL);

                pygi_struct_foreign_convert_to_g_argument (foreign_struct,
                                                           iface_cache->interface_info,
                                                           GI_TRANSFER_EVERYTHING,
                                                           arg);
        } else {
                gssize size = g_struct_info_get_size(
                    (GIStructInfo *)iface_cache->interface_info);
                arg->v_pointer = g_malloc0 (size);
        }
    } else if (arg_cache->type_tag == GI_TYPE_TAG_ARRAY) {
        PyGIArgGArray *array_cache = (PyGIArgGArray *)arg_cache;

        arg->v_pointer = g_array_new (TRUE, TRUE, (guint)array_cache->item_size);
    } else {
        return FALSE;
    }

    if (arg->v_pointer == NULL)
        return FALSE;


    return TRUE;
}

/* pygi_invoke_marshal_in_args:
 *
 * Fills out the state struct argument lists. arg_values will always hold
 * actual values marshaled either to or from Python and C. arg_pointers will
 * hold pointers (via v_pointer) to auxilary value storage. This will normally
 * point to values stored in arg_values. In the case of caller allocated
 * out args, arg_pointers[x].v_pointer will point to newly allocated memory.
 * arg_pointers inserts a level of pointer indirection between arg_values
 * and the argument list ffi receives when dealing with non-caller allocated
 * out arguments.
 *
 * For example:
 * [[
 *  void callee (int *i, int j) { *i = 50 - j; }
 *  void caller () {
 *    int i = 0;
 *    callee (&i, 8);
 *  }
 *
 *  args[0] == &arg_pointers[0];
 *  arg_pointers[0].v_pointer == &arg_values[0];
 *  arg_values[0].v_int == 42;
 *
 *  args[1] == &arg_values[1];
 *  arg_values[1].v_int == 8;
 * ]]
 *
 */
static gboolean
_invoke_marshal_in_args (PyGIInvokeState *state, PyGIFunctionCache *function_cache)
{
    PyGICallableCache *cache = (PyGICallableCache *) function_cache;
    gssize i;

    if (state->n_py_in_args > cache->n_py_args) {
        char *full_name = pygi_callable_cache_get_full_name (cache);
        PyErr_Format (PyExc_TypeError,
                      "%s() takes exactly %zd argument(s) (%zd given)",
                      full_name,
                      cache->n_py_args,
                      state->n_py_in_args);
        g_free (full_name);
        return FALSE;
    }

    for (i = 0; (gsize)i < _pygi_callable_cache_args_len (cache); i++) {
        GIArgument *c_arg = &state->args[i].arg_value;
        PyGIArgCache *arg_cache = g_ptr_array_index (cache->args_cache, i);
        PyObject *py_arg = NULL;

        switch (arg_cache->direction) {
            case PYGI_DIRECTION_FROM_PYTHON:
                /* The ffi argument points directly at memory in arg_values. */
                state->ffi_args[i] = c_arg;

                if (arg_cache->meta_type == PYGI_META_ARG_TYPE_CLOSURE) {
                    state->ffi_args[i]->v_pointer = state->user_data;
                    continue;
                } else if (arg_cache->meta_type != PYGI_META_ARG_TYPE_PARENT)
                    continue;

                if (arg_cache->py_arg_index >= state->n_py_in_args) {
                    char *full_name = pygi_callable_cache_get_full_name (cache);
                    PyErr_Format (PyExc_TypeError,
                                  "%s() takes exactly %zd argument(s) (%zd given)",
                                   full_name,
                                   cache->n_py_args,
                                   state->n_py_in_args);
                    g_free (full_name);

                    /* clean up all of the args we have already marshalled,
                     * since invoke will not be called
                     */
                    pygi_marshal_cleanup_args_from_py_parameter_fail (state,
                                                                      cache,
                                                                      i);
                    return FALSE;
                }

                py_arg =
                    PyTuple_GET_ITEM (state->py_in_args,
                                      arg_cache->py_arg_index);

                break;
            case PYGI_DIRECTION_BIDIRECTIONAL:
                if (arg_cache->meta_type != PYGI_META_ARG_TYPE_CHILD) {
                    if (arg_cache->py_arg_index >= state->n_py_in_args) {
                        char *full_name = pygi_callable_cache_get_full_name (cache);
                        PyErr_Format (PyExc_TypeError,
                                      "%s() takes exactly %zd argument(s) (%zd given)",
                                       full_name,
                                       cache->n_py_args,
                                       state->n_py_in_args);
                        g_free (full_name);
                        pygi_marshal_cleanup_args_from_py_parameter_fail (state,
                                                                          cache,
                                                                          i);
                        return FALSE;
                    }

                    py_arg =
                        PyTuple_GET_ITEM (state->py_in_args,
                                          arg_cache->py_arg_index);
                }
                /* Fall through */

            case PYGI_DIRECTION_TO_PYTHON:
                /* arg_pointers always stores a pointer to the data to be marshaled "to python"
                 * even in cases where arg_pointers is not being used as indirection between
                 * ffi and arg_values. This gives a guarantee that out argument marshaling
                 * (_invoke_marshal_out_args) can always rely on arg_pointers pointing to
                 * the correct chunk of memory to marshal.
                 */
                state->args[i].arg_pointer.v_pointer = c_arg;

                if (arg_cache->is_caller_allocates) {
                    /* In the case of caller allocated out args, we don't use
                     * an extra level of indirection and state->args will point
                     * directly at the data to be marshaled. However, as noted
                     * above, arg_pointers will also point to this caller allocated
                     * chunk of memory used by out argument marshaling.
                     */
                    state->ffi_args[i] = c_arg;

                    if (!_caller_alloc (arg_cache, c_arg)) {
                        char *full_name = pygi_callable_cache_get_full_name (cache);
                        PyErr_Format (PyExc_TypeError,
                                      "Could not caller allocate argument %zd of callable %s",
                                      i, full_name);
                        g_free (full_name);
                        pygi_marshal_cleanup_args_from_py_parameter_fail (state,
                                                                          cache,
                                                                          i);
                        return FALSE;
                    }
                } else {
                    /* Non-caller allocated out args will use arg_pointers as an
                     * extra level of indirection */
                    state->ffi_args[i] = &state->args[i].arg_pointer;
                }

                break;
            default:
                g_assert_not_reached();
                break;
        }

        if (py_arg == _PyGIDefaultArgPlaceholder) {
            *c_arg = arg_cache->default_value;
        } else if (arg_cache->from_py_marshaller != NULL &&
                   arg_cache->meta_type != PYGI_META_ARG_TYPE_CHILD) {
            gboolean success;
            gpointer cleanup_data = NULL;

            if (!arg_cache->allow_none && py_arg == Py_None) {
                PyErr_Format (PyExc_TypeError,
                              "Argument %zd does not allow None as a value",
                              i);

                 pygi_marshal_cleanup_args_from_py_parameter_fail (state,
                                                                   cache,
                                                                   i);
                 return FALSE;
            }
            success = arg_cache->from_py_marshaller (state,
                                                     cache,
                                                     arg_cache,
                                                     py_arg,
                                                     c_arg,
                                                     &cleanup_data);
            state->args[i].arg_cleanup_data = cleanup_data;

            if (!success) {
                pygi_marshal_cleanup_args_from_py_parameter_fail (state,
                                                                  cache,
                                                                  i);
                return FALSE;
            }

        }

    }

    return TRUE;
}

static PyObject *
_invoke_marshal_out_args (PyGIInvokeState *state, PyGIFunctionCache *function_cache)
{
    PyGICallableCache *cache = (PyGICallableCache *) function_cache;
    PyObject *py_out = NULL;
    PyObject *py_return = NULL;
    gssize n_out_args = cache->n_to_py_args - cache->n_to_py_child_args;

    if (cache->return_cache) {
        if (!cache->return_cache->is_skipped) {
            gpointer cleanup_data = NULL;
            py_return = cache->return_cache->to_py_marshaller ( state,
                                                                cache,
                                                                cache->return_cache,
                                                               &state->return_arg,
                                                               &cleanup_data);
            state->to_py_return_arg_cleanup_data = cleanup_data;
            if (py_return == NULL) {
                pygi_marshal_cleanup_args_return_fail (state,
                                                       cache);
                return NULL;
            }
        } else {
            if (cache->return_cache->transfer == GI_TRANSFER_EVERYTHING) {
                PyGIMarshalToPyCleanupFunc to_py_cleanup =
                    cache->return_cache->to_py_cleanup;

                if (to_py_cleanup != NULL)
                    to_py_cleanup ( state,
                                    cache->return_cache,
                                    NULL,
                                   &state->return_arg,
                                    FALSE);
            }
        }
    }

    if (n_out_args == 0) {
        if (cache->return_cache->is_skipped && state->error == NULL) {
            /* we skip the return value and have no (out) arguments to return,
             * so py_return should be NULL. But we must not return NULL,
             * otherwise Python will expect an exception.
             */
            g_assert (py_return == NULL);
            Py_INCREF(Py_None);
            py_return = Py_None;
        }

        py_out = py_return;
    } else if (!cache->has_return && n_out_args == 1) {
        /* if we get here there is one out arg an no return */
        PyGIArgCache *arg_cache = (PyGIArgCache *)cache->to_py_args->data;
        gpointer cleanup_data = NULL;
        py_out = arg_cache->to_py_marshaller (state,
                                              cache,
                                              arg_cache,
                                              state->args[arg_cache->c_arg_index].arg_pointer.v_pointer,
                                              &cleanup_data);
        state->args[arg_cache->c_arg_index].to_py_arg_cleanup_data = cleanup_data;
        if (py_out == NULL) {
            pygi_marshal_cleanup_args_to_py_parameter_fail (state,
                                                            cache,
                                                            0);
            return NULL;
        }

    } else {
        /* return a tuple */
        gssize py_arg_index = 0;
        GSList *cache_item = cache->to_py_args;
        gssize tuple_len = cache->has_return + n_out_args;

        py_out = pygi_resulttuple_new (cache->resulttuple_type, tuple_len);

        if (py_out == NULL) {
            pygi_marshal_cleanup_args_to_py_parameter_fail (state,
                                                            cache,
                                                            py_arg_index);
            return NULL;
        }

        if (cache->has_return) {
            PyTuple_SET_ITEM (py_out, py_arg_index, py_return);
            py_arg_index++;
        }

        for (; py_arg_index < tuple_len; py_arg_index++) {
            PyGIArgCache *arg_cache = (PyGIArgCache *)cache_item->data;
            gpointer cleanup_data = NULL;
            PyObject *py_obj = arg_cache->to_py_marshaller (state,
                                                            cache,
                                                            arg_cache,
                                                            state->args[arg_cache->c_arg_index].arg_pointer.v_pointer,
                                                            &cleanup_data);
            state->args[arg_cache->c_arg_index].to_py_arg_cleanup_data = cleanup_data;

            if (py_obj == NULL) {
                if (cache->has_return)
                    py_arg_index--;

                pygi_marshal_cleanup_args_to_py_parameter_fail (state,
                                                                cache,
                                                                py_arg_index);
                Py_DECREF (py_out);
                return NULL;
            }

            PyTuple_SET_ITEM (py_out, py_arg_index, py_obj);
            cache_item = cache_item->next;
        }
    }
    return py_out;
}

PyObject *
pygi_invoke_c_callable (PyGIFunctionCache *function_cache,
                        PyGIInvokeState *state,
                        PyObject *py_args,
                        PyObject *py_kwargs)
{
    PyGICallableCache *cache = (PyGICallableCache *) function_cache;
    GIFFIReturnValue ffi_return_value = {0};
    PyObject *ret = NULL;

    if (!_invoke_state_init_from_cache (state, function_cache,
                                        py_args, py_kwargs))
         goto err;

    if (!_invoke_marshal_in_args (state, function_cache))
         goto err;

    Py_BEGIN_ALLOW_THREADS;

        ffi_call (&function_cache->invoker.cif,
                  state->function_ptr,
                  (void *) &ffi_return_value,
                  (void **) state->ffi_args);

    Py_END_ALLOW_THREADS;

    /* If the callable throws, the address of state->error will be bound into
     * the state->args as the last value. When the callee sets an error using
     * the state->args passed, it will have the side effect of setting
     * state->error allowing for easy checking here.
     */
    if (state->error != NULL) {
        if (pygi_error_check (&state->error)) {
            /* even though we errored out, the call itself was successful,
               so we assume the call processed all of the parameters */
            pygi_marshal_cleanup_args_from_py_marshal_success (state, cache);
            goto err;
        }
    }

    if (cache->return_cache) {
        gi_type_info_extract_ffi_return_value (cache->return_cache->type_info,
                                               &ffi_return_value,
                                               &state->return_arg);
    }

    ret = _invoke_marshal_out_args (state, function_cache);
    pygi_marshal_cleanup_args_from_py_marshal_success (state, cache);

    if (ret != NULL)
        pygi_marshal_cleanup_args_to_py_marshal_success (state, cache);

err:
    _invoke_state_clear (state, function_cache);
    return ret;
}

PyObject *
pygi_callable_info_invoke (GIBaseInfo *info, PyObject *py_args,
                           PyObject *kwargs, PyGICallableCache *cache,
                           gpointer user_data)
{
    return pygi_function_cache_invoke ((PyGIFunctionCache *) cache,
                                       py_args, kwargs);
}

PyObject *
_wrap_g_callable_info_invoke (PyGIBaseInfo *self, PyObject *py_args,
                              PyObject *kwargs)
{
    if (self->cache == NULL) {
        PyGIFunctionCache *function_cache;
        GIInfoType type = g_base_info_get_type (self->info);

        if (type == GI_INFO_TYPE_FUNCTION) {
            GIFunctionInfoFlags flags;

            flags = g_function_info_get_flags ( (GIFunctionInfo *)self->info);

            if (flags & GI_FUNCTION_IS_CONSTRUCTOR) {
                function_cache = pygi_constructor_cache_new (self->info);
            } else if (flags & GI_FUNCTION_IS_METHOD) {
                function_cache = pygi_method_cache_new (self->info);
            } else {
                function_cache = pygi_function_cache_new (self->info);
            }
        } else if (type == GI_INFO_TYPE_VFUNC) {
            function_cache = pygi_vfunc_cache_new (self->info);
        } else if (type == GI_INFO_TYPE_CALLBACK) {
            g_error ("Cannot invoke callback types");
        } else {
            function_cache = pygi_method_cache_new (self->info);
        }

        self->cache = (PyGICallableCache *)function_cache;
        if (self->cache == NULL)
            return NULL;
    }

    return pygi_callable_info_invoke (self->info, py_args, kwargs, self->cache, NULL);
}

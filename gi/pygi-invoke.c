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

#include <pythoncapi_compat.h>

#include "pygi-async.h"
#include "pygi-boxed.h"
#include "pygi-error.h"
#include "pygi-foreign.h"
#include "pygi-invoke.h"
#include "pygi-marshal-cleanup.h"
#include "pygi-resulttuple.h"

extern PyObject *_PyGIDefaultArgPlaceholder;

static PyGIArgCache *
next_python_argument (PyGICallableCache *cache, Py_ssize_t i,
                      Py_ssize_t *skipped_args)
{
    PyGIArgCache *arg_cache =
        g_ptr_array_index (cache->args_cache, i + *skipped_args);

    /* Skip over automatically filled in arguments (e.g. GDestroyNotify) */
    while (arg_cache->py_arg_index < 0 || i != arg_cache->py_arg_index) {
        *skipped_args += 1;
        g_assert (i + *skipped_args < (Py_ssize_t)cache->args_cache->len);
        arg_cache = g_ptr_array_index (cache->args_cache, i + *skipped_args);
    }
    return arg_cache;
}

/**
 * _py_args_combine_and_check_length:
 * @cache: PyGICallableCache
 * @py_args: an array of arguments as in the vectorcall protocol
 * @py_nargsf: the argument count plus flags
 * @py_kwnames: a tuple of keyword names
 *
 * Returns: New value reference to the combined py_args and py_kwargs.
 */
static PyObject *
_py_args_combine_and_check_length (PyGICallableCache *cache,
                                   PyObject *const *py_args, size_t py_nargsf,
                                   PyObject *py_kwnames)
{
    PyObject *combined_py_args = NULL;
    Py_ssize_t n_py_args, n_py_kwargs, i, skipped_args = 0;
    gssize n_expected_args = cache->n_py_args;

    n_py_args = PyVectorcall_NARGS (py_nargsf);
    if (py_kwnames == NULL)
        n_py_kwargs = 0;
    else
        n_py_kwargs = PyTuple_GET_SIZE (py_kwnames);

    if (cache->user_data_varargs_arg == NULL && n_expected_args < n_py_args) {
        char *full_name = pygi_callable_cache_get_full_name (cache);
        PyErr_Format (PyExc_TypeError,
                      "%.200s() takes exactly %zd %sargument%s (%zd given)",
                      full_name, n_expected_args,
                      n_py_kwargs > 0 ? "non-keyword " : "",
                      n_expected_args == 1 ? "" : "s", n_py_args);
        g_free (full_name);
        return NULL;
    }

    if (cache->user_data_varargs_arg != NULL && n_py_kwargs > 0
        && n_expected_args < n_py_args) {
        char *full_name = pygi_callable_cache_get_full_name (cache);
        PyErr_Format (PyExc_TypeError,
                      "%.200s() cannot use variable user data arguments with "
                      "keyword arguments",
                      full_name);
        g_free (full_name);
        return NULL;
    }

    /* will hold arguments from both py_args and py_kwargs
     * when they are combined into a single tuple */
    combined_py_args = PyTuple_New (n_expected_args);

    /* Add the positional arguments */
    for (i = 0; i < n_py_args && i < n_expected_args; i++) {
        PyGIArgCache *arg_cache =
            next_python_argument (cache, i, &skipped_args);

        if (arg_cache == cache->user_data_varargs_arg) {
            PyObject *user_data = PyTuple_New (n_py_args - i);
            Py_ssize_t j;

            for (j = i; j < n_py_args; j++) {
                Py_INCREF (py_args[j]);
                PyTuple_SET_ITEM (user_data, j - i, py_args[j]);
            }
            PyTuple_SET_ITEM (combined_py_args, i, user_data);
        } else {
            Py_INCREF (py_args[i]);
            PyTuple_SET_ITEM (combined_py_args, i, py_args[i]);
        }
    }

    /* Process keyword arguments */
    for (i = 0; i < n_py_kwargs; i++) {
        PyObject *py_kwname, *arg_item;
        const char *kwname;
        PyGIArgCache *arg_cache;
        gboolean is_varargs_user_data;

        py_kwname = PyTuple_GET_ITEM (py_kwnames, i);
        kwname = PyUnicode_AsUTF8AndSize (py_kwname, NULL);
        if (kwname == NULL) {
            Py_DECREF (combined_py_args);
            return NULL;
        }
        arg_cache = g_hash_table_lookup (cache->arg_name_hash, kwname);
        if (!arg_cache) {
            char *full_name = pygi_callable_cache_get_full_name (cache);
            PyErr_Format (
                PyExc_TypeError,
                "%.200s() got an unexpected keyword argument '%.400s'",
                full_name, kwname);
            g_free (full_name);
            Py_DECREF (combined_py_args);
            return NULL;
        }
        is_varargs_user_data = arg_cache == cache->user_data_varargs_arg;

        /* Have we already seen this argument? */
        arg_item =
            PyTuple_GET_ITEM (combined_py_args, arg_cache->py_arg_index);
        if (arg_item != NULL) {
            char *full_name = pygi_callable_cache_get_full_name (cache);
            PyErr_Format (
                PyExc_TypeError,
                "%.200s() got multiple values for keyword argument '%.200s'",
                full_name, kwname);
            g_free (full_name);
            Py_DECREF (combined_py_args);
            return NULL;
        }
        arg_item = py_args[n_py_args + i];
        if (is_varargs_user_data) {
            /* Special case where user_data is passed as a keyword
             * argument (user_data=foo) Wrap the value in a tuple to
             * represent variable args for marshaling later on.
             */
            PyObject *user_data = Py_BuildValue ("(O)", arg_item, NULL);
            PyTuple_SET_ITEM (combined_py_args, arg_cache->py_arg_index,
                              user_data);
        } else {
            Py_INCREF (arg_item);
            PyTuple_SET_ITEM (combined_py_args, arg_cache->py_arg_index,
                              arg_item);
        }
    }

    /* Fill in defaults and check for missing arguments */
    for (i = n_py_args; i < n_expected_args; i++) {
        PyObject *arg_item = PyTuple_GET_ITEM (combined_py_args, i);
        PyGIArgCache *arg_cache;

        if (arg_item != NULL) continue;

        arg_cache = next_python_argument (cache, i, &skipped_args);

        if (arg_cache == cache->user_data_varargs_arg) {
            /* For varargs user_data, pass an empty tuple when nothing
             * is given. */
            PyTuple_SET_ITEM (combined_py_args, i, PyTuple_New (0));
        } else if (pygi_arg_cache_allow_none (arg_cache)) {
            /* If the argument supports a default, use a place holder in the
             * argument tuple, this will be checked later during marshaling.
             */
            Py_INCREF (_PyGIDefaultArgPlaceholder);
            PyTuple_SET_ITEM (combined_py_args, i, _PyGIDefaultArgPlaceholder);
        } else {
            char *full_name = pygi_callable_cache_get_full_name (cache);
            PyErr_Format (
                PyExc_TypeError,
                "%.200s() takes exactly %zd %sargument%s (%zd given)",
                full_name, n_expected_args,
                n_py_kwargs > 0 ? "non-keyword " : "",
                n_expected_args == 1 ? "" : "s", n_py_args);
            g_free (full_name);
            Py_DECREF (combined_py_args);
            return NULL;
        }
    }

    return combined_py_args;
}

/* To reduce calls to g_slice_*() we (1) allocate all the memory depended on
 * the argument count in one go and (2) keep one version per argument count
 * around for faster reuse.
 */

#define PyGI_INVOKE_ARG_STATE_SIZE(n)                                         \
    (n * (sizeof (PyGIInvokeArgState) + sizeof (GIArgument *)))
#define PyGI_INVOKE_ARG_STATE_N_MAX 10
static gpointer free_arg_state[PyGI_INVOKE_ARG_STATE_N_MAX];

/**
 * _pygi_invoke_arg_state_init:
 * Sets PyGIInvokeState.args and PyGIInvokeState.ffi_args.
 * On error returns FALSE and sets an exception.
 */
gboolean
_pygi_invoke_arg_state_init (PyGIInvokeState *state)
{
    gpointer mem;

    if (state->n_args < PyGI_INVOKE_ARG_STATE_N_MAX
        && (mem = free_arg_state[state->n_args]) != NULL) {
        free_arg_state[state->n_args] = NULL;
        memset (mem, 0, PyGI_INVOKE_ARG_STATE_SIZE (state->n_args));
    } else {
        mem = g_slice_alloc0 (PyGI_INVOKE_ARG_STATE_SIZE (state->n_args));
    }

    if (mem == NULL && state->n_args != 0) {
        PyErr_NoMemory ();
        return FALSE;
    }

    if (mem != NULL) {
        state->args = mem;
        state->ffi_args =
            (gpointer)((gchar *)mem
                       + state->n_args * sizeof (PyGIInvokeArgState));
    }

    return TRUE;
}

/**
 * _pygi_invoke_arg_state_free:
 * Frees PyGIInvokeState.args and PyGIInvokeState.ffi_args
 */
void
_pygi_invoke_arg_state_free (PyGIInvokeState *state)
{
    if (state->n_args < PyGI_INVOKE_ARG_STATE_N_MAX
        && free_arg_state[state->n_args] == NULL) {
        free_arg_state[state->n_args] = state->args;
        return;
    }

    g_slice_free1 (PyGI_INVOKE_ARG_STATE_SIZE (state->n_args), state->args);
}

static gboolean
_invoke_state_init_from_cache (PyGIInvokeState *state,
                               PyGIFunctionCache *function_cache,
                               PyObject *const *py_args, size_t py_nargsf,
                               PyObject *kwnames)
{
    PyGICallableCache *cache = (PyGICallableCache *)function_cache;

    state->n_args = _pygi_callable_cache_args_len (cache);

    if (pygi_callable_cache_can_throw_gerror (cache)) {
        state->n_args++;
    }

    /* Copy the function pointer to the state for the normal case. For vfuncs,
     * this has already been filled out based on the implementor's GType.
     */
    if (state->function_ptr == NULL)
        state->function_ptr = function_cache->invoker.native_address;

    state->py_in_args =
        _py_args_combine_and_check_length (cache, py_args, py_nargsf, kwnames);

    if (state->py_in_args == NULL) {
        return FALSE;
    }
    state->n_py_in_args = PyTuple_Size (state->py_in_args);

    if (!_pygi_invoke_arg_state_init (state)) {
        return FALSE;
    }

    state->error = NULL;

    if (pygi_callable_cache_can_throw_gerror (cache)) {
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
    Py_XDECREF (state->py_async);
}

static gboolean
_caller_alloc (PyGIArgCache *arg_cache, GIArgument *arg)
{
    if (arg_cache->type_tag == GI_TYPE_TAG_INTERFACE) {
        PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

        arg->v_pointer = NULL;
        if (g_type_is_a (iface_cache->g_type, G_TYPE_BOXED)) {
            arg->v_pointer = pygi_boxed_alloc (
                GI_BASE_INFO (iface_cache->interface_info), NULL);
        } else if (iface_cache->g_type == G_TYPE_VALUE) {
            arg->v_pointer = g_slice_new0 (GValue);
        } else if (iface_cache->is_foreign) {
            PyObject *foreign_struct =
                pygi_struct_foreign_convert_from_g_argument (
                    iface_cache->interface_info, GI_TRANSFER_NOTHING, NULL);

            pygi_struct_foreign_convert_to_g_argument (
                foreign_struct, iface_cache->interface_info,
                GI_TRANSFER_EVERYTHING, arg);
        } else {
            gssize size = gi_struct_info_get_size (
                (GIStructInfo *)iface_cache->interface_info);
            arg->v_pointer = g_malloc0 (size);
        }
    } else if (arg_cache->type_tag == GI_TYPE_TAG_ARRAY) {
        PyGIArgGArray *array_cache = (PyGIArgGArray *)arg_cache;

        arg->v_pointer =
            g_array_new (TRUE, TRUE, (guint)array_cache->item_size);
    } else {
        return FALSE;
    }

    if (arg->v_pointer == NULL) return FALSE;


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
_invoke_marshal_in_args (PyGIInvokeState *state,
                         PyGIFunctionCache *function_cache)
{
    PyGICallableCache *cache = (PyGICallableCache *)function_cache;
    gssize i;

    if (state->n_py_in_args > cache->n_py_args) {
        char *full_name = pygi_callable_cache_get_full_name (cache);
        PyErr_Format (PyExc_TypeError,
                      "%s() takes exactly %zd argument(s) (%zd given)",
                      full_name, cache->n_py_args, state->n_py_in_args);
        g_free (full_name);
        return FALSE;
    }

    if (function_cache->async_finish && function_cache->async_callback
        && function_cache->async_callback->py_arg_index < state->n_py_in_args
        && PyTuple_GET_ITEM (state->py_in_args,
                             function_cache->async_callback->py_arg_index)
               == _PyGIDefaultArgPlaceholder) {
        /* We are dealing with an async call that returns an awaitable */
        PyObject *cancellable = NULL;

        /* Try to resolve any passed GCancellable. */
        if (function_cache->async_cancellable
            && function_cache->async_cancellable->py_arg_index
                   < state->n_py_in_args)
            cancellable = PyTuple_GET_ITEM (
                state->py_in_args,
                function_cache->async_cancellable->py_arg_index);

        if (cancellable == _PyGIDefaultArgPlaceholder) cancellable = NULL;

        state->py_async =
            pygi_async_new (function_cache->async_finish, cancellable);
    }

    for (i = 0; (gsize)i < _pygi_callable_cache_args_len (cache); i++) {
        GIArgument *c_arg = &state->args[i].arg_value;
        PyGIArgCache *arg_cache = g_ptr_array_index (cache->args_cache, i);
        PyObject *py_arg = NULL;
        gboolean marshal = TRUE;

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
                              full_name, cache->n_py_args,
                              state->n_py_in_args);
                g_free (full_name);

                /* clean up all of the args we have already marshalled,
                     * since invoke will not be called
                     */
                pygi_marshal_cleanup_args_from_py_parameter_fail (state, cache,
                                                                  i);
                return FALSE;
            }

            py_arg =
                PyTuple_GET_ITEM (state->py_in_args, arg_cache->py_arg_index);

            break;
        case PYGI_DIRECTION_BIDIRECTIONAL:
            if (arg_cache->meta_type != PYGI_META_ARG_TYPE_CHILD) {
                if (arg_cache->py_arg_index >= state->n_py_in_args) {
                    char *full_name =
                        pygi_callable_cache_get_full_name (cache);
                    PyErr_Format (
                        PyExc_TypeError,
                        "%s() takes exactly %zd argument(s) (%zd given)",
                        full_name, cache->n_py_args, state->n_py_in_args);
                    g_free (full_name);
                    pygi_marshal_cleanup_args_from_py_parameter_fail (
                        state, cache, i);
                    return FALSE;
                }

                py_arg = PyTuple_GET_ITEM (state->py_in_args,
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

            if (pygi_arg_cache_is_caller_allocates (arg_cache)) {
                /* In the case of caller allocated out args, we don't use
                     * an extra level of indirection and state->args will point
                     * directly at the data to be marshaled. However, as noted
                     * above, arg_pointers will also point to this caller allocated
                     * chunk of memory used by out argument marshaling.
                     */
                state->ffi_args[i] = c_arg;

                if (!_caller_alloc (arg_cache, c_arg)) {
                    char *full_name =
                        pygi_callable_cache_get_full_name (cache);
                    PyErr_Format (PyExc_TypeError,
                                  "Could not caller allocate argument %zd of "
                                  "callable %s",
                                  i, full_name);
                    g_free (full_name);
                    pygi_marshal_cleanup_args_from_py_parameter_fail (
                        state, cache, i);
                    return FALSE;
                }
            } else {
                /* Non-caller allocated out args will use arg_pointers as an
                     * extra level of indirection */
                state->ffi_args[i] = &state->args[i].arg_pointer;
            }

            break;
        default:
            g_assert_not_reached ();
            break;
        }

        if (py_arg == _PyGIDefaultArgPlaceholder) {
            /* If this is the cancellable, then we may override it later if we
             * detect an async call.
             */
            marshal = FALSE;

            if (state->py_async
                && arg_cache->async_context
                       == PYGI_ASYNC_CONTEXT_CANCELLABLE) {
                marshal = TRUE;
                py_arg = ((PyGIAsync *)state->py_async)->cancellable;
            } else if (state->py_async
                       && arg_cache->async_context
                              == PYGI_ASYNC_CONTEXT_CALLBACK) {
                marshal = TRUE;
            } else {
                c_arg->v_pointer = NULL;
            }
        }

        if (marshal && arg_cache->from_py_marshaller != NULL
            && arg_cache->meta_type != PYGI_META_ARG_TYPE_CHILD) {
            gboolean success;
            gpointer cleanup_data = NULL;

            if (!pygi_arg_cache_allow_none (arg_cache) && Py_IsNone (py_arg)) {
                PyErr_Format (PyExc_TypeError,
                              "Argument %zd does not allow None as a value",
                              i);

                pygi_marshal_cleanup_args_from_py_parameter_fail (state, cache,
                                                                  i);
                return FALSE;
            }
            success = arg_cache->from_py_marshaller (
                state, cache, arg_cache, py_arg, c_arg, &cleanup_data);
            state->args[i].arg_cleanup_data = cleanup_data;

            if (!success) {
                pygi_marshal_cleanup_args_from_py_parameter_fail (state, cache,
                                                                  i);
                return FALSE;
            }
        }
    }

    return TRUE;
}

static PyObject *
_invoke_marshal_out_args (PyGIInvokeState *state,
                          PyGIFunctionCache *function_cache)
{
    PyGICallableCache *cache = (PyGICallableCache *)function_cache;
    PyObject *py_out = NULL;
    PyObject *py_return = NULL;
    gssize n_out_args = cache->n_to_py_args - cache->n_to_py_child_args;

    if (cache->return_cache) {
        if (!pygi_callable_cache_skip_return (cache)) {
            gpointer cleanup_data = NULL;
            py_return = cache->return_cache->to_py_marshaller (
                state, cache, cache->return_cache, &state->return_arg,
                &cleanup_data);
            state->to_py_return_arg_cleanup_data = cleanup_data;
            if (py_return == NULL) {
                pygi_marshal_cleanup_args_return_fail (state, cache);
                return NULL;
            }
        } else {
            if (cache->return_cache->transfer == GI_TRANSFER_EVERYTHING) {
                PyGIMarshalToPyCleanupFunc to_py_cleanup =
                    cache->return_cache->to_py_cleanup;

                if (to_py_cleanup != NULL)
                    to_py_cleanup (state, cache->return_cache, NULL,
                                   &state->return_arg, FALSE);
            }
        }
    }

    /* Return the async future if we have one. */
    if (state->py_async) {
        /* We must have no return value */
        g_assert (n_out_args == 0);
        g_assert (pygi_callable_cache_skip_return (cache)
                  || cache->return_cache->type_tag == GI_TYPE_TAG_VOID);

        Py_DECREF (py_return);
        return Py_NewRef (state->py_async);
    }

    if (n_out_args == 0) {
        if (pygi_callable_cache_skip_return (cache) && state->error == NULL) {
            /* we skip the return value and have no (out) arguments to return,
             * so py_return should be NULL. But we must not return NULL,
             * otherwise Python will expect an exception.
             */
            g_assert (py_return == NULL);
            py_return = Py_NewRef (Py_None);
        }

        py_out = py_return;
    } else if (!cache->has_return && n_out_args == 1) {
        /* if we get here there is one out arg an no return */
        PyGIArgCache *arg_cache = (PyGIArgCache *)cache->to_py_args->data;
        gpointer cleanup_data = NULL;
        py_out = arg_cache->to_py_marshaller (
            state, cache, arg_cache,
            state->args[arg_cache->c_arg_index].arg_pointer.v_pointer,
            &cleanup_data);
        state->args[arg_cache->c_arg_index].to_py_arg_cleanup_data =
            cleanup_data;
        if (py_out == NULL) {
            pygi_marshal_cleanup_args_to_py_parameter_fail (state, cache, 0);
            return NULL;
        }

    } else {
        /* return a tuple */
        gssize py_arg_index = 0;
        GSList *cache_item = cache->to_py_args;
        gssize tuple_len = cache->has_return + n_out_args;

        py_out = pygi_resulttuple_new (cache->resulttuple_type, tuple_len);

        if (py_out == NULL) {
            pygi_marshal_cleanup_args_to_py_parameter_fail (state, cache,
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
            PyObject *py_obj = arg_cache->to_py_marshaller (
                state, cache, arg_cache,
                state->args[arg_cache->c_arg_index].arg_pointer.v_pointer,
                &cleanup_data);
            state->args[arg_cache->c_arg_index].to_py_arg_cleanup_data =
                cleanup_data;

            if (py_obj == NULL) {
                if (cache->has_return) py_arg_index--;

                pygi_marshal_cleanup_args_to_py_parameter_fail (state, cache,
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
                        PyGIInvokeState *state, PyObject *const *py_args,
                        size_t py_nargsf, PyObject *py_kwnames)
{
    PyGICallableCache *cache = (PyGICallableCache *)function_cache;
    GIFFIReturnValue ffi_return_value = { 0 };
    PyObject *ret = NULL;

    if (Py_EnterRecursiveCall (" while calling a GICallable")) return NULL;
    ;

    if (!_invoke_state_init_from_cache (state, function_cache, py_args,
                                        py_nargsf, py_kwnames))
        goto err;

    if (!_invoke_marshal_in_args (state, function_cache)) goto err;

    Py_BEGIN_ALLOW_THREADS;

    ffi_call (&function_cache->invoker.cif, state->function_ptr,
              (void *)&ffi_return_value, (void **)state->ffi_args);

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
    Py_LeaveRecursiveCall ();
    return ret;
}

PyObject *
pygi_callable_info_invoke (PyGICallableInfo *self, PyObject *const *py_args,
                           size_t py_nargsf, PyObject *py_kwnames)
{
    PyGIFunctionCache *cache = pygi_callable_info_get_cache (self);

    if (cache == NULL) return NULL;

    return pygi_function_cache_invoke (cache, py_args, py_nargsf, py_kwnames);
}

/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 *   pygi-closure.c: PyGI C Closure functions
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

#include "pygi-closure.h"
#include "pygi-argument.h"
#include "pygi-async.h"
#include "pygi-ccallback.h"
#include "pygi-error.h"
#include "pygi-invoke.h"
#include "pygi-marshal-cleanup.h"

/* This maintains a list of closures which can be free'd whenever
   as they have been called.  We will free them on the next
   library function call.
 */
static GSList *async_free_list;

static void
_pygi_closure_assign_pyobj_to_retval (gpointer retval, GIArgument *arg,
                                      PyGIArgCache *arg_cache)
{
    if (retval == NULL) return;

    switch (arg_cache->type_tag) {
    case GI_TYPE_TAG_BOOLEAN:
        *((ffi_sarg *)retval) = arg->v_boolean;
        break;
    case GI_TYPE_TAG_INT8:
        *((ffi_sarg *)retval) = arg->v_int8;
        break;
    case GI_TYPE_TAG_UINT8:
        *((ffi_arg *)retval) = arg->v_uint8;
        break;
    case GI_TYPE_TAG_INT16:
        *((ffi_sarg *)retval) = arg->v_int16;
        break;
    case GI_TYPE_TAG_UINT16:
        *((ffi_arg *)retval) = arg->v_uint16;
        break;
    case GI_TYPE_TAG_INT32:
        *((ffi_sarg *)retval) = arg->v_int32;
        break;
    case GI_TYPE_TAG_UINT32:
        *((ffi_arg *)retval) = arg->v_uint32;
        break;
    case GI_TYPE_TAG_INT64:
        *((ffi_sarg *)retval) = (ffi_sarg)arg->v_int64;
        break;
    case GI_TYPE_TAG_UINT64:
        *((ffi_arg *)retval) = (ffi_arg)arg->v_uint64;
        break;
    case GI_TYPE_TAG_FLOAT:
        *((gfloat *)retval) = arg->v_float;
        break;
    case GI_TYPE_TAG_DOUBLE:
        *((gdouble *)retval) = arg->v_double;
        break;
    case GI_TYPE_TAG_GTYPE:
        *((ffi_arg *)retval) = arg->v_size;
        break;
    case GI_TYPE_TAG_UNICHAR:
        *((ffi_arg *)retval) = arg->v_uint32;
        break;
    case GI_TYPE_TAG_INTERFACE: {
        GIRegisteredTypeInfo *interface_info;

        interface_info = ((PyGIInterfaceCache *)arg_cache)->interface_info;

        if (GI_IS_FLAGS_INFO (interface_info)) {
            /* Check flags before enums: flags are a subtype of enum. */
            *(ffi_arg *)retval = arg->v_uint;
        } else if (GI_IS_ENUM_INFO (interface_info)) {
            *(ffi_sarg *)retval = arg->v_int;
        } else {
            *(ffi_arg *)retval = (ffi_arg)arg->v_pointer;
        }

        break;
    }
    case GI_TYPE_TAG_VOID:
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_ARRAY:
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
    case GI_TYPE_TAG_GHASH:
    case GI_TYPE_TAG_ERROR:
        *(ffi_arg *)retval = (ffi_arg)arg->v_pointer;
        break;
    default:
        g_assert_not_reached ();
    }
}

static void
_pygi_closure_assign_pyobj_to_out_argument (gpointer out_arg, GIArgument *arg,
                                            PyGIArgCache *arg_cache)
{
    if (out_arg == NULL) return;

    switch (arg_cache->type_tag) {
    case GI_TYPE_TAG_BOOLEAN:
        *((gboolean *)out_arg) = arg->v_boolean;
        break;
    case GI_TYPE_TAG_INT8:
        *((gint8 *)out_arg) = arg->v_int8;
        break;
    case GI_TYPE_TAG_UINT8:
        *((guint8 *)out_arg) = arg->v_uint8;
        break;
    case GI_TYPE_TAG_INT16:
        *((gint16 *)out_arg) = arg->v_int16;
        break;
    case GI_TYPE_TAG_UINT16:
        *((guint16 *)out_arg) = arg->v_uint16;
        break;
    case GI_TYPE_TAG_INT32:
        *((gint32 *)out_arg) = arg->v_int32;
        break;
    case GI_TYPE_TAG_UINT32:
        *((guint32 *)out_arg) = arg->v_uint32;
        break;
    case GI_TYPE_TAG_INT64:
        *((gint64 *)out_arg) = arg->v_int64;
        break;
    case GI_TYPE_TAG_UINT64:
        *((guint64 *)out_arg) = arg->v_uint64;
        break;
    case GI_TYPE_TAG_FLOAT:
        *((gfloat *)out_arg) = arg->v_float;
        break;
    case GI_TYPE_TAG_DOUBLE:
        *((gdouble *)out_arg) = arg->v_double;
        break;
    case GI_TYPE_TAG_GTYPE:
        *((GType *)out_arg) = arg->v_size;
        break;
    case GI_TYPE_TAG_UNICHAR:
        *((guint32 *)out_arg) = arg->v_uint32;
        break;
    case GI_TYPE_TAG_INTERFACE: {
        GIRegisteredTypeInfo *interface_info;

        interface_info = ((PyGIInterfaceCache *)arg_cache)->interface_info;

        if (GI_IS_FLAGS_INFO (interface_info)) {
            /* Check flags before enums: flags are a subtype of enum. */
            *(guint *)out_arg = arg->v_uint;
        } else if (GI_IS_ENUM_INFO (interface_info)) {
            *(gint *)out_arg = arg->v_int;
        } else if (GI_IS_STRUCT_INFO (interface_info)) {
            if (!arg_cache->is_pointer) {
                if (arg->v_pointer != NULL) {
                    gsize item_size =
                        _pygi_gi_type_info_size (arg_cache->type_info);
                    memcpy (out_arg, arg->v_pointer, item_size);
                }
            } else {
                *((gpointer *)out_arg) = arg->v_pointer;
            }
        } else {
            *((gpointer *)out_arg) = arg->v_pointer;
        }
        break;
    }
    case GI_TYPE_TAG_VOID:
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_ARRAY:
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
    case GI_TYPE_TAG_GHASH:
    case GI_TYPE_TAG_ERROR:
        *((gpointer *)out_arg) = arg->v_pointer;
        break;
    default:
        g_assert_not_reached ();
    }
}

static void
_pygi_closure_convert_ffi_arguments (PyGIInvokeArgState *state,
                                     PyGICallableCache *cache, void **args)
{
    for (guint i = 0; i < _pygi_callable_cache_args_len (cache); i++) {
        PyGIArgCache *arg_cache = g_ptr_array_index (cache->args_cache, i);
        gpointer arg_pointer;

        if (arg_cache->direction & PYGI_DIRECTION_FROM_PYTHON) {
            state[i].arg_value.v_pointer = *(gpointer *)args[i];

            if (state[i].arg_value.v_pointer == NULL) continue;

            state[i].arg_pointer.v_pointer = state[i].arg_value.v_pointer;
            arg_pointer = state[i].arg_value.v_pointer;
        } else {
            arg_pointer = args[i];
        }

        switch (arg_cache->type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
            state[i].arg_value.v_boolean = *(gboolean *)arg_pointer;
            break;
        case GI_TYPE_TAG_INT8:
            state[i].arg_value.v_int8 = *(gint8 *)arg_pointer;
            break;
        case GI_TYPE_TAG_UINT8:
            state[i].arg_value.v_uint8 = *(guint8 *)arg_pointer;
            break;
        case GI_TYPE_TAG_INT16:
            state[i].arg_value.v_int16 = *(gint16 *)arg_pointer;
            break;
        case GI_TYPE_TAG_UINT16:
            state[i].arg_value.v_uint16 = *(guint16 *)arg_pointer;
            break;
        case GI_TYPE_TAG_INT32:
            state[i].arg_value.v_int32 = *(gint32 *)arg_pointer;
            break;
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_UNICHAR:
            state[i].arg_value.v_uint32 = *(guint32 *)arg_pointer;
            break;
        case GI_TYPE_TAG_INT64:
            state[i].arg_value.v_int64 = *(gint64 *)arg_pointer;
            break;
        case GI_TYPE_TAG_UINT64:
            state[i].arg_value.v_uint64 = *(guint64 *)arg_pointer;
            break;
        case GI_TYPE_TAG_GTYPE:
            state[i].arg_value.v_size = *(gsize *)arg_pointer;
            break;
        case GI_TYPE_TAG_FLOAT:
            state[i].arg_value.v_float = *(gfloat *)arg_pointer;
            break;
        case GI_TYPE_TAG_DOUBLE:
            state[i].arg_value.v_double = *(gdouble *)arg_pointer;
            break;
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
            state[i].arg_value.v_string = *(gchar **)arg_pointer;
            break;
        case GI_TYPE_TAG_INTERFACE: {
            GIRegisteredTypeInfo *interface;

            interface = ((PyGIInterfaceCache *)arg_cache)->interface_info;

            if (GI_IS_FLAGS_INFO (interface)) {
                /* Check flags before enums: flags are a subtype of enum. */
                state[i].arg_value.v_uint = *(guint *)arg_pointer;
            } else if (GI_IS_ENUM_INFO (interface)) {
                state[i].arg_value.v_int = *(gint *)arg_pointer;
            } else {
                state[i].arg_value.v_pointer = *(gpointer *)arg_pointer;
            }
            break;
        }
        case GI_TYPE_TAG_ERROR:
        case GI_TYPE_TAG_GHASH:
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        case GI_TYPE_TAG_ARRAY:
        case GI_TYPE_TAG_VOID:
            state[i].arg_value.v_pointer = *(gpointer *)arg_pointer;
            break;
        default:
            g_assert_not_reached ();
        }
    }

    if (pygi_callable_cache_can_throw_gerror (cache)) {
        guint error_index = _pygi_callable_cache_args_len (cache);

        state[error_index].arg_value.v_pointer =
            *(gpointer *)args[error_index];
    }
}

static gboolean
_invoke_state_init_from_cache (PyGIInvokeState *state,
                               PyGIClosureCache *closure_cache, void **args)
{
    PyGICallableCache *cache = (PyGICallableCache *)closure_cache;

    state->n_args = _pygi_callable_cache_args_len (cache);
    state->n_py_in_args = state->n_args;

    /* Increment after setting the number of Python input args */
    if (pygi_callable_cache_can_throw_gerror (cache)) {
        state->n_args++;
    }

    state->py_in_args = PyTuple_New (state->n_py_in_args);
    if (state->py_in_args == NULL) {
        PyErr_NoMemory ();
        return FALSE;
    }

    state->args = NULL;
    state->error = NULL;

    if (!_pygi_invoke_arg_state_init (state)) {
        return FALSE;
    }

    state->ffi_args = NULL;

    _pygi_closure_convert_ffi_arguments (state->args, cache, args);
    return TRUE;
}

static void
_invoke_state_clear (PyGIInvokeState *state)
{
    _pygi_invoke_arg_state_free (state);
    Py_XDECREF (state->py_in_args);
}

static gboolean
_pygi_closure_convert_arguments (PyGIInvokeState *state,
                                 PyGIClosureCache *closure_cache)
{
    PyGICallableCache *cache = (PyGICallableCache *)closure_cache;
    gssize n_in_args = 0;

    for (guint i = 0; i < _pygi_callable_cache_args_len (cache); i++) {
        PyGIArgCache *arg_cache = g_ptr_array_index (cache->args_cache, i);

        if (arg_cache->direction & PYGI_DIRECTION_TO_PYTHON) {
            PyObject *value;

            if (cache->has_user_data && (cache->user_data_index) == i) {
                if (state->user_data == NULL) {
                    /* user_data can be NULL for connect functions which don't accept
                     * user_data or as the default for user_data in the middle of function
                     * arguments.
                     */
                    Py_INCREF (Py_None);
                    value = Py_None;
                } else {
                    /* Extend the callbacks args with user_data as variable args. */
                    gssize j, user_data_len;
                    PyObject *py_user_data = state->user_data;

                    if (!PyTuple_Check (py_user_data)) {
                        PyErr_SetString (
                            PyExc_TypeError,
                            "expected tuple for callback user_data");
                        return FALSE;
                    }

                    user_data_len = PyTuple_Size (py_user_data);
                    _PyTuple_Resize (&state->py_in_args,
                                     state->n_py_in_args + user_data_len - 1);

                    for (j = 0; j < user_data_len; j++, n_in_args++) {
                        value = PyTuple_GetItem (py_user_data, j);
                        Py_INCREF (value);
                        PyTuple_SET_ITEM (state->py_in_args, n_in_args, value);
                    }
                    /* We can assume user_data args are never going to be inout,
                     * so just continue here.
                     */
                    continue;
                }
            } else if (arg_cache->meta_type != PYGI_META_ARG_TYPE_PARENT) {
                continue;
            } else {
                MarshalCleanupData cleanup_data = { NULL, NULL };

                value = arg_cache->to_py_marshaller (state, cache, arg_cache,
                                                     &state->args[i].arg_value,
                                                     &cleanup_data);
                state->args[i].to_py_arg_cleanup_data = cleanup_data;

                if (value == NULL) {
                    pygi_marshal_cleanup_args_to_py_parameter_fail (state,
                                                                    cache, i);
                    return FALSE;
                }
            }

            PyTuple_SET_ITEM (state->py_in_args, n_in_args, value);
            n_in_args++;
        }
    }

    if (_PyTuple_Resize (&state->py_in_args, n_in_args) == -1) return FALSE;

    return TRUE;
}

static gboolean
_pygi_closure_set_out_arguments (PyGIInvokeState *state,
                                 PyGICallableCache *cache, PyObject *py_retval,
                                 void *resp)
{
    gssize i_py_retval = 0;
    gboolean success;

    if (cache->return_cache->type_tag != GI_TYPE_TAG_VOID) {
        PyObject *item = py_retval;

        if (PyTuple_Check (py_retval)) {
            item = PyTuple_GET_ITEM (py_retval, 0);
        }

        success = cache->return_cache->from_py_marshaller (
            state, cache, cache->return_cache, item, &state->return_arg,
            &state->args[0].arg_cleanup_data);

        if (!success) {
            pygi_marshal_cleanup_args_return_fail (state, cache);
            return FALSE;
        }

        _pygi_closure_assign_pyobj_to_retval (resp, &state->return_arg,
                                              cache->return_cache);
        i_py_retval++;
    }

    for (guint i = 0; i < _pygi_callable_cache_args_len (cache); i++) {
        PyGIArgCache *arg_cache = g_ptr_array_index (cache->args_cache, i);

        if (arg_cache->direction & PYGI_DIRECTION_FROM_PYTHON) {
            PyObject *item = py_retval;

            if (arg_cache->type_tag == GI_TYPE_TAG_ERROR) {
                *(GError **)state->args[i].arg_pointer.v_pointer = NULL;
                continue;
            }

            if (PyTuple_Check (py_retval)) {
                item = PyTuple_GET_ITEM (py_retval, i_py_retval);
            } else if (i_py_retval != 0) {
                pygi_marshal_cleanup_args_to_py_parameter_fail (state, cache,
                                                                i_py_retval);
                return FALSE;
            }

            success = arg_cache->from_py_marshaller (
                state, cache, arg_cache, item, &state->args[i].arg_value,
                &state->args[i_py_retval].arg_cleanup_data);

            if (!success) {
                pygi_marshal_cleanup_args_to_py_parameter_fail (state, cache,
                                                                i_py_retval);
                return FALSE;
            }

            _pygi_closure_assign_pyobj_to_out_argument (
                state->args[i].arg_pointer.v_pointer,
                &state->args[i].arg_value, arg_cache);

            i_py_retval++;
        }
    }

    return TRUE;
}

static void
_pygi_closure_clear_retvals (PyGIInvokeState *state, PyGICallableCache *cache,
                             gpointer resp)
{
    GIArgument arg = PYGI_ARG_INIT;

    if (cache->return_cache->type_tag != GI_TYPE_TAG_VOID) {
        _pygi_closure_assign_pyobj_to_retval (resp, &arg, cache->return_cache);
    }

    for (guint i = 0; i < _pygi_callable_cache_args_len (cache); i++) {
        PyGIArgCache *arg_cache = g_ptr_array_index (cache->args_cache, i);

        if (arg_cache->direction & PYGI_DIRECTION_FROM_PYTHON) {
            _pygi_closure_assign_pyobj_to_out_argument (
                state->args[i].arg_pointer.v_pointer, &arg, arg_cache);
        }
    }

    if (pygi_callable_cache_can_throw_gerror (cache)) {
        gssize error_index = state->n_args - 1;
        GError **error =
            (GError **)state->args[error_index].arg_value.v_pointer;

        if (error != NULL) {
            pygi_gerror_exception_check (error);
        }
    }
}

static void
_pygi_invoke_closure_clear_py_data (PyGICClosure *invoke_closure)
{
    PyGILState_STATE state = PyGILState_Ensure ();

    Py_CLEAR (invoke_closure->function);
    Py_CLEAR (invoke_closure->user_data);

    PyGILState_Release (state);
}

static void
_pygi_closure_handle (ffi_cif *cif, void *result, void **args, void *data)
{
    PyGILState_STATE py_state;
    PyGICClosure *closure = data;
    PyObject *retval;
    gboolean success;
    PyGIInvokeState state = {
        0,
    };

    /* Ignore closures when Python is not initialized. This can happen in cases
     * where calling Python implemented vfuncs can happen at shutdown time.
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=722562 */
    if (!Py_IsInitialized ()) {
        return;
    }

    /* Lock the GIL as we are coming into this code without the lock and we
      may be executing python code */
    py_state = PyGILState_Ensure ();

    if (closure->cache == NULL) goto end;

    state.user_data = closure->user_data;

    _invoke_state_init_from_cache (&state, closure->cache, args);

    if (!_pygi_closure_convert_arguments (&state, closure->cache)) {
        _pygi_closure_clear_retvals (&state, closure->cache, result);
        goto end;
    }

    retval =
        PyObject_CallObject ((PyObject *)closure->function, state.py_in_args);

    if (retval == NULL) {
        _pygi_closure_clear_retvals (&state, closure->cache, result);
        goto end;
    }

    pygi_marshal_cleanup_args_to_py_marshal_success (&state, closure->cache);
    success = _pygi_closure_set_out_arguments (&state, closure->cache, retval,
                                               result);

    if (!success) {
        pygi_marshal_cleanup_args_from_py_marshal_success (&state,
                                                           closure->cache);
        _pygi_closure_clear_retvals (&state, closure->cache, result);
    }

    Py_DECREF (retval);

end:

    if (PyErr_Occurred ()) PyErr_Print ();

    /* Now that the closure has finished we can make a decision about how
       to free it.  Scope call gets free'd at the end of wrap_gi_function_info_invoke.
       Scope notified will be freed when the notify is called.
       Scope async closures free only their python data now and the closure later
       during the next creation of a closure. This minimizes potential ref leaks
       at least in regards to the python objects.
       (you can't free the closure you are currently using!)
    */
    switch (closure->scope) {
    case GI_SCOPE_TYPE_CALL:
    case GI_SCOPE_TYPE_NOTIFIED:
        break;
    case GI_SCOPE_TYPE_ASYNC:
        /* Append this PyGICClosure to a list of closure that we will free
               after we're done with this function invokation */
        _pygi_invoke_closure_clear_py_data (closure);
        async_free_list = g_slist_prepend (async_free_list, closure);
        break;
    case GI_SCOPE_TYPE_INVALID:
    case GI_SCOPE_TYPE_FOREVER:
        /* Handle new scopes added by gobject-introspection */
        g_critical (
            "Unknown scope reached inside %s. Please file an issue "
            "at https://gitlab.gnome.org/GNOME/pygobject/issues/new",
            gi_base_info_get_name (GI_BASE_INFO (closure->info)));
        break;
    default:
        g_assert_not_reached ();
    }

    _invoke_state_clear (&state);
    PyGILState_Release (py_state);
}

void
_pygi_invoke_closure_free (PyGICClosure *invoke_closure)
{
    gi_callable_info_destroy_closure (invoke_closure->info,
                                      invoke_closure->closure);

    if (invoke_closure->info)
        gi_base_info_unref ((GIBaseInfo *)invoke_closure->info);

    invoke_closure->cache = NULL;

    _pygi_invoke_closure_clear_py_data (invoke_closure);

    g_slice_free (PyGICClosure, invoke_closure);
}


PyGICClosure *
_pygi_make_native_closure (GICallableInfo *info, PyGIClosureCache *cache,
                           GIScopeType scope, PyObject *py_function,
                           PyObject *py_user_data)
{
    PyGICClosure *closure;
    ffi_closure *fficlosure;

    /* Begin by cleaning up old async functions */
    g_slist_free_full (async_free_list,
                       (GDestroyNotify)_pygi_invoke_closure_free);
    async_free_list = NULL;

    /* Build the closure itself */
    closure = g_slice_new0 (PyGICClosure);
    closure->info = (GICallableInfo *)gi_base_info_ref ((GIBaseInfo *)info);
    closure->function = py_function;
    closure->user_data = py_user_data;
    closure->cache = cache;

    Py_INCREF (py_function);
    Py_XINCREF (closure->user_data);

    fficlosure = gi_callable_info_create_closure (
        info, &closure->cif, _pygi_closure_handle, closure);

    closure->closure = fficlosure;

    /* Give the closure the information it needs to determine when
       to free itself later */
    closure->scope = scope;

    return closure;
}

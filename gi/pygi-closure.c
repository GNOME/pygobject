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

#include "pygi-private.h"
#include "pygi-closure.h"
#include "pygi-marshal-cleanup.h"


typedef struct _PyGICallbackCache
{
    PyGIArgCache arg_cache;
    gssize user_data_index;
    gssize destroy_notify_index;
    GIScopeType scope;
    GIInterfaceInfo *interface_info;
} PyGICallbackCache;

/* This maintains a list of closures which can be free'd whenever
   as they have been called.  We will free them on the next
   library function call.
 */
static GSList* async_free_list;

static void
_pygi_closure_assign_pyobj_to_retval (gpointer retval,
                                      GIArgument *arg,
                                      PyGIArgCache *arg_cache)
{
    if (PyErr_Occurred () || retval == NULL)
        return;

    switch (arg_cache->type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
           *((ffi_sarg *) retval) = arg->v_boolean;
           break;
        case GI_TYPE_TAG_INT8:
           *((ffi_sarg *) retval) = arg->v_int8;
           break;
        case GI_TYPE_TAG_UINT8:
           *((ffi_arg *) retval) = arg->v_uint8;
           break;
        case GI_TYPE_TAG_INT16:
           *((ffi_sarg *) retval) = arg->v_int16;
           break;
        case GI_TYPE_TAG_UINT16:
           *((ffi_arg *) retval) = arg->v_uint16;
           break;
        case GI_TYPE_TAG_INT32:
           *((ffi_sarg *) retval) = arg->v_int32;
           break;
        case GI_TYPE_TAG_UINT32:
           *((ffi_arg *) retval) = arg->v_uint32;
           break;
        case GI_TYPE_TAG_INT64:
           *((ffi_sarg *) retval) = arg->v_int64;
           break;
        case GI_TYPE_TAG_UINT64:
           *((ffi_arg *) retval) = arg->v_uint64;
           break;
        case GI_TYPE_TAG_FLOAT:
           *((gfloat *) retval) = arg->v_float;
           break;
        case GI_TYPE_TAG_DOUBLE:
           *((gdouble *) retval) = arg->v_double;
           break;
        case GI_TYPE_TAG_GTYPE:
           *((ffi_arg *) retval) = arg->v_ulong;
           break;
        case GI_TYPE_TAG_UNICHAR:
            *((ffi_arg *) retval) = arg->v_uint32;
            break;
        case GI_TYPE_TAG_INTERFACE:
            {
                GIBaseInfo *interface_info;

                interface_info = ((PyGIInterfaceCache *) arg_cache)->interface_info;

                switch (g_base_info_get_type (interface_info)) {
                case GI_INFO_TYPE_ENUM:
                    *(ffi_sarg *) retval = arg->v_int;
                    break;
                case GI_INFO_TYPE_FLAGS:
                    *(ffi_arg *) retval = arg->v_uint;
                    break;
                default:
                    *(ffi_arg *) retval = (ffi_arg) arg->v_pointer;
                    break;
                }

                break;
            }
        default:
            *(ffi_arg *) retval = (ffi_arg) arg->v_pointer;
            break;
      }
}

static void
_pygi_closure_clear_retval (PyGICallableCache *cache, gpointer retval)
{
    if (cache->return_cache->type_tag != GI_TYPE_TAG_VOID) {
        *((ffi_arg *) retval) = 0;
    }
}

static void
_pygi_closure_assign_pyobj_to_out_argument (gpointer out_arg,
                                            GIArgument *arg,
                                            PyGIArgCache *arg_cache)
{
    if (out_arg == NULL)
        return;

    switch (arg_cache->type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
           *((gboolean *) out_arg) = arg->v_boolean;
           break;
        case GI_TYPE_TAG_INT8:
           *((gint8 *) out_arg) = arg->v_int8;
           break;
        case GI_TYPE_TAG_UINT8:
           *((guint8 *) out_arg) = arg->v_uint8;
           break;
        case GI_TYPE_TAG_INT16:
           *((gint16 *) out_arg) = arg->v_int16;
           break;
        case GI_TYPE_TAG_UINT16:
           *((guint16 *) out_arg) = arg->v_uint16;
           break;
        case GI_TYPE_TAG_INT32:
           *((gint32 *) out_arg) = arg->v_int32;
           break;
        case GI_TYPE_TAG_UINT32:
           *((guint32 *) out_arg) = arg->v_uint32;
           break;
        case GI_TYPE_TAG_INT64:
           *((gint64 *) out_arg) = arg->v_int64;
           break;
        case GI_TYPE_TAG_UINT64:
           *((glong *) out_arg) = arg->v_uint64;
           break;
        case GI_TYPE_TAG_FLOAT:
           *((gfloat *) out_arg) = arg->v_float;
           break;
        case GI_TYPE_TAG_DOUBLE:
           *((gdouble *) out_arg) = arg->v_double;
           break;
        case GI_TYPE_TAG_GTYPE:
           *((gulong *) out_arg) = arg->v_ulong;
           break;
        case GI_TYPE_TAG_UNICHAR:
            *((guint32 *) out_arg) = arg->v_uint32;
            break;
        case GI_TYPE_TAG_INTERFACE:
        {
           GIBaseInfo *interface_info;

           interface_info = ((PyGIInterfaceCache *) arg_cache)->interface_info;

           switch (g_base_info_get_type (interface_info)) {
           case GI_INFO_TYPE_ENUM:
               *(gint *) out_arg = arg->v_int;
               break;
           case GI_INFO_TYPE_FLAGS:
               *(guint *) out_arg = arg->v_uint;
               break;
           case GI_INFO_TYPE_STRUCT:
               if (!arg_cache->is_pointer) {
                   if (arg->v_pointer != NULL) {
                       gsize item_size = _pygi_g_type_info_size (arg_cache->type_info);
                       memcpy (out_arg, arg->v_pointer, item_size);
                   }
                   break;
               }

           /* Fall through if pointer */
           default:
               *((gpointer *) out_arg) = arg->v_pointer;
               break;
           }
           break;
        }

        default:
           *((gpointer *) out_arg) = arg->v_pointer;
           break;
      }
}

static GIArgument *
_pygi_closure_convert_ffi_arguments (PyGICallableCache *cache, void **args)
{
    gint num_args, i;
    GIArgument *g_args;

    num_args = _pygi_callable_cache_args_len (cache);
    g_args = g_new0 (GIArgument, num_args);

    for (i = 0; i < num_args; i++) {
        PyGIArgCache *arg_cache = g_ptr_array_index (cache->args_cache, i);

        if (arg_cache->direction & PYGI_DIRECTION_FROM_PYTHON) {
            g_args[i].v_pointer = * (gpointer *) args[i];
        } else {
            switch (arg_cache->type_tag) {
                case GI_TYPE_TAG_BOOLEAN:
                    g_args[i].v_boolean = * (gboolean *) args[i];
                    break;
                case GI_TYPE_TAG_INT8:
                    g_args[i].v_int8 = * (gint8 *) args[i];
                    break;
                case GI_TYPE_TAG_UINT8:
                    g_args[i].v_uint8 = * (guint8 *) args[i];
                    break;
                case GI_TYPE_TAG_INT16:
                    g_args[i].v_int16 = * (gint16 *) args[i];
                    break;
                case GI_TYPE_TAG_UINT16:
                    g_args[i].v_uint16 = * (guint16 *) args[i];
                    break;
                case GI_TYPE_TAG_INT32:
                    g_args[i].v_int32 = * (gint32 *) args[i];
                    break;
                case GI_TYPE_TAG_UINT32:
                    g_args[i].v_uint32 = * (guint32 *) args[i];
                    break;
                case GI_TYPE_TAG_INT64:
                    g_args[i].v_int64 = * (glong *) args[i];
                    break;
                case GI_TYPE_TAG_UINT64:
                    g_args[i].v_uint64 = * (glong *) args[i];
                    break;
                case GI_TYPE_TAG_FLOAT:
                    g_args[i].v_float = * (gfloat *) args[i];
                    break;
                case GI_TYPE_TAG_DOUBLE:
                    g_args[i].v_double = * (gdouble *) args[i];
                    break;
                case GI_TYPE_TAG_UTF8:
                    g_args[i].v_string = * (gchar **) args[i];
                    break;
                case GI_TYPE_TAG_INTERFACE:
                {
                    GIBaseInfo *interface;
                    GIInfoType interface_type;

                    interface = ((PyGIInterfaceCache *) arg_cache)->interface_info;
                    interface_type = g_base_info_get_type (interface);

                    if (interface_type == GI_INFO_TYPE_OBJECT ||
                            interface_type == GI_INFO_TYPE_INTERFACE) {
                        g_args[i].v_pointer = * (gpointer *) args[i];
                        break;
                    } else if (interface_type == GI_INFO_TYPE_ENUM ||
                               interface_type == GI_INFO_TYPE_FLAGS) {
                        g_args[i].v_uint = * (guint *) args[i];
                        break;
                    } else if (interface_type == GI_INFO_TYPE_STRUCT ||
                               interface_type == GI_INFO_TYPE_CALLBACK) {
                        g_args[i].v_pointer = * (gpointer *) args[i];
                        break;
                    }
                }
                case GI_TYPE_TAG_ERROR:
                case GI_TYPE_TAG_GHASH:
                case GI_TYPE_TAG_GLIST:
                case GI_TYPE_TAG_GSLIST:
                case GI_TYPE_TAG_ARRAY:
                case GI_TYPE_TAG_VOID:
                    g_args[i].v_pointer = * (gpointer *) args[i];
                    break;
                default:
                    g_warning ("Unhandled type tag %s",
                               g_type_tag_to_string (arg_cache->type_tag));
                    g_args[i].v_pointer = 0;
            }
        }
    }
    return g_args;
}

static gboolean
_invoke_state_init_from_cache (PyGIInvokeState *state,
                               PyGIClosureCache *closure_cache,
                               void **args)
{
    PyGICallableCache *cache = (PyGICallableCache *) closure_cache;

    state->n_args = _pygi_callable_cache_args_len (cache);

    state->py_in_args = PyTuple_New (state->n_args);
    if (state->py_in_args == NULL) {
        PyErr_NoMemory ();
        return FALSE;
    }
    state->n_py_in_args = state->n_args;

    if (cache->throws) {
        state->n_args++;
    }

    state->args = NULL;

    state->args_cleanup_data = g_slice_alloc0 (state->n_args * sizeof (gpointer));
    if (state->args_cleanup_data == NULL && state->n_args != 0) {
        PyErr_NoMemory();
        return FALSE;
    }

    state->arg_values = _pygi_closure_convert_ffi_arguments (cache, args);
    if (state->arg_values == NULL && state->n_args != 0) {
        PyErr_NoMemory ();
        return FALSE;
    }

    state->arg_pointers = g_slice_alloc0 (state->n_args * sizeof(GIArgument));
    if (state->arg_pointers == NULL && state->n_args != 0) {
        PyErr_NoMemory ();
        return FALSE;
    }

    state->error = NULL;

    if (cache->throws) {
        gssize error_index = state->n_args - 1;

        state->arg_pointers[error_index].v_pointer = &state->error;
        state->arg_values[error_index].v_pointer = state->error;
    }

    return TRUE;
}

static void
_invoke_state_clear (PyGIInvokeState *state)
{
    g_slice_free1 (state->n_args * sizeof(gpointer), state->args_cleanup_data);
    g_free (state->arg_values);
    g_slice_free1 (state->n_args * sizeof(GIArgument), state->arg_pointers);

    Py_XDECREF (state->py_in_args);
}

static gboolean
_pygi_closure_convert_arguments (PyGIInvokeState *state,
                                 PyGIClosureCache *closure_cache)
{
    PyGICallableCache *cache = (PyGICallableCache *) closure_cache;
    gssize n_in_args = 0;
    gssize i;

    /* Must set all the arg_pointers and update the arg_values before
     * marshaling otherwise out args wouldn't have the correct values.
     */
    for (i = 0; i < _pygi_callable_cache_args_len (cache); i++) {
        PyGIArgCache *arg_cache = g_ptr_array_index (cache->args_cache, i);

        if (arg_cache->direction & PYGI_DIRECTION_FROM_PYTHON &&
                state->arg_values[i].v_pointer) {
            state->arg_pointers[i].v_pointer = state->arg_values[i].v_pointer;
            state->arg_values[i] = *(GIArgument *) state->arg_values[i].v_pointer;
        }
    }

    for (i = 0; i < _pygi_callable_cache_args_len (cache); i++) {
        PyGIArgCache *arg_cache;
        PyGIDirection direction;

        arg_cache = g_ptr_array_index (cache->args_cache, i);
        direction = arg_cache->direction;

        if (direction & PYGI_DIRECTION_TO_PYTHON) {
            PyObject *value;

            if (direction == PYGI_DIRECTION_TO_PYTHON &&
                    arg_cache->type_tag == GI_TYPE_TAG_VOID &&
                    arg_cache->is_pointer) {
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
                        PyErr_SetString (PyExc_TypeError, "expected tuple for callback user_data");
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
                value = arg_cache->to_py_marshaller (state,
                                                     cache,
                                                     arg_cache,
                                                     &state->arg_values[i]);

                if (value == NULL) {
                    pygi_marshal_cleanup_args_to_py_parameter_fail (state,
                                                                    cache,
                                                                    i);
                    return FALSE;
                }
            }

            PyTuple_SET_ITEM (state->py_in_args, n_in_args, value);
            n_in_args++;
        }
    }

    if (_PyTuple_Resize (&state->py_in_args, n_in_args) == -1)
        return FALSE;

    return TRUE;
}

static gboolean
_pygi_closure_set_out_arguments (PyGIInvokeState *state,
                                 PyGICallableCache *cache,
                                 PyObject *py_retval,
                                 void *resp)
{
    gssize i;
    gssize i_py_retval = 0;
    gboolean success;

    if (cache->return_cache->type_tag != GI_TYPE_TAG_VOID) {
        PyObject *item = py_retval;

        if (PyTuple_Check (py_retval)) {
            item = PyTuple_GET_ITEM (py_retval, 0);
        }

        success = cache->return_cache->from_py_marshaller (state,
                                                           cache,
                                                           cache->return_cache,
                                                           item,
                                                           &state->return_arg,
                                                           &state->args_cleanup_data[0]);

        if (!success) {
            pygi_marshal_cleanup_args_return_fail (state,
                                                   cache);
            return FALSE;
        }

        _pygi_closure_assign_pyobj_to_retval (resp, &state->return_arg,
                                              cache->return_cache);
        i_py_retval++;
    }

    for (i = 0; i < _pygi_callable_cache_args_len (cache); i++) {
        PyGIArgCache *arg_cache = g_ptr_array_index (cache->args_cache, i);

        if (arg_cache->direction & PYGI_DIRECTION_FROM_PYTHON) {
            PyObject *item = py_retval;

            if (arg_cache->type_tag == GI_TYPE_TAG_ERROR) {
                /* TODO: check if an exception has been set and convert it to a GError */
                * (GError **) state->arg_pointers[i].v_pointer = NULL;
                continue;
            }

            if (PyTuple_Check (py_retval)) {
                item = PyTuple_GET_ITEM (py_retval, i_py_retval);
            } else if (i_py_retval != 0) {
                pygi_marshal_cleanup_args_to_py_parameter_fail (state,
                                                                cache,
                                                                i_py_retval);
                return FALSE;
            }

            success = arg_cache->from_py_marshaller (state,
                                                     cache,
                                                     arg_cache,
                                                     item,
                                                     &state->arg_values[i],
                                                     &state->args_cleanup_data[i_py_retval]);

            if (!success) {
                pygi_marshal_cleanup_args_to_py_parameter_fail (state,
                                                                cache,
                                                                i_py_retval);
                return FALSE;
            }

            _pygi_closure_assign_pyobj_to_out_argument (state->arg_pointers[i].v_pointer,
                                                        &state->arg_values[i], arg_cache);

            i_py_retval++;
        }
    }

    return TRUE;
}

static void
_pygi_invoke_closure_clear_py_data(PyGICClosure *invoke_closure)
{
    PyGILState_STATE state = PyGILState_Ensure();

    Py_CLEAR (invoke_closure->function);
    Py_CLEAR (invoke_closure->user_data);

    PyGILState_Release (state);
}

void
_pygi_closure_handle (ffi_cif *cif,
                      void    *result,
                      void   **args,
                      void    *data)
{
    PyGILState_STATE py_state;
    PyGICClosure *closure = data;
    PyObject *retval;
    gboolean success;
    PyGIInvokeState state = { 0, };

    /* Ignore closures when Python is not initialized. This can happen in cases
     * where calling Python implemented vfuncs can happen at shutdown time.
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=722562 */
    if (!Py_IsInitialized()) {
        return;
    }

    /* Lock the GIL as we are coming into this code without the lock and we
      may be executing python code */
    py_state = PyGILState_Ensure ();

    if (closure->cache == NULL) {
        closure->cache = pygi_closure_cache_new ((GICallableInfo *) closure->info);

        if (closure->cache == NULL)
            goto end;
    }

    state.user_data = closure->user_data;

    _invoke_state_init_from_cache (&state, closure->cache, args);

    if (!_pygi_closure_convert_arguments (&state, closure->cache)) {
        if (PyErr_Occurred ())
            PyErr_Print ();
        goto end;
    }

    retval = PyObject_CallObject ( (PyObject *) closure->function, state.py_in_args);

    if (retval == NULL) {
        _pygi_closure_clear_retval (closure->cache, result);
        PyErr_Print ();
        goto end;
    }

    pygi_marshal_cleanup_args_to_py_marshal_success (&state, closure->cache);
    success = _pygi_closure_set_out_arguments (&state, closure->cache, retval, result);

    if (!success) {
        pygi_marshal_cleanup_args_from_py_marshal_success (&state, closure->cache);
        _pygi_closure_clear_retval (closure->cache, result);

        if (PyErr_Occurred ())
            PyErr_Print ();
    }

    Py_DECREF (retval);

end:

    /* Now that the closure has finished we can make a decision about how
       to free it.  Scope call gets free'd at the end of wrap_g_function_info_invoke.
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
            _pygi_invoke_closure_clear_py_data(closure);
            async_free_list = g_slist_prepend (async_free_list, closure);
            break;
        default:
            g_error ("Invalid scope reached inside %s.  Possibly a bad annotation?",
                     g_base_info_get_name (closure->info));
    }

    _invoke_state_clear (&state);
    PyGILState_Release (py_state);
}

void _pygi_invoke_closure_free (gpointer data)
{
    PyGICClosure* invoke_closure = (PyGICClosure *) data;

    g_callable_info_free_closure (invoke_closure->info,
                                  invoke_closure->closure);

    if (invoke_closure->info)
        g_base_info_unref ( (GIBaseInfo*) invoke_closure->info);

    if (invoke_closure->cache != NULL)
        pygi_callable_cache_free ((PyGICallableCache *) invoke_closure->cache);

    _pygi_invoke_closure_clear_py_data(invoke_closure);

    g_slice_free (PyGICClosure, invoke_closure);
}


PyGICClosure*
_pygi_make_native_closure (GICallableInfo* info,
                           GIScopeType scope,
                           PyObject *py_function,
                           gpointer py_user_data)
{
    PyGICClosure *closure;
    ffi_closure *fficlosure;

    /* Begin by cleaning up old async functions */
    g_slist_free_full (async_free_list, (GDestroyNotify) _pygi_invoke_closure_free);
    async_free_list = NULL;

    /* Build the closure itself */
    closure = g_slice_new0 (PyGICClosure);
    closure->info = (GICallableInfo *) g_base_info_ref ( (GIBaseInfo *) info);
    closure->function = py_function;
    closure->user_data = py_user_data;

    Py_INCREF (py_function);
    Py_XINCREF (closure->user_data);

    fficlosure =
        g_callable_info_prepare_closure (info, &closure->cif, _pygi_closure_handle,
                                         closure);
    closure->closure = fficlosure;

    /* Give the closure the information it needs to determine when
       to free itself later */
    closure->scope = scope;

    return closure;
}

/* _pygi_destroy_notify_dummy:
 *
 * Dummy method used in the occasion when a method has a GDestroyNotify
 * argument without user data.
 */
static void
_pygi_destroy_notify_dummy (gpointer data) {
}

static gboolean
_pygi_marshal_from_py_interface_callback (PyGIInvokeState   *state,
                                          PyGICallableCache *callable_cache,
                                          PyGIArgCache      *arg_cache,
                                          PyObject          *py_arg,
                                          GIArgument        *arg,
                                          gpointer          *cleanup_data)
{
    GICallableInfo *callable_info;
    PyGICClosure *closure;
    PyGIArgCache *user_data_cache = NULL;
    PyGIArgCache *destroy_cache = NULL;
    PyGICallbackCache *callback_cache;
    PyObject *py_user_data = NULL;

    callback_cache = (PyGICallbackCache *)arg_cache;

    if (callback_cache->user_data_index > 0) {
        user_data_cache = _pygi_callable_cache_get_arg (callable_cache, callback_cache->user_data_index);
        if (user_data_cache->py_arg_index < state->n_py_in_args) {
            /* py_user_data is a borrowed reference. */
            py_user_data = PyTuple_GetItem (state->py_in_args, user_data_cache->py_arg_index);
            if (!py_user_data)
                return FALSE;
            /* NULL out user_data if it was not supplied and the default arg placeholder
             * was used instead.
             */
            if (py_user_data == _PyGIDefaultArgPlaceholder) {
                py_user_data = NULL;
            } else if (callable_cache->user_data_varargs_index < 0) {
                /* For non-variable length user data, place the user data in a
                 * single item tuple which is concatenated to the callbacks arguments.
                 * This allows callback input arg marshaling to always expect a
                 * tuple for user data. Note the
                 */
                py_user_data = Py_BuildValue("(O)", py_user_data, NULL);
            } else {
                /* increment the ref borrowed from PyTuple_GetItem above */
                Py_INCREF (py_user_data);
            }
        }
    }

    if (py_arg == Py_None) {
        return TRUE;
    }

    if (!PyCallable_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError,
                      "Callback needs to be a function or method not %s",
                      py_arg->ob_type->tp_name);

        return FALSE;
    }

    callable_info = (GICallableInfo *)callback_cache->interface_info;

    closure = _pygi_make_native_closure (callable_info, callback_cache->scope, py_arg, py_user_data);
    arg->v_pointer = closure->closure;

    /* always decref the user data as _pygi_make_native_closure adds its own ref */
    Py_XDECREF (py_user_data);

    /* The PyGICClosure instance is used as user data passed into the C function.
     * The return trip to python will marshal this back and pull the python user data out.
     */
    if (user_data_cache != NULL) {
        state->arg_values[user_data_cache->c_arg_index].v_pointer = closure;
    }

    /* Setup a GDestroyNotify callback if this method supports it along with
     * a user data field. The user data field is a requirement in order
     * free resources and ref counts associated with this arguments closure.
     * In case a user data field is not available, show a warning giving
     * explicit information and setup a dummy notification to avoid a crash
     * later on in _pygi_destroy_notify_callback_closure.
     */
    if (callback_cache->destroy_notify_index > 0) {
        destroy_cache = _pygi_callable_cache_get_arg (callable_cache, callback_cache->destroy_notify_index);
    }

    if (destroy_cache) {
        if (user_data_cache != NULL) {
            state->arg_values[destroy_cache->c_arg_index].v_pointer = _pygi_invoke_closure_free;
        } else {
            char *full_name = pygi_callable_cache_get_full_name (callable_cache);
            gchar *msg = g_strdup_printf("Callables passed to %s will leak references because "
                                         "the method does not support a user_data argument. "
                                         "See: https://bugzilla.gnome.org/show_bug.cgi?id=685598",
                                         full_name);
            g_free (full_name);
            if (PyErr_WarnEx(PyExc_RuntimeWarning, msg, 2)) {
                g_free(msg);
                _pygi_invoke_closure_free(closure);
                return FALSE;
            }
            g_free(msg);
            state->arg_values[destroy_cache->c_arg_index].v_pointer = _pygi_destroy_notify_dummy;
        }
    }

    /* Use the PyGIClosure as data passed to cleanup for GI_SCOPE_TYPE_CALL. */
    *cleanup_data = closure;

    return TRUE;
}

static PyObject *
_pygi_marshal_to_py_interface_callback (PyGIInvokeState   *state,
                                        PyGICallableCache *callable_cache,
                                        PyGIArgCache      *arg_cache,
                                        GIArgument        *arg)
{
    PyGICallbackCache *callback_cache = (PyGICallbackCache *) arg_cache;
    gssize user_data_index;
    gssize destroy_notify_index;
    gpointer user_data = NULL;
    GDestroyNotify destroy_notify = NULL;

    user_data_index = callback_cache->user_data_index;
    destroy_notify_index = callback_cache->destroy_notify_index;

    if (user_data_index != -1)
        user_data = state->arg_values[user_data_index].v_pointer;

    if (destroy_notify_index != -1)
        destroy_notify = state->arg_values[destroy_notify_index].v_pointer;

    return _pygi_ccallback_new (arg->v_pointer,
                                user_data,
                                callback_cache->scope,
                                (GIFunctionInfo *) callback_cache->interface_info,
                                destroy_notify);
}

static void
_callback_cache_free_func (PyGICallbackCache *cache)
{
    if (cache != NULL) {
        if (cache->interface_info != NULL)
            g_base_info_unref ( (GIBaseInfo *)cache->interface_info);

        g_slice_free (PyGICallbackCache, cache);
    }
}

static void
_pygi_marshal_cleanup_from_py_interface_callback (PyGIInvokeState *state,
                                                  PyGIArgCache    *arg_cache,
                                                  PyObject        *py_arg,
                                                  gpointer         data,
                                                  gboolean         was_processed)
{
    PyGICallbackCache *callback_cache = (PyGICallbackCache *)arg_cache;

    if (was_processed && callback_cache->scope == GI_SCOPE_TYPE_CALL) {
        _pygi_invoke_closure_free (data);
    }
}

static gboolean
pygi_arg_callback_setup_from_info (PyGICallbackCache  *arg_cache,
                                   GITypeInfo         *type_info,
                                   GIArgInfo          *arg_info,   /* may be null */
                                   GITransfer          transfer,
                                   PyGIDirection       direction,
                                   GIInterfaceInfo    *iface_info,
                                   PyGICallableCache  *callable_cache)
{
    PyGIArgCache *cache = (PyGIArgCache *)arg_cache;
    gssize child_offset = 0;

    if (!pygi_arg_base_setup ((PyGIArgCache *)arg_cache,
                              type_info,
                              arg_info,
                              transfer,
                              direction)) {
        return FALSE;
    }

    if (callable_cache != NULL)
        child_offset = callable_cache->args_offset;

    ( (PyGIArgCache *)arg_cache)->destroy_notify = (GDestroyNotify)_callback_cache_free_func;

    arg_cache->user_data_index = g_arg_info_get_closure (arg_info);
    if (arg_cache->user_data_index != -1)
        arg_cache->user_data_index += child_offset;

    arg_cache->destroy_notify_index = g_arg_info_get_destroy (arg_info);
    if (arg_cache->destroy_notify_index != -1)
        arg_cache->destroy_notify_index += child_offset;

    if (arg_cache->user_data_index >= 0) {
        PyGIArgCache *user_data_arg_cache = pygi_arg_cache_alloc ();
        user_data_arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD_WITH_PYARG;
        user_data_arg_cache->direction = direction;
        user_data_arg_cache->has_default = TRUE; /* always allow user data with a NULL default. */
        _pygi_callable_cache_set_arg (callable_cache, arg_cache->user_data_index,
                                      user_data_arg_cache);
    }

    if (arg_cache->destroy_notify_index >= 0) {
        PyGIArgCache *destroy_arg_cache = pygi_arg_cache_alloc ();
        destroy_arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
        destroy_arg_cache->direction = direction;
        _pygi_callable_cache_set_arg (callable_cache, arg_cache->destroy_notify_index,
                                      destroy_arg_cache);
    }

    arg_cache->scope = g_arg_info_get_scope (arg_info);
    g_base_info_ref( (GIBaseInfo *)iface_info);
    arg_cache->interface_info = iface_info;

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        cache->from_py_marshaller = _pygi_marshal_from_py_interface_callback;
        cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_interface_callback;
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        cache->to_py_marshaller = _pygi_marshal_to_py_interface_callback;
    }

    return TRUE;
}

PyGIArgCache *
pygi_arg_callback_new_from_info  (GITypeInfo        *type_info,
                                  GIArgInfo         *arg_info,   /* may be null */
                                  GITransfer         transfer,
                                  PyGIDirection      direction,
                                  GIInterfaceInfo   *iface_info,
                                  PyGICallableCache *callable_cache)
{
    gboolean res = FALSE;
    PyGICallbackCache *callback_cache;

    callback_cache = g_slice_new0 (PyGICallbackCache);
    if (callback_cache == NULL)
        return NULL;

    res = pygi_arg_callback_setup_from_info (callback_cache,
                                             type_info,
                                             arg_info,
                                             transfer,
                                             direction,
                                             iface_info,
                                             callable_cache);
    if (res) {
        return (PyGIArgCache *)callback_cache;
    } else {
        pygi_arg_cache_free ((PyGIArgCache *)callback_cache);
        return NULL;
    }
}

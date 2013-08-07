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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#include "pygi-private.h"

/* This maintains a list of closures which can be free'd whenever
   as they have been called.  We will free them on the next
   library function call.
 */
static GSList* async_free_list;

static void
_pygi_closure_assign_pyobj_to_retval (gpointer retval, PyObject *object,
                                      GITypeInfo *type_info,
                                      GITransfer transfer)
{
    GIArgument arg = _pygi_argument_from_object (object, type_info, transfer);
    GITypeTag type_tag;
    if (PyErr_Occurred ())
        return;

    type_tag = g_type_info_get_tag (type_info);

    if (retval == NULL)
        return;

    switch (type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
           *((ffi_sarg *) retval) = arg.v_boolean;
           break;
        case GI_TYPE_TAG_INT8:
           *((ffi_sarg *) retval) = arg.v_int8;
           break;
        case GI_TYPE_TAG_UINT8:
           *((ffi_arg *) retval) = arg.v_uint8;
           break;
        case GI_TYPE_TAG_INT16:
           *((ffi_sarg *) retval) = arg.v_int16;
           break;
        case GI_TYPE_TAG_UINT16:
           *((ffi_arg *) retval) = arg.v_uint16;
           break;
        case GI_TYPE_TAG_INT32:
           *((ffi_sarg *) retval) = arg.v_int32;
           break;
        case GI_TYPE_TAG_UINT32:
           *((ffi_arg *) retval) = arg.v_uint32;
           break;
        case GI_TYPE_TAG_INT64:
           *((ffi_sarg *) retval) = arg.v_int64;
           break;
        case GI_TYPE_TAG_UINT64:
           *((ffi_arg *) retval) = arg.v_uint64;
           break;
        case GI_TYPE_TAG_FLOAT:
           *((gfloat *) retval) = arg.v_float;
           break;
        case GI_TYPE_TAG_DOUBLE:
           *((gdouble *) retval) = arg.v_double;
           break;
        case GI_TYPE_TAG_GTYPE:
           *((ffi_arg *) retval) = arg.v_ulong;
           break;
        case GI_TYPE_TAG_UNICHAR:
            *((ffi_arg *) retval) = arg.v_uint32;
            break;
        case GI_TYPE_TAG_INTERFACE:
            {
                GIBaseInfo* interface_info;
                GIInfoType interface_type;

                interface_info = g_type_info_get_interface(type_info);
                interface_type = g_base_info_get_type(interface_info);

                switch (interface_type) {
                case GI_INFO_TYPE_ENUM:
                    *(ffi_sarg *) retval = arg.v_int;
                    break;
                case GI_INFO_TYPE_FLAGS:
                    *(ffi_arg *) retval = arg.v_uint;
                    break;
                default:
                    *(ffi_arg *) retval = (ffi_arg) arg.v_pointer;
                    break;
                }

                g_base_info_unref (interface_info);
                break;
            }
        default:
            *(ffi_arg *) retval = (ffi_arg) arg.v_pointer;
            break;
      }
}

static void
_pygi_closure_clear_retval (GICallableInfo *callable_info, gpointer retval)
{
    GITypeInfo return_type_info;
    GITypeTag return_type_tag;

    g_callable_info_load_return_type (callable_info, &return_type_info);
    return_type_tag = g_type_info_get_tag (&return_type_info);
    if (return_type_tag != GI_TYPE_TAG_VOID) {
        *((ffi_arg *) retval) = 0;
    }
}

static void
_pygi_closure_assign_pyobj_to_out_argument (gpointer out_arg, PyObject *object,
                                            GITypeInfo *type_info,
                                            GITransfer transfer)
{
    GIArgument arg = _pygi_argument_from_object (object, type_info, transfer);
    GITypeTag type_tag = g_type_info_get_tag (type_info);

    if (out_arg == NULL)
        return;

    switch (type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
           *((gboolean *) out_arg) = arg.v_boolean;
           break;
        case GI_TYPE_TAG_INT8:
           *((gint8 *) out_arg) = arg.v_int8;
           break;
        case GI_TYPE_TAG_UINT8:
           *((guint8 *) out_arg) = arg.v_uint8;
           break;
        case GI_TYPE_TAG_INT16:
           *((gint16 *) out_arg) = arg.v_int16;
           break;
        case GI_TYPE_TAG_UINT16:
           *((guint16 *) out_arg) = arg.v_uint16;
           break;
        case GI_TYPE_TAG_INT32:
           *((gint32 *) out_arg) = arg.v_int32;
           break;
        case GI_TYPE_TAG_UINT32:
           *((guint32 *) out_arg) = arg.v_uint32;
           break;
        case GI_TYPE_TAG_INT64:
           *((gint64 *) out_arg) = arg.v_int64;
           break;
        case GI_TYPE_TAG_UINT64:
           *((glong *) out_arg) = arg.v_uint64;
           break;
        case GI_TYPE_TAG_FLOAT:
           *((gfloat *) out_arg) = arg.v_float;
           break;
        case GI_TYPE_TAG_DOUBLE:
           *((gdouble *) out_arg) = arg.v_double;
           break;
        case GI_TYPE_TAG_GTYPE:
           *((gulong *) out_arg) = arg.v_ulong;
           break;
        case GI_TYPE_TAG_UNICHAR:
            *((guint32 *) out_arg) = arg.v_uint32;
            break;
        case GI_TYPE_TAG_INTERFACE:
        {
           GIBaseInfo *interface;
           GIInfoType interface_type;

           interface = g_type_info_get_interface (type_info);
           interface_type = g_base_info_get_type (interface);

           switch (interface_type) {
           case GI_INFO_TYPE_ENUM:
               *(gint *) out_arg = arg.v_int;
               break;
           case GI_INFO_TYPE_FLAGS:
               *(guint *) out_arg = arg.v_uint;
               break;
           case GI_INFO_TYPE_STRUCT:
               if (!g_type_info_is_pointer (type_info)) {
                   if (object != Py_None) {
                       gsize item_size = _pygi_g_type_info_size (type_info);
                       memcpy (out_arg, arg.v_pointer, item_size);
                   }
                   break;
               }

           /* Fall through if pointer */
           default:
               *((gpointer *) out_arg) = arg.v_pointer;
               break;
           }

           g_base_info_unref (interface);
           break;
        }

        default:
           *((gpointer *) out_arg) = arg.v_pointer;
           break;
      }
}

static GIArgument *
_pygi_closure_convert_ffi_arguments (GICallableInfo *callable_info, void **args)
{
    gint num_args, i;
    GIArgInfo arg_info;
    GITypeInfo arg_type;
    GITypeTag tag;
    GIDirection direction;
    GIArgument *g_args;

    num_args = g_callable_info_get_n_args (callable_info);
    g_args = g_new0 (GIArgument, num_args);

    for (i = 0; i < num_args; i++) {
        g_callable_info_load_arg (callable_info, i, &arg_info);
        g_arg_info_load_type (&arg_info, &arg_type);
        tag = g_type_info_get_tag (&arg_type);
        direction = g_arg_info_get_direction (&arg_info);

        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
            g_args[i].v_pointer = * (gpointer *) args[i];
        } else {
            switch (tag) {
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

                    interface = g_type_info_get_interface (&arg_type);
                    interface_type = g_base_info_get_type (interface);

                    if (interface_type == GI_INFO_TYPE_OBJECT ||
                            interface_type == GI_INFO_TYPE_INTERFACE) {
                        g_args[i].v_pointer = * (gpointer *) args[i];
                        g_base_info_unref (interface);
                        break;
                    } else if (interface_type == GI_INFO_TYPE_ENUM ||
                               interface_type == GI_INFO_TYPE_FLAGS) {
                        g_args[i].v_uint = * (guint *) args[i];
                        g_base_info_unref (interface);
                        break;
                    } else if (interface_type == GI_INFO_TYPE_STRUCT ||
                               interface_type == GI_INFO_TYPE_CALLBACK) {
                        g_args[i].v_pointer = * (gpointer *) args[i];
                        g_base_info_unref (interface);
                        break;
                    }

                    g_base_info_unref (interface);
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
                    g_warning ("Unhandled type tag %s", g_type_tag_to_string (tag));
                    g_args[i].v_pointer = 0;
            }
        }
    }
    return g_args;
}

static gboolean
_pygi_closure_convert_arguments (GICallableInfo *callable_info, void **args,
                                 void *user_data, PyObject **py_args,
                                 GIArgument **out_args)
{
    int n_args = g_callable_info_get_n_args (callable_info);
    int n_in_args = 0;
    int n_out_args = 0;
    int i;
    int user_data_arg = -1;
    int destroy_notify_arg = -1;

    GIArgument *g_args = NULL;

    *py_args = NULL;
    *py_args = PyTuple_New (n_args);
    if (*py_args == NULL)
        goto error;

    *out_args = NULL;
    *out_args = g_new0 (GIArgument, n_args);
    g_args = _pygi_closure_convert_ffi_arguments (callable_info, args);

    for (i = 0; i < n_args; i++) {
        GIArgInfo arg_info;
        GIDirection direction;

        /* Special case callbacks and skip over userdata and Destroy Notify */
        if (i == user_data_arg || i == destroy_notify_arg)
            continue;

        g_callable_info_load_arg (callable_info, i, &arg_info);
        direction = g_arg_info_get_direction (&arg_info);

        if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
            GITypeInfo arg_type;
            GITypeTag arg_tag;
            GITransfer transfer;
            PyObject *value;
            GIArgument *arg;
            gboolean free_array;

            g_arg_info_load_type (&arg_info, &arg_type);
            arg_tag = g_type_info_get_tag (&arg_type);
            transfer = g_arg_info_get_ownership_transfer (&arg_info);
            free_array = FALSE;

            if (direction == GI_DIRECTION_IN && arg_tag == GI_TYPE_TAG_VOID &&
                    g_type_info_is_pointer (&arg_type)) {

                if (user_data == NULL) {
                    /* user_data can be NULL for connect functions which don't accept
                     * user_data or as the default for user_data in the middle of function
                     * arguments.
                     */
                    Py_INCREF (Py_None);
                    value = Py_None;
                } else {
                    /* Extend the callbacks args with user_data as variable args. */
                    int j, user_data_len;
                    PyObject *py_user_data = user_data;

                    if (!PyTuple_Check (py_user_data)) {
                        PyErr_SetString (PyExc_TypeError, "expected tuple for callback user_data");
                        goto error;
                    }

                    user_data_len = PyTuple_Size (py_user_data);
                    _PyTuple_Resize (py_args, n_args + user_data_len - 1);
                    for (j = 0; j < user_data_len; j++, n_in_args++) {
                        value = PyTuple_GetItem (py_user_data, j);
                        Py_INCREF (value);
                        PyTuple_SET_ITEM (*py_args, n_in_args, value);
                    }
                    /* We can assume user_data args are never going to be inout,
                     * so just continue here.
                     */
                    continue;
                }
            } else if (direction == GI_DIRECTION_IN &&
                       arg_tag == GI_TYPE_TAG_INTERFACE) {
                /* Handle callbacks as a special case */
                GIBaseInfo *info;
                GIInfoType info_type;

                info = g_type_info_get_interface (&arg_type);
                info_type = g_base_info_get_type (info);

                arg = (GIArgument*) &g_args[i];

                if (info_type == GI_INFO_TYPE_CALLBACK) {
                    gpointer user_data = NULL;
                    GDestroyNotify destroy_notify = NULL;
                    GIScopeType scope = g_arg_info_get_scope(&arg_info);

                    user_data_arg = g_arg_info_get_closure(&arg_info);
                    destroy_notify_arg = g_arg_info_get_destroy(&arg_info);

                    if (user_data_arg != -1)
                        user_data = g_args[user_data_arg].v_pointer;

                    if (destroy_notify_arg != -1)
                        user_data = (GDestroyNotify) g_args[destroy_notify_arg].v_pointer;

                    value = _pygi_ccallback_new(arg->v_pointer,
                                                user_data,
                                                scope,
                                                (GIFunctionInfo *) info,
                                                destroy_notify);
                } else
                    value = _pygi_argument_to_object (arg, &arg_type, transfer);

                g_base_info_unref (info);
                if (value == NULL)
                    goto error;
            } else {
                if (direction == GI_DIRECTION_IN)
                    arg = (GIArgument*) &g_args[i];
                else
                    arg = (GIArgument*) g_args[i].v_pointer;
                
                if (g_type_info_get_tag (&arg_type) == GI_TYPE_TAG_ARRAY)
                    arg->v_pointer = _pygi_argument_to_array (arg, (GIArgument **) args, NULL,
                                                              callable_info, &arg_type, &free_array);

                value = _pygi_argument_to_object (arg, &arg_type, transfer);
                
                if (free_array)
                    g_array_free (arg->v_pointer, FALSE);
                
                if (value == NULL)
                    goto error;
            }
            PyTuple_SET_ITEM (*py_args, n_in_args, value);
            n_in_args++;
        }

        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
            (*out_args) [n_out_args] = g_args[i];
            n_out_args++;
        }

    }

    if (_PyTuple_Resize (py_args, n_in_args) == -1)
        goto error;

    g_free (g_args);
    return TRUE;

error:
    Py_CLEAR (*py_args);
    g_free (*out_args);
    *out_args = NULL;
    g_free (g_args);

    return FALSE;
}

static void
_pygi_closure_set_out_arguments (GICallableInfo *callable_info,
                                 PyObject *py_retval, GIArgument *out_args,
                                 void *resp)
{
    int n_args, i, i_py_retval, i_out_args;
    GITypeInfo return_type_info;
    GITypeTag return_type_tag;

    i_py_retval = 0;
    g_callable_info_load_return_type (callable_info, &return_type_info);
    return_type_tag = g_type_info_get_tag (&return_type_info);
    if (return_type_tag != GI_TYPE_TAG_VOID) {
        GITransfer transfer = g_callable_info_get_caller_owns (callable_info);
        if (PyTuple_Check (py_retval)) {
            PyObject *item = PyTuple_GET_ITEM (py_retval, 0);
            _pygi_closure_assign_pyobj_to_retval (resp, item,
                &return_type_info, transfer);
        } else {
            _pygi_closure_assign_pyobj_to_retval (resp, py_retval,
                &return_type_info, transfer);
        }
        i_py_retval++;
    }

    i_out_args = 0;
    n_args = g_callable_info_get_n_args (callable_info);
    for (i = 0; i < n_args; i++) {
        GIArgInfo arg_info;
        GITypeInfo type_info;
        GIDirection direction;
        g_callable_info_load_arg (callable_info, i, &arg_info);
        g_arg_info_load_type (&arg_info, &type_info);
        direction = g_arg_info_get_direction (&arg_info);

        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
            GITransfer transfer = g_arg_info_get_ownership_transfer (&arg_info);

            if (g_type_info_get_tag (&type_info) == GI_TYPE_TAG_ERROR) {
                /* TODO: check if an exception has been set and convert it to a GError */
                out_args[i_out_args].v_pointer = NULL;
                i_out_args++;
                continue;
            }

            if (PyTuple_Check (py_retval)) {
                PyObject *item = PyTuple_GET_ITEM (py_retval, i_py_retval);
                _pygi_closure_assign_pyobj_to_out_argument (
                    out_args[i_out_args].v_pointer, item, &type_info, transfer);
            } else if (i_py_retval == 0) {
                _pygi_closure_assign_pyobj_to_out_argument (
                    out_args[i_out_args].v_pointer, py_retval, &type_info,
                    transfer);
            } else
                g_assert_not_reached();

            i_out_args++;
            i_py_retval++;
        }
    }
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
    PyGILState_STATE state;
    PyGICClosure *closure = data;
    PyObject *retval;
    PyObject *py_args;
    GIArgument *out_args = NULL;

    /* Lock the GIL as we are coming into this code without the lock and we
      may be executing python code */
    state = PyGILState_Ensure();

    if (!_pygi_closure_convert_arguments ( (GICallableInfo *) closure->info, args,
                                           closure->user_data,
                                           &py_args, &out_args)) {
        if (PyErr_Occurred ())
            PyErr_Print();
        goto end;
    }

    retval = PyObject_CallObject ( (PyObject *) closure->function, py_args);
    Py_DECREF (py_args);

    if (retval == NULL) {
        _pygi_closure_clear_retval (closure->info, result);
        PyErr_Print();
        goto end;
    }

    _pygi_closure_set_out_arguments (closure->info, retval, out_args, result);
    if (PyErr_Occurred ()) {
        _pygi_closure_clear_retval (closure->info, result);
        PyErr_Print();
    }

    Py_DECREF (retval);

end:
    g_free (out_args);

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

    PyGILState_Release (state);
}

void _pygi_invoke_closure_free (gpointer data)
{
    PyGICClosure* invoke_closure = (PyGICClosure *) data;

    g_callable_info_free_closure (invoke_closure->info,
                                  invoke_closure->closure);

    if (invoke_closure->info)
        g_base_info_unref ( (GIBaseInfo*) invoke_closure->info);

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

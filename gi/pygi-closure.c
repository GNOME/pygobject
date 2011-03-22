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
_pygi_closure_assign_pyobj_to_out_argument (gpointer out_arg, PyObject *object,
                                            GITypeInfo *type_info,
                                            GITransfer transfer)
{
    GIArgument arg = _pygi_argument_from_object (object, type_info, transfer);
    GITypeTag type_tag = g_type_info_get_tag (type_info);

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
        default:
           *((GIArgument *) out_arg) = arg;
           break;
      }
}

static GIArgument *
_pygi_closure_convert_ffi_arguments (GICallableInfo *callable_info, void **args)
{
    gint num_args, i;
    GIArgInfo *arg_info;
    GITypeInfo *arg_type;
    GITypeTag tag;
    GIDirection direction;
    GIArgument *g_args;

    num_args = g_callable_info_get_n_args (callable_info);
    g_args = g_new0 (GIArgument, num_args);

    for (i = 0; i < num_args; i++) {
        arg_info = g_callable_info_get_arg (callable_info, i);
        arg_type = g_arg_info_get_type (arg_info);
        tag = g_type_info_get_tag (arg_type);
        direction = g_arg_info_get_direction (arg_info);

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

                    interface = g_type_info_get_interface (arg_type);
                    interface_type = g_base_info_get_type (interface);

                    if (interface_type == GI_INFO_TYPE_OBJECT ||
                            interface_type == GI_INFO_TYPE_INTERFACE) {
                        g_args[i].v_pointer = * (gpointer *) args[i];
                        g_base_info_unref (interface);
                        break;
                    } else if (interface_type == GI_INFO_TYPE_ENUM ||
                               interface_type == GI_INFO_TYPE_FLAGS) {
                        g_args[i].v_double = * (double *) args[i];
                        g_base_info_unref (interface);
                        break;
                    } else if (interface_type == GI_INFO_TYPE_STRUCT) {
                        g_args[i].v_pointer = * (gpointer *) args[i];
                        g_base_info_unref (interface);
                        break;
                    }

                    g_base_info_unref (interface);
                }
                case GI_TYPE_TAG_GLIST:
                case GI_TYPE_TAG_GSLIST:
                    g_args[i].v_pointer = * (gpointer *) args[i];
                    break;
                default:
                    g_args[i].v_pointer = 0;
            }
        }
        g_base_info_unref ( (GIBaseInfo *) arg_info);
        g_base_info_unref ( (GIBaseInfo *) arg_type);
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
    GIArgument *g_args = NULL;

    *py_args = NULL;
    *py_args = PyTuple_New (n_args);
    if (*py_args == NULL)
        goto error;

    *out_args = NULL;
    *out_args = g_new0 (GIArgument, n_args);
    g_args = _pygi_closure_convert_ffi_arguments (callable_info, args);

    for (i = 0; i < n_args; i++) {
        GIArgInfo *arg_info = g_callable_info_get_arg (callable_info, i);
        GIDirection direction = g_arg_info_get_direction (arg_info);

        if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
            GITypeInfo *arg_type = g_arg_info_get_type (arg_info);
            GITypeTag arg_tag = g_type_info_get_tag (arg_type);
            GITransfer transfer = g_arg_info_get_ownership_transfer (arg_info);
            PyObject *value;
            GIArgument *arg;

            if (direction == GI_DIRECTION_IN && arg_tag == GI_TYPE_TAG_VOID &&
                    g_type_info_is_pointer (arg_type)) {

                if (user_data == NULL) {
                    Py_INCREF (Py_None);
                    value = Py_None;
                } else {
                    value = user_data;
                    Py_INCREF (value);
                }
            } else {
                if (direction == GI_DIRECTION_IN)
                    arg = (GIArgument*) &g_args[i];
                else
                    arg = (GIArgument*) g_args[i].v_pointer;

                value = _pygi_argument_to_object (arg, arg_type, transfer);
                if (value == NULL) {
                    g_base_info_unref (arg_type);
                    g_base_info_unref (arg_info);
                    goto error;
                }
            }
            PyTuple_SET_ITEM (*py_args, n_in_args, value);
            n_in_args++;

            g_base_info_unref (arg_type);
        }

        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
            (*out_args) [n_out_args] = g_args[i];
            n_out_args++;
        }

        g_base_info_unref (arg_info);
    }

    if (_PyTuple_Resize (py_args, n_in_args) == -1)
        goto error;

    g_free (g_args);
    return TRUE;

error:
    Py_CLEAR (*py_args);

    if (*out_args != NULL)
        g_free (*out_args);

    if (g_args != NULL)
        g_free (g_args);

    return FALSE;
}

static void
_pygi_closure_set_out_arguments (GICallableInfo *callable_info,
                                 PyObject *py_retval, GIArgument *out_args,
                                 void *resp)
{
    int n_args, i, i_py_retval, i_out_args;
    GITypeInfo *return_type_info;
    GITypeTag return_type_tag;

    i_py_retval = 0;
    return_type_info = g_callable_info_get_return_type (callable_info);
    return_type_tag = g_type_info_get_tag (return_type_info);
    if (return_type_tag != GI_TYPE_TAG_VOID) {
        GITransfer transfer = g_callable_info_get_caller_owns (callable_info);
        if (PyTuple_Check (py_retval)) {
            PyObject *item = PyTuple_GET_ITEM (py_retval, 0);
            _pygi_closure_assign_pyobj_to_out_argument (resp, item,
                return_type_info, transfer);
        } else {
            _pygi_closure_assign_pyobj_to_out_argument (resp, py_retval,
                return_type_info, transfer);
        }
        i_py_retval++;
    }
    g_base_info_unref (return_type_info);

    i_out_args = 0;
    n_args = g_callable_info_get_n_args (callable_info);
    for (i = 1; i < n_args; i++) {
        GIArgInfo *arg_info = g_callable_info_get_arg (callable_info, i);
        GITypeInfo *type_info = g_arg_info_get_type (arg_info);
        GIDirection direction = g_arg_info_get_direction (arg_info);

        if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT) {
            GITransfer transfer = g_arg_info_get_ownership_transfer (arg_info);

            if (g_type_info_get_tag (type_info) == GI_TYPE_TAG_ERROR) {
                /* TODO: check if an exception has been set and convert it to a GError */
                out_args[i_out_args].v_pointer = NULL;
                i_out_args++;
                continue;
            }

            if (PyTuple_Check (py_retval)) {
                PyObject *item = PyTuple_GET_ITEM (py_retval, i_py_retval);
                _pygi_closure_assign_pyobj_to_out_argument (
                    out_args[i_out_args].v_pointer, item, type_info, transfer);
            } else if (i_py_retval == 0) {
                _pygi_closure_assign_pyobj_to_out_argument (
                    out_args[i_out_args].v_pointer, py_retval, type_info,
                    transfer);
            } else
                g_assert_not_reached();

            i_out_args++;
            i_py_retval++;
        }
        g_base_info_unref (type_info);
        g_base_info_unref (arg_info);
    }
}

void
_pygi_closure_handle (ffi_cif *cif,
                      void    *result,
                      void   **args,
                      void    *data)
{
    PyGILState_STATE state;
    PyGICClosure *closure = data;
    GITypeTag  return_tag;
    GITransfer return_transfer;
    GITypeInfo *return_type;
    PyObject *retval;
    PyObject *py_args;
    GIArgument *out_args = NULL;

    /* Lock the GIL as we are coming into this code without the lock and we
      may be executing python code */
    state = PyGILState_Ensure();

    return_type = g_callable_info_get_return_type (closure->info);
    return_tag = g_type_info_get_tag (return_type);
    return_transfer = g_callable_info_get_caller_owns (closure->info);

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
        PyErr_Print();
        goto end;
    }

    _pygi_closure_set_out_arguments (closure->info, retval, out_args, result);

end:
    if (out_args != NULL)
        g_free (out_args);
    g_base_info_unref ( (GIBaseInfo*) return_type);

    PyGILState_Release (state);

    /* Now that the closure has finished we can make a decision about how
       to free it.  Scope call gets free'd at the end of wrap_g_function_info_invoke
       scope notified will be freed,  when the notify is called and we can free async
       anytime we want as long as its after we return from this function (you can't free the closure
       you are currently using!)
    */
    switch (closure->scope) {
        case GI_SCOPE_TYPE_CALL:
        case GI_SCOPE_TYPE_NOTIFIED:
            break;
        case GI_SCOPE_TYPE_ASYNC:
            /* Append this PyGICClosure to a list of closure that we will free
               after we're done with this function invokation */
            async_free_list = g_slist_prepend (async_free_list, closure);
            break;
        default:
            g_error ("Invalid scope reached inside %s.  Possibly a bad annotation?",
                     g_base_info_get_name (closure->info));
    }
}

void _pygi_invoke_closure_free (gpointer data)
{
    PyGICClosure* invoke_closure = (PyGICClosure *) data;

    Py_DECREF (invoke_closure->function);

    g_callable_info_free_closure (invoke_closure->info,
                                  invoke_closure->closure);

    if (invoke_closure->info)
        g_base_info_unref ( (GIBaseInfo*) invoke_closure->info);

    Py_XDECREF (invoke_closure->user_data);

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
    g_slist_foreach (async_free_list, (GFunc) _pygi_invoke_closure_free, NULL);
    g_slist_free (async_free_list);
    async_free_list = NULL;

    /* Build the closure itself */
    closure = g_slice_new0 (PyGICClosure);
    closure->info = (GICallableInfo *) g_base_info_ref ( (GIBaseInfo *) info);
    closure->function = py_function;
    closure->user_data = py_user_data;

    Py_INCREF (py_function);
    if (closure->user_data)
        Py_INCREF (closure->user_data);

    fficlosure =
        g_callable_info_prepare_closure (info, &closure->cif, _pygi_closure_handle,
                                         closure);
    closure->closure = fficlosure;

    /* Give the closure the information it needs to determine when
       to free itself later */
    closure->scope = scope;

    return closure;
}

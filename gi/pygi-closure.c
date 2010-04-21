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

void
_pygi_closure_handle (ffi_cif *cif,
                      void    *result,
                      void   **args,
                      void    *data)
{
    PyGILState_STATE state;
    PyGICClosure *closure = data;
    gint n_args, i;
    GIArgInfo  *arg_info;
    GIDirection arg_direction;
    GITypeInfo *arg_type;
    GITransfer arg_transfer;
    GITypeTag  arg_tag;
    GITypeTag  return_tag;
    GITransfer return_transfer;
    GITypeInfo *return_type;
    PyObject *retval;
    PyObject *py_args;
    PyObject *pyarg;
    gint n_in_args, n_out_args;


    /* Lock the GIL as we are coming into this code without the lock and we
      may be executing python code */
    state = PyGILState_Ensure();

    return_type = g_callable_info_get_return_type(closure->info);
    return_tag = g_type_info_get_tag(return_type);
    return_transfer = g_callable_info_get_caller_owns(closure->info);

    n_args = g_callable_info_get_n_args (closure->info);  

    py_args = PyTuple_New(n_args);
    if (py_args == NULL) {
        PyErr_Print();
        goto end;
    }

    n_in_args = 0;

    for (i = 0; i < n_args; i++) {
        arg_info = g_callable_info_get_arg (closure->info, i);
        arg_type = g_arg_info_get_type (arg_info);
        arg_transfer = g_arg_info_get_ownership_transfer(arg_info);
        arg_tag = g_type_info_get_tag(arg_type);
        arg_direction = g_arg_info_get_direction(arg_info);
        switch (arg_tag){
            case GI_TYPE_TAG_VOID:
                {
                    if (g_type_info_is_pointer(arg_type)) {
                        if (PyTuple_SetItem(py_args, n_in_args, closure->user_data) != 0) {
                            PyErr_Print();
                            goto end;
                        }
                        n_in_args++;
                        continue;
                    }
                }
            case GI_TYPE_TAG_ERROR:
                 {
                     continue;
                 }
            default:
                {
                    pyarg = _pygi_argument_to_object (args[i],
                                                      arg_type,
                                                      arg_transfer);
                    
                    if(PyTuple_SetItem(py_args, n_in_args, pyarg) != 0) {
                        PyErr_Print();
                        goto end;
                    }
                    n_in_args++;
                    g_base_info_unref((GIBaseInfo*)arg_info);
                    g_base_info_unref((GIBaseInfo*)arg_type);                                    
                }
        }
        
    }

    if(_PyTuple_Resize (&py_args, n_in_args) != 0) {
        PyErr_Print();
        goto end;
    }

    retval = PyObject_CallObject((PyObject *)closure->function, py_args);

    Py_DECREF(py_args);

    if (retval == NULL) {
        PyErr_Print();
        goto end;
    }

    *(GArgument*)result = _pygi_argument_from_object(retval, return_type, return_transfer);

end:
    g_base_info_unref((GIBaseInfo*)return_type);

    PyGILState_Release(state);

    if (closure->user_data)
        Py_XDECREF(closure->user_data);

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
        async_free_list = g_slist_prepend(async_free_list, closure);
        break;
    default:
        /* Invalid scope type.  Perhaps a typo in the annotation? */
        g_warn_if_reached();
    }
}

void _pygi_invoke_closure_free(gpointer data)
{
    PyGICClosure* invoke_closure = (PyGICClosure *)data;
    

    Py_DECREF(invoke_closure->function);

    g_callable_info_free_closure(invoke_closure->info,
                                 invoke_closure->closure);

    if (invoke_closure->info)
        g_base_info_unref((GIBaseInfo*)invoke_closure->info);

    g_slice_free(PyGICClosure, invoke_closure);
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
    g_slist_foreach(async_free_list, (GFunc)_pygi_invoke_closure_free, NULL);
    g_slist_free(async_free_list);
    async_free_list = NULL;

    /* Build the closure itself */
    closure = g_slice_new0(PyGICClosure);   
    closure->info = (GICallableInfo *) g_base_info_ref ((GIBaseInfo *) info);  
    closure->function = py_function;
    closure->user_data = py_user_data;

    Py_INCREF(py_function);
    if (closure->user_data)
        Py_INCREF(closure->user_data);

    fficlosure =
        g_callable_info_prepare_closure (info, &closure->cif, _pygi_closure_handle,
                                         closure);
    closure->closure = fficlosure;
    
    /* Give the closure the information it needs to determine when
       to free itself later */
    closure->scope = scope;

    return closure;
}

/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 *   pygi-callbacks.c: PyGI C Callback Functions and Helpers
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

static PyGICClosure *global_destroy_notify;

static void
_pygi_destroy_notify_callback_closure (ffi_cif *cif,
                                       void *result,
                                       void **args,
                                       void *data)
{
    PyGICClosure *info = * (void**) (args[0]);

    g_assert (info);

    _pygi_invoke_closure_free (info);
}


PyGICClosure*
_pygi_destroy_notify_create (void)
{
    if (!global_destroy_notify) {

        PyGICClosure *destroy_notify = g_slice_new0 (PyGICClosure);

        g_assert (destroy_notify);

        GIBaseInfo* glib_destroy_notify = g_irepository_find_by_name (NULL, "GLib", "DestroyNotify");
        g_assert (glib_destroy_notify != NULL);
        g_assert (g_base_info_get_type (glib_destroy_notify) == GI_INFO_TYPE_CALLBACK);

        destroy_notify->closure = g_callable_info_prepare_closure ( (GICallableInfo*) glib_destroy_notify,
                                                                    &destroy_notify->cif,
                                                                    _pygi_destroy_notify_callback_closure,
                                                                    NULL);

        global_destroy_notify = destroy_notify;
    }

    return global_destroy_notify;
}


gboolean
_pygi_scan_for_callbacks (GIFunctionInfo *function_info,
                          gboolean       is_method,
                          guint8        *callback_index,
                          guint8        *user_data_index,
                          guint8        *destroy_notify_index)
{
    guint i, n_args;

    *callback_index = G_MAXUINT8;
    *user_data_index = G_MAXUINT8;
    *destroy_notify_index = G_MAXUINT8;

    n_args = g_callable_info_get_n_args ( (GICallableInfo *) function_info);
    for (i = 0; i < n_args; i++) {
        GIDirection direction;
        GIArgInfo *arg_info;
        GITypeInfo *type_info;
        guint8 destroy, closure;
        GITypeTag type_tag;

        arg_info = g_callable_info_get_arg ( (GICallableInfo*) function_info, i);
        type_info = g_arg_info_get_type (arg_info);
        type_tag = g_type_info_get_tag (type_info);

        if (type_tag == GI_TYPE_TAG_INTERFACE) {
            GIBaseInfo* interface_info;
            GIInfoType interface_type;

            interface_info = g_type_info_get_interface (type_info);
            interface_type = g_base_info_get_type (interface_info);
            if (interface_type == GI_INFO_TYPE_CALLBACK &&
                    ! (strcmp (g_base_info_get_namespace ( (GIBaseInfo*) interface_info), "GLib") == 0 &&
                       (strcmp (g_base_info_get_name ( (GIBaseInfo*) interface_info), "DestroyNotify") == 0 ||
                       (strcmp (g_base_info_get_name ( (GIBaseInfo*) interface_info), "FreeFunc") == 0)))) {
                if (*callback_index != G_MAXUINT8) {
                    PyErr_Format (PyExc_TypeError, "Function %s.%s has multiple callbacks, not supported",
                                  g_base_info_get_namespace ( (GIBaseInfo*) function_info),
                                  g_base_info_get_name ( (GIBaseInfo*) function_info));
                    g_base_info_unref (interface_info);
                    return FALSE;
                }
                *callback_index = i;
            }
            g_base_info_unref (interface_info);
        }
        destroy = g_arg_info_get_destroy (arg_info);
        
        closure = g_arg_info_get_closure (arg_info);
        direction = g_arg_info_get_direction (arg_info);

        if (destroy > 0 && destroy < n_args) {
            if (*destroy_notify_index != G_MAXUINT8) {
                PyErr_Format (PyExc_TypeError, "Function %s has multiple GDestroyNotify, not supported",
                              g_base_info_get_name ( (GIBaseInfo*) function_info));
                return FALSE;
            }
            *destroy_notify_index = destroy;
        }

        if (closure > 0 && closure < n_args) {
            if (*user_data_index != G_MAXUINT8) {
                PyErr_Format (PyExc_TypeError, "Function %s has multiple user_data arguments, not supported",
                              g_base_info_get_name ( (GIBaseInfo*) function_info));
                return FALSE;
            }
            *user_data_index = closure;
        }

        g_base_info_unref ( (GIBaseInfo*) arg_info);
        g_base_info_unref ( (GIBaseInfo*) type_info);
    }

    return TRUE;
}

gboolean
_pygi_create_callback (GIBaseInfo  *function_info,
                       gboolean       is_method,
                       gboolean       is_constructor,
                       int            n_args,
                       Py_ssize_t     py_argc,
                       PyObject      *py_argv,
                       guint8         callback_index,
                       guint8         user_data_index,
                       guint8         destroy_notify_index,
                       PyGICClosure **closure_out)
{
    GIArgInfo *callback_arg;
    GITypeInfo *callback_type;
    GICallbackInfo *callback_info;
    GIScopeType scope;
    gboolean found_py_function;
    PyObject *py_function;
    guint8 i, py_argv_pos;
    PyObject *py_user_data;
    gboolean allow_none;

    callback_arg = g_callable_info_get_arg ( (GICallableInfo*) function_info, callback_index);
    scope = g_arg_info_get_scope (callback_arg);
    allow_none = g_arg_info_may_be_null (callback_arg);

    callback_type = g_arg_info_get_type (callback_arg);
    g_assert (g_type_info_get_tag (callback_type) == GI_TYPE_TAG_INTERFACE);

    callback_info = (GICallbackInfo*) g_type_info_get_interface (callback_type);
    g_assert (g_base_info_get_type ( (GIBaseInfo*) callback_info) == GI_INFO_TYPE_CALLBACK);

    /* Find the Python function passed for the callback */
    found_py_function = FALSE;
    py_function = Py_None;
    py_user_data = NULL;

    /* if its a method then we need to skip over 'self' */
    if (is_method || is_constructor)
        py_argv_pos = 1;
    else
        py_argv_pos = 0;

    for (i = 0; i < n_args && i < py_argc; i++) {
        if (i == callback_index) {
            py_function = PyTuple_GetItem (py_argv, py_argv_pos);
            /* if we allow none then set the closure to NULL and return */
            if (allow_none && py_function == Py_None) {
                *closure_out = NULL;
                goto out;
            }
            found_py_function = TRUE;
        } else if (i == user_data_index) {
            py_user_data = PyTuple_GetItem (py_argv, py_argv_pos);
        }
        py_argv_pos++;
    }

    if (!found_py_function
            || (py_function == Py_None || !PyCallable_Check (py_function))) {
        PyErr_Format (PyExc_TypeError, "Error invoking %s.%s: Unexpected value "
                      "for argument '%s'",
                      g_base_info_get_namespace ( (GIBaseInfo*) function_info),
                      g_base_info_get_name ( (GIBaseInfo*) function_info),
                      g_base_info_get_name ( (GIBaseInfo*) callback_arg));
        g_base_info_unref ( (GIBaseInfo*) callback_info);
        g_base_info_unref ( (GIBaseInfo*) callback_type);
        return FALSE;
    }

    /** Now actually build the closure **/
    *closure_out = _pygi_make_native_closure ( (GICallableInfo *) callback_info,
                                               g_arg_info_get_scope (callback_arg),
                                               py_function,
                                               py_user_data);
out:
    g_base_info_unref ( (GIBaseInfo*) callback_info);
    g_base_info_unref ( (GIBaseInfo*) callback_type);

    return TRUE;
}

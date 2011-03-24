/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2011  Laszlo Pandy <lpandy@src.gnome.org>
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

/* Copied from glib */
static void
canonicalize_key (gchar *key)
{
    gchar *p;

    for (p = key; *p != 0; p++)
    {
        gchar c = *p;

        if (c != '-' &&
            (c < '0' || c > '9') &&
            (c < 'A' || c > 'Z') &&
            (c < 'a' || c > 'z'))
                *p = '-';
    }
}

static GISignalInfo *
_pygi_lookup_signal_from_g_type (GType g_type,
                                 const gchar *signal_name)
{
    GIRepository *repository;
    GIBaseInfo *info;
    gssize n_infos;
    gssize i;
    GType parent;

    repository = g_irepository_get_default();
    info = g_irepository_find_by_gtype (repository, g_type);
    if (info != NULL) {
        n_infos = g_object_info_get_n_signals ( (GIObjectInfo *) info);
        for (i = 0; i < n_infos; i++) {
            GISignalInfo *signal_info;

            signal_info = g_object_info_get_signal ( (GIObjectInfo *) info, i);
            g_assert (info != NULL);

            if (strcmp (signal_name, g_base_info_get_name (signal_info)) == 0) {
                g_base_info_unref (info);
                return signal_info;
            }

            g_base_info_unref (signal_info);
        }

        g_base_info_unref (info);
    }

    parent = g_type_parent (g_type);
    if (parent > 0)
        return _pygi_lookup_signal_from_g_type (parent, signal_name);

    return NULL;
}

static void
pygi_signal_closure_invalidate(gpointer data,
                               GClosure *closure)
{
    PyGClosure *pc = (PyGClosure *)closure;
    PyGILState_STATE state;

    state = PyGILState_Ensure();
    Py_XDECREF(pc->callback);
    Py_XDECREF(pc->extra_args);
    Py_XDECREF(pc->swap_data);
    PyGILState_Release(state);

    pc->callback = NULL;
    pc->extra_args = NULL;
    pc->swap_data = NULL;

    g_base_info_unref (((PyGISignalClosure *) pc)->signal_info);
    ((PyGISignalClosure *) pc)->signal_info = NULL;
}

static void
pygi_signal_closure_marshal(GClosure *closure,
                            GValue *return_value,
                            guint n_param_values,
                            const GValue *param_values,
                            gpointer invocation_hint,
                            gpointer marshal_data)
{
    PyGILState_STATE state;
    PyGClosure *pc = (PyGClosure *)closure;
    PyObject *params, *ret = NULL;
    guint i;
    GISignalInfo *signal_info;
    gint n_sig_info_args;
    gint sig_info_highest_arg;

    state = PyGILState_Ensure();

    signal_info = ((PyGISignalClosure *)closure)->signal_info;
    n_sig_info_args = g_callable_info_get_n_args(signal_info);
    /* the first argument to a signal callback is instance,
       but instance is not counted in the introspection data */
    sig_info_highest_arg = n_sig_info_args + 1;
    g_assert_cmpint(sig_info_highest_arg, ==, n_param_values);

    /* construct Python tuple for the parameter values */
    params = PyTuple_New(n_param_values);
    for (i = 0; i < n_param_values; i++) {
        /* swap in a different initial data for connect_object() */
        if (i == 0 && G_CCLOSURE_SWAP_DATA(closure)) {
            g_return_if_fail(pc->swap_data != NULL);
            Py_INCREF(pc->swap_data);
            PyTuple_SetItem(params, 0, pc->swap_data);

        } else if (i == 0) {
            PyObject *item = pyg_value_as_pyobject(&param_values[i], FALSE);

            if (!item) {
                goto out;
            }
            PyTuple_SetItem(params, i, item);

        } else if (i < sig_info_highest_arg) {
            GIArgInfo arg_info;
            GITypeInfo type_info;
            GITransfer transfer;
            GIArgument arg = { 0, };
            PyObject *item = NULL;

            g_callable_info_load_arg(signal_info, i - 1, &arg_info);
            g_arg_info_load_type(&arg_info, &type_info);
            transfer = g_arg_info_get_ownership_transfer(&arg_info);

            arg = _pygi_argument_from_g_value(&param_values[i], &type_info);
            item = _pygi_argument_to_object(&arg, &type_info, transfer);

            if (item == NULL) {
                goto out;
            }
            PyTuple_SetItem(params, i, item);
        }
    }
    /* params passed to function may have extra arguments */
    if (pc->extra_args) {
        PyObject *tuple = params;
        params = PySequence_Concat(tuple, pc->extra_args);
        Py_DECREF(tuple);
    }
    ret = PyObject_CallObject(pc->callback, params);
    if (ret == NULL) {
        if (pc->exception_handler)
            pc->exception_handler(return_value, n_param_values, param_values);
        else
            PyErr_Print();
        goto out;
    }

    if (return_value && pyg_value_from_pyobject(return_value, ret) != 0) {
        PyErr_SetString(PyExc_TypeError,
                        "can't convert return value to desired type");

        if (pc->exception_handler)
            pc->exception_handler(return_value, n_param_values, param_values);
        else
            PyErr_Print();
    }
    Py_DECREF(ret);

 out:
    Py_DECREF(params);
    PyGILState_Release(state);
}

GClosure *
pygi_signal_closure_new_real (PyGObject *instance,
                              const gchar *sig_name,
                              PyObject *callback,
                              PyObject *extra_args,
                              PyObject *swap_data)
{
    GClosure *closure = NULL;
    PyGISignalClosure *pygi_closure = NULL;
    GType g_type;
    GISignalInfo *signal_info = NULL;
    char *signal_name = g_strdup (sig_name);

    g_return_val_if_fail(callback != NULL, NULL);

    canonicalize_key(signal_name);

    g_type = pyg_type_from_object ((PyObject *)instance);
    signal_info = _pygi_lookup_signal_from_g_type (g_type, signal_name);

    if (signal_info == NULL)
        goto out;

    closure = g_closure_new_simple(sizeof(PyGISignalClosure), NULL);
    g_closure_add_invalidate_notifier(closure, NULL, pygi_signal_closure_invalidate);
    g_closure_set_marshal(closure, pygi_signal_closure_marshal);

    pygi_closure = (PyGISignalClosure *)closure;

    pygi_closure->signal_info = signal_info;
    Py_INCREF(callback);
    pygi_closure->pyg_closure.callback = callback;

    if (extra_args != NULL && extra_args != Py_None) {
        Py_INCREF(extra_args);
        if (!PyTuple_Check(extra_args)) {
            PyObject *tmp = PyTuple_New(1);
            PyTuple_SetItem(tmp, 0, extra_args);
            extra_args = tmp;
        }
        pygi_closure->pyg_closure.extra_args = extra_args;
    }
    if (swap_data) {
        Py_INCREF(swap_data);
        pygi_closure->pyg_closure.swap_data = swap_data;
        closure->derivative_flag = TRUE;
    }

out:
    g_free (signal_name);

    return closure;
}

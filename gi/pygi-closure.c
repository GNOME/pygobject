/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 *   pygi-closure.c: PyGI Closure functions
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

#include "pygboxed.h"
#include "pygi-argument.h"
#include "pygi-boxed.h"
#include "pygi-closure.h"
#include "pygi-repository.h"
#include "pygi-value.h"
#include "pygobject-object.h"


/* -------------- PyGClosure ----------------- */

static void
pyg_closure_invalidate (gpointer data, GClosure *closure)
{
    PyGClosure *pc = (PyGClosure *)closure;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();
    Py_XDECREF (pc->callback);
    Py_XDECREF (pc->extra_args);
    if (!pc->swap_data_is_gobject) Py_XDECREF (pc->swap_data);
    PyGILState_Release (state);

    pc->callback = NULL;
    pc->extra_args = NULL;
    pc->swap_data = NULL;
}

static void
pyg_closure_marshal (GClosure *closure, GValue *return_value,
                     guint n_param_values, const GValue *param_values,
                     gpointer invocation_hint, gpointer marshal_data)
{
    PyGILState_STATE state;
    PyGClosure *pc = (PyGClosure *)closure;
    PyObject *params, *ret;
    guint i;

    state = PyGILState_Ensure ();

    /* construct Python tuple for the parameter values */
    params = PyTuple_New (n_param_values);
    for (i = 0; i < n_param_values; i++) {
        /* swap in a different initial data for connect_object() */
        if (i == 0 && G_CCLOSURE_SWAP_DATA (closure)) {
            g_return_if_fail (pc->swap_data != NULL);
            if (pc->swap_data_is_gobject)
                PyTuple_SetItem (params, 0,
                                 pygobject_new (G_OBJECT (pc->swap_data)));
            else
                PyTuple_SetItem (params, 0, Py_NewRef (pc->swap_data));
        } else {
            PyObject *item = pyg_value_to_pyobject (&param_values[i], FALSE);

            /* error condition */
            if (!item) {
                if (!PyErr_Occurred ())
                    PyErr_SetString (
                        PyExc_TypeError,
                        "can't convert parameter to desired type");

                if (pc->exception_handler)
                    pc->exception_handler (return_value, n_param_values,
                                           param_values);
                else
                    PyErr_Print ();

                goto out;
            }
            PyTuple_SetItem (params, i, item);
        }
    }
    /* params passed to function may have extra arguments */
    if (pc->extra_args) {
        PyObject *tuple = params;
        params = PySequence_Concat (tuple, pc->extra_args);
        Py_DECREF (tuple);
    }
    ret = PyObject_CallObject (pc->callback, params);
    if (ret == NULL) {
        if (pc->exception_handler)
            pc->exception_handler (return_value, n_param_values, param_values);
        else
            PyErr_Print ();
        goto out;
    }

    if (G_IS_VALUE (return_value)
        && pyg_value_from_pyobject (return_value, ret) != 0) {
        /* If we already have an exception set, use that, otherwise set a
	 * generic one */
        if (!PyErr_Occurred ())
            PyErr_SetString (PyExc_TypeError,
                             "can't convert return value to desired type");

        if (pc->exception_handler)
            pc->exception_handler (return_value, n_param_values, param_values);
        else
            PyErr_Print ();
    }
    Py_DECREF (ret);

out:
    Py_DECREF (params);
    PyGILState_Release (state);
}

GClosure *
pyg_closure_new_full (guint sizeof_closure, PyObject *callback,
                      PyObject *extra_args, PyObject *swap_data,
                      GClosureMarshal closure_marshal,
                      GClosureNotify closure_invalidate)
{
    GClosure *closure;

    g_return_val_if_fail (callback != NULL, NULL);
    g_assert (sizeof_closure >= sizeof (PyGClosure));

    closure = g_closure_new_simple (sizeof_closure, NULL);

    if (swap_data != NULL && PyObject_TypeCheck (swap_data, &PyGObject_Type)) {
        GObject *data = pygobject_get (swap_data);
        g_object_watch_closure (data, closure);
        ((PyGClosure *)closure)->swap_data = data;
        ((PyGClosure *)closure)->swap_data_is_gobject = TRUE;
        closure->derivative_flag = TRUE;
    } else if (swap_data != NULL) {
        ((PyGClosure *)closure)->swap_data = Py_NewRef (swap_data);
        closure->derivative_flag = TRUE;
    }

    g_closure_set_marshal (closure, closure_marshal);
    g_closure_add_invalidate_notifier (closure, NULL, closure_invalidate);

    ((PyGClosure *)closure)->callback = Py_NewRef (callback);

    if (extra_args != NULL && !Py_IsNone (extra_args)) {
        Py_INCREF (extra_args);
        if (!PyTuple_Check (extra_args)) {
            PyObject *tmp = PyTuple_New (1);
            PyTuple_SetItem (tmp, 0, extra_args);
            extra_args = tmp;
        }
        ((PyGClosure *)closure)->extra_args = extra_args;
    }
    return closure;
}

/**
 * pyg_closure_new:
 * callback: a Python callable object
 * extra_args: a tuple of extra arguments, or None/NULL.
 * swap_data: an alternative python object to pass first.
 *
 * Creates a GClosure wrapping a Python callable and optionally a set
 * of additional function arguments.  This is needed to attach python
 * handlers to signals, for instance.
 *
 * If swap_data is a GObject and the object is destroyed,
 * then the closure will be automatically invalidated.
 *
 * Returns: the new closure.
 */
GClosure *
pyg_closure_new (PyObject *callback, PyObject *extra_args, PyObject *swap_data)
{
    g_return_val_if_fail (callback != NULL, NULL);

    return pyg_closure_new_full (sizeof (PyGClosure), callback, extra_args,
                                 swap_data, pyg_closure_marshal,
                                 pyg_closure_invalidate);
}

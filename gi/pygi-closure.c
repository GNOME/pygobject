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
pyg_closure_set_swap_data (PyGClosure *pc, PyObject *params)
{
    if (pc->swap_data_is_gobject)
        PyTuple_SetItem (params, 0, pygobject_new (G_OBJECT (pc->swap_data)));
    else
        PyTuple_SetItem (params, 0, Py_NewRef (pc->swap_data));
}

static gboolean
pyg_closure_invoke_with_extra_data (PyGClosure *pc, PyObject *params,
                                    GValue *return_value)
{
    PyObject *all_params, *ret;
    gboolean result = TRUE;

    /* params passed to function may have extra arguments */
    if (pc->extra_args)
        all_params = PySequence_Concat (params, pc->extra_args);
    else
        all_params = params;

    ret = PyObject_CallObject (pc->callback, all_params);
    if (pc->extra_args) Py_DECREF (all_params);
    if (ret == NULL) {
        PyErr_Print ();
        return FALSE;
    }

    if (G_IS_VALUE (return_value)
        && pyg_value_from_pyobject (return_value, ret) != 0) {
        PyErr_SetString (PyExc_TypeError,
                         "can't convert return value to desired type");

        PyErr_Print ();
        result = FALSE;
    }
    Py_DECREF (ret);
    return result;
}

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
    PyObject *params;
    guint i;

    state = PyGILState_Ensure ();

    /* construct Python tuple for the parameter values */
    params = PyTuple_New (n_param_values);
    for (i = 0; i < n_param_values; i++) {
        /* swap in a different initial data for connect_object() */
        if (i == 0 && G_CCLOSURE_SWAP_DATA (closure)) {
            g_return_if_fail (pc->swap_data != NULL);
            pyg_closure_set_swap_data (pc, params);
        } else {
            PyObject *item = pyg_value_to_pyobject (&param_values[i], FALSE);

            if (item == NULL) {
                if (!PyErr_Occurred ())
                    PyErr_SetString (
                        PyExc_TypeError,
                        "can't convert parameter to desired type");

                PyErr_Print ();

                goto out;
            }
            PyTuple_SetItem (params, i, item);
        }
    }

    pyg_closure_invoke_with_extra_data (pc, params, return_value);

out:
    Py_DECREF (params);
    PyGILState_Release (state);
}

static GClosure *
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

/* -------------- PySignalClosure ----------------- */
/* The signal marshaller is different from PyGClosure's marshaller in that it's
 * using GIArgInfo to convert GValue to PyObjects, where PyGClosure is converting
 * GValues directly.
 * GValue has less detailed type information compared to GIArgument conversion.
 */

typedef struct _PyGISignalClosure {
    PyGClosure pyg_closure;
    GISignalInfo *signal_info;
} PyGISignalClosure;

static GISignalInfo *
lookup_signal_from_g_type (GType g_type, const gchar *signal_name)
{
    GIBaseInfo *info;
    GISignalInfo *signal_info = NULL;

    info = pygi_repository_find_by_gtype (g_type);
    if (info == NULL) return NULL;

    if (GI_IS_OBJECT_INFO (info))
        signal_info =
            gi_object_info_find_signal (GI_OBJECT_INFO (info), signal_name);
    else if (GI_IS_INTERFACE_INFO (info))
        signal_info = gi_interface_info_find_signal (GI_INTERFACE_INFO (info),
                                                     signal_name);

    gi_base_info_unref (info);

    return signal_info;
}


static void
pygi_signal_closure_invalidate (gpointer data, GClosure *closure)
{
    pyg_closure_invalidate (data, closure);

    gi_base_info_unref (((PyGISignalClosure *)closure)->signal_info);
    ((PyGISignalClosure *)closure)->signal_info = NULL;
}

/* Hack to ensure struct arguments are passed-by-reference allowing
 * callback implementors to modify the struct values. This is needed
 * for keeping backwards compatibility and should be removed in future
 * versions which support signal output arguments as return values.
 * See: https://bugzilla.gnome.org/show_bug.cgi?id=735486
 *
 * Note the logic here must match the logic path taken in pygi_argument_to_py.
 */
static gboolean
should_pass_struct_by_ref (GITypeInfo *type_info, GITypeTag type_tag)
{
    gboolean pass_struct_by_ref = FALSE;

    if (type_tag == GI_TYPE_TAG_INTERFACE) {
        GIBaseInfo *info = gi_type_info_get_interface (type_info);

        if (GI_IS_STRUCT_INFO (info) || GI_IS_UNION_INFO (info)) {
            GType gtype = gi_registered_type_info_get_g_type (
                (GIRegisteredTypeInfo *)info);
            gboolean is_foreign =
                (GI_IS_STRUCT_INFO (info))
                && (gi_struct_info_is_foreign ((GIStructInfo *)info));

            if (!is_foreign && !g_type_is_a (gtype, G_TYPE_VALUE)
                && g_type_is_a (gtype, G_TYPE_BOXED)) {
                pass_struct_by_ref = TRUE;
            }
        }

        gi_base_info_unref (info);
    }

    return pass_struct_by_ref;
}

static gboolean
array_length_from_parameter (GICallableInfo *callable_info,
                             GITypeInfo *type_info, GValue *values,
                             guint n_values, gsize *array_len)
{
    guint length_arg_index;
    GIArgInfo length_arg_info;
    GITypeInfo length_type_info;
    GIArgument length_arg;
    gboolean result;

    if (!gi_type_info_get_array_length_index (type_info, &length_arg_index)) {
        return FALSE;
    }

    g_assert (length_arg_index + 1 < n_values);

    gi_callable_info_load_arg (callable_info, length_arg_index,
                               &length_arg_info);
    gi_arg_info_load_type_info (&length_arg_info, &length_type_info);

    /* Take the next value: values[0] is "self". */
    length_arg = pygi_argument_from_g_value (&(values[length_arg_index + 1]),
                                             &length_type_info);
    result = pygi_argument_to_gsize (
        length_arg, gi_type_info_get_tag (&length_type_info), array_len);

    gi_base_info_clear (&length_arg_info);
    gi_base_info_clear (&length_type_info);

    return result;
}

static void
pygi_signal_closure_marshal (GClosure *closure, GValue *return_value,
                             guint n_param_values, const GValue *param_values,
                             gpointer invocation_hint, gpointer marshal_data)
{
    PyGILState_STATE state;
    PyGClosure *pc = (PyGClosure *)closure;
    PyObject *params;
    guint i;
    GISignalInfo *signal_info;
    gint n_sig_info_args;
    gint sig_info_highest_arg;
    GSList *pass_by_ref_structs = NULL;

    state = PyGILState_Ensure ();

    signal_info = ((PyGISignalClosure *)closure)->signal_info;
    n_sig_info_args =
        gi_callable_info_get_n_args (GI_CALLABLE_INFO (signal_info));
    g_assert_cmpint (n_sig_info_args, >=, 0);
    /* the first argument to a signal callback is instance,
       but instance is not counted in the introspection data */
    sig_info_highest_arg = n_sig_info_args + 1;
    g_assert_cmpint (sig_info_highest_arg, ==, n_param_values);

    /* construct Python tuple for the parameter values */
    params = PyTuple_New (n_param_values);
    for (i = 0; i < n_param_values; i++) {
        /* swap in a different initial data for connect_object() */
        if (i == 0 && G_CCLOSURE_SWAP_DATA (closure)) {
            g_return_if_fail (pc->swap_data != NULL);
            pyg_closure_set_swap_data (pc, params);
        } else if (i == 0) {
            PyObject *item = pyg_value_to_pyobject (&param_values[i], FALSE);

            if (item == NULL) {
                PyErr_Print ();
                goto out;
            }
            PyTuple_SetItem (params, i, item);

        } else {
            GIArgInfo arg_info;
            GITypeInfo *type_info;
            GITypeTag type_tag;
            GIArgument arg;
            PyObject *item = NULL;
            gboolean pass_struct_by_ref;
            GITransfer transfer;
            gsize array_length;

            gi_callable_info_load_arg (GI_CALLABLE_INFO (signal_info), i - 1,
                                       &arg_info);
            type_info = gi_arg_info_get_type_info (&arg_info);

            arg = pygi_argument_from_g_value (&param_values[i], type_info);
            type_tag = gi_type_info_get_tag (type_info);

            pass_struct_by_ref =
                should_pass_struct_by_ref (type_info, type_tag);
            transfer = pass_struct_by_ref ? GI_TRANSFER_EVERYTHING
                                          : GI_TRANSFER_NOTHING;

            if (array_length_from_parameter (GI_CALLABLE_INFO (signal_info),
                                             type_info, param_values,
                                             n_param_values, &array_length))
                item = pygi_argument_to_py_with_array_length (
                    type_info, arg, transfer, array_length);
            else
                item = pygi_argument_to_py (type_info, arg, transfer);

            if (pass_struct_by_ref && item
                && PyObject_IsInstance (item, (PyObject *)&PyGIBoxed_Type)) {
                ((PyGBoxed *)item)->free_on_dealloc = FALSE;
                pass_by_ref_structs =
                    g_slist_prepend (pass_by_ref_structs, item);
            }

            gi_base_info_unref (type_info);
            gi_base_info_clear (&arg_info);

            if (item == NULL) {
                PyErr_Print ();
                goto out;
            }
            PyTuple_SetItem (params, i, item);
        }
    }

    if (pyg_closure_invoke_with_extra_data (pc, params, return_value)) {
        /* Run through the list of structs which have been passed by reference and
         * check if they are being held longer than the duration of the callback
         * execution. This is determined if the ref count is greater than 1.
         * A single ref is held by the argument list and any more would mean the callback
         * stored a ref somewhere else. In this case we make an internal copy of
         * the boxed struct so Python can own the memory to it.
         */
        GSList *list_item = pass_by_ref_structs;
        while (list_item) {
            PyObject *item = list_item->data;
            if (Py_REFCNT (item) > 1) {
                pygi_boxed_copy_in_place ((PyGIBoxed *)item);
            }
            list_item = g_slist_next (list_item);
        }
    }

out:
    g_slist_free (pass_by_ref_structs);
    Py_DECREF (params);
    PyGILState_Release (state);
}

GClosure *
pygi_signal_closure_new (GType g_type, const gchar *signal_name,
                         PyObject *callback, PyObject *extra_args,
                         PyObject *swap_data)
{
    GClosure *closure = NULL;
    GISignalInfo *signal_info = NULL;

    g_return_val_if_fail (callback != NULL, NULL);

    signal_info = lookup_signal_from_g_type (g_type, signal_name);
    if (signal_info == NULL) return NULL;

    closure = pyg_closure_new_full (
        sizeof (PyGISignalClosure), callback, extra_args, swap_data,
        pygi_signal_closure_marshal, pygi_signal_closure_invalidate);

    ((PyGISignalClosure *)closure)->signal_info = signal_info;

    return closure;
}

/* -------------- PySignalClassClosure ----------------- */
/* a closure used for the `class closure' of a signal.  As this gets
 * all the info from the first argument to the closure and the
 * invocation hint, we can have a single closure that handles all
 * class closure cases.  We call a method by the name of the signal
 * with "do_" prepended.
 *
 *  We also remove the first argument from the * param list, as it is
 *  the instance object, which is passed * implicitly to the method
 *  object. */

static void
pyg_signal_class_closure_marshal (GClosure *closure, GValue *return_value,
                                  guint n_param_values,
                                  const GValue *param_values,
                                  gpointer invocation_hint,
                                  gpointer marshal_data)
{
    PyGILState_STATE state;
    GObject *object;
    PyObject *object_wrapper;
    GSignalInvocationHint *hint = (GSignalInvocationHint *)invocation_hint;
    gchar *method_name, *tmp;
    PyObject *method;
    PyObject *params, *ret;
    Py_ssize_t py_len;
    guint i, len;

    state = PyGILState_Ensure ();

    g_return_if_fail (invocation_hint != NULL);
    /* get the object passed as the first argument to the closure */
    object = g_value_get_object (&param_values[0]);
    g_return_if_fail (object != NULL && G_IS_OBJECT (object));

    /* get the wrapper for this object */
    object_wrapper = pygobject_new (object);
    g_return_if_fail (object_wrapper != NULL);

    /* construct method name for this class closure */
    method_name = g_strconcat ("do_", g_signal_name (hint->signal_id), NULL);

    /* convert dashes to underscores.  For some reason, g_signal_name
     * seems to convert all the underscores in the signal name to
       dashes??? */
    for (tmp = method_name; *tmp != '\0'; tmp++)
        if (*tmp == '-') *tmp = '_';

    method = PyObject_GetAttrString (object_wrapper, method_name);
    g_free (method_name);

    if (!method) {
        PyErr_Clear ();
        Py_DECREF (object_wrapper);
        PyGILState_Release (state);
        return;
    }
    Py_DECREF (object_wrapper);

    /* construct Python tuple for the parameter values; don't copy boxed values
       initially because we'll check after the call to see if a copy is needed. */
    params = PyTuple_New (n_param_values - 1);
    for (i = 1; i < n_param_values; i++) {
        PyObject *item = pyg_value_to_pyobject (&param_values[i], FALSE);

        /* error condition */
        if (!item) {
            Py_DECREF (params);
            PyGILState_Release (state);
            return;
        }
        PyTuple_SetItem (params, i - 1, item);
    }

    ret = PyObject_CallObject (method, params);

    /* Copy boxed values if others ref them, this needs to be done regardless of
       exception status. */
    py_len = PyTuple_Size (params);
    len = (guint)py_len;
    for (i = 0; i < len; i++) {
        PyObject *item = PyTuple_GetItem (params, i);
        if (item != NULL && PyObject_TypeCheck (item, &PyGBoxed_Type)
            && Py_REFCNT (item) != 1) {
            PyGBoxed *boxed_item = (PyGBoxed *)item;
            if (!boxed_item->free_on_dealloc) {
                gpointer boxed_ptr = pyg_boxed_get_ptr (boxed_item);
                pyg_boxed_set_ptr (
                    boxed_item, g_boxed_copy (boxed_item->gtype, boxed_ptr));
                boxed_item->free_on_dealloc = TRUE;
            }
        }
    }

    if (ret == NULL) {
        PyErr_Print ();
        Py_DECREF (method);
        Py_DECREF (params);
        PyGILState_Release (state);
        return;
    }
    Py_DECREF (method);
    Py_DECREF (params);
    if (G_IS_VALUE (return_value)) pyg_value_from_pyobject (return_value, ret);
    Py_DECREF (ret);
    PyGILState_Release (state);
}

static gpointer
init_once_closure (gpointer user_data)
{
    GClosure *closure = g_closure_new_simple (sizeof (GClosure), NULL);
    g_closure_set_marshal (closure, pyg_signal_class_closure_marshal);

    g_closure_ref (closure);
    g_closure_sink (closure);

    return closure;
}

/**
 * pyg_signal_class_closure_get:
 *
 * Returns the GClosure used for the class closure of signals.  When
 * called, it will invoke the method do_signalname (for the signal
 * "signalname").
 *
 * Returns: the closure.
 */
GClosure *
pyg_signal_class_closure_get (void)
{
    static GOnce once_closure = G_ONCE_INIT;

    g_once (&once_closure, init_once_closure, NULL);

    return once_closure.retval;
}

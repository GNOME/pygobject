/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2014 Simon Feltman <sfeltman@gnome.org>
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

#include "pygi-struct-marshal.h"

#include "pygboxed.h"
#include "pygi-boxed.h"
#include "pygi-foreign.h"
#include "pygi-info.h"
#include "pygi-struct.h"
#include "pygi-type.h"
#include "pygi-value.h"
#include "pygpointer.h"

/*
 * _is_union_member - check to see if the py_arg is actually a member of the
 * expected C union
 */
static gboolean
_is_union_member (GIRegisteredTypeInfo *interface_info, PyObject *py_arg)
{
    guint i;
    guint n_fields;
    GIUnionInfo *union_info;
    gboolean is_member = FALSE;

    if (!GI_IS_UNION_INFO (interface_info)) return FALSE;

    union_info = (GIUnionInfo *)interface_info;
    n_fields = gi_union_info_get_n_fields (union_info);

    for (i = 0; i < n_fields; i++) {
        GIFieldInfo *field_info;
        GITypeInfo *field_type_info;

        field_info = gi_union_info_get_field (union_info, i);
        field_type_info = gi_field_info_get_type_info (field_info);

        /* we can only check if the members are interfaces */
        if (gi_type_info_get_tag (field_type_info) == GI_TYPE_TAG_INTERFACE) {
            GIBaseInfo *field_iface_info;
            PyObject *py_type;

            field_iface_info = gi_type_info_get_interface (field_type_info);
            py_type = pygi_type_import_by_gi_info (field_iface_info);

            if (py_type != NULL && PyObject_IsInstance (py_arg, py_type)) {
                is_member = TRUE;
            }

            Py_XDECREF (py_type);
            gi_base_info_unref (field_iface_info);
        }

        gi_base_info_unref ((GIBaseInfo *)field_type_info);
        gi_base_info_unref ((GIBaseInfo *)field_info);

        if (is_member) break;
    }

    return is_member;
}


/*
 * GValue from Python
 */

/* pygi_arg_gvalue_from_py_marshal:
 * py_arg: (in):
 * arg: (out):
 * transfer:
 * copy_reference: TRUE if arg should use the pointer reference held by py_arg
 *                 when it is already holding a GValue vs. copying the value.
 */
gboolean
pygi_arg_gvalue_from_py_marshal (PyObject *py_arg, GIArgument *arg,
                                 GITransfer transfer, gboolean copy_reference)
{
    GValue *value;
    GType object_type;

    object_type =
        pyg_type_from_object_strict ((PyObject *)Py_TYPE (py_arg), FALSE);
    if (object_type == G_TYPE_INVALID) {
        PyErr_SetString (PyExc_RuntimeError,
                         "unable to retrieve object's GType");
        return FALSE;
    }

    /* if already a gvalue, use that, else marshal into gvalue */
    if (object_type == G_TYPE_VALUE) {
        GValue *source_value = pyg_boxed_get (py_arg, GValue);
        if (copy_reference) {
            value = source_value;
        } else {
            value = g_slice_new0 (GValue);
            g_value_init (value, G_VALUE_TYPE (source_value));
            g_value_copy (source_value, value);
        }
    } else {
        value = g_slice_new0 (GValue);
        g_value_init (value, object_type);
        if (pyg_value_from_pyobject_with_error (value, py_arg) < 0) {
            g_slice_free (GValue, value);
            return FALSE;
        }
    }

    arg->v_pointer = value;
    return TRUE;
}

/* pygi_arg_gclosure_from_py_marshal:
 * py_arg: (in):
 * arg: (out):
 */
static gboolean
pygi_arg_gclosure_from_py_marshal (PyObject *py_arg, GIArgument *arg,
                                   GITransfer transfer)
{
    GClosure *closure;
    GType object_gtype = pyg_type_from_object_strict (py_arg, FALSE);

    if (!(PyCallable_Check (py_arg)
          || g_type_is_a (object_gtype, G_TYPE_CLOSURE))) {
        PyErr_Format (PyExc_TypeError, "Must be callable, not %s",
                      Py_TYPE (py_arg)->tp_name);
        return FALSE;
    }

    if (g_type_is_a (object_gtype, G_TYPE_CLOSURE)) {
        closure = (GClosure *)pyg_boxed_get (py_arg, void);
        /* Make sure we own a ref which is held until cleanup. */
        if (closure != NULL) {
            g_closure_ref (closure);
        }
    } else {
        PyObject *functools;
        PyObject *partial = NULL;

        functools = PyImport_ImportModule ("functools");
        if (functools) {
            partial = PyObject_GetAttrString (functools, "partial");
            Py_DECREF (functools);
        }

        if (partial && PyObject_IsInstance (py_arg, partial) > 0
            && PyObject_HasAttrString (py_arg, "__gtk_template__")) {
            PyObject *partial_func;
            PyObject *partial_args;
            PyObject *partial_keywords;
            PyObject *swap_data;

            partial_func = PyObject_GetAttrString (py_arg, "func");
            partial_args = PyObject_GetAttrString (py_arg, "args");
            partial_keywords = PyObject_GetAttrString (py_arg, "keywords");
            swap_data = PyDict_GetItemString (partial_keywords, "swap_data");

            closure = pyg_closure_new (partial_func, partial_args, swap_data);

            Py_DECREF (partial_func);
            Py_DECREF (partial_args);
            Py_DECREF (partial_keywords);
            g_closure_ref (closure);
            g_closure_sink (closure);
        } else {
            closure = pyg_closure_new (py_arg, NULL, NULL);
            g_closure_ref (closure);
            g_closure_sink (closure);
        }

        if (partial) {
            Py_DECREF (partial);
        }
    }

    if (closure == NULL) {
        PyErr_SetString (PyExc_RuntimeError,
                         "PyObject conversion to GClosure failed");
        return FALSE;
    }

    /* Add an additional ref when transfering everything to the callee. */
    if (transfer == GI_TRANSFER_EVERYTHING) {
        g_closure_ref (closure);
    }

    arg->v_pointer = closure;
    return TRUE;
}

/* pygi_arg_struct_from_py_marshal:
 *
 * Dispatcher to various sub marshalers
 */
gboolean
pygi_arg_struct_from_py_marshal (PyObject *py_arg, GIArgument *arg,
                                 const gchar *arg_name,
                                 GIRegisteredTypeInfo *interface_info,
                                 GType g_type, PyObject *py_type,
                                 GITransfer transfer, gboolean copy_reference,
                                 gboolean is_foreign, gboolean is_pointer)
{
    gboolean is_union = FALSE;

    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    /* FIXME: handle this large if statement in the cache
     *        and set the correct marshaller
     */

    if (g_type_is_a (g_type, G_TYPE_CLOSURE)) {
        return pygi_arg_gclosure_from_py_marshal (py_arg, arg, transfer);
    } else if (g_type_is_a (g_type, G_TYPE_VALUE)) {
        return pygi_arg_gvalue_from_py_marshal (py_arg, arg, transfer,
                                                copy_reference);
    } else if (is_foreign) {
        PyObject *success;
        success = pygi_struct_foreign_convert_to_g_argument (
            py_arg, GI_REGISTERED_TYPE_INFO (interface_info), transfer, arg);

        return (Py_IsNone (success));
    } else if (!PyObject_IsInstance (py_arg, py_type)) {
        /* first check to see if this is a member of the expected union */
        is_union = _is_union_member (GI_REGISTERED_TYPE_INFO (interface_info),
                                     py_arg);
        if (!is_union) {
            goto type_error;
        }
    }

    if (g_type_is_a (g_type, G_TYPE_BOXED)) {
        /* Additionally use pyg_type_from_object to pull the stashed __gtype__
         * attribute off of the input argument for type checking. This is needed
         * to work around type discrepancies in cases with aliased (typedef) types.
         * e.g. GtkAllocation, GdkRectangle.
         * See: https://bugzilla.gnomethere are .org/show_bug.cgi?id=707140
         */
        if (is_union || pyg_boxed_check (py_arg, g_type)
            || g_type_is_a (pyg_type_from_object (py_arg), g_type)) {
            arg->v_pointer = pyg_boxed_get (py_arg, void);
            if (transfer == GI_TRANSFER_EVERYTHING) {
                arg->v_pointer = g_boxed_copy (g_type, arg->v_pointer);
            }
        } else {
            goto type_error;
        }

    } else if (g_type_is_a (g_type, G_TYPE_POINTER)
               || g_type_is_a (g_type, G_TYPE_VARIANT)
               || g_type == G_TYPE_NONE) {
        g_warn_if_fail (g_type_is_a (g_type, G_TYPE_VARIANT) || !is_pointer
                        || transfer == GI_TRANSFER_NOTHING);

        if (g_type_is_a (g_type, G_TYPE_VARIANT)
            && pyg_type_from_object (py_arg) != G_TYPE_VARIANT) {
            PyErr_SetString (PyExc_TypeError, "expected GLib.Variant");
            return FALSE;
        }
        arg->v_pointer = pyg_pointer_get (py_arg, void);
        if (transfer == GI_TRANSFER_EVERYTHING) {
            g_variant_ref ((GVariant *)arg->v_pointer);
        }

    } else {
        PyErr_Format (PyExc_NotImplementedError,
                      "structure type '%s' is not supported yet",
                      g_type_name (g_type));
        return FALSE;
    }
    return TRUE;

type_error: {
    gchar *type_name =
        _pygi_gi_base_info_get_fullname (GI_BASE_INFO (interface_info));
    PyObject *module = PyObject_GetAttrString (py_arg, "__module__");

    PyErr_Format (PyExc_TypeError, "argument %s: Expected %s, but got %s%s%s",
                  arg_name ? arg_name : "self", type_name,
                  module ? PyUnicode_AsUTF8 (module) : "", module ? "." : "",
                  Py_TYPE (py_arg)->tp_name);
    if (module) Py_DECREF (module);
    g_free (type_name);
    return FALSE;
}
}

PyObject *
pygi_arg_struct_to_py_marshal (GIArgument *arg,
                               GIRegisteredTypeInfo *interface_info,
                               GType g_type, PyObject *py_type,
                               GITransfer transfer, gboolean is_allocated,
                               gboolean is_foreign)
{
    PyObject *py_obj = NULL;

    if (arg->v_pointer == NULL) {
        Py_RETURN_NONE;
    }

    if (g_type_is_a (g_type, G_TYPE_VALUE)) {
        py_obj = pyg_value_to_pyobject (arg->v_pointer, is_allocated);
    } else if (is_foreign) {
        py_obj = pygi_struct_foreign_convert_from_g_argument (
            interface_info, transfer, arg->v_pointer);
    } else if (g_type_is_a (g_type, G_TYPE_BOXED)) {
        if (py_type) {
            py_obj = pygi_boxed_new (
                (PyTypeObject *)py_type, arg->v_pointer,
                transfer == GI_TRANSFER_EVERYTHING || is_allocated,
                is_allocated
                    ? gi_struct_info_get_size (GI_STRUCT_INFO (interface_info))
                    : 0);
        }
    } else if (g_type_is_a (g_type, G_TYPE_POINTER)) {
        if (py_type == NULL
            || !PyType_IsSubtype ((PyTypeObject *)py_type, &PyGIStruct_Type)) {
            g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
            py_obj = pyg_pointer_new (g_type, arg->v_pointer);
        } else {
            py_obj = pygi_struct_new ((PyTypeObject *)py_type, arg->v_pointer,
                                      transfer == GI_TRANSFER_EVERYTHING);
        }
    } else if (g_type_is_a (g_type, G_TYPE_VARIANT)) {
        /* Note: sink the variant (add a ref) only if we are not transfered ownership.
         * GLib.Variant overrides __del__ which will then call "g_variant_unref" for
         * cleanup in either case. */
        if (py_type) {
            if (transfer == GI_TRANSFER_NOTHING) {
                g_variant_ref_sink (arg->v_pointer);
            }
            py_obj = pygi_struct_new ((PyTypeObject *)py_type, arg->v_pointer,
                                      FALSE);
        }
    } else if (g_type == G_TYPE_NONE) {
        if (py_type) {
            py_obj = pygi_struct_new (
                (PyTypeObject *)py_type, arg->v_pointer,
                transfer == GI_TRANSFER_EVERYTHING || is_allocated);
        }
    } else {
        PyErr_Format (PyExc_NotImplementedError,
                      "structure type '%s' is not supported yet",
                      g_type_name (g_type));
    }

    return py_obj;
}

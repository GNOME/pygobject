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

#include "pygboxed.h"
#include "pygpointer.h"
#include "pygi-boxed.h"
#include "pygi-foreign.h"
#include "pygi-info.h"
#include "pygi-struct.h"
#include "pygi-type.h"
#include "pygi-value.h"
#include "pygi-cache-private.h"

/*
 * _is_union_member - check to see if the py_arg is actually a member of the
 * expected C union
 */
static gboolean
_is_union_member (GIRegisteredTypeInfo *interface_info, PyObject *py_arg)
{
    gint i;
    gint n_fields;
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

static void
unset_and_free_gvalue (gpointer data)
{
    g_value_unset ((GValue *)data);
    g_free (data);
}

static gboolean
pygi_arg_gvalue_from_py_marshal (PyGIInvokeState *state,
                                 PyGICallableCache *callable_cache,
                                 PyGIArgCache *arg_cache, PyObject *py_arg,
                                 GIArgument *arg,
                                 MarshalCleanupData *cleanup_data)
{
    GValue *value;
    GType object_type;

    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    object_type =
        pyg_type_from_object_strict ((PyObject *)Py_TYPE (py_arg), FALSE);

    if (object_type == G_TYPE_INVALID) {
        PyErr_SetString (PyExc_RuntimeError,
                         "unable to retrieve object's GType");
        return FALSE;
    }

    /* if already a gvalue, use that, else marshal into gvalue */
    if (object_type == G_TYPE_VALUE) {
        /* We borrow a reference*/
        value = pyg_boxed_get (py_arg, GValue);
    } else {
        value = g_slice_new0 (GValue);
        g_value_init (value, object_type);
        if (pyg_value_from_pyobject_with_error (value, py_arg) < 0) {
            g_slice_free (GValue, value);
            return FALSE;
        }

        if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
            /* Free everything in cleanup. */
            cleanup_data->data = value;
            cleanup_data->destroy = unset_and_free_gvalue;
        }
    }

    arg->v_pointer = value;

    return TRUE;
}

void
pygi_arg_gvalue_from_py_cleanup (PyGIInvokeState *state,
                                 PyGIArgCache *arg_cache, PyObject *py_arg,
                                 MarshalCleanupData cleanup_data,
                                 gboolean was_processed)
{
    /* Note py_arg can be NULL for hash table which is a bug. */
    if (was_processed && py_arg != NULL && cleanup_data.data) {
        pygi_marshal_cleanup_data_destroy (&cleanup_data);
    }
}

static gboolean
pygi_arg_gclosure_from_py_marshal (PyGIInvokeState *state,
                                   PyGICallableCache *callable_cache,
                                   PyGIArgCache *arg_cache, PyObject *py_arg,
                                   GIArgument *arg,
                                   MarshalCleanupData *cleanup_data)
{
    GClosure *closure;
    GType object_gtype;

    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    object_gtype = pyg_type_from_object_strict (py_arg, FALSE);

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

    arg->v_pointer = closure;

    if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
        /* Free everything in cleanup. */
        cleanup_data->data = closure;
        cleanup_data->destroy = (GDestroyNotify)g_closure_unref;
    } else { /* GI_TRANSFER_EVERYTHING */
        /* No cleanup, everything is given to the callee. */
        cleanup_data->data = NULL;
    }

    return TRUE;
}


static void
arg_gclosure_from_py_cleanup (PyGIInvokeState *state, PyGIArgCache *arg_cache,
                              PyObject *py_arg,
                              MarshalCleanupData cleanup_data,
                              gboolean was_processed)
{
    if (cleanup_data.data != NULL) {
        pygi_marshal_cleanup_data_destroy (&cleanup_data);
    }
}

typedef struct {
    GIBaseInfo *base_info;
    gpointer struct_;
} ForeignReleaseData;

static void
release_foreign (ForeignReleaseData *data)
{
    pygi_struct_foreign_release (data->base_info, data->struct_);
    gi_base_info_unref (data->base_info);
    g_free (data);
}

static gboolean
pygi_arg_foreign_from_py_marshal (PyGIInvokeState *state,
                                  PyGICallableCache *callable_cache,
                                  PyGIArgCache *arg_cache, PyObject *py_arg,
                                  GIArgument *arg,
                                  MarshalCleanupData *cleanup_data)
{
    PyObject *success;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    success = pygi_struct_foreign_convert_to_g_argument (
        py_arg, GI_REGISTERED_TYPE_INFO (iface_cache->interface_info),
        arg_cache->transfer, arg);

    ForeignReleaseData *release_data = g_malloc (sizeof (ForeignReleaseData));
    release_data->base_info = gi_base_info_ref (
        GI_BASE_INFO (((PyGIInterfaceCache *)arg_cache)->interface_info));
    release_data->struct_ = arg->v_pointer;
    cleanup_data->data = release_data;
    cleanup_data->destroy = (GDestroyNotify)release_foreign;

    return Py_IsNone (success);
}

static void
arg_foreign_from_py_cleanup (PyGIInvokeState *state, PyGIArgCache *arg_cache,
                             PyObject *py_arg, MarshalCleanupData cleanup_data,
                             gboolean was_processed)
{
    if (state->failed) {
        pygi_marshal_cleanup_data_destroy (&cleanup_data);
    } else {
        g_free (cleanup_data.data);
    }
}

static void
raise_type_error (PyObject *py_arg, const gchar *arg_name,
                  GIRegisteredTypeInfo *interface_info)
{
    gchar *type_name =
        _pygi_gi_base_info_get_fullname (GI_BASE_INFO (interface_info));
    PyObject *module = PyObject_GetAttrString (py_arg, "__module__");

    PyErr_Format (PyExc_TypeError, "argument %s: Expected %s, but got %s%s%s",
                  arg_name ? arg_name : "self", type_name,
                  module ? PyUnicode_AsUTF8 (module) : "", module ? "." : "",
                  Py_TYPE (py_arg)->tp_name);
    if (module) Py_DECREF (module);
    g_free (type_name);
}

static gboolean
pygi_arg_struct_from_py_marshal (PyGIInvokeState *state,
                                 PyGICallableCache *callable_cache,
                                 PyGIArgCache *arg_cache, PyObject *py_arg,
                                 GIArgument *arg,
                                 MarshalCleanupData *cleanup_data)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    const gchar *arg_name = pygi_arg_cache_get_name (arg_cache);
    GIRegisteredTypeInfo *interface_info =
        GI_REGISTERED_TYPE_INFO (iface_cache->interface_info);
    GType g_type = iface_cache->g_type;
    gboolean is_union = FALSE;

    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!PyObject_IsInstance (py_arg, iface_cache->py_type)) {
        /* first check to see if this is a member of the expected union */
        is_union = _is_union_member (GI_REGISTERED_TYPE_INFO (interface_info),
                                     py_arg);
        if (!is_union) {
            raise_type_error (py_arg, arg_name, interface_info);
            return FALSE;
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
            if (arg_cache->transfer == GI_TRANSFER_EVERYTHING) {
                arg->v_pointer = g_boxed_copy (g_type, arg->v_pointer);
            }
        } else {
            raise_type_error (py_arg, arg_name, interface_info);
            return FALSE;
        }

    } else if (g_type_is_a (g_type, G_TYPE_POINTER)
               || g_type_is_a (g_type, G_TYPE_VARIANT)
               || g_type == G_TYPE_NONE) {
        g_warn_if_fail (g_type_is_a (g_type, G_TYPE_VARIANT)
                        || !arg_cache->is_pointer
                        || arg_cache->transfer == GI_TRANSFER_NOTHING);

        if (g_type_is_a (g_type, G_TYPE_VARIANT)
            && pyg_type_from_object (py_arg) != G_TYPE_VARIANT) {
            PyErr_SetString (PyExc_TypeError, "expected GLib.Variant");
            return FALSE;
        }
        arg->v_pointer = pyg_pointer_get (py_arg, void);
        if (arg_cache->transfer == GI_TRANSFER_EVERYTHING) {
            g_variant_ref ((GVariant *)arg->v_pointer);
        }

    } else {
        PyErr_Format (PyExc_NotImplementedError,
                      "structure type '%s' is not supported yet",
                      g_type_name (g_type));
        return FALSE;
    }

    /* Assume struct marshaling is always a pointer and assign cleanup_data
     * here rather than passing it further down the chain.
     */
    // TODO: fix cleanup
    // cleanup_data->data = arg->v_pointer;

    return TRUE;
}

static PyObject *
pygi_arg_gvalue_to_py_marshal (PyGIInvokeState *state,
                               PyGICallableCache *callable_cache,
                               PyGIArgCache *arg_cache, GIArgument *arg,
                               MarshalCleanupData *cleanup_data)
{
    PyObject *py_obj;

    if (arg->v_pointer == NULL) {
        Py_RETURN_NONE;
    }

    py_obj = pyg_value_to_pyobject (
        arg->v_pointer, pygi_arg_cache_is_caller_allocates (arg_cache));

    // TODO: clean up GValue if we own it

    return py_obj;
}

static PyObject *
pygi_arg_foreign_to_py_marshal (PyGIInvokeState *state,
                                PyGICallableCache *callable_cache,
                                PyGIArgCache *arg_cache, GIArgument *arg,
                                MarshalCleanupData *cleanup_data)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    PyObject *py_obj;

    if (arg->v_pointer == NULL) {
        Py_RETURN_NONE;
    }

    py_obj = pygi_struct_foreign_convert_from_g_argument (
        iface_cache->interface_info, arg_cache->transfer, arg->v_pointer);

    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING) {
        ForeignReleaseData *release_data =
            g_malloc (sizeof (ForeignReleaseData));
        release_data->base_info = gi_base_info_ref (
            GI_BASE_INFO (((PyGIInterfaceCache *)arg_cache)->interface_info));
        release_data->struct_ = arg->v_pointer;
        cleanup_data->data = release_data;
        cleanup_data->destroy = (GDestroyNotify)release_foreign;
    }

    return py_obj;
}

static void
arg_foreign_to_py_cleanup (PyGIInvokeState *state, PyGIArgCache *arg_cache,
                           MarshalCleanupData cleanup_data, gpointer data,
                           gboolean was_processed)
{
    if (!was_processed) {
        pygi_marshal_cleanup_data_destroy (&cleanup_data);
    }
}

static PyObject *
pygi_arg_struct_to_py_marshal (PyGIInvokeState *state,
                               PyGICallableCache *callable_cache,
                               PyGIArgCache *arg_cache, GIArgument *arg,
                               MarshalCleanupData *cleanup_data)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIRegisteredTypeInfo *interface_info = iface_cache->interface_info;
    GType g_type = iface_cache->g_type;
    PyObject *py_type = iface_cache->py_type;
    GITransfer transfer = arg_cache->transfer;
    gboolean is_allocated = pygi_arg_cache_is_caller_allocates (arg_cache);
    PyObject *py_obj = NULL;

    if (arg->v_pointer == NULL) {
        Py_RETURN_NONE;
    }

    if (g_type_is_a (g_type, G_TYPE_BOXED)) {
        if (py_type) {
            py_obj = pygi_boxed_new (
                (PyTypeObject *)py_type, arg->v_pointer,
                transfer == GI_TRANSFER_EVERYTHING || is_allocated,
                is_allocated
                    ? gi_struct_info_get_size (GI_STRUCT_INFO (interface_info))
                    : 0);

            if (transfer == GI_TRANSFER_NOTHING) {
                cleanup_data->data = py_obj;
                cleanup_data->destroy =
                    (GDestroyNotify)pygi_boxed_copy_in_place;
            }
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

static gboolean
arg_type_class_from_py_marshal (PyGIInvokeState *state,
                                PyGICallableCache *callable_cache,
                                PyGIArgCache *arg_cache, PyObject *py_arg,
                                GIArgument *arg,
                                MarshalCleanupData *cleanup_data)
{
    GType gtype = pyg_type_from_object (py_arg);

    if (G_TYPE_IS_CLASSED (gtype)) {
        arg->v_pointer = g_type_class_ref (gtype);

        if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
            cleanup_data->data = arg->v_pointer;
            cleanup_data->destroy = (GDestroyNotify)g_type_class_unref;
        }
        return TRUE;
    } else {
        PyErr_Format (PyExc_TypeError,
                      "Unable to retrieve a GObject type class from \"%s\".",
                      Py_TYPE (py_arg)->tp_name);
        return FALSE;
    }
}

static void
arg_type_class_from_py_cleanup (PyGIInvokeState *state,
                                PyGIArgCache *arg_cache, PyObject *py_arg,
                                MarshalCleanupData cleanup_data,
                                gboolean was_processed)
{
    if (was_processed) {
        pygi_marshal_cleanup_data_destroy (&cleanup_data);
    }
}

static void
arg_boxed_to_py_cleanup (PyGIInvokeState *state, PyGIArgCache *arg_cache,
                         MarshalCleanupData cleanup_data, gpointer data,
                         gboolean was_processed)
{
    pygi_marshal_cleanup_data_destroy (&cleanup_data);
}

static void
arg_struct_from_py_setup (PyGIArgCache *arg_cache,
                          GIRegisteredTypeInfo *iface_info,
                          GITransfer transfer)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (gi_struct_info_is_gtype_struct ((GIStructInfo *)iface_info)) {
        arg_cache->from_py_marshaller = arg_type_class_from_py_marshal;
        arg_cache->from_py_cleanup = arg_type_class_from_py_cleanup;
    } else if (g_type_is_a (iface_cache->g_type, G_TYPE_CLOSURE)) {
        arg_cache->from_py_marshaller = pygi_arg_gclosure_from_py_marshal;
        arg_cache->from_py_cleanup = arg_gclosure_from_py_cleanup;
    } else if (iface_cache->g_type == G_TYPE_VALUE) {
        arg_cache->from_py_marshaller = pygi_arg_gvalue_from_py_marshal;
        arg_cache->from_py_cleanup = pygi_arg_gvalue_from_py_cleanup;
    } else if (iface_cache->is_foreign) {
        arg_cache->from_py_marshaller = pygi_arg_foreign_from_py_marshal;
        arg_cache->from_py_cleanup = arg_foreign_from_py_cleanup;
    } else {
        arg_cache->from_py_marshaller = pygi_arg_struct_from_py_marshal;
    }
}

static void
arg_struct_to_py_setup (PyGIArgCache *arg_cache,
                        GIRegisteredTypeInfo *iface_info, GITransfer transfer)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (iface_cache->g_type == G_TYPE_VALUE) {
        arg_cache->to_py_marshaller = pygi_arg_gvalue_to_py_marshal;
    } else if (iface_cache->is_foreign) {
        arg_cache->to_py_marshaller = pygi_arg_foreign_to_py_marshal;
        arg_cache->to_py_cleanup = arg_foreign_to_py_cleanup;
    } else {
        arg_cache->to_py_marshaller = pygi_arg_struct_to_py_marshal;
        if (iface_cache->py_type
            && g_type_is_a (iface_cache->g_type, G_TYPE_BOXED))
            arg_cache->to_py_cleanup = arg_boxed_to_py_cleanup;
    }
}

PyGIArgCache *
pygi_arg_struct_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                               GITransfer transfer, PyGIDirection direction,
                               GIRegisteredTypeInfo *iface_info)
{
    PyGIArgCache *cache = NULL;
    PyGIInterfaceCache *iface_cache;

    cache = pygi_arg_interface_new_from_info (type_info, arg_info, transfer,
                                              direction, iface_info);
    if (cache == NULL) return NULL;

    iface_cache = (PyGIInterfaceCache *)cache;
    iface_cache->is_foreign =
        (GI_IS_STRUCT_INFO ((GIBaseInfo *)iface_info))
        && (gi_struct_info_is_foreign ((GIStructInfo *)iface_info));

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        arg_struct_from_py_setup (cache, iface_info, transfer);
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        arg_struct_to_py_setup (cache, iface_info, transfer);
    }

    return cache;
}

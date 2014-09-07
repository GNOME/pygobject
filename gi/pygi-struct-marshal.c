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

#include <glib.h>
#include <Python.h>
#include <pyglib-python-compat.h>

#include "pygi-struct-marshal.h"
#include "pygi-private.h"
#include "pygi-value.h"

/*
 * _is_union_member - check to see if the py_arg is actually a member of the
 * expected C union
 */
static gboolean
_is_union_member (GIInterfaceInfo *interface_info, PyObject *py_arg) {
    gint i;
    gint n_fields;
    GIUnionInfo *union_info;
    GIInfoType info_type;
    gboolean is_member = FALSE;

    info_type = g_base_info_get_type (interface_info);

    if (info_type != GI_INFO_TYPE_UNION)
        return FALSE;

    union_info = (GIUnionInfo *) interface_info;
    n_fields = g_union_info_get_n_fields (union_info);

    for (i = 0; i < n_fields; i++) {
        GIFieldInfo *field_info;
        GITypeInfo *field_type_info;

        field_info = g_union_info_get_field (union_info, i);
        field_type_info = g_field_info_get_type (field_info);

        /* we can only check if the members are interfaces */
        if (g_type_info_get_tag (field_type_info) == GI_TYPE_TAG_INTERFACE) {
            GIInterfaceInfo *field_iface_info;
            PyObject *py_type;

            field_iface_info = g_type_info_get_interface (field_type_info);
            py_type = _pygi_type_import_by_gi_info ((GIBaseInfo *) field_iface_info);

            if (py_type != NULL && PyObject_IsInstance (py_arg, py_type)) {
                is_member = TRUE;
            }

            Py_XDECREF (py_type);
            g_base_info_unref ( ( GIBaseInfo *) field_iface_info);
        }

        g_base_info_unref ( ( GIBaseInfo *) field_type_info);
        g_base_info_unref ( ( GIBaseInfo *) field_info);

        if (is_member)
            break;
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
pygi_arg_gvalue_from_py_marshal (PyObject *py_arg,
                                 GIArgument *arg,
                                 GITransfer transfer,
                                 gboolean copy_reference) {
    GValue *value;
    GType object_type;

    object_type = pyg_type_from_object_strict ( (PyObject *) py_arg->ob_type, FALSE);
    if (object_type == G_TYPE_INVALID) {
        PyErr_SetString (PyExc_RuntimeError, "unable to retrieve object's GType");
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
        if (pyg_value_from_pyobject (value, py_arg) < 0) {
            g_slice_free (GValue, value);
            PyErr_SetString (PyExc_RuntimeError, "PyObject conversion to GValue failed");
            return FALSE;
        }
    }

    arg->v_pointer = value;
    return TRUE;
}

void
pygi_arg_gvalue_from_py_cleanup (PyGIInvokeState *state,
                                 PyGIArgCache    *arg_cache,
                                 PyObject        *py_arg,
                                 gpointer         data,
                                 gboolean         was_processed)
{
    /* Note py_arg can be NULL for hash table which is a bug. */
    if (was_processed && py_arg != NULL) {
        GType py_object_type =
            pyg_type_from_object_strict ( (PyObject *) py_arg->ob_type, FALSE);

        /* When a GValue was not passed, it means the marshalers created a new
         * one to pass in, clean this up.
         */
        if (py_object_type != G_TYPE_VALUE) {
            g_value_unset ((GValue *) data);
            g_slice_free (GValue, data);
        }
    }
}

/* pygi_arg_gclosure_from_py_marshal:
 * py_arg: (in):
 * arg: (out):
 */
static gboolean
pygi_arg_gclosure_from_py_marshal (PyObject   *py_arg,
                                   GIArgument *arg,
                                   GITransfer  transfer)
{
    GClosure *closure;
    GType object_gtype = pyg_type_from_object_strict (py_arg, FALSE);

    if ( !(PyCallable_Check(py_arg) ||
           g_type_is_a (object_gtype, G_TYPE_CLOSURE))) {
        PyErr_Format (PyExc_TypeError, "Must be callable, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    if (g_type_is_a (object_gtype, G_TYPE_CLOSURE)) {
        closure = (GClosure *)pyg_boxed_get (py_arg, void);
        /* Make sure we own a ref which is held until cleanup. */
        if (closure != NULL) {
            g_closure_ref (closure);
        }
    } else {
        closure = pyg_closure_new (py_arg, NULL, NULL);
        g_closure_ref (closure);
        g_closure_sink (closure);
    }

    if (closure == NULL) {
        PyErr_SetString (PyExc_RuntimeError, "PyObject conversion to GClosure failed");
        return FALSE;
    }

    /* Add an additional ref when transfering everything to the callee. */
    if (transfer == GI_TRANSFER_EVERYTHING) {
        g_closure_ref (closure);
    }

    arg->v_pointer = closure;
    return TRUE;
}

static void
arg_gclosure_from_py_cleanup (PyGIInvokeState *state,
                              PyGIArgCache    *arg_cache,
                              PyObject        *py_arg,
                              gpointer         cleanup_data,
                              gboolean         was_processed)
{
    if (cleanup_data != NULL) {
        g_closure_unref (cleanup_data);
    }
}

/* pygi_arg_struct_from_py_marshal:
 *
 * Dispatcher to various sub marshalers
 */
gboolean
pygi_arg_struct_from_py_marshal (PyObject *py_arg,
                                 GIArgument *arg,
                                 const gchar *arg_name,
                                 GIBaseInfo *interface_info,
                                 GType g_type,
                                 PyObject *py_type,
                                 GITransfer transfer,
                                 gboolean copy_reference,
                                 gboolean is_foreign,
                                 gboolean is_pointer)
{
    gboolean is_union = FALSE;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    /* FIXME: handle this large if statement in the cache
     *        and set the correct marshaller
     */

    if (g_type_is_a (g_type, G_TYPE_CLOSURE)) {
        return pygi_arg_gclosure_from_py_marshal (py_arg, arg, transfer);
    } else if (g_type_is_a (g_type, G_TYPE_VALUE)) {
        return pygi_arg_gvalue_from_py_marshal(py_arg,
                                               arg,
                                               transfer,
                                               copy_reference);
    } else if (is_foreign) {
        PyObject *success;
        success = pygi_struct_foreign_convert_to_g_argument (py_arg,
                                                             interface_info,
                                                             transfer,
                                                             arg);

        return (success == Py_None);
    } else if (!PyObject_IsInstance (py_arg, py_type)) {
        /* first check to see if this is a member of the expected union */
        is_union = _is_union_member (interface_info, py_arg);
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
        if (is_union || pyg_boxed_check (py_arg, g_type) ||
                g_type_is_a (pyg_type_from_object (py_arg), g_type)) {
            arg->v_pointer = pyg_boxed_get (py_arg, void);
            if (transfer == GI_TRANSFER_EVERYTHING) {
                arg->v_pointer = g_boxed_copy (g_type, arg->v_pointer);
            }
        } else {
            goto type_error;
        }

    } else if (g_type_is_a (g_type, G_TYPE_POINTER) ||
               g_type_is_a (g_type, G_TYPE_VARIANT) ||
               g_type  == G_TYPE_NONE) {
        g_warn_if_fail (g_type_is_a (g_type, G_TYPE_VARIANT) || !is_pointer || transfer == GI_TRANSFER_NOTHING);

        if (g_type_is_a (g_type, G_TYPE_VARIANT) &&
                pyg_type_from_object (py_arg) != G_TYPE_VARIANT) {
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
                      g_type_name(g_type));
        return FALSE;
    }
    return TRUE;

type_error:
    {
        gchar *type_name = _pygi_g_base_info_get_fullname (interface_info);
        PyObject *module = PyObject_GetAttrString(py_arg, "__module__");

        PyErr_Format (PyExc_TypeError, "argument %s: Expected %s, but got %s%s%s",
                      arg_name ? arg_name : "self",
                      type_name,
                      module ? PYGLIB_PyUnicode_AsString(module) : "",
                      module ? "." : "",
                      py_arg->ob_type->tp_name);
        if (module)
            Py_DECREF (module);
        g_free (type_name);
        return FALSE;
    }
}

static gboolean
arg_struct_from_py_marshal_adapter (PyGIInvokeState   *state,
                                    PyGICallableCache *callable_cache,
                                    PyGIArgCache      *arg_cache,
                                    PyObject          *py_arg,
                                    GIArgument        *arg,
                                    gpointer          *cleanup_data)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    gboolean res =  pygi_arg_struct_from_py_marshal (py_arg,
                                                     arg,
                                                     arg_cache->arg_name,
                                                     iface_cache->interface_info,
                                                     iface_cache->g_type,
                                                     iface_cache->py_type,
                                                     arg_cache->transfer,
                                                     TRUE, /*copy_reference*/
                                                     iface_cache->is_foreign,
                                                     arg_cache->is_pointer);

    /* Assume struct marshaling is always a pointer and assign cleanup_data
     * here rather than passing it further down the chain.
     */
    *cleanup_data = arg->v_pointer;
    return res;
}

static void
arg_foreign_from_py_cleanup (PyGIInvokeState *state,
                             PyGIArgCache    *arg_cache,
                             PyObject        *py_arg,
                             gpointer         data,
                             gboolean         was_processed)
{
    if (state->failed && was_processed) {
        pygi_struct_foreign_release (
            ( (PyGIInterfaceCache *)arg_cache)->interface_info,
            data);
    }
}


PyObject *
pygi_arg_struct_to_py_marshal (GIArgument *arg,
                               GIInterfaceInfo *interface_info,
                               GType g_type,
                               PyObject *py_type,
                               GITransfer transfer,
                               gboolean is_allocated,
                               gboolean is_foreign)
{
    PyObject *py_obj = NULL;

    if (arg->v_pointer == NULL) {
        Py_RETURN_NONE;
    }

    if (g_type_is_a (g_type, G_TYPE_VALUE)) {
        py_obj = pyg_value_as_pyobject (arg->v_pointer, FALSE);
    } else if (is_foreign) {
        py_obj = pygi_struct_foreign_convert_from_g_argument (interface_info,
                                                              transfer,
                                                              arg->v_pointer);
    } else if (g_type_is_a (g_type, G_TYPE_BOXED)) {
        if (py_type) {
            /* Force a boxed copy if we are not transfered ownership and the
             * memory is not caller allocated. */
            py_obj = _pygi_boxed_new ((PyTypeObject *) py_type,
                                      arg->v_pointer,
                                      transfer == GI_TRANSFER_NOTHING && !is_allocated,
                                      is_allocated ?
                                              g_struct_info_get_size(interface_info) : 0);
        }
    } else if (g_type_is_a (g_type, G_TYPE_POINTER)) {
        if (py_type == NULL ||
                !PyType_IsSubtype ((PyTypeObject *) py_type, &PyGIStruct_Type)) {
            g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
            py_obj = pyg_pointer_new (g_type, arg->v_pointer);
        } else {
            py_obj = _pygi_struct_new ( (PyTypeObject *) py_type,
                                       arg->v_pointer,
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
            py_obj = _pygi_struct_new ((PyTypeObject *) py_type,
                                       arg->v_pointer,
                                       FALSE);
        }
    } else if (g_type == G_TYPE_NONE) {
        if (py_type) {
            py_obj = _pygi_struct_new ((PyTypeObject *) py_type,
                                       arg->v_pointer,
                                       transfer == GI_TRANSFER_EVERYTHING || is_allocated);
        }
    } else {
        PyErr_Format (PyExc_NotImplementedError,
                      "structure type '%s' is not supported yet",
                      g_type_name (g_type));
    }

    return py_obj;
}

static PyObject *
arg_struct_to_py_marshal_adapter (PyGIInvokeState   *state,
                                  PyGICallableCache *callable_cache,
                                  PyGIArgCache      *arg_cache,
                                  GIArgument        *arg)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    return pygi_arg_struct_to_py_marshal (arg,
                                          iface_cache->interface_info,
                                          iface_cache->g_type,
                                          iface_cache->py_type,
                                          arg_cache->transfer,
                                          arg_cache->is_caller_allocates,
                                          iface_cache->is_foreign);
}

static PyObject *
arg_boxed_to_py_marshal_pass_by_ref (PyGIInvokeState   *state,
                                     PyGICallableCache *callable_cache,
                                     PyGIArgCache      *arg_cache,
                                     GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (arg->v_pointer == NULL) {
        Py_RETURN_NONE;
    }

    if (g_type_is_a (iface_cache->g_type, G_TYPE_BOXED)) {
        if (iface_cache->py_type) {
            py_obj = _pygi_boxed_new ((PyTypeObject *) iface_cache->py_type,
                                      arg->v_pointer,
                                      FALSE, /* copy_boxed */
                                      0);    /* slice_alloc */
            ((PyGBoxed *)py_obj)->free_on_dealloc = FALSE;
        }
    } else {
        PyErr_Format (PyExc_NotImplementedError,
                      "expected boxed type but got %s",
                      g_type_name (iface_cache->g_type));
    }

    return py_obj;
}

static void
arg_foreign_to_py_cleanup (PyGIInvokeState *state,
                           PyGIArgCache    *arg_cache,
                           PyObject        *dummy,
                           gpointer         data,
                           gboolean         was_processed)
{
    if (!was_processed && arg_cache->transfer == GI_TRANSFER_EVERYTHING) {
        pygi_struct_foreign_release (
            ( (PyGIInterfaceCache *)arg_cache)->interface_info,
            data);
    }
}

static gboolean
arg_type_class_from_py_marshal (PyGIInvokeState   *state,
                                PyGICallableCache *callable_cache,
                                PyGIArgCache      *arg_cache,
                                PyObject          *py_arg,
                                GIArgument        *arg,
                                gpointer          *cleanup_data)
{
    GType gtype = pyg_type_from_object (py_arg);

    if (G_TYPE_IS_CLASSED (gtype)) {
        arg->v_pointer = g_type_class_ref (gtype);
        *cleanup_data = arg->v_pointer;
        return TRUE;
    } else {
        PyErr_Format (PyExc_TypeError,
                      "Unable to retrieve a GObject type class from \"%s\".",
                      Py_TYPE(py_arg)->tp_name);
        return FALSE;
    }
}

static void
arg_type_class_from_py_cleanup (PyGIInvokeState *state,
                                PyGIArgCache    *arg_cache,
                                PyObject        *py_arg,
                                gpointer         data,
                                gboolean         was_processed)
{
    if (was_processed) {
        g_type_class_unref (data);
    }
}

static void
arg_struct_from_py_setup (PyGIArgCache     *arg_cache,
                          GIInterfaceInfo  *iface_info,
                          GITransfer        transfer)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (g_struct_info_is_gtype_struct ((GIStructInfo*)iface_info)) {
        arg_cache->from_py_marshaller = arg_type_class_from_py_marshal;
        /* Since we always add a ref in the marshalling, only unref the
         * GTypeClass when we don't transfer ownership. */
        if (transfer == GI_TRANSFER_NOTHING) {
            arg_cache->from_py_cleanup = arg_type_class_from_py_cleanup;
        }

    } else {
        arg_cache->from_py_marshaller = arg_struct_from_py_marshal_adapter;

        if (g_type_is_a (iface_cache->g_type, G_TYPE_CLOSURE)) {
            arg_cache->from_py_cleanup = arg_gclosure_from_py_cleanup;

        } else if (iface_cache->g_type == G_TYPE_VALUE) {
            arg_cache->from_py_cleanup = pygi_arg_gvalue_from_py_cleanup;

        } else if (iface_cache->is_foreign) {
            arg_cache->from_py_cleanup = arg_foreign_from_py_cleanup;
        }
    }
}

static void
arg_struct_to_py_setup (PyGIArgCache     *arg_cache,
                        GIInterfaceInfo  *iface_info,
                        GITransfer        transfer,
                        GIArgInfo        *arg_info)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    /* HACK to force GtkTreeModel:iter_next() and iter_previous() vfunc implementations
     * to receive their Gtk.TreeIter argument as pass-by-reference. We create a new
     * PyGIBoxed wrapper which does not copy the memory and also does not free it.
     * This is needed to hack the noted vfunc implementations so they can continue
     * working with bug https://bugzilla.gnome.org/show_bug.cgi?id=722899
     * being fixed. This hack should be removed once GTK+ has fixed bug
     * https://bugzilla.gnome.org/show_bug.cgi?id=734465
     * and we've moved to a new major version.
     */
    if (arg_info && g_strcmp0 (iface_cache->type_name, "Gtk.TreeIter") == 0) {

        /* GICallbackInfo */
        GIBaseInfo *info = g_base_info_get_container (arg_info);
        if (info && g_base_info_get_type (info) == GI_INFO_TYPE_CALLBACK &&
                (g_strcmp0 (g_base_info_get_name (info), "iter_next") == 0 ||
                 g_strcmp0 (g_base_info_get_name (info), "iter_previous") == 0)) {

            /* GITypeInfo */
            info = g_base_info_get_container (info);
            if (info && g_base_info_get_type (info) == GI_INFO_TYPE_TYPE &&
                    g_type_info_get_tag ((GITypeInfo *)info) == GI_TYPE_TAG_INTERFACE) {

                /* GIFieldInfo */
                info = g_base_info_get_container (info);
                if (info && g_base_info_get_type (info) == GI_INFO_TYPE_FIELD) {

                    /* GIStructInfo */
                    info = g_base_info_get_container (info);
                    if (info && g_base_info_get_type (info) == GI_INFO_TYPE_STRUCT &&
                            g_strcmp0 (g_base_info_get_name (info), "TreeModelIface") == 0) {
                        arg_cache->to_py_marshaller = arg_boxed_to_py_marshal_pass_by_ref;
                    }
                }
            }
        }
    }

    if (arg_cache->to_py_marshaller == NULL) {
        arg_cache->to_py_marshaller = arg_struct_to_py_marshal_adapter;
    }

    if (iface_cache->is_foreign)
        arg_cache->to_py_cleanup = arg_foreign_to_py_cleanup;
}

PyGIArgCache *
pygi_arg_struct_new_from_info (GITypeInfo      *type_info,
                               GIArgInfo       *arg_info,
                               GITransfer       transfer,
                               PyGIDirection    direction,
                               GIInterfaceInfo *iface_info)
{
    PyGIArgCache *cache = NULL;
    PyGIInterfaceCache *iface_cache;

    cache = pygi_arg_interface_new_from_info (type_info,
                                              arg_info,
                                              transfer,
                                              direction,
                                              iface_info);
    if (cache == NULL)
        return NULL;

    iface_cache = (PyGIInterfaceCache *)cache;
    iface_cache->is_foreign = (g_base_info_get_type ((GIBaseInfo *) iface_info) == GI_INFO_TYPE_STRUCT) &&
                              (g_struct_info_is_foreign ((GIStructInfo*) iface_info));

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        arg_struct_from_py_setup (cache, iface_info, transfer);
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        arg_struct_to_py_setup (cache, iface_info, transfer, arg_info);
    }

    return cache;
}

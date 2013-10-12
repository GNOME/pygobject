/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>,  Red Hat, Inc.
 *
 *   pygi-marshal-from-py.c: Functions to convert PyObjects to C types.
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

#include <string.h>
#include <time.h>
#include <pygobject.h>
#include <pyglib-python-compat.h>

#include "pygi-cache.h"
#include "pygi-marshal-cleanup.h"
#include "pygi-marshal-from-py.h"

#ifdef _WIN32
#ifdef _MSC_VER
#include <math.h>

#ifndef NAN
static const unsigned long __nan[2] = {0xffffffff, 0x7fffffff};
#define NAN (*(const float *) __nan)
#endif

#ifndef INFINITY
#define INFINITY HUGE_VAL
#endif

#endif
#endif

static gboolean
gi_argument_from_c_long (GIArgument *arg_out,
                         long        c_long_in,
                         GITypeTag   type_tag)
{
    switch (type_tag) {
      case GI_TYPE_TAG_INT8:
          arg_out->v_int8 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_UINT8:
          arg_out->v_uint8 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_INT16:
          arg_out->v_int16 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_UINT16:
          arg_out->v_uint16 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_INT32:
          arg_out->v_int32 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_UINT32:
          arg_out->v_uint32 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_INT64:
          arg_out->v_int64 = c_long_in;
          return TRUE;
      case GI_TYPE_TAG_UINT64:
          arg_out->v_uint64 = c_long_in;
          return TRUE;
      default:
          PyErr_Format (PyExc_TypeError,
                        "Unable to marshal C long %ld to %s",
                        c_long_in,
                        g_type_tag_to_string (type_tag));
          return FALSE;
    }
}

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

gboolean
_pygi_marshal_from_py_gerror (PyGIInvokeState   *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache      *arg_cache,
                              PyObject          *py_arg,
                              GIArgument        *arg,
                              gpointer          *cleanup_data)
{
    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for GErrors is not implemented");
    return FALSE;
}

/* _pygi_destroy_notify_dummy:
 *
 * Dummy method used in the occasion when a method has a GDestroyNotify
 * argument without user data.
 */
static void
_pygi_destroy_notify_dummy (gpointer data) {
}

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

/* _pygi_destroy_notify_create:
 *
 * Method used in the occasion when a method has a GDestroyNotify
 * argument with user data.
 */
static PyGICClosure*
_pygi_destroy_notify_create (void)
{
    if (!global_destroy_notify) {

        PyGICClosure *destroy_notify = g_slice_new0 (PyGICClosure);
        GIBaseInfo* glib_destroy_notify;

        g_assert (destroy_notify);

        glib_destroy_notify = g_irepository_find_by_name (NULL, "GLib", "DestroyNotify");
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
_pygi_marshal_from_py_interface_callback (PyGIInvokeState   *state,
                                          PyGICallableCache *callable_cache,
                                          PyGIArgCache      *arg_cache,
                                          PyObject          *py_arg,
                                          GIArgument        *arg,
                                          gpointer          *cleanup_data)
{
    GICallableInfo *callable_info;
    PyGICClosure *closure;
    PyGIArgCache *user_data_cache = NULL;
    PyGIArgCache *destroy_cache = NULL;
    PyGICallbackCache *callback_cache;
    PyObject *py_user_data = NULL;

    callback_cache = (PyGICallbackCache *)arg_cache;

    if (callback_cache->user_data_index > 0) {
        user_data_cache = _pygi_callable_cache_get_arg (callable_cache, callback_cache->user_data_index);
        if (user_data_cache->py_arg_index < state->n_py_in_args) {
            /* py_user_data is a borrowed reference. */
            py_user_data = PyTuple_GetItem (state->py_in_args, user_data_cache->py_arg_index);
            if (!py_user_data)
                return FALSE;
            /* NULL out user_data if it was not supplied and the default arg placeholder
             * was used instead.
             */
            if (py_user_data == _PyGIDefaultArgPlaceholder) {
                py_user_data = NULL;
            } else if (callable_cache->user_data_varargs_index < 0) {
                /* For non-variable length user data, place the user data in a
                 * single item tuple which is concatenated to the callbacks arguments.
                 * This allows callback input arg marshaling to always expect a
                 * tuple for user data. Note the
                 */
                py_user_data = Py_BuildValue("(O)", py_user_data, NULL);
            } else {
                /* increment the ref borrowed from PyTuple_GetItem above */
                Py_INCREF (py_user_data);
            }
        }
    }

    if (py_arg == Py_None) {
        return TRUE;
    }

    if (!PyCallable_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError,
                      "Callback needs to be a function or method not %s",
                      py_arg->ob_type->tp_name);

        return FALSE;
    }

    callable_info = (GICallableInfo *)callback_cache->interface_info;

    closure = _pygi_make_native_closure (callable_info, callback_cache->scope, py_arg, py_user_data);
    arg->v_pointer = closure->closure;

    /* always decref the user data as _pygi_make_native_closure adds its own ref */
    Py_XDECREF (py_user_data);

    /* The PyGICClosure instance is used as user data passed into the C function.
     * The return trip to python will marshal this back and pull the python user data out.
     */
    if (user_data_cache != NULL) {
        state->in_args[user_data_cache->c_arg_index].v_pointer = closure;
    }

    /* Setup a GDestroyNotify callback if this method supports it along with
     * a user data field. The user data field is a requirement in order
     * free resources and ref counts associated with this arguments closure.
     * In case a user data field is not available, show a warning giving
     * explicit information and setup a dummy notification to avoid a crash
     * later on in _pygi_destroy_notify_callback_closure.
     */
    if (callback_cache->destroy_notify_index > 0) {
        destroy_cache = _pygi_callable_cache_get_arg (callable_cache, callback_cache->destroy_notify_index);
    }

    if (destroy_cache) {
        if (user_data_cache != NULL) {
            PyGICClosure *destroy_notify = _pygi_destroy_notify_create ();
            state->in_args[destroy_cache->c_arg_index].v_pointer = destroy_notify->closure;
        } else {
            gchar *msg = g_strdup_printf("Callables passed to %s will leak references because "
                                         "the method does not support a user_data argument. "
                                         "See: https://bugzilla.gnome.org/show_bug.cgi?id=685598",
                                         callable_cache->name);
            if (PyErr_WarnEx(PyExc_RuntimeWarning, msg, 2)) {
                g_free(msg);
                _pygi_invoke_closure_free(closure);
                return FALSE;
            }
            g_free(msg);
            state->in_args[destroy_cache->c_arg_index].v_pointer = _pygi_destroy_notify_dummy;
        }
    }

    /* Use the PyGIClosure as data passed to cleanup for GI_SCOPE_TYPE_CALL. */
    *cleanup_data = closure;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_interface_enum (PyGIInvokeState   *state,
                                      PyGICallableCache *callable_cache,
                                      PyGIArgCache      *arg_cache,
                                      PyObject          *py_arg,
                                      GIArgument        *arg,
                                      gpointer          *cleanup_data)
{
    PyObject *py_long;
    long c_long;
    gint is_instance;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIBaseInfo *interface = NULL;

    is_instance = PyObject_IsInstance (py_arg, iface_cache->py_type);

    py_long = PYGLIB_PyNumber_Long (py_arg);
    if (py_long == NULL) {
        PyErr_Clear();
        goto err;
    }

    c_long = PYGLIB_PyLong_AsLong (py_long);
    Py_DECREF (py_long);

    /* Write c_long into arg */
    interface = g_type_info_get_interface (arg_cache->type_info);
    assert(g_base_info_get_type (interface) == GI_INFO_TYPE_ENUM);
    if (!gi_argument_from_c_long(arg,
                                 c_long,
                                 g_enum_info_get_storage_type ((GIEnumInfo *)interface))) {
          g_assert_not_reached();
          g_base_info_unref (interface);
          return FALSE;
    }

    /* If this is not an instance of the Enum type that we want
     * we need to check if the value is equivilant to one of the
     * Enum's memebers */
    if (!is_instance) {
        int i;
        gboolean is_found = FALSE;

        for (i = 0; i < g_enum_info_get_n_values (iface_cache->interface_info); i++) {
            GIValueInfo *value_info =
                g_enum_info_get_value (iface_cache->interface_info, i);
            glong enum_value = g_value_info_get_value (value_info);
            g_base_info_unref ( (GIBaseInfo *)value_info);
            if (c_long == enum_value) {
                is_found = TRUE;
                break;
            }
        }

        if (!is_found)
            goto err;
    }

    g_base_info_unref (interface);
    return TRUE;

err:
    if (interface)
        g_base_info_unref (interface);
    PyErr_Format (PyExc_TypeError, "Expected a %s, but got %s",
                  iface_cache->type_name, py_arg->ob_type->tp_name);
    return FALSE;
}

gboolean
_pygi_marshal_from_py_interface_flags (PyGIInvokeState   *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg,
                                       gpointer          *cleanup_data)
{
    PyObject *py_long;
    long c_long;
    gint is_instance;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIBaseInfo *interface;

    is_instance = PyObject_IsInstance (py_arg, iface_cache->py_type);

    py_long = PYGLIB_PyNumber_Long (py_arg);
    if (py_long == NULL) {
        PyErr_Clear ();
        goto err;
    }

    c_long = PYGLIB_PyLong_AsLong (py_long);
    Py_DECREF (py_long);

    /* only 0 or argument of type Flag is allowed */
    if (!is_instance && c_long != 0)
        goto err;

    /* Write c_long into arg */
    interface = g_type_info_get_interface (arg_cache->type_info);
    g_assert (g_base_info_get_type (interface) == GI_INFO_TYPE_FLAGS);
    if (!gi_argument_from_c_long(arg, c_long,
                                 g_enum_info_get_storage_type ((GIEnumInfo *)interface))) {
        g_base_info_unref (interface);
        return FALSE;
    }

    g_base_info_unref (interface);
    return TRUE;

err:
    PyErr_Format (PyExc_TypeError, "Expected a %s, but got %s",
                  iface_cache->type_name, py_arg->ob_type->tp_name);
    return FALSE;

}

gboolean
_pygi_marshal_from_py_interface_struct_cache_adapter (PyGIInvokeState   *state,
                                                      PyGICallableCache *callable_cache,
                                                      PyGIArgCache      *arg_cache,
                                                      PyObject          *py_arg,
                                                      GIArgument        *arg,
                                                      gpointer          *cleanup_data)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    gboolean res =  _pygi_marshal_from_py_interface_struct (py_arg,
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

gboolean
_pygi_marshal_from_py_interface_boxed (PyGIInvokeState   *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg,
                                       gpointer          *cleanup_data)
{
    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for this type is not implemented yet");
    return FALSE;
}

gboolean
_pygi_marshal_from_py_interface_object (PyGIInvokeState   *state,
                                        PyGICallableCache *callable_cache,
                                        PyGIArgCache      *arg_cache,
                                        PyObject          *py_arg,
                                        GIArgument        *arg,
                                        gpointer          *cleanup_data)
{
    gboolean res = FALSE;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!PyObject_IsInstance (py_arg, ( (PyGIInterfaceCache *)arg_cache)->py_type)) {
        PyObject *module = PyObject_GetAttrString(py_arg, "__module__");

        PyErr_Format (PyExc_TypeError, "argument %s: Expected %s, but got %s%s%s",
                      arg_cache->arg_name ? arg_cache->arg_name : "self",
                      ( (PyGIInterfaceCache *)arg_cache)->type_name,
                      module ? PYGLIB_PyUnicode_AsString(module) : "",
                      module ? "." : "",
                      py_arg->ob_type->tp_name);
        if (module)
            Py_DECREF (module);
        return FALSE;
    }

    res = _pygi_marshal_from_py_gobject (py_arg, arg, arg_cache->transfer);
    *cleanup_data = arg->v_pointer;
    return res;
}

gboolean
_pygi_marshal_from_py_interface_union (PyGIInvokeState   *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg,
                                       gpointer          *cleanup_data)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Marshalling for this type is not implemented yet");
    return FALSE;
}

/* _pygi_marshal_from_py_gobject:
 * py_arg: (in):
 * arg: (out):
 */
gboolean
_pygi_marshal_from_py_gobject (PyObject *py_arg, /*in*/
                               GIArgument *arg,  /*out*/
                               GITransfer transfer) {
    GObject *gobj;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!pygobject_check (py_arg, &PyGObject_Type)) {
        PyObject *repr = PyObject_Repr (py_arg);
        PyErr_Format(PyExc_TypeError, "expected GObject but got %s",
                     PYGLIB_PyUnicode_AsString (repr));
        Py_DECREF (repr);
        return FALSE;
    }

    gobj = pygobject_get (py_arg);
    if (transfer == GI_TRANSFER_EVERYTHING) {
        /* For transfer everything, add a new ref that the callee will take ownership of.
         * Pythons existing ref to the GObject will be managed with the PyGObject wrapper.
         */
        g_object_ref (gobj);
    }

    arg->v_pointer = gobj;
    return TRUE;
}

/* _pygi_marshal_from_py_gobject_out_arg:
 * py_arg: (in):
 * arg: (out):
 *
 * A specialization for marshaling Python GObjects used for out/return values
 * from a Python implemented vfuncs, signals, or an assignment to a GObject property.
 */
gboolean
_pygi_marshal_from_py_gobject_out_arg (PyObject *py_arg, /*in*/
                                       GIArgument *arg,  /*out*/
                                       GITransfer transfer) {
    GObject *gobj;
    if (!_pygi_marshal_from_py_gobject (py_arg, arg, transfer)) {
        return FALSE;
    }

    /* HACK: At this point the basic marshaling of the GObject was successful
     * but we add some special case hacks for vfunc returns due to buggy APIs:
     * https://bugzilla.gnome.org/show_bug.cgi?id=693393
     */
    gobj = arg->v_pointer;
    if (py_arg->ob_refcnt == 1 && gobj->ref_count == 1) {
        /* If both object ref counts are only 1 at this point (the reference held
         * in a return tuple), we assume the GObject will be free'd before reaching
         * its target and become invalid. So instead of getting invalid object errors
         * we add a new GObject ref.
         */
        g_object_ref (gobj);

        if (((PyGObject *)py_arg)->private_flags.flags & PYGOBJECT_GOBJECT_WAS_FLOATING) {
            /*
             * We want to re-float instances that were floating and the Python
             * wrapper assumed ownership. With the additional caveat that there
             * are not any strong references beyond the return tuple.
             */
            g_object_force_floating (gobj);

        } else {
            PyObject *repr = PyObject_Repr (py_arg);
            gchar *msg = g_strdup_printf ("Expecting to marshal a borrowed reference for %s, "
                                          "but nothing in Python is holding a reference to this object. "
                                          "See: https://bugzilla.gnome.org/show_bug.cgi?id=687522",
                                          PYGLIB_PyUnicode_AsString(repr));
            Py_DECREF (repr);
            if (PyErr_WarnEx (PyExc_RuntimeWarning, msg, 2)) {
                g_free (msg);
                return FALSE;
            }
            g_free (msg);
        }
    }

    return TRUE;
}

/* _pygi_marshal_from_py_gvalue:
 * py_arg: (in):
 * arg: (out):
 * transfer:
 * copy_reference: TRUE if arg should use the pointer reference held by py_arg
 *                 when it is already holding a GValue vs. copying the value.
 */
gboolean
_pygi_marshal_from_py_gvalue (PyObject *py_arg,
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

/* _pygi_marshal_from_py_gclosure:
 * py_arg: (in):
 * arg: (out):
 */
gboolean
_pygi_marshal_from_py_gclosure(PyObject *py_arg,
                               GIArgument *arg)
{
    GClosure *closure;
    GType object_gtype = pyg_type_from_object_strict (py_arg, FALSE);

    if ( !(PyCallable_Check(py_arg) ||
           g_type_is_a (object_gtype, G_TYPE_CLOSURE))) {
        PyErr_Format (PyExc_TypeError, "Must be callable, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    if (g_type_is_a (object_gtype, G_TYPE_CLOSURE))
        closure = (GClosure *)pyg_boxed_get (py_arg, void);
    else
        closure = pyg_closure_new (py_arg, NULL, NULL);

    if (closure == NULL) {
        PyErr_SetString (PyExc_RuntimeError, "PyObject conversion to GClosure failed");
        return FALSE;
    }

    arg->v_pointer = closure;
    return TRUE;
}

gboolean
_pygi_marshal_from_py_interface_struct (PyObject *py_arg,
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
        return _pygi_marshal_from_py_gclosure (py_arg, arg);
    } else if (g_type_is_a (g_type, G_TYPE_VALUE)) {
        return _pygi_marshal_from_py_gvalue(py_arg,
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

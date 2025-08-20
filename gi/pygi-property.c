/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (c) 2010  Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <girepository/girepository.h>

#include "pygi-argument.h"
#include "pygi-fundamental.h"
#include "pygi-property.h"
#include "pygi-repository.h"
#include "pygi-type.h"
#include "pygi-value.h"

static GIPropertyInfo *
lookup_property_from_object_info (GIObjectInfo *info, const gchar *attr_name)
{
    gssize n_infos;
    gint i;

    n_infos = gi_object_info_get_n_properties (info);
    for (i = 0; i < n_infos; i++) {
        GIPropertyInfo *property_info;

        property_info = gi_object_info_get_property (info, i);
        g_assert (info != NULL);

        if (strcmp (attr_name,
                    gi_base_info_get_name (GI_BASE_INFO (property_info)))
            == 0) {
            return property_info;
        }

        gi_base_info_unref (property_info);
    }

    return NULL;
}

static GIPropertyInfo *
lookup_property_from_interface_info (GIInterfaceInfo *info,
                                     const gchar *attr_name)
{
    gssize n_infos;
    gint i;

    n_infos = gi_interface_info_get_n_properties (info);
    for (i = 0; i < n_infos; i++) {
        GIPropertyInfo *property_info;

        property_info = gi_interface_info_get_property (info, i);
        g_assert (info != NULL);

        if (strcmp (attr_name,
                    gi_base_info_get_name (GI_BASE_INFO (property_info)))
            == 0) {
            return property_info;
        }

        gi_base_info_unref (property_info);
    }

    return NULL;
}

static GIPropertyInfo *
_pygi_lookup_property_from_g_type (GType g_type, const gchar *attr_name)
{
    GIPropertyInfo *ret = NULL;
    GIRepository *repository;
    GIBaseInfo *info;

    repository = pygi_repository_get_default ();
    info = gi_repository_find_by_gtype (repository, g_type);
    if (info == NULL) return NULL;

    if (GI_IS_OBJECT_INFO (info))
        ret =
            lookup_property_from_object_info ((GIObjectInfo *)info, attr_name);
    else if (GI_IS_INTERFACE_INFO (info))
        ret = lookup_property_from_interface_info ((GIInterfaceInfo *)info,
                                                   attr_name);

    gi_base_info_unref (info);

    return ret;
}

PyObject *
pygi_call_do_get_property (PyObject *instance, GParamSpec *pspec)
{
    PyObject *py_pspec;
    PyObject *retval;

    py_pspec = pygi_fundamental_new (pspec);
    retval = PyObject_CallMethod (instance, "do_get_property", "O", py_pspec);
    Py_DECREF (py_pspec);
    return retval;
}

PyObject *
pygi_get_property_value (PyGObject *instance, GParamSpec *pspec)
{
    GIPropertyInfo *property_info = NULL;
    GValue value = {
        0,
    };
    PyObject *py_value = NULL;
    GType fundamental;
    gboolean handled;

    if (!(pspec->flags & G_PARAM_READABLE)) {
        PyErr_Format (PyExc_TypeError, "property %s is not readable",
                      g_param_spec_get_name (pspec));
        return NULL;
    }

    /* Fast path which calls the Python getter implementation directly.
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=723872 */
    if (pyg_gtype_is_custom (pspec->owner_type)) {
        return pygi_call_do_get_property ((PyObject *)instance, pspec);
    }

    Py_BEGIN_ALLOW_THREADS;
    g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
    g_object_get_property (instance->obj, pspec->name, &value);
    fundamental = G_TYPE_FUNDAMENTAL (G_VALUE_TYPE (&value));
    Py_END_ALLOW_THREADS;


    /* Fast path basic types which don't need GI type info. */
    py_value = pygi_value_to_py_basic_type (&value, fundamental, &handled);
    if (handled) {
        goto out;
    }

    /* Attempt to marshal through GI.
     * The owner_type of the pspec gives us the exact type that introduced the
     * property, even if it is a parent class of the instance in question. */
    property_info =
        _pygi_lookup_property_from_g_type (pspec->owner_type, pspec->name);
    if (property_info) {
        GITypeInfo *type_info = NULL;
        gboolean free_array = FALSE;
        GIArgument arg = {
            0,
        };
        GITransfer transfer = GI_TRANSFER_NOTHING;

        type_info = gi_property_info_get_type_info (property_info);
        arg = _pygi_argument_from_g_value (&value, type_info);

        /* Arrays are special cased, see note in _pygi_argument_to_array. */
        if (gi_type_info_get_tag (type_info) == GI_TYPE_TAG_ARRAY) {
            arg.v_pointer = _pygi_argument_to_array (&arg, NULL, NULL, NULL,
                                                     type_info, &free_array);
        } else if (g_type_is_a (pspec->value_type, G_TYPE_BOXED)) {
            arg.v_pointer = g_value_dup_boxed (&value);
            transfer = GI_TRANSFER_EVERYTHING;
        }

        py_value = _pygi_argument_to_object (&arg, type_info, transfer);

        if (free_array) {
            g_array_free (arg.v_pointer, FALSE);
        }

        gi_base_info_unref (type_info);
        gi_base_info_unref (property_info);

        if (PyErr_Occurred ()) {
            return NULL;
        }
    }

    /* Fallback to GValue marshalling. */
    if (py_value == NULL) {
        py_value = pyg_param_gvalue_as_pyobject (&value, TRUE, pspec);
    }

out:
    g_value_unset (&value);
    return py_value;
}

PyObject *
pygi_get_property_value_by_name (PyGObject *self, gchar *param_name)
{
    GParamSpec *pspec;

    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self->obj),
                                          param_name);
    if (!pspec) {
        PyErr_Format (PyExc_TypeError,
                      "object of type `%s' does not have property `%s'",
                      g_type_name (G_OBJECT_TYPE (self->obj)), param_name);
        return NULL;
    }

    return pygi_get_property_value (self, pspec);
}

gint
pygi_set_property_value (PyGObject *instance, GParamSpec *pspec,
                         PyObject *py_value)
{
    GIPropertyInfo *property_info = NULL;
    GITypeInfo *type_info = NULL;
    GIBaseInfo *info = NULL;
    GITypeTag type_tag;
    GITransfer transfer;
    GValue value = {
        0,
    };
    GIArgument arg = {
        0,
    };
    gint ret_value = -1;

    /* The owner_type of the pspec gives us the exact type that introduced the
     * property, even if it is a parent class of the instance in question. */
    property_info =
        _pygi_lookup_property_from_g_type (pspec->owner_type, pspec->name);
    if (property_info == NULL) goto out;

    if (!(pspec->flags & G_PARAM_WRITABLE)) goto out;

    type_info = gi_property_info_get_type_info (property_info);
    transfer = gi_property_info_get_ownership_transfer (property_info);
    arg = _pygi_argument_from_object (py_value, type_info, transfer);

    if (PyErr_Occurred ()) goto out;

    g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

    /* FIXME: Lots of types still unhandled */
    type_tag = gi_type_info_get_tag (type_info);
    switch (type_tag) {
    case GI_TYPE_TAG_INTERFACE: {
        GType type;

        info = gi_type_info_get_interface (type_info);
        type = gi_registered_type_info_get_g_type (
            GI_REGISTERED_TYPE_INFO (info));

        if (GI_IS_FLAGS_INFO (info)) {
            /* Check flags before enums: flags are a subtype of enum. */
            g_value_set_flags (&value, arg.v_uint);
        } else if (GI_IS_ENUM_INFO (info)) {
            g_value_set_enum (&value, arg.v_int);
        } else if (GI_IS_INTERFACE_INFO (info) || GI_IS_OBJECT_INFO (info)) {
            if (arg.v_pointer == NULL || G_IS_OBJECT (arg.v_pointer)) {
                g_value_set_object (&value, arg.v_pointer);
            } else if (!pygi_fundamental_set_value (&value, arg.v_pointer)) {
                PyErr_Format (
                    PyExc_NotImplementedError,
                    "Setting properties of type '%s' is not implemented",
                    g_type_name (type));
                goto out;
            }
        } else if (GI_IS_STRUCT_INFO (info) || GI_IS_UNION_INFO (info)) {
            if (g_type_is_a (type, G_TYPE_BOXED)) {
                g_value_set_boxed (&value, arg.v_pointer);
            } else if (g_type_is_a (type, G_TYPE_VARIANT)) {
                g_value_set_variant (&value, arg.v_pointer);
            } else {
                PyErr_Format (
                    PyExc_NotImplementedError,
                    "Setting properties of type '%s' is not implemented",
                    g_type_name (type));
                goto out;
            }
        } else {
            PyErr_Format (PyExc_NotImplementedError,
                          "Setting properties of type '%s' is not implemented",
                          g_type_name (type));
            goto out;
        }
        break;
    }
    case GI_TYPE_TAG_BOOLEAN:
        g_value_set_boolean (&value, arg.v_boolean);
        break;
    case GI_TYPE_TAG_INT8:
        g_value_set_schar (&value, arg.v_int8);
        break;
    case GI_TYPE_TAG_INT16:
    case GI_TYPE_TAG_INT32:
        if (G_VALUE_HOLDS_LONG (&value))
            g_value_set_long (&value, arg.v_long);
        else
            g_value_set_int (&value, arg.v_int);
        break;
    case GI_TYPE_TAG_INT64:
        if (G_VALUE_HOLDS_LONG (&value))
            g_value_set_long (&value, arg.v_long);
        else
            g_value_set_int64 (&value, arg.v_int64);
        break;
    case GI_TYPE_TAG_UINT8:
        g_value_set_uchar (&value, arg.v_uint8);
        break;
    case GI_TYPE_TAG_UINT16:
    case GI_TYPE_TAG_UINT32:
        if (G_VALUE_HOLDS_ULONG (&value))
            g_value_set_ulong (&value, arg.v_ulong);
        else
            g_value_set_uint (&value, arg.v_uint);
        break;
    case GI_TYPE_TAG_UINT64:
        if (G_VALUE_HOLDS_ULONG (&value))
            g_value_set_ulong (&value, arg.v_ulong);
        else
            g_value_set_uint64 (&value, arg.v_uint64);
        break;
    case GI_TYPE_TAG_FLOAT:
        g_value_set_float (&value, arg.v_float);
        break;
    case GI_TYPE_TAG_DOUBLE:
        g_value_set_double (&value, arg.v_double);
        break;
    case GI_TYPE_TAG_GTYPE:
        g_value_set_gtype (&value, arg.v_size);
        break;
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
        g_value_set_string (&value, arg.v_string);
        break;
    case GI_TYPE_TAG_GHASH:
        g_value_set_boxed (&value, arg.v_pointer);
        break;
    case GI_TYPE_TAG_GLIST:
        if (G_VALUE_HOLDS_BOXED (&value))
            g_value_set_boxed (&value, arg.v_pointer);
        else
            g_value_set_pointer (&value, arg.v_pointer);
        break;
    case GI_TYPE_TAG_ARRAY: {
        /* This is assumes GI_TYPE_TAG_ARRAY is always a GStrv
             * https://bugzilla.gnome.org/show_bug.cgi?id=688232
             */
        GArray *arg_items = (GArray *)arg.v_pointer;
        gchar **strings;
        guint i;

        if (arg_items == NULL) goto out;

        strings = g_new0 (char *, arg_items->len + 1);
        for (i = 0; i < arg_items->len; ++i) {
            strings[i] = g_array_index (arg_items, GIArgument, i).v_string;
        }
        strings[arg_items->len] = NULL;
        g_value_take_boxed (&value, strings);
        g_array_free (arg_items, TRUE);
        break;
    }
    default:
        PyErr_Format (
            PyExc_NotImplementedError,
            "Setting properties of type %s is not implemented",
            gi_type_tag_to_string (gi_type_info_get_tag (type_info)));
        goto out;
    }

    g_object_set_property (instance->obj, pspec->name, &value);
    g_value_unset (&value);

    ret_value = 0;

out:
    if (property_info != NULL) gi_base_info_unref (property_info);
    if (type_info != NULL) gi_base_info_unref (type_info);
    if (info != NULL) gi_base_info_unref (info);

    return ret_value;
}

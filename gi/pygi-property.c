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

#include "pygi-private.h"

#include <girepository.h>

static GIPropertyInfo *
lookup_property_from_object_info (GIObjectInfo *info, const gchar *attr_name)
{
    gssize n_infos;
    gssize i;

    n_infos = g_object_info_get_n_properties (info);
    for (i = 0; i < n_infos; i++) {
        GIPropertyInfo *property_info;

        property_info = g_object_info_get_property (info, i);
        g_assert (info != NULL);

        if (strcmp (attr_name, g_base_info_get_name (property_info)) == 0) {
            return property_info;
        }

        g_base_info_unref (property_info);
    }

    return NULL;
}

static GIPropertyInfo *
lookup_property_from_interface_info (GIInterfaceInfo *info,
                                     const gchar *attr_name)
{
    gssize n_infos;
    gssize i;

    n_infos = g_interface_info_get_n_properties (info);
    for (i = 0; i < n_infos; i++) {
        GIPropertyInfo *property_info;

        property_info = g_interface_info_get_property (info, i);
        g_assert (info != NULL);

        if (strcmp (attr_name, g_base_info_get_name (property_info)) == 0) {
            return property_info;
        }

        g_base_info_unref (property_info);
    }

    return NULL;
}

static GIPropertyInfo *
_pygi_lookup_property_from_g_type (GType g_type, const gchar *attr_name)
{
    GIPropertyInfo *ret = NULL;
    GIRepository *repository;
    GIBaseInfo *info;

    repository = g_irepository_get_default();
    info = g_irepository_find_by_gtype (repository, g_type);
    if (info == NULL)
       return NULL;

    if (GI_IS_OBJECT_INFO (info))
        ret = lookup_property_from_object_info ((GIObjectInfo *) info,
                                                attr_name);
    else if (GI_IS_INTERFACE_INFO (info))
        ret = lookup_property_from_interface_info ((GIInterfaceInfo *) info,
                                                   attr_name);

    g_base_info_unref (info);
    return ret;
}

static inline gpointer
g_value_get_or_dup_boxed (const GValue *value, GITransfer transfer)
{
    if (transfer == GI_TRANSFER_EVERYTHING)
        return g_value_dup_boxed (value);
    else
        return g_value_get_boxed (value);
}

PyObject *
pygi_get_property_value_real (PyGObject *instance, GParamSpec *pspec)
{
    GIPropertyInfo *property_info = NULL;
    GValue value = { 0, };
    GIArgument arg = { 0, };
    PyObject *py_value = NULL;
    GITypeInfo *type_info = NULL;
    GITransfer transfer;
    GITypeTag type_tag;

    /* The owner_type of the pspec gives us the exact type that introduced the
     * property, even if it is a parent class of the instance in question. */
    property_info = _pygi_lookup_property_from_g_type (pspec->owner_type, pspec->name);

    if (property_info == NULL)
        goto out;

    g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
    g_object_get_property (instance->obj, pspec->name, &value);

    type_info = g_property_info_get_type (property_info);
    transfer = g_property_info_get_ownership_transfer (property_info);

    type_tag = g_type_info_get_tag (type_info);
    switch (type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
            arg.v_boolean = g_value_get_boolean (&value);
            break;
        case GI_TYPE_TAG_INT8:
            arg.v_int8 = g_value_get_schar (&value);
            break;
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_INT32:
            if (G_VALUE_HOLDS_LONG (&value))
                arg.v_long = g_value_get_long (&value);
            else
                arg.v_int = g_value_get_int (&value);
            break;
        case GI_TYPE_TAG_INT64:
            if (G_VALUE_HOLDS_LONG (&value))
                arg.v_long = g_value_get_long (&value);
            else
                arg.v_int64 = g_value_get_int64 (&value);
            break;
        case GI_TYPE_TAG_UINT8:
            arg.v_uint8 = g_value_get_uchar (&value);
            break;
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_UINT32:
            if (G_VALUE_HOLDS_ULONG (&value))
                arg.v_ulong = g_value_get_ulong (&value);
            else
                arg.v_uint = g_value_get_uint (&value);
            break;
        case GI_TYPE_TAG_UINT64:
            if (G_VALUE_HOLDS_ULONG (&value))
                arg.v_ulong = g_value_get_ulong (&value);
            else
                arg.v_uint64 = g_value_get_uint64 (&value);
            break;
        case GI_TYPE_TAG_FLOAT:
            arg.v_float = g_value_get_float (&value);
            break;
        case GI_TYPE_TAG_DOUBLE:
            arg.v_double = g_value_get_double (&value);
            break;
        case GI_TYPE_TAG_GTYPE:
            arg.v_size = g_value_get_gtype (&value);
            break;
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
            arg.v_string = g_value_dup_string (&value);
            break;
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;
            GType type;

            info = g_type_info_get_interface (type_info);
            type = g_registered_type_info_get_g_type (info);
            info_type = g_base_info_get_type (info);

            g_base_info_unref (info);

            switch (info_type) {
                case GI_INFO_TYPE_ENUM:
                    arg.v_int = g_value_get_enum (&value);
                    break;
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_OBJECT:
                    arg.v_pointer = g_value_get_object (&value);
                    break;
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_STRUCT:
                case GI_INFO_TYPE_UNION:

                    if (g_type_is_a (type, G_TYPE_BOXED)) {
                        arg.v_pointer = g_value_dup_boxed (&value);
                    } else if (g_type_is_a (type, G_TYPE_POINTER)) {
                        arg.v_pointer = g_value_get_pointer (&value);
                    } else if (g_type_is_a (type, G_TYPE_VARIANT)) {
                        arg.v_pointer = g_value_get_variant (&value);
                    } else {
                        PyErr_Format (PyExc_NotImplementedError,
                                      "Retrieving properties of type '%s' is not implemented",
                                      g_type_name (type));
                    }
                    break;
                default:
                    PyErr_Format (PyExc_NotImplementedError,
                                  "Retrieving properties of type '%s' is not implemented",
                                  g_type_name (type));
                    goto out;
            }
            break;
        }
        case GI_TYPE_TAG_GHASH:
            arg.v_pointer = g_value_get_or_dup_boxed (&value, transfer);
            break;
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
            if (G_VALUE_HOLDS_BOXED(&value))
                arg.v_pointer = g_value_get_or_dup_boxed (&value, transfer);
            else
                arg.v_pointer = g_value_get_pointer (&value);
            break;
        case GI_TYPE_TAG_ARRAY:
        {
            gchar** strings;
            GArray *arg_items;
            int i;

            strings = g_value_get_or_dup_boxed (&value, transfer);
            if (strings == NULL)
                arg.v_pointer = NULL;
            else {
                arg_items = g_array_sized_new (TRUE, TRUE, sizeof (GIArgument), g_strv_length (strings));
                g_array_set_size (arg_items, g_strv_length (strings));
                for (i = 0; strings[i] != NULL; ++i) {
                    g_array_index (arg_items, GIArgument, i).v_string = strings[i];
                }
                arg.v_pointer = arg_items;
            }
            break;
        }
        default:
            PyErr_Format (PyExc_NotImplementedError,
                          "Retrieving properties of type %s is not implemented",
                          g_type_tag_to_string (g_type_info_get_tag (type_info)));
            goto out;
    }

    py_value = _pygi_argument_to_object (&arg, type_info, transfer);
    g_value_unset (&value);

out:
    if (property_info != NULL)
        g_base_info_unref (property_info);
    if (type_info != NULL)
        g_base_info_unref (type_info);

    return py_value;
}

gint
pygi_set_property_value_real (PyGObject *instance,
                              GParamSpec *pspec,
                              PyObject *py_value)
{
    GIPropertyInfo *property_info = NULL;
    GITypeInfo *type_info = NULL;
    GITypeTag type_tag;
    GITransfer transfer;
    GValue value = { 0, };
    GIArgument arg = { 0, };
    gint ret_value = -1;

    /* The owner_type of the pspec gives us the exact type that introduced the
     * property, even if it is a parent class of the instance in question. */
    property_info = _pygi_lookup_property_from_g_type (pspec->owner_type,
                                                       pspec->name);
    if (property_info == NULL)
        goto out;

    if (! (pspec->flags & G_PARAM_WRITABLE))
        goto out;

    type_info = g_property_info_get_type (property_info);
    transfer = g_property_info_get_ownership_transfer (property_info);
    arg = _pygi_argument_from_object (py_value, type_info, transfer);

    if (PyErr_Occurred())
        goto out;

    g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

    // FIXME: Lots of types still unhandled
    type_tag = g_type_info_get_tag (type_info);
    switch (type_tag) {
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;
            GType type;

            info = g_type_info_get_interface (type_info);
            type = g_registered_type_info_get_g_type (info);
            info_type = g_base_info_get_type (info);

            g_base_info_unref (info);

            switch (info_type) {
                case GI_INFO_TYPE_ENUM:
                    g_value_set_enum (&value, arg.v_int);
                    break;
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_OBJECT:
                    g_value_set_object (&value, arg.v_pointer);
                    break;
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_STRUCT:
                case GI_INFO_TYPE_UNION:
                    if (g_type_is_a (type, G_TYPE_BOXED)) {
                        g_value_set_boxed (&value, arg.v_pointer);
                    } else if (g_type_is_a (type, G_TYPE_VARIANT)) {
                        g_value_set_variant (&value, arg.v_pointer);
                    } else {
                        PyErr_Format (PyExc_NotImplementedError,
                                      "Setting properties of type '%s' is not implemented",
                                      g_type_name (type));
                        goto out;
                    }
                    break;
                default:
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
            if (G_VALUE_HOLDS_BOXED(&value))
                g_value_set_boxed (&value, arg.v_pointer);
            else
                g_value_set_pointer (&value, arg.v_pointer);
            break;
        case GI_TYPE_TAG_ARRAY:
        {
            /* This is assumes GI_TYPE_TAG_ARRAY is always a GStrv
             * https://bugzilla.gnome.org/show_bug.cgi?id=688232
             */
            GArray *arg_items = (GArray*) arg.v_pointer;
            gchar** strings;
            int i;

            if (arg_items == NULL)
                goto out;

            strings = g_new0 (char*, arg_items->len + 1);
            for (i = 0; i < arg_items->len; ++i) {
                strings[i] = g_array_index (arg_items, GIArgument, i).v_string;
            }
            strings[arg_items->len] = NULL;
            g_value_take_boxed (&value, strings);
            g_array_free (arg_items, TRUE);
            break;
        }
        default:
            PyErr_Format (PyExc_NotImplementedError,
                          "Setting properties of type %s is not implemented",
                          g_type_tag_to_string (g_type_info_get_tag (type_info)));
            goto out;
    }

    g_object_set_property (instance->obj, pspec->name, &value);
    g_value_unset (&value);

    ret_value = 0;

out:
    if (property_info != NULL)
        g_base_info_unref (property_info);
    if (type_info != NULL)
        g_base_info_unref (type_info);

    return ret_value;
}


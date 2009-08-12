/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   pygi-boxed.c: Boxed utility functions.
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

#include <pygobject.h>

gboolean
_pygi_g_struct_info_is_simple (GIStructInfo *struct_info)
{
    gboolean is_simple;
    gsize n_field_infos;
    gsize i;

    is_simple = TRUE;

    n_field_infos = g_struct_info_get_n_fields(struct_info);

    for (i = 0; i < n_field_infos && is_simple; i++) {
        GIFieldInfo *field_info;
        GITypeInfo *field_type_info;
        gboolean is_pointer;

        field_info = g_struct_info_get_field(struct_info, i);
        field_type_info = g_field_info_get_type(field_info);
        is_pointer = g_type_info_is_pointer(field_type_info);

        if (is_pointer) {
            is_simple = FALSE;
        } else {
            GITypeTag field_type_tag;

            field_type_tag = g_type_info_get_tag(field_type_info);

            switch (field_type_tag) {
                case GI_TYPE_TAG_BOOLEAN:
                case GI_TYPE_TAG_INT8:
                case GI_TYPE_TAG_UINT8:
                case GI_TYPE_TAG_INT16:
                case GI_TYPE_TAG_UINT16:
                case GI_TYPE_TAG_INT32:
                case GI_TYPE_TAG_UINT32:
                case GI_TYPE_TAG_SHORT:
                case GI_TYPE_TAG_USHORT:
                case GI_TYPE_TAG_INT:
                case GI_TYPE_TAG_UINT:
                case GI_TYPE_TAG_INT64:
                case GI_TYPE_TAG_UINT64:
                case GI_TYPE_TAG_LONG:
                case GI_TYPE_TAG_ULONG:
                case GI_TYPE_TAG_SSIZE:
                case GI_TYPE_TAG_SIZE:
                case GI_TYPE_TAG_FLOAT:
                case GI_TYPE_TAG_DOUBLE:
                case GI_TYPE_TAG_TIME_T:
                    break;
                case GI_TYPE_TAG_VOID:
                case GI_TYPE_TAG_GTYPE:
                case GI_TYPE_TAG_ERROR:
                case GI_TYPE_TAG_UTF8:
                case GI_TYPE_TAG_FILENAME:
                case GI_TYPE_TAG_ARRAY:
                case GI_TYPE_TAG_GLIST:
                case GI_TYPE_TAG_GSLIST:
                case GI_TYPE_TAG_GHASH:
                    /* Should have been catched by is_pointer above. */
                    g_assert_not_reached();
                    break;
                case GI_TYPE_TAG_INTERFACE:
                {
                    GIBaseInfo *info;
                    GIInfoType info_type;

                    info = g_type_info_get_interface(field_type_info);
                    info_type = g_base_info_get_type(info);

                    switch (info_type) {
                        case GI_INFO_TYPE_BOXED:
                        case GI_INFO_TYPE_STRUCT:
                            is_simple = _pygi_g_struct_info_is_simple((GIStructInfo *)info);
                            break;
                        case GI_INFO_TYPE_UNION:
                            /* TODO */
                            is_simple = FALSE;
                            break;
                        case GI_INFO_TYPE_ENUM:
                        case GI_INFO_TYPE_FLAGS:
                            break;
                        case GI_INFO_TYPE_OBJECT:
                        case GI_INFO_TYPE_VFUNC:
                        case GI_INFO_TYPE_CALLBACK:
                        case GI_INFO_TYPE_INVALID:
                        case GI_INFO_TYPE_INTERFACE:
                        case GI_INFO_TYPE_FUNCTION:
                        case GI_INFO_TYPE_CONSTANT:
                        case GI_INFO_TYPE_ERROR_DOMAIN:
                        case GI_INFO_TYPE_VALUE:
                        case GI_INFO_TYPE_SIGNAL:
                        case GI_INFO_TYPE_PROPERTY:
                        case GI_INFO_TYPE_FIELD:
                        case GI_INFO_TYPE_ARG:
                        case GI_INFO_TYPE_TYPE:
                        case GI_INFO_TYPE_UNRESOLVED:
                            is_simple = FALSE;
                            break;
                    }

                    g_base_info_unref(info);
                    break;
	            }
            }
        }

        g_base_info_unref((GIBaseInfo *)field_type_info);
        g_base_info_unref((GIBaseInfo *)field_info);
    }

    return is_simple;
}

PyObject *
pygi_boxed_new_from_type (PyTypeObject *type,
                          gpointer      pointer,
                          gboolean      own_pointer)
{
    GIBaseInfo *info;
    GIInfoType info_type;
    GType g_type;
    PyGBoxed *self = NULL;

    info = _pygi_object_get_gi_info((PyObject *)type, &PyGIRegisteredTypeInfo_Type);
    if (info == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Format(PyExc_TypeError, "cannot create '%s' instances", type->tp_name);
        }
        return NULL;
    }

    info_type = g_base_info_get_type(info);
    g_type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)info);

    if (pointer == NULL) {
        if (info_type == GI_INFO_TYPE_STRUCT && !g_type_is_a(g_type, G_TYPE_BOXED)) {
            gboolean is_simple;
            gsize size;

            is_simple = _pygi_g_struct_info_is_simple((GIStructInfo *)info);
            if (!is_simple) {
                PyErr_Format(PyExc_TypeError,
                        "cannot create '%s' instances; needs a specific constructor", type->tp_name);
                goto out;
            }

            size = g_struct_info_get_size((GIStructInfo *)info);

            pointer = g_try_malloc(size);
            if (pointer == NULL) {
                PyErr_NoMemory();
                goto out;
            }

            own_pointer = TRUE;
        } else {
            PyErr_Format(PyExc_TypeError,
                    "cannot create '%s' instances; needs a specific constructor", type->tp_name);
            goto out;
        }
    }

    self = (PyGBoxed *)type->tp_alloc(type, 0);
    if (self == NULL) {
        g_free(pointer);
        goto out;
    }

    self->boxed = pointer;
    self->gtype = g_type;
    self->free_on_dealloc = own_pointer;

out:
    g_base_info_unref(info);

    return (PyObject *)self;
}


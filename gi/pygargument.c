/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   pygargument.c: GArgument - PyObject conversion fonctions.
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
#include <pygobject.h>

gint
pygi_gi_registered_type_info_check_py_object(GIRegisteredTypeInfo *info,
        PyObject *object, gboolean is_instance)
{
    gint retval;
    PyObject *type;
    gchar *type_name_expected;

    type = pygi_py_type_find_by_gi_info((GIBaseInfo *)info);
    if (type == NULL) {
        return FALSE;
    }
    g_assert(PyType_Check(type));

    if (is_instance) {
        retval = PyObject_IsInstance(object, type);
        if (!retval) {
            type_name_expected = pygi_gi_base_info_get_fullname(
                    (GIBaseInfo *)info);
        }
    } else {
        if (!PyObject_Type(type)) {
            type_name_expected = "type";
            retval = 0;
        } else if (!PyType_IsSubtype((PyTypeObject *)object,
                (PyTypeObject *)type)) {
            type_name_expected = pygi_gi_base_info_get_fullname(
                    (GIBaseInfo *)info);
            retval = 0;
        } else {
            retval = 1;
        }
    }

    Py_DECREF(type);

    if (!retval) {
        PyTypeObject *object_type;

        if (type_name_expected == NULL) {
            return -1;
        }

        object_type = (PyTypeObject *)PyObject_Type(object);
        if (object_type == NULL) {
            return -1;
        }

        PyErr_Format(PyExc_TypeError, "Must be %s, not %s",
            type_name_expected, object_type->tp_name);

        g_free(type_name_expected);
    }

    return retval;
}

void
pygi_gi_type_tag_get_py_bounds(GITypeTag type_tag, PyObject **lower,
        PyObject **upper)
{
    switch(type_tag) {
        case GI_TYPE_TAG_INT8:
            *lower = PyInt_FromLong(-128);
            *upper = PyInt_FromLong(127);
            break;
        case GI_TYPE_TAG_INT16:
            *lower = PyInt_FromLong(-32768);
            *upper = PyInt_FromLong(32767);
            break;
        case GI_TYPE_TAG_INT32:
            *lower = PyInt_FromLong(-2147483648);
            *upper = PyInt_FromLong(2147483647);
            break;
        case GI_TYPE_TAG_INT64:
            /* Note: On 32-bit archs, these numbers don't fit in a long. */
            *lower = PyLong_FromLongLong(-9223372036854775808u);
            *upper = PyLong_FromLongLong(9223372036854775807);
            break;
        case GI_TYPE_TAG_SHORT:
            *lower = PyInt_FromLong(G_MINSHORT);
            *upper = PyInt_FromLong(G_MAXSHORT);
            break;
        case GI_TYPE_TAG_INT:
            *lower = PyInt_FromLong(G_MININT);
            *upper = PyInt_FromLong(G_MAXINT);
            break;
        case GI_TYPE_TAG_LONG:
        case GI_TYPE_TAG_SSIZE:
            *lower = PyInt_FromLong(G_MINLONG);
            *upper = PyInt_FromLong(G_MAXLONG);
            break;
        case GI_TYPE_TAG_UINT8:
            *upper = PyInt_FromLong(255);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_UINT16:
            *upper = PyInt_FromLong(65535);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_UINT32:
            /* Note: On 32-bit archs, this number doesn't fit in a long. */
            *upper = PyLong_FromLongLong(4294967295);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_UINT64:
            *upper = PyLong_FromUnsignedLongLong(18446744073709551615u);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_USHORT:
            *upper = PyInt_FromLong(G_MAXUSHORT);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_UINT:
            /* Note: On 32-bit archs, this number doesn't fit in a long. */
            *upper = PyLong_FromLongLong(G_MAXUINT);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_ULONG:
        case GI_TYPE_TAG_SIZE:
            *upper = PyLong_FromUnsignedLongLong(G_MAXULONG);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_FLOAT:
            *upper = PyFloat_FromDouble(G_MAXFLOAT);
            *lower = PyFloat_FromDouble(-G_MAXFLOAT);
            break;
        case GI_TYPE_TAG_DOUBLE:
            *upper = PyFloat_FromDouble(G_MAXDOUBLE);
            *lower = PyFloat_FromDouble(-G_MAXDOUBLE);
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "Non-numeric type tag");
            *lower = *upper = NULL;
            return;
    }
}

gint
pygi_gi_type_info_check_py_object(GITypeInfo *type_info, PyObject *object)
{
    gint retval;

    GITypeTag type_tag;

    type_tag = g_type_info_get_tag(type_info);

    retval = 1;

    switch(type_tag) {
        case GI_TYPE_TAG_VOID:
            /* No check possible. */
            break;
        case GI_TYPE_TAG_BOOLEAN:
            /* No check; every Python object has a truth value. */
            break;
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
        case GI_TYPE_TAG_SHORT:
        case GI_TYPE_TAG_INT:
        case GI_TYPE_TAG_LONG:
        case GI_TYPE_TAG_SSIZE:
        case GI_TYPE_TAG_UINT8:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_USHORT:
        case GI_TYPE_TAG_UINT:
        case GI_TYPE_TAG_ULONG:
        case GI_TYPE_TAG_SIZE:
        case GI_TYPE_TAG_FLOAT:
        case GI_TYPE_TAG_DOUBLE:
        {
            PyObject *lower, *upper;

            if (!PyNumber_Check(object)) {
                PyErr_Format(PyExc_TypeError, "Must be int or long, not %s",
                        object->ob_type->tp_name);
                retval = 0;
                break;
            }

            pygi_gi_type_tag_get_py_bounds(type_tag, &lower, &upper);
            if (lower == NULL || upper == NULL) {
                retval = -1;
                goto gi_type_info_check_py_object_check_number_clean;
            }

            /* Check bounds */
            if (PyObject_Compare(lower, object) > 0
                || PyObject_Compare(upper, object) < 0) {
                PyObject *lower_str, *upper_str;

                if (PyErr_Occurred()) {
                    retval = -1;
                    goto gi_type_info_check_py_object_check_number_error_clean;
                }

                lower_str = PyObject_Str(lower);
                upper_str = PyObject_Str(upper);
                if (lower_str == NULL || upper_str == NULL) {
                    retval = -1;
                    goto gi_type_info_check_py_object_check_number_error_clean;
                }

                PyErr_Format(PyExc_ValueError, "Must range from %s to %s",
                        PyString_AS_STRING(lower_str),
                        PyString_AS_STRING(upper_str));

                retval = 0;

gi_type_info_check_py_object_check_number_error_clean:
                Py_XDECREF(lower_str);
                Py_XDECREF(upper_str);
            }

gi_type_info_check_py_object_check_number_clean:
            Py_XDECREF(lower);
            Py_XDECREF(upper);
            break;
        }
        case GI_TYPE_TAG_UTF8:
            if (!PyString_Check(object)) {
                PyErr_Format(PyExc_TypeError, "Must be string, not %s",
                        object->ob_type->tp_name);
                retval = 0;
            }
            break;
        case GI_TYPE_TAG_ARRAY:
        {
            gssize required_size;
            Py_ssize_t object_size;
            GITypeInfo *item_type_info;
            gsize i;

            if (!PyTuple_Check(object)) {
                PyErr_Format(PyExc_TypeError, "Must be tuple, not %s",
                        object->ob_type->tp_name);
                retval = 0;
                break;
            }

            object_size = PyTuple_Size(object);
            if (object_size < 0) {
                retval = -1;
                break;
            }

            required_size = g_type_info_get_array_fixed_size(type_info);
            if (required_size >= 0 && object_size != required_size) {
                PyErr_Format(PyExc_ValueError, "Must contain %zd items, not %zd",
                        required_size, object_size);
                retval = 0;
                break;
            }

            item_type_info = g_type_info_get_param_type(type_info, 0);

            for (i = 0; i < object_size; i++) {
                PyObject *item;

                item = PyTuple_GetItem(object, i);
                if (item == NULL) {
                    retval = -1;
                    break;
                }

                retval = pygi_gi_type_info_check_py_object(item_type_info, item);
                if (retval < 0) {
                    break;
                }

                if (!retval) {
                    PyErr_PREFIX_FROM_FORMAT("Item %zu :", i);
                    break;
                }
            }

            g_base_info_unref((GIBaseInfo *)item_type_info);

            break;
        }
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface(type_info);
            info_type = g_base_info_get_type(info);

            switch (info_type) {
                case GI_INFO_TYPE_ENUM:
                {
                    (void) PyInt_AsLong(object);
                    if (PyErr_Occurred()) {
                        PyErr_Clear();
                        PyErr_Format(PyExc_TypeError, "Must be int, not %s",
                                object->ob_type->tp_name);
                        retval = 0;
                    }
                    /* XXX: What if the value doesn't correspond to any enum field? */
                    break;
                }
                case GI_INFO_TYPE_STRUCT:
                {
                    GType type;

                    /* Handle special cases. */
                    type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)info);
                    if (g_type_is_a(type, G_TYPE_VALUE)) {
                        /* Nothing to check. */
                        break;
                    } else if (g_type_is_a(type, G_TYPE_CLOSURE)) {
                        if (!PyCallable_Check(object)) {
                            PyErr_Format(PyExc_TypeError, "Must be callable, not %s",
                                    object->ob_type->tp_name);
                            retval = 0;
                        }
                        break;
                    }
                }
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_OBJECT:
                    retval = pygi_gi_registered_type_info_check_py_object((GIRegisteredTypeInfo *)info, object, TRUE);
                    break;
                default:
                    /* TODO: To complete with other types. */
                    g_assert_not_reached();
            }

            g_base_info_unref(info);
            break;
        }
        case GI_TYPE_TAG_GTYPE:
        {
            gint is_instance;

            is_instance = PyObject_IsInstance(object, (PyObject *)&PyGTypeWrapper_Type);
            if (is_instance < 0) {
                retval = -1;
                break;
            }

            if (!is_instance && (!PyType_Check(object) || pyg_type_from_object(object) == 0)) {
                PyErr_Format(PyExc_TypeError, "Must be gobject.GType, not %s",
                        object->ob_type->tp_name);
                retval = 0;
            }
            break;
        }
        case GI_TYPE_TAG_TIME_T:
        case GI_TYPE_TAG_FILENAME:
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        case GI_TYPE_TAG_GHASH:
        case GI_TYPE_TAG_ERROR:
            /* TODO */
        default:
            g_assert_not_reached();
    }

    return retval;
}

GArgument
pyg_argument_from_pyobject(PyObject *object, GITypeInfo *type_info)
{
    GArgument arg;
    GITypeTag type_tag;

    type_tag = g_type_info_get_tag((GITypeInfo*)type_info);
    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
        /* Nothing to do */
        break;
    case GI_TYPE_TAG_UTF8:
        if (object == Py_None)
            arg.v_pointer = NULL;
        else
            arg.v_pointer = g_strdup(PyString_AsString(object));
        break;
    case GI_TYPE_TAG_USHORT:
        arg.v_ushort = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_UINT8:
        arg.v_uint8 = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_UINT:
        arg.v_uint = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_UINT16:
        arg.v_uint16 = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_UINT32:
        arg.v_uint32 = PyLong_AsLongLong(object);
        break;
    case GI_TYPE_TAG_UINT64:
        if (PyInt_Check(object)) {
            PyObject *long_obj = PyNumber_Long(object);
            arg.v_uint64 = PyLong_AsUnsignedLongLong(long_obj);
            Py_DECREF(long_obj);
        } else
            arg.v_uint64 = PyLong_AsUnsignedLongLong(object);
        break;
    case GI_TYPE_TAG_SHORT:
        arg.v_short = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_INT8:
        arg.v_int8 = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_INT:
        arg.v_int = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_SSIZE:
    case GI_TYPE_TAG_LONG:
        arg.v_long = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_SIZE:
    case GI_TYPE_TAG_ULONG:
        arg.v_ulong = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_BOOLEAN:
        arg.v_boolean = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_INT16:
        arg.v_int16 = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_INT32:
        arg.v_int32 = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_INT64:
        arg.v_int64 = PyLong_AsLongLong(object);
        break;
    case GI_TYPE_TAG_FLOAT:
        arg.v_float = (float)PyFloat_AsDouble(object);
        break;
    case GI_TYPE_TAG_DOUBLE:
        arg.v_double = PyFloat_AsDouble(object);
        break;
    case GI_TYPE_TAG_INTERFACE:
    {
        GIBaseInfo* interface_info;
        GIInfoType interface_info_type;

        interface_info = g_type_info_get_interface(type_info);
        interface_info_type = g_base_info_get_type(interface_info);

        switch (interface_info_type) {
            case GI_INFO_TYPE_ENUM:
                arg.v_int = PyInt_AsLong(object);
                break;
            case GI_INFO_TYPE_STRUCT:
            {
                GType gtype;
                PyObject *py_buffer;

                gtype = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)interface_info);

                if (g_type_is_a(gtype, G_TYPE_VALUE)) {
                    GValue *value;
                    int retval;
                    PyObject *py_type;

                    value = g_slice_new0(GValue);

                    py_type = PyObject_Type(object);
                    g_assert(py_type != NULL);

                    g_value_init(value, pyg_type_from_object(py_type));

                    retval = pyg_value_from_pyobject(value, object);
                    g_assert(retval == 0);

                    arg.v_pointer = value;
                    break;
                } else if (g_type_is_a(gtype, G_TYPE_CLOSURE)) {
                    arg.v_pointer = pyg_closure_new(object, NULL, NULL);
                    break;
                }

                py_buffer = PyObject_GetAttrString(object, "__buffer__");
                g_assert(py_buffer != NULL);
                (*py_buffer->ob_type->tp_as_buffer->bf_getreadbuffer)(py_buffer, 0, &arg.v_pointer);

                break;
            }
            case GI_INFO_TYPE_OBJECT:
                if (object == Py_None) {
                    arg.v_pointer = NULL;
                    break;
                }
                arg.v_pointer = pygobject_get(object);
                break;
            default:
                /* TODO: To complete with other types. */
                g_assert_not_reached();
        }
        g_base_info_unref((GIBaseInfo *)interface_info);
        break;
    }
    case GI_TYPE_TAG_ARRAY:
    {
        gsize length;
        arg.v_pointer = pyg_array_from_pyobject(object, type_info, &length);
        break;
    }
    case GI_TYPE_TAG_ERROR:
        /* Allow NULL GError, otherwise fall through */
        if (object == Py_None) {
            arg.v_pointer = NULL;
            break;
        }
    case GI_TYPE_TAG_GTYPE:
        arg.v_int = pyg_type_from_object(object);
        break;
    default:
        g_print("<PyO->GArg> GITypeTag %s is unhandled\n",
                g_type_tag_to_string(type_tag));
        break;
    }

    return arg;
}

static PyObject *
glist_to_pyobject(GITypeTag list_tag, GITypeInfo *type_info, GList *list, GSList *slist)
{
    PyObject *py_list;
    int i;
    GArgument arg;
    PyObject *child_obj;

    if ((py_list = PyList_New(0)) == NULL) {
        g_list_free(list);
        return NULL;
    }
    i = 0;
    if (list_tag == GI_TYPE_TAG_GLIST) {
        for ( ; list != NULL; list = list->next) {
            arg.v_pointer = list->data;

            child_obj = pyg_argument_to_pyobject(&arg, type_info);

            if (child_obj == NULL) {
                g_list_free(list);
                Py_DECREF(py_list);
                return NULL;
            }
            PyList_Append(py_list, child_obj);
            Py_DECREF(child_obj);

            ++i;
        }
    } else {
        for ( ; slist != NULL; slist = slist->next) {
            arg.v_pointer = slist->data;

            child_obj = pyg_argument_to_pyobject(&arg, type_info);

            if (child_obj == NULL) {
                g_list_free(list);
                Py_DECREF(py_list);
                return NULL;
            }
            PyList_Append(py_list, child_obj);
            Py_DECREF(child_obj);

            ++i;
        }
    }
    g_list_free(list);
    return py_list;
}

static
gsize
pyg_type_get_size(GITypeTag type_tag)
{
    gsize size;

    switch(type_tag) {
        case GI_TYPE_TAG_VOID:
            size = sizeof(void);
            break;
        case GI_TYPE_TAG_BOOLEAN:
            size = sizeof(gboolean);
            break;
        case GI_TYPE_TAG_INT:
        case GI_TYPE_TAG_UINT:
            size = sizeof(gint);
            break;
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_UINT8:
            size = sizeof(gint8);
            break;
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
            size = sizeof(gint16);
            break;
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_UINT32:
            size = sizeof(gint32);
            break;
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
            size = sizeof(gint64);
            break;
        case GI_TYPE_TAG_LONG:
        case GI_TYPE_TAG_ULONG:
            size = sizeof(glong);
            break;
        case GI_TYPE_TAG_SIZE:
        case GI_TYPE_TAG_SSIZE:
            size = sizeof(gsize);
            break;
        case GI_TYPE_TAG_FLOAT:
            size = sizeof(gfloat);
            break;
        case GI_TYPE_TAG_DOUBLE:
            size = sizeof(gdouble);
            break;
        case GI_TYPE_TAG_UTF8:
            size = sizeof(gchar *);
            break;
        default:
            /* TODO: Complete with other types */
            g_assert_not_reached();
    }

    return size;
}

gpointer
pyg_array_from_pyobject(PyObject *object, GITypeInfo *type_info, gsize *length)
{
    gpointer items;
    gpointer current_item;
    gssize item_size;
    gboolean is_zero_terminated;
    GITypeInfo *item_type_info;
    GITypeTag type_tag;
    gsize i;

    is_zero_terminated = g_type_info_is_zero_terminated(type_info);
    item_type_info = g_type_info_get_param_type(type_info, 0);

    type_tag = g_type_info_get_tag(item_type_info);
    item_size = pyg_type_get_size(type_tag);

    *length = PyTuple_Size(object);
    items = g_try_malloc(*length * (item_size + (is_zero_terminated ? 1 : 0)));

    if (items == NULL) {
        g_base_info_unref((GIBaseInfo *)item_type_info);
        return NULL;
    }

    current_item = items;
    for (i = 0; i < *length; i++) {
        GArgument arg;
        PyObject *py_item;

        py_item = PyTuple_GetItem(object, i);
        g_assert(py_item != NULL);

        arg = pyg_argument_from_pyobject(py_item, item_type_info);

        g_memmove(current_item, &arg, item_size);

        current_item += item_size;
    }

    if (is_zero_terminated) {
        memset(current_item, 0, item_size);
    }

    g_base_info_unref((GIBaseInfo *)item_type_info);

    return items;
}

PyObject *
pyg_array_to_pyobject(gpointer items, gsize length, GITypeInfo *type_info)
{
    PyObject *py_items;
    gsize item_size;
    GITypeInfo *item_type_info;
    GITypeTag type_tag;
    gsize i;
    gpointer current_item;

    if (g_type_info_is_zero_terminated(type_info)) {
        length = g_strv_length(items);
    }

    py_items = PyTuple_New(length);
    if (py_items == NULL) {
        return NULL;
    }

    item_type_info = g_type_info_get_param_type (type_info, 0);
    type_tag = g_type_info_get_tag(item_type_info);
    item_size = pyg_type_get_size(type_tag);

    current_item = items;
    for(i = 0; i < length; i++) {
        PyObject *item;
        int retval;

        item = pyg_argument_to_pyobject((GArgument *)current_item, item_type_info);
        if (item == NULL) {
            g_base_info_unref((GIBaseInfo *)item_type_info);
            Py_DECREF(py_items);
            return NULL;
        }

        retval = PyTuple_SetItem(py_items, i, item);
        if (retval) {
            g_base_info_unref((GIBaseInfo *)item_type_info);
            Py_DECREF(py_items);
            return NULL;
        }

        current_item += item_size;
    }

    return py_items;
}

PyObject *
pyg_argument_to_pyobject(GArgument *arg, GITypeInfo *type_info)
{
    GITypeTag type_tag;
    PyObject *obj;
    GITypeInfo *param_info;

    g_return_val_if_fail(type_info != NULL, NULL);
    type_tag = g_type_info_get_tag(type_info);

    obj = NULL;

    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
        // TODO: Should we take this as a buffer?
        g_warning("pybank doesn't know what to do with void types");
        obj = Py_None;
        break;
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
        param_info = g_type_info_get_param_type(type_info, 0);
        g_assert(param_info != NULL);
        obj = glist_to_pyobject(type_tag,
                                param_info,
                                type_tag == GI_TYPE_TAG_GLIST ? arg->v_pointer : NULL,
                                type_tag == GI_TYPE_TAG_GSLIST ? arg->v_pointer : NULL);
        break;
    case GI_TYPE_TAG_BOOLEAN:
        obj = PyBool_FromLong(arg->v_boolean);
        break;
    case GI_TYPE_TAG_USHORT:
        obj = PyInt_FromLong(arg->v_ushort);
        break;
    case GI_TYPE_TAG_UINT8:
        obj = PyInt_FromLong(arg->v_uint8);
        break;
    case GI_TYPE_TAG_UINT:
        obj = PyInt_FromLong(arg->v_uint);
        break;
    case GI_TYPE_TAG_UINT16:
        obj = PyInt_FromLong(arg->v_uint16);
        break;
    case GI_TYPE_TAG_UINT32:
        obj = PyLong_FromLongLong(arg->v_uint32);
        break;
    case GI_TYPE_TAG_UINT64:
        obj = PyLong_FromUnsignedLongLong(arg->v_uint64);
        break;
    case GI_TYPE_TAG_SHORT:
        obj = PyInt_FromLong(arg->v_short);
        break;
    case GI_TYPE_TAG_INT:
        obj = PyInt_FromLong(arg->v_int);
        break;
    case GI_TYPE_TAG_LONG:
        obj = PyInt_FromLong(arg->v_long);
        break;
    case GI_TYPE_TAG_ULONG:
        obj = PyInt_FromLong(arg->v_ulong);
        break;
    case GI_TYPE_TAG_SSIZE:
        obj = PyInt_FromLong(arg->v_ssize);
        break;
    case GI_TYPE_TAG_SIZE:
        obj = PyInt_FromLong(arg->v_size);
        break;
    case GI_TYPE_TAG_INT8:
        obj = PyInt_FromLong(arg->v_int8);
        break;
    case GI_TYPE_TAG_INT16:
        obj = PyInt_FromLong(arg->v_int16);
        break;
    case GI_TYPE_TAG_INT32:
        obj = PyInt_FromLong(arg->v_int32);
        break;
    case GI_TYPE_TAG_INT64:
        obj = PyLong_FromLongLong(arg->v_int64);
        break;
    case GI_TYPE_TAG_FLOAT:
        obj = PyFloat_FromDouble(arg->v_float);
        break;
    case GI_TYPE_TAG_DOUBLE:
        obj = PyFloat_FromDouble(arg->v_double);
        break;
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_UTF8:
        if (arg->v_string == NULL)
            obj = Py_None;
        else
            obj = PyString_FromString(arg->v_string);
        break;
    case GI_TYPE_TAG_INTERFACE:
    {
        GIBaseInfo* interface_info;
        GIInfoType interface_info_type;

        interface_info = g_type_info_get_interface(type_info);
        interface_info_type = g_base_info_get_type(interface_info);

        if (arg->v_pointer == NULL) {
            obj = Py_None;
        }

        switch (interface_info_type) {
            case GI_INFO_TYPE_ENUM:
               obj = PyInt_FromLong(arg->v_int);
                break;
            case GI_INFO_TYPE_STRUCT:
            {
                GType gtype;
                PyObject *py_type;
                gsize size;
                PyObject *buffer;
                PyObject **dict;
                int retval;

                gtype = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)interface_info);

                if (g_type_is_a(gtype, G_TYPE_VALUE)) {
                    obj = pyg_value_as_pyobject(arg->v_pointer, FALSE);
                    g_value_unset(arg->v_pointer);
                    break;
                }

                /* Create a Python buffer. */
                size = g_struct_info_get_size ((GIStructInfo *)interface_info);
                buffer = PyBuffer_FromReadWriteMemory(arg->v_pointer, size);
                if (buffer == NULL) {
                    break;
                }

                /* Wrap the structure. */
                py_type = pygi_py_type_find_by_gi_info((GIBaseInfo *)interface_info);
                g_assert(py_type != NULL);

                obj = PyObject_GC_New(PyObject, (PyTypeObject *)py_type);

                Py_DECREF(py_type);

                if (obj == NULL) {
                    Py_DECREF(buffer);
                    break;
                }

                /* FIXME: Any better way to initialize the dict pointer? */
                dict = (PyObject **)((char *)obj + ((PyTypeObject *)py_type)->tp_dictoffset);
                *dict = NULL;

                retval = PyObject_SetAttrString(obj, "__buffer__", buffer);
                if (retval < 0) {
                    Py_DECREF(obj);
                    obj = NULL;
                }

                break;
            }
            case GI_INFO_TYPE_OBJECT:
            {
                PyObject *py_type;

                /* Make sure the class is initialized. */
                py_type = pygi_py_type_find_by_gi_info((GIBaseInfo *)interface_info);
                g_assert(py_type != NULL);
                Py_DECREF(py_type);

                obj = pygobject_new(arg->v_pointer);

                break;
            }
            default:
                /* TODO: To complete with other types. */
                g_assert_not_reached();
        }

        g_base_info_unref((GIBaseInfo *)interface_info);

        break;
    }
    case GI_TYPE_TAG_ARRAY:
        g_warning("pyg_argument_to_pyobject: use pyarray_to_pyobject instead for arrays");
        obj = Py_None;
        break;
    case GI_TYPE_TAG_GTYPE:
    {
        GType gtype;
        gtype = arg->v_int;
        obj = pyg_type_wrapper_new(gtype);
        break;
    }
    default:
        g_print("<GArg->PyO> GITypeTag %s is unhandled\n",
                g_type_tag_to_string(type_tag));
        obj = PyString_FromString("<unhandled return value!>"); /*  */
        break;
    }

    Py_XINCREF(obj);
    return obj;
}


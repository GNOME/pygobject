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

static
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
                goto check_number_clean;
            }

            /* Check bounds */
            if (PyObject_Compare(lower, object) > 0
                || PyObject_Compare(upper, object) < 0) {
                PyObject *lower_str, *upper_str;

                if (PyErr_Occurred()) {
                    retval = -1;
                    goto check_number_error_clean;
                }

                lower_str = PyObject_Str(lower);
                upper_str = PyObject_Str(upper);
                if (lower_str == NULL || upper_str == NULL) {
                    retval = -1;
                    goto check_number_error_clean;
                }

                PyErr_Format(PyExc_ValueError, "Must range from %s to %s",
                        PyString_AS_STRING(lower_str),
                        PyString_AS_STRING(upper_str));

                retval = 0;

check_number_error_clean:
                Py_XDECREF(lower_str);
                Py_XDECREF(upper_str);
            }

check_number_clean:
            Py_XDECREF(lower);
            Py_XDECREF(upper);
            break;
        }
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
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
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        {
            GITypeInfo *item_type_info;
            Py_ssize_t length;
            Py_ssize_t i;

            if (!PySequence_Check(object)) {
                PyErr_Format(PyExc_TypeError, "Must be sequence, not %s",
                        object->ob_type->tp_name);
                retval = 0;
                break;
            }

            length = PySequence_Length(object);
            if (length < 0) {
                retval = -1;
                break;
            }

            item_type_info = g_type_info_get_param_type(type_info, 0);

            for (i = 0; i < length; i++) {
                PyObject *item;

                item = PySequence_GetItem(object, i);
                if (item == NULL) {
                    retval = -1;
                    break;
                }

                retval = pygi_gi_type_info_check_py_object(item_type_info, item);

                Py_DECREF(item);

                if (retval < 0) {
                    break;
                }
                if (!retval) {
                    PyErr_PREFIX_FROM_FORMAT("Item %zd :", i);
                    break;
                }
            }

            g_base_info_unref((GIBaseInfo *)item_type_info);

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
        case GI_TYPE_TAG_GHASH:
        case GI_TYPE_TAG_ERROR:
            /* TODO */
        default:
            g_assert_not_reached();
    }

    return retval;
}

gsize
pygi_gi_type_tag_get_size(GITypeTag type_tag)
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
        case GI_TYPE_TAG_FILENAME:
            size = sizeof(gchar *);
            break;
        default:
            /* TODO: Complete with other types */
            g_assert_not_reached();
    }

    return size;
}

GArgument
pygi_g_argument_from_py_object(PyObject *object, GITypeInfo *type_info)
{
    GArgument arg;
    GITypeTag type_tag;

    type_tag = g_type_info_get_tag(type_info);

    switch (type_tag)
    {
        case GI_TYPE_TAG_VOID:
            /* Nothing to do */
            break;
        case GI_TYPE_TAG_BOOLEAN:
            arg.v_boolean = PyObject_IsTrue(object);
            break;
        case GI_TYPE_TAG_SHORT:
        case GI_TYPE_TAG_USHORT:
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_UINT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_INT:
        case GI_TYPE_TAG_SSIZE:
        case GI_TYPE_TAG_LONG:
        {
            PyObject *int_;
            int_ = PyNumber_Int(object);
            if (int_ == NULL) {
                break;
            }
            arg.v_long = PyInt_AsLong(int_);
            Py_DECREF(int_);
            break;
        }
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_UINT64:
        case GI_TYPE_TAG_UINT:
        case GI_TYPE_TAG_SIZE:
        case GI_TYPE_TAG_ULONG:
        {
            PyObject *long_;
            long_ = PyNumber_Long(object);
            if (long_ == NULL) {
                break;
            }
            arg.v_uint64 = PyLong_AsUnsignedLongLong(long_);
            Py_DECREF(long_);
            break;
        }
        case GI_TYPE_TAG_INT64:
        {
            PyObject *long_;
            long_ = PyNumber_Long(object);
            if (long_ == NULL) {
                break;
            }
            arg.v_int64 = PyLong_AsLongLong(long_);
            Py_DECREF(long_);
            break;
        }
        case GI_TYPE_TAG_FLOAT:
        {
            PyObject *float_;
            float_ = PyNumber_Float(object);
            if (float_ == NULL) {
                break;
            }
            arg.v_float = (float)PyFloat_AsDouble(float_);
            Py_DECREF(float_);
            break;
        }
        case GI_TYPE_TAG_DOUBLE:
        {
            PyObject *float_;
            float_ = PyNumber_Float(object);
            if (float_ == NULL) {
                break;
            }
            arg.v_double = PyFloat_AsDouble(float_);
            Py_DECREF(float_);
            break;
        }
        case GI_TYPE_TAG_UTF8:
            arg.v_string = g_strdup(PyString_AsString(object));
            break;
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;

            info = g_type_info_get_interface(type_info);

            switch (g_base_info_get_type(info)) {
                case GI_INFO_TYPE_ENUM:
                {
                    PyObject *int_;
                    int_ = PyNumber_Int(object);
                    if (int_ == NULL) {
                        break;
                    }
                    arg.v_long = PyInt_AsLong(int_);
                    Py_DECREF(int_);
                    break;
                }
                case GI_INFO_TYPE_STRUCT:
                {
                    GType type;
                    gsize size;
                    gpointer buffer;

                    type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)info);

                    /* Handle special cases first. */
                    if (g_type_is_a(type, G_TYPE_VALUE)) {
                        GValue *value;
                        GType object_type;
                        gint retval;

                        object_type = pyg_type_from_object(object->ob_type);
                        if (object_type == G_TYPE_INVALID) {
                            PyErr_SetString(PyExc_RuntimeError, "Unable to retrieve object's GType");
                            break;
                        }

                        value = g_slice_new0(GValue);
                        g_value_init(value, object_type);

                        retval = pyg_value_from_pyobject(value, object);
                        if (retval < 0) {
                            g_slice_free(GValue, value);
                            PyErr_SetString(PyExc_RuntimeError, "PyObject conversion to GValue failed");
                            break;
                        }

                        arg.v_pointer = value;
                        break;
                    } else if (g_type_is_a(type, G_TYPE_CLOSURE)) {
                        GClosure *closure;

                        closure = pyg_closure_new(object, NULL, NULL);
                        if (closure == NULL) {
                            PyErr_SetString(PyExc_RuntimeError, "GClosure creation failed");
                            break;
                        }

                        arg.v_pointer = closure;
                        break;
                    }

                    buffer = pygi_py_object_get_buffer(object, &size);

                    arg.v_pointer = buffer;
                    break;
                }
                case GI_INFO_TYPE_OBJECT:
                    arg.v_pointer = pygobject_get(object);
                    break;
                default:
                    /* TODO */
                    g_assert_not_reached();
            }
            g_base_info_unref(info);
            break;
        }
        case GI_TYPE_TAG_FILENAME:
        {
            GError *error = NULL;
            const gchar *string;

            string = PyString_AsString(object);
            if (string == NULL) {
                break;
            }

            arg.v_string = g_filename_from_utf8(string, -1, NULL, NULL, &error);
            if (arg.v_string == NULL) {
                PyErr_SetString(PyExc_Exception, error->message);
                /* TODO: Convert the error to an exception. */
            }

            break;
        }
        case GI_TYPE_TAG_ARRAY:
        {
            gboolean is_zero_terminated;
            GITypeInfo *item_type_info;
            GITypeTag item_type_tag;
            GArray *array;
            gsize item_size;
            gssize length;
            gsize i;

            is_zero_terminated = g_type_info_is_zero_terminated(type_info);

            item_type_info = g_type_info_get_param_type(type_info, 0);
            item_type_tag = g_type_info_get_tag(item_type_info);

            item_size = pygi_gi_type_tag_get_size(item_type_tag);
            length = PyTuple_Size(object);
            if (length < 0) {
                goto array_clean;
            }

            array = g_array_sized_new(is_zero_terminated, FALSE, item_size, length);
            if (array == NULL) {
                PyErr_NoMemory();
                goto array_clean;
            }

            for (i = 0; i < length; i++) {
                PyObject *py_item;
                GArgument item;

                py_item = PyTuple_GetItem(object, i);
                if (py_item == NULL) {
                    /* TODO: free the previous items */
                    PyErr_PREFIX_FROM_FORMAT("Item %zu :", i);
                    goto array_clean;
                }

                item = pygi_g_argument_from_py_object(py_item, item_type_info);
                if (PyErr_Occurred()) {
                    /* TODO: free the previous items */
                    PyErr_PREFIX_FROM_FORMAT("Item %zu :", i);
                    goto array_clean;
                }

                g_array_insert_val(array, i, item);
            }

            arg.v_pointer = array;

array_clean:
            g_base_info_unref((GIBaseInfo *)item_type_info);
            break;
        }
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        {
            Py_ssize_t length;
            GITypeInfo *item_type_info;
            GSList *list = NULL;
            Py_ssize_t i;

            length = PySequence_Length(object);
            if (length < 0) {
                break;
            }

            item_type_info = g_type_info_get_param_type(type_info, 0);

            for (i = length - 1; i >= 0; i--) {
                PyObject *py_item;
                GArgument item;

                py_item = PySequence_GetItem(object, i);
                if (py_item == NULL) {
                    /* TODO: free the previous items */
                    break;
                }

                item = pygi_g_argument_from_py_object(py_item, item_type_info);

                Py_DECREF(py_item);

                if (PyErr_Occurred()) {
                    /* TODO: free the previous items */
                    PyErr_PREFIX_FROM_FORMAT("Item %zd :", i);
                    break;
                }

                if (type_tag == GI_TYPE_TAG_GLIST) {
                    list = (GSList *)g_list_prepend((GList *)list, item.v_pointer);
                } else {
                    list = g_slist_prepend(list, item.v_pointer);
                }
            }

            arg.v_pointer = list;

            g_base_info_unref((GIBaseInfo *)item_type_info);

            break;
        }
        case GI_TYPE_TAG_ERROR:
            /* Allow NULL GError, otherwise fall through */
            if (object == Py_None) {
                arg.v_pointer = NULL;
                break;
            }
            /* TODO */
            g_assert_not_reached();
            break;
        case GI_TYPE_TAG_GTYPE:
        {
            GType type;

            type = pyg_type_from_object(object);
            if (type == G_TYPE_INVALID) {
                PyErr_SetString(PyExc_RuntimeError, "GType conversion failed");
            }

            arg.v_long = type;
            break;
        }
        default:
            /* TODO */
            g_assert_not_reached();
    }

    return arg;
}

PyObject *
pygi_g_argument_to_py_object(GArgument arg, GITypeInfo *type_info)
{
    GITypeTag type_tag;
    PyObject *object;

    type_tag = g_type_info_get_tag(type_info);

    object = NULL;

    switch (type_tag) {
        case GI_TYPE_TAG_VOID:
            Py_INCREF(Py_None);
            object = Py_None;
            break;
        case GI_TYPE_TAG_BOOLEAN:
            object = PyBool_FromLong(arg.v_boolean);
            break;
        case GI_TYPE_TAG_UINT8:
            object = PyInt_FromLong(arg.v_uint8);
            break;
        case GI_TYPE_TAG_UINT16:
            object = PyInt_FromLong(arg.v_uint16);
            break;
        case GI_TYPE_TAG_UINT32:
            object = PyLong_FromLongLong(arg.v_uint32);
            break;
        case GI_TYPE_TAG_UINT64:
            object = PyLong_FromUnsignedLongLong(arg.v_uint64);
            break;
        case GI_TYPE_TAG_USHORT:
            object = PyInt_FromLong(arg.v_ushort);
            break;
        case GI_TYPE_TAG_UINT:
            object = PyLong_FromLongLong(arg.v_uint);
            break;
        case GI_TYPE_TAG_ULONG:
            object = PyLong_FromUnsignedLongLong(arg.v_ulong);
            break;
        case GI_TYPE_TAG_SIZE:
            object = PyLong_FromUnsignedLongLong(arg.v_size);
            break;
        case GI_TYPE_TAG_INT8:
            object = PyInt_FromLong(arg.v_int8);
            break;
        case GI_TYPE_TAG_INT16:
            object = PyInt_FromLong(arg.v_int16);
            break;
        case GI_TYPE_TAG_INT32:
            object = PyInt_FromLong(arg.v_int32);
            break;
        case GI_TYPE_TAG_INT64:
            object = PyLong_FromLongLong(arg.v_int64);
            break;
        case GI_TYPE_TAG_SHORT:
            object = PyInt_FromLong(arg.v_short);
            break;
        case GI_TYPE_TAG_INT:
            object = PyInt_FromLong(arg.v_int);
            break;
        case GI_TYPE_TAG_SSIZE:
            object = PyInt_FromLong(arg.v_ssize);
            break;
        case GI_TYPE_TAG_LONG:
            object = PyInt_FromLong(arg.v_long);
            break;
        case GI_TYPE_TAG_FLOAT:
            object = PyFloat_FromDouble(arg.v_float);
            break;
        case GI_TYPE_TAG_DOUBLE:
            object = PyFloat_FromDouble(arg.v_double);
            break;
        case GI_TYPE_TAG_UTF8:
            object = PyString_FromString(arg.v_string);
            break;
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface(type_info);
            info_type = g_base_info_get_type(info);

            switch (info_type) {
                case GI_INFO_TYPE_ENUM:
                    object = PyInt_FromLong(arg.v_int);
                    break;
                case GI_INFO_TYPE_STRUCT:
                {
                    GType type;
                    PyObject *py_type = NULL;
                    gsize size;
                    PyObject *buffer = NULL;

                    /* Handle special cases first. */
                    type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)info);
                    if (g_type_is_a(type, G_TYPE_VALUE)) {
                        object = pyg_value_as_pyobject(arg.v_pointer, FALSE);
                        g_value_unset(arg.v_pointer);
                        break;
                    }

                    /* Create a Python buffer. */
                    size = g_struct_info_get_size((GIStructInfo *)info);
                    buffer = PyBuffer_FromReadWriteMemory(arg.v_pointer, size);
                    if (buffer == NULL) {
                        goto struct_error_clean;
                    }

                    /* Wrap the structure. */
                    py_type = pygi_py_type_find_by_gi_info(info);
                    if (py_type == NULL) {
                        goto struct_error_clean;
                    }

                    object = PyObject_CallFunction(py_type, "O", buffer);
                    if (object == NULL) {
                        goto struct_error_clean;
                    }

                    Py_DECREF(buffer);
                    Py_DECREF(py_type);

                    break;

struct_error_clean:
                    Py_XDECREF(buffer);
                    Py_XDECREF(py_type);
                    Py_XDECREF(object);
                    object = NULL;
                    break;
                }
                case GI_INFO_TYPE_OBJECT:
                {
                    PyObject *py_type;

                    /* Make sure the class is initialized. */
                    py_type = pygi_py_type_find_by_gi_info(info);
                    if (py_type == NULL) {
                        break;
                    }

                    object = pygobject_new(arg.v_pointer);

                    Py_DECREF(py_type);

                    break;
                }
                default:
                    /* TODO: To complete with other types. */
                    g_assert_not_reached();
            }

            g_base_info_unref(info);

            break;
        }
        case GI_TYPE_TAG_FILENAME:
        {
            GError *error = NULL;
            gchar *string;

            string = g_filename_to_utf8(arg.v_string, -1, NULL, NULL, &error);
            if (string == NULL) {
                PyErr_SetString(PyExc_Exception, error->message);
                /* TODO: Convert the error to an exception. */
                break;
            }

            object = PyString_FromString(string);

            g_free(string);

            break;
        }
        case GI_TYPE_TAG_ARRAY:
        {
            GArray *array;
            GITypeInfo *item_type_info;
            GITypeTag item_type_tag;
            gsize item_size;
            gsize i;

            array = arg.v_pointer;

            object = PyTuple_New(array->len);
            if (object == NULL) {
                break;
            }

            item_type_info = g_type_info_get_param_type(type_info, 0);
            item_type_tag = g_type_info_get_tag(item_type_info);
            item_size = pygi_gi_type_tag_get_size(item_type_tag);

            for(i = 0; i < array->len; i++) {
                GArgument item;
                PyObject *py_item;

                item = *(GArgument *)(array->data + item_size * i);
                py_item = pygi_g_argument_to_py_object(item, item_type_info);
                if (py_item == NULL) {
                    Py_DECREF(object);
                    object = NULL;
                    break;
                }

                PyTuple_SET_ITEM(object, i, py_item);
            }

            g_base_info_unref((GIBaseInfo *)item_type_info);
            break;
        }
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        {
            GSList *list;
            gsize length;
            GITypeInfo *item_type_info;
            gsize i;

            list = arg.v_pointer;
            length = g_slist_length(list);

            object = PyList_New(length);
            if (object == NULL) {
                break;
            }

            item_type_info = g_type_info_get_param_type(type_info, 0);

            for (i = 0; list != NULL; list = g_slist_next(list), i++) {
                GArgument item;
                PyObject *py_item;

                item.v_pointer = list->data;

                py_item = pygi_g_argument_to_py_object(item, item_type_info);
                if (py_item == NULL) {
                    Py_DECREF(object);
                    object = NULL;
                    PyErr_PREFIX_FROM_FORMAT("Item %zd :", i);
                    break;
                }

                PyList_SET_ITEM(object, i, py_item);
            }

            g_base_info_unref((GIBaseInfo *)item_type_info);
            break;
        }
        case GI_TYPE_TAG_GTYPE:
        {
            object = pyg_type_wrapper_new(arg.v_long);
            break;
        }
        default:
            /* TODO */
            g_assert_not_reached();
    }

    return object;
}


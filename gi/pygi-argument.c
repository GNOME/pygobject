/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   pygi-argument.c: GArgument - PyObject conversion functions.
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

#include <datetime.h>
#include <pygobject.h>

static gsize
_pygi_gi_type_tag_size (GITypeTag type_tag)
{
    gsize size;

    switch(type_tag) {

        /* Basic types */
        case GI_TYPE_TAG_VOID:
            size = sizeof(void);
            break;
        case GI_TYPE_TAG_BOOLEAN:
            size = sizeof(gboolean);
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
        case GI_TYPE_TAG_SHORT:
        case GI_TYPE_TAG_USHORT:
            size = sizeof(gshort);
            break;
        case GI_TYPE_TAG_INT:
        case GI_TYPE_TAG_UINT:
            size = sizeof(gint);
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
        case GI_TYPE_TAG_TIME_T:
            size = sizeof(time_t);
            break;
        case GI_TYPE_TAG_GTYPE:
            size = sizeof(GType);
            break;
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
            size = sizeof(gchar *);
            break;

        /* Non-basic types */
        case GI_TYPE_TAG_ARRAY:
        case GI_TYPE_TAG_INTERFACE:
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        case GI_TYPE_TAG_GHASH:
            /* TODO */
            g_assert_not_reached();
            break;
        case GI_TYPE_TAG_ERROR:
            g_assert_not_reached();
            break;
    }

    return size;
}

static void
_pygi_gi_type_tag_py_bounds (GITypeTag   type_tag,
                             PyObject  **lower,
                             PyObject  **upper)
{
    switch(type_tag) {
        case GI_TYPE_TAG_INT8:
            *lower = PyInt_FromLong(-128);
            *upper = PyInt_FromLong(127);
            break;
        case GI_TYPE_TAG_UINT8:
            *upper = PyInt_FromLong(255);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_INT16:
            *lower = PyInt_FromLong(-32768);
            *upper = PyInt_FromLong(32767);
            break;
        case GI_TYPE_TAG_UINT16:
            *upper = PyInt_FromLong(65535);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_INT32:
            *lower = PyInt_FromLong(-2147483648);
            *upper = PyInt_FromLong(2147483647);
            break;
        case GI_TYPE_TAG_UINT32:
            /* Note: On 32-bit archs, this number doesn't fit in a long. */
            *upper = PyLong_FromLongLong(4294967295);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_INT64:
            /* Note: On 32-bit archs, these numbers don't fit in a long. */
            *lower = PyLong_FromLongLong(-9223372036854775808u);
            *upper = PyLong_FromLongLong(9223372036854775807);
            break;
        case GI_TYPE_TAG_UINT64:
            *upper = PyLong_FromUnsignedLongLong(18446744073709551615u);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_SHORT:
            *lower = PyInt_FromLong(G_MINSHORT);
            *upper = PyInt_FromLong(G_MAXSHORT);
            break;
        case GI_TYPE_TAG_USHORT:
            *upper = PyInt_FromLong(G_MAXUSHORT);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_INT:
            *lower = PyInt_FromLong(G_MININT);
            *upper = PyInt_FromLong(G_MAXINT);
            break;
        case GI_TYPE_TAG_UINT:
            /* Note: On 32-bit archs, this number doesn't fit in a long. */
            *upper = PyLong_FromLongLong(G_MAXUINT);
            *lower = PyInt_FromLong(0);
            break;
        case GI_TYPE_TAG_LONG:
        case GI_TYPE_TAG_SSIZE:
            *lower = PyInt_FromLong(G_MINLONG);
            *upper = PyInt_FromLong(G_MAXLONG);
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
_pygi_g_registered_type_info_check_object (GIRegisteredTypeInfo *info,
                                           gboolean is_instance,
                                           PyObject *object)
{
    gint retval;

    PyObject *type;
    gchar *type_name_expected;

    type = pygi_type_find_by_info((GIBaseInfo *)info);
    if (type == NULL) {
        return FALSE;
    }
    g_assert(PyType_Check(type));

    if (is_instance) {
        retval = PyObject_IsInstance(object, type);
        if (!retval) {
            type_name_expected = _pygi_g_base_info_get_fullname(
                    (GIBaseInfo *)info);
        }
    } else {
        if (!PyObject_Type(type)) {
            type_name_expected = "type";
            retval = 0;
        } else if (!PyType_IsSubtype((PyTypeObject *)object,
                (PyTypeObject *)type)) {
            type_name_expected = _pygi_g_base_info_get_fullname(
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

gint
_pygi_g_type_info_check_object (GITypeInfo *type_info,
                                gboolean    may_be_null,
                                PyObject   *object)
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
        case GI_TYPE_TAG_UINT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
        case GI_TYPE_TAG_SHORT:
        case GI_TYPE_TAG_USHORT:
        case GI_TYPE_TAG_INT:
        case GI_TYPE_TAG_UINT:
        case GI_TYPE_TAG_LONG:
        case GI_TYPE_TAG_ULONG:
        case GI_TYPE_TAG_SSIZE:
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

            _pygi_gi_type_tag_py_bounds(type_tag, &lower, &upper);
            if (lower == NULL || upper == NULL) {
                retval = -1;
                goto check_number_release;
            }

            /* Check bounds */
            if (PyObject_Compare(lower, object) > 0
                || PyObject_Compare(upper, object) < 0) {
                PyObject *lower_str, *upper_str;

                if (PyErr_Occurred()) {
                    retval = -1;
                    goto check_number_error_release;
                }

                lower_str = PyObject_Str(lower);
                upper_str = PyObject_Str(upper);
                if (lower_str == NULL || upper_str == NULL) {
                    retval = -1;
                    goto check_number_error_release;
                }

                PyErr_Format(PyExc_ValueError, "Must range from %s to %s",
                        PyString_AS_STRING(lower_str),
                        PyString_AS_STRING(upper_str));

                retval = 0;

check_number_error_release:
                Py_XDECREF(lower_str);
                Py_XDECREF(upper_str);
            }

check_number_release:
            Py_XDECREF(lower);
            Py_XDECREF(upper);
            break;
        }
        case GI_TYPE_TAG_TIME_T:
            if (!PyDateTime_Check(object)) {
                PyErr_Format(PyExc_TypeError, "Must be datetime.datetime, not %s",
                        object->ob_type->tp_name);
                retval = 0;
                break;
            }
            break;
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
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
            if (!PyString_Check(object) && (!may_be_null || object != Py_None)) {
                PyErr_Format(PyExc_TypeError, "Must be string, not %s",
                        object->ob_type->tp_name);
                retval = 0;
            }
            break;
        case GI_TYPE_TAG_ARRAY:
        {
            gssize fixed_size;
            Py_ssize_t length;
            GITypeInfo *item_type_info;
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

            fixed_size = g_type_info_get_array_fixed_size(type_info);
            if (fixed_size >= 0 && length != fixed_size) {
                PyErr_Format(PyExc_ValueError, "Must contain %zd items, not %zd",
                        fixed_size, length);
                retval = 0;
                break;
            }

            item_type_info = g_type_info_get_param_type(type_info, 0);
            g_assert(item_type_info != NULL);

            for (i = 0; i < length; i++) {
                PyObject *item;

                item = PySequence_GetItem(object, i);
                if (item == NULL) {
                    retval = -1;
                    break;
                }

                retval = _pygi_g_type_info_check_object(item_type_info, FALSE, item);

                Py_DECREF(item);

                if (retval < 0) {
                    break;
                }
                if (!retval) {
                    _PyGI_ERROR_PREFIX("Item %zd: ", i);
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
            g_assert(info != NULL);

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
                    if (may_be_null && object == Py_None) {
                        break;
                    }
                    retval = _pygi_g_registered_type_info_check_object((GIRegisteredTypeInfo *)info, TRUE, object);
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
            Py_ssize_t length;
            GITypeInfo *item_type_info;
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
            g_assert(item_type_info != NULL);

            for (i = 0; i < length; i++) {
                PyObject *item;

                item = PySequence_GetItem(object, i);
                if (item == NULL) {
                    retval = -1;
                    break;
                }

                retval = _pygi_g_type_info_check_object(item_type_info, FALSE, item);

                Py_DECREF(item);

                if (retval < 0) {
                    break;
                }
                if (!retval) {
                    _PyGI_ERROR_PREFIX("Item %zd: ", i);
                    break;
                }
            }

            g_base_info_unref((GIBaseInfo *)item_type_info);
            break;
        }
        case GI_TYPE_TAG_GHASH:
        {
            Py_ssize_t length;
            PyObject *keys;
            PyObject *values;
            GITypeInfo *key_type_info;
            GITypeInfo *value_type_info;
            Py_ssize_t i;

            if (!PyMapping_Check(object)) {
                PyErr_Format(PyExc_TypeError, "Must be mapping, not %s",
                        object->ob_type->tp_name);
                retval = 0;
                break;
            }

            length = PyMapping_Length(object);
            if (length < 0) {
                retval = -1;
                break;
            }

            keys = PyMapping_Keys(object);
            if (keys == NULL) {
                retval = -1;
                break;
            }

            values = PyMapping_Values(object);
            if (values == NULL) {
                retval = -1;
                Py_DECREF(keys);
                break;
            }

            key_type_info = g_type_info_get_param_type(type_info, 0);
            g_assert(key_type_info != NULL);

            value_type_info = g_type_info_get_param_type(type_info, 1);
            g_assert(value_type_info != NULL);

            for (i = 0; i < length; i++) {
                PyObject *key;
                PyObject *value;

                key = PyList_GET_ITEM(keys, i);
                value = PyList_GET_ITEM(values, i);

                retval = _pygi_g_type_info_check_object(key_type_info, FALSE, key);
                if (retval < 0) {
                    break;
                }
                if (!retval) {
                    _PyGI_ERROR_PREFIX("Key %zd :", i);
                    break;
                }

                retval = _pygi_g_type_info_check_object(value_type_info, FALSE, value);
                if (retval < 0) {
                    break;
                }
                if (!retval) {
                    _PyGI_ERROR_PREFIX("Value %zd :", i);
                    break;
                }
            }

            g_base_info_unref((GIBaseInfo *)key_type_info);
            g_base_info_unref((GIBaseInfo *)value_type_info);
            Py_DECREF(values);
            Py_DECREF(keys);
            break;
        }
        case GI_TYPE_TAG_ERROR:
            g_assert_not_reached();
            break;
    }

    return retval;
}

GArray *
_pygi_argument_to_array (GArgument  *arg,
                         GArgument  *args[],
                         GITypeInfo *type_info)
{
    /* Create a GArray. */
    GITypeInfo *item_type_info;
    gboolean is_zero_terminated;
    GITypeTag item_type_tag;
    gsize item_size;
    gssize length;
    GArray *g_array;

    is_zero_terminated = g_type_info_is_zero_terminated(type_info);
    item_type_info = g_type_info_get_param_type(type_info, 0);
    g_assert(item_type_info != NULL);

    item_type_tag = g_type_info_get_tag(item_type_info);
    item_size = _pygi_gi_type_tag_size(item_type_tag);

    g_base_info_unref((GIBaseInfo *)item_type_info);

    if (is_zero_terminated) {
        length = g_strv_length(arg->v_pointer);
    } else {
        length = g_type_info_get_array_fixed_size(type_info);
        if (length < 0) {
            gint length_arg_pos;

            length_arg_pos = g_type_info_get_array_length(type_info);
            g_assert(length_arg_pos >= 0);

            /* FIXME: Take into account the type of the argument. */
            length = args[length_arg_pos]->v_int;
        }
    }

    g_array = g_array_new(is_zero_terminated, FALSE, item_size);

    g_array->data = arg->v_pointer;
    g_array->len = length;

    return g_array;
}

GArgument
_pygi_argument_from_object (PyObject   *object,
                            GITypeInfo *type_info,
                            GITransfer  transfer)
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
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_UINT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_SHORT:
        case GI_TYPE_TAG_USHORT:
        case GI_TYPE_TAG_INT:
        case GI_TYPE_TAG_LONG:
        case GI_TYPE_TAG_SSIZE:
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
        case GI_TYPE_TAG_ULONG:
        case GI_TYPE_TAG_SIZE:
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
        case GI_TYPE_TAG_TIME_T:
        {
            PyDateTime_DateTime *py_datetime;
            struct tm datetime;
            time_t time_;

            py_datetime = (PyDateTime_DateTime *)object;

            if (py_datetime->hastzinfo) {
                if (PyErr_WarnEx(NULL, "tzinfo ignored; only local time is supported", 1) < 0) {
                    break;
                }
            }

            datetime.tm_sec = PyDateTime_DATE_GET_SECOND(py_datetime);
            datetime.tm_min = PyDateTime_DATE_GET_MINUTE(py_datetime);
            datetime.tm_hour = PyDateTime_DATE_GET_HOUR(py_datetime);
            datetime.tm_mday = PyDateTime_GET_DAY(py_datetime);
            datetime.tm_mon = PyDateTime_GET_MONTH(py_datetime) - 1;
            datetime.tm_year = PyDateTime_GET_YEAR(py_datetime) - 1900;
            datetime.tm_isdst = -1;

            time_ = mktime(&datetime);
            if (time_ == -1) {
                PyErr_SetString(PyExc_RuntimeError, "datetime conversion failed");
            }

            arg.v_long = time_;
            break;
        }
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
        case GI_TYPE_TAG_UTF8:
        {
            const gchar *string;

            if (object == Py_None) {
                arg.v_string = NULL;
                break;
            }

            string = PyString_AsString(object);

            /* Don't need to check for errors, since g_strdup is NULL-proof. */
            arg.v_string = g_strdup(string);
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
            Py_ssize_t length;
            Py_ssize_t i;

            length = PySequence_Length(object);
            if (length < 0) {
                break;
            }

            is_zero_terminated = g_type_info_is_zero_terminated(type_info);
            item_type_info = g_type_info_get_param_type(type_info, 0);
            g_assert(item_type_info != NULL);

            item_type_tag = g_type_info_get_tag(item_type_info);
            item_size = _pygi_gi_type_tag_size(item_type_tag);

            array = g_array_sized_new(is_zero_terminated, FALSE, item_size, length);
            if (array == NULL) {
                g_base_info_unref((GIBaseInfo *)item_type_info);
                PyErr_NoMemory();
                break;
            }

            for (i = 0; i < length; i++) {
                PyObject *py_item;
                GArgument item;

                py_item = PySequence_GetItem(object, i);
                if (py_item == NULL) {
                    goto array_item_error;
                }

                item = _pygi_argument_from_object(py_item, item_type_info,
                        transfer == GI_TRANSFER_EVERYTHING ? GI_TRANSFER_EVERYTHING : GI_TRANSFER_NOTHING);

                Py_DECREF(py_item);

                if (PyErr_Occurred()) {
                    goto array_item_error;
                }

                g_array_insert_val(array, i, item);
                continue;

array_item_error:
                /* Free everything we have converted so far. */
                _pygi_argument_release((GArgument *)array, type_info,
                        GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                array = NULL;

                _PyGI_ERROR_PREFIX("Item %zd: ", i);
                break;
            }

            arg.v_pointer = array;

            g_base_info_unref((GIBaseInfo *)item_type_info);
            break;
        }
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;

            info = g_type_info_get_interface(type_info);
            g_assert(info != NULL);

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

                        object_type = pyg_type_from_object((PyObject *)object->ob_type);
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

                    if (object == Py_None) {
                        arg.v_pointer = NULL;
                        break;
                    }

                    buffer = _pygi_object_get_buffer(object, &size);

                    arg.v_pointer = buffer;
                    break;
                }
                case GI_INFO_TYPE_OBJECT:
                    if (object == Py_None) {
                        arg.v_pointer = NULL;
                        break;
                    }

                    arg.v_pointer = pygobject_get(object);
                    if (transfer == GI_TRANSFER_EVERYTHING) {
                        g_object_ref(arg.v_pointer);
                    }
                    break;
                default:
                    /* TODO */
                    g_assert_not_reached();
            }
            g_base_info_unref(info);
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
            g_assert(item_type_info != NULL);

            for (i = length - 1; i >= 0; i--) {
                PyObject *py_item;
                GArgument item;

                py_item = PySequence_GetItem(object, i);
                if (py_item == NULL) {
                    goto list_item_error;
                }

                item = _pygi_argument_from_object(py_item, item_type_info,
                        transfer == GI_TRANSFER_EVERYTHING ? GI_TRANSFER_EVERYTHING : GI_TRANSFER_NOTHING);

                Py_DECREF(py_item);

                if (PyErr_Occurred()) {
                    goto list_item_error;
                }

                if (type_tag == GI_TYPE_TAG_GLIST) {
                    list = (GSList *)g_list_prepend((GList *)list, item.v_pointer);
                } else {
                    list = g_slist_prepend(list, item.v_pointer);
                }

                continue;

list_item_error:
                /* Free everything we have converted so far. */
                _pygi_argument_release((GArgument *)list, type_info,
                        GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                list = NULL;

                _PyGI_ERROR_PREFIX("Item %zd: ", i);
                break;
            }

            arg.v_pointer = list;

            g_base_info_unref((GIBaseInfo *)item_type_info);

            break;
        }
        case GI_TYPE_TAG_GHASH:
        {
            Py_ssize_t length;
            PyObject *keys;
            PyObject *values;
            GITypeInfo *key_type_info;
            GITypeInfo *value_type_info;
            GITypeTag key_type_tag;
            GHashFunc hash_func;
            GEqualFunc equal_func;
            GHashTable *hash_table;
            Py_ssize_t i;

            length = PyMapping_Length(object);
            if (length < 0) {
                break;
            }

            keys = PyMapping_Keys(object);
            if (keys == NULL) {
                break;
            }

            values = PyMapping_Values(object);
            if (values == NULL) {
                Py_DECREF(keys);
                break;
            }

            key_type_info = g_type_info_get_param_type(type_info, 0);
            g_assert(key_type_info != NULL);

            value_type_info = g_type_info_get_param_type(type_info, 1);
            g_assert(value_type_info != NULL);

            key_type_tag = g_type_info_get_tag(key_type_info);

            switch(key_type_tag) {
                case GI_TYPE_TAG_UTF8:
                case GI_TYPE_TAG_FILENAME:
                    hash_func = g_str_hash;
                    equal_func = g_str_equal;
                    break;
                case GI_TYPE_TAG_SHORT:
                case GI_TYPE_TAG_USHORT:
                case GI_TYPE_TAG_INT:
                case GI_TYPE_TAG_UINT:
                case GI_TYPE_TAG_INT8:
                case GI_TYPE_TAG_UINT8:
                case GI_TYPE_TAG_INT16:
                case GI_TYPE_TAG_UINT16:
                case GI_TYPE_TAG_INT32:
                case GI_TYPE_TAG_UINT32:
                    hash_func = g_int_hash;
                    equal_func = g_int_equal;
                    break;
                case GI_TYPE_TAG_INT64:
                case GI_TYPE_TAG_UINT64:
                case GI_TYPE_TAG_LONG:
                case GI_TYPE_TAG_ULONG:
                case GI_TYPE_TAG_SSIZE:
                case GI_TYPE_TAG_SIZE:
                    hash_func = g_int64_hash;
                    equal_func = g_int64_equal;
                    break;
                case GI_TYPE_TAG_FLOAT:
                case GI_TYPE_TAG_DOUBLE:
                    hash_func = g_double_hash;
                    equal_func = g_double_equal;
                    break;
                default:
                    if (PyErr_WarnEx(NULL, "No suited hash function available; using pointers", 1) < 0) {
                        goto hash_table_release;
                    }
                    hash_func = g_direct_hash;
                    equal_func = g_direct_equal;
            }

            hash_table = g_hash_table_new(hash_func, equal_func);
            if (hash_table == NULL) {
                PyErr_NoMemory();
                goto hash_table_release;
            }

            for (i = 0; i < length; i++) {
                PyObject *py_key;
                PyObject *py_value;
                GArgument key;
                GArgument value;

                py_key = PyList_GET_ITEM(keys, i);
                py_value = PyList_GET_ITEM(values, i);

                key = _pygi_argument_from_object(py_key, key_type_info,
                        transfer == GI_TRANSFER_EVERYTHING ? GI_TRANSFER_EVERYTHING : GI_TRANSFER_NOTHING);
                if (PyErr_Occurred()) {
                    goto hash_table_item_error;
                }

                value = _pygi_argument_from_object(py_value, value_type_info,
                        transfer == GI_TRANSFER_EVERYTHING ? GI_TRANSFER_EVERYTHING : GI_TRANSFER_NOTHING);
                if (PyErr_Occurred()) {
                    goto hash_table_item_error;
                }

                g_hash_table_insert(hash_table, key.v_pointer, value.v_pointer);
                continue;

hash_table_item_error:
                /* Free everything we have converted so far. */
                _pygi_argument_release((GArgument *)hash_table, type_info,
                        GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                hash_table = NULL;

                _PyGI_ERROR_PREFIX("Item %zd: ", i);
                break;
            }

            arg.v_pointer = hash_table;

hash_table_release:
            g_base_info_unref((GIBaseInfo *)key_type_info);
            g_base_info_unref((GIBaseInfo *)value_type_info);
            Py_DECREF(keys);
            Py_DECREF(values);
            break;
        }
        case GI_TYPE_TAG_ERROR:
            g_assert_not_reached();
            break;
    }

    return arg;
}

PyObject *
_pygi_argument_to_object (GArgument  *arg,
                          GITypeInfo *type_info)
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
            object = PyBool_FromLong(arg->v_boolean);
            break;
        case GI_TYPE_TAG_INT8:
            object = PyInt_FromLong(arg->v_int8);
            break;
        case GI_TYPE_TAG_UINT8:
            object = PyInt_FromLong(arg->v_uint8);
            break;
        case GI_TYPE_TAG_INT16:
            object = PyInt_FromLong(arg->v_int16);
            break;
        case GI_TYPE_TAG_UINT16:
            object = PyInt_FromLong(arg->v_uint16);
            break;
        case GI_TYPE_TAG_INT32:
            object = PyInt_FromLong(arg->v_int32);
            break;
        case GI_TYPE_TAG_UINT32:
            object = PyLong_FromLongLong(arg->v_uint32);
            break;
        case GI_TYPE_TAG_INT64:
            object = PyLong_FromLongLong(arg->v_int64);
            break;
        case GI_TYPE_TAG_UINT64:
            object = PyLong_FromUnsignedLongLong(arg->v_uint64);
            break;
        case GI_TYPE_TAG_SHORT:
            object = PyInt_FromLong(arg->v_short);
            break;
        case GI_TYPE_TAG_USHORT:
            object = PyInt_FromLong(arg->v_ushort);
            break;
        case GI_TYPE_TAG_INT:
            object = PyInt_FromLong(arg->v_int);
            break;
        case GI_TYPE_TAG_UINT:
            object = PyLong_FromLongLong(arg->v_uint);
            break;
        case GI_TYPE_TAG_LONG:
            object = PyInt_FromLong(arg->v_long);
            break;
        case GI_TYPE_TAG_ULONG:
            object = PyLong_FromUnsignedLongLong(arg->v_ulong);
            break;
        case GI_TYPE_TAG_SSIZE:
            object = PyInt_FromLong(arg->v_ssize);
            break;
        case GI_TYPE_TAG_SIZE:
            object = PyLong_FromUnsignedLongLong(arg->v_size);
            break;
        case GI_TYPE_TAG_FLOAT:
            object = PyFloat_FromDouble(arg->v_float);
            break;
        case GI_TYPE_TAG_DOUBLE:
            object = PyFloat_FromDouble(arg->v_double);
            break;
        case GI_TYPE_TAG_TIME_T:
        {
            struct tm *datetime;
            datetime = localtime(&arg->v_long);
            object = PyDateTime_FromDateAndTime(datetime->tm_year + 1900,
                datetime->tm_mon + 1, datetime->tm_mday, datetime->tm_hour,
                datetime->tm_min, datetime->tm_sec, 0);
            break;
        }
        case GI_TYPE_TAG_GTYPE:
            object = pyg_type_wrapper_new(arg->v_long);
            break;
        case GI_TYPE_TAG_UTF8:
            if (arg->v_string == NULL) {
                object = Py_None;
                Py_INCREF(object);
                break;
            }
            object = PyString_FromString(arg->v_string);
            break;
        case GI_TYPE_TAG_FILENAME:
        {
            GError *error = NULL;
            gchar *string;

            string = g_filename_to_utf8(arg->v_string, -1, NULL, NULL, &error);
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
            gsize i;

            array = arg->v_pointer;

            object = PyTuple_New(array->len);
            if (object == NULL) {
                break;
            }

            item_type_info = g_type_info_get_param_type(type_info, 0);
            g_assert(item_type_info != NULL);

            for(i = 0; i < array->len; i++) {
                GArgument item;
                PyObject *py_item;

                item = _g_array_index(array, GArgument, i);
                py_item = _pygi_argument_to_object(&item, item_type_info);
                if (py_item == NULL) {
                    Py_CLEAR(object);
                    _PyGI_ERROR_PREFIX("Item %zu: ", i);
                    break;
                }

                PyTuple_SET_ITEM(object, i, py_item);
            }

            g_base_info_unref((GIBaseInfo *)item_type_info);
            break;
        }
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface(type_info);
            g_assert(info != NULL);

            info_type = g_base_info_get_type(info);

            switch (info_type) {
                case GI_INFO_TYPE_ENUM:
                    object = PyInt_FromLong(arg->v_int);
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
                        object = pyg_value_as_pyobject(arg->v_pointer, FALSE);
                        g_value_unset(arg->v_pointer);
                        break;
                    }

                    /* Create a Python buffer. */
                    size = g_struct_info_get_size((GIStructInfo *)info);
                    buffer = PyBuffer_FromReadWriteMemory(arg->v_pointer, size);
                    if (buffer == NULL) {
                        break;
                    }

                    /* Wrap the structure. */
                    py_type = pygi_type_find_by_info(info);
                    if (py_type == NULL) {
                        Py_DECREF(buffer);
                        break;
                    }

                    object = PyObject_CallFunction(py_type, "O", buffer);

                    Py_DECREF(buffer);
                    Py_DECREF(py_type);

                    break;
                }
                case GI_INFO_TYPE_OBJECT:
                {
                    PyObject *py_type;

                    /* Make sure the class is initialized. */
                    py_type = pygi_type_find_by_info(info);
                    if (py_type == NULL) {
                        break;
                    }

                    object = pygobject_new(arg->v_pointer);

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
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        {
            GSList *list;
            gsize length;
            GITypeInfo *item_type_info;
            gsize i;

            list = arg->v_pointer;
            length = g_slist_length(list);

            object = PyList_New(length);
            if (object == NULL) {
                break;
            }

            item_type_info = g_type_info_get_param_type(type_info, 0);
            g_assert(item_type_info != NULL);

            for (i = 0; list != NULL; list = g_slist_next(list), i++) {
                GArgument item;
                PyObject *py_item;

                item.v_pointer = list->data;

                py_item = _pygi_argument_to_object(&item, item_type_info);
                if (py_item == NULL) {
                    Py_CLEAR(object);
                    _PyGI_ERROR_PREFIX("Item %zu: ", i);
                    break;
                }

                PyList_SET_ITEM(object, i, py_item);
            }

            g_base_info_unref((GIBaseInfo *)item_type_info);
            break;
        }
        case GI_TYPE_TAG_GHASH:
        {
            GITypeInfo *key_type_info;
            GITypeInfo *value_type_info;
            GHashTableIter hash_table_iter;
            GArgument key;
            GArgument value;

            object = PyDict_New();
            if (object == NULL) {
                break;
            }

            key_type_info = g_type_info_get_param_type(type_info, 0);
            g_assert(key_type_info != NULL);

            value_type_info = g_type_info_get_param_type(type_info, 1);
            g_assert(value_type_info != NULL);

            g_hash_table_iter_init(&hash_table_iter, (GHashTable *)arg->v_pointer);
            while (g_hash_table_iter_next(&hash_table_iter, &key.v_pointer, &value.v_pointer)) {
                PyObject *py_key;
                PyObject *py_value;
                int retval;

                py_key = _pygi_argument_to_object(&key, key_type_info);
                if (py_key == NULL) {
                    break;
                }

                py_value = _pygi_argument_to_object(&value, value_type_info);
                if (py_value == NULL) {
                    Py_DECREF(py_key);
                    break;
                }

                retval = PyDict_SetItem(object, py_key, py_value);

                Py_DECREF(py_key);
                Py_DECREF(py_value);

                if (retval < 0) {
                    Py_CLEAR(object);
                    break;
                }
            }

            g_base_info_unref((GIBaseInfo *)key_type_info);
            g_base_info_unref((GIBaseInfo *)value_type_info);
            break;
        }
        case GI_TYPE_TAG_ERROR:
            g_assert_not_reached();
            break;
    }

    return object;
}

void
_pygi_argument_release (GArgument   *arg,
                        GITypeInfo  *type_info,
                        GITransfer   transfer,
                        GIDirection  direction)
{
    GITypeTag type_tag;

    type_tag = g_type_info_get_tag(type_info);

    switch(type_tag) {
        case GI_TYPE_TAG_VOID:
        case GI_TYPE_TAG_BOOLEAN:
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_UINT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
        case GI_TYPE_TAG_SHORT:
        case GI_TYPE_TAG_USHORT:
        case GI_TYPE_TAG_INT:
        case GI_TYPE_TAG_UINT:
        case GI_TYPE_TAG_LONG:
        case GI_TYPE_TAG_ULONG:
        case GI_TYPE_TAG_SSIZE:
        case GI_TYPE_TAG_SIZE:
        case GI_TYPE_TAG_FLOAT:
        case GI_TYPE_TAG_DOUBLE:
        case GI_TYPE_TAG_TIME_T:
        case GI_TYPE_TAG_GTYPE:
            /* Values, nothing to free. */
            break;
        case GI_TYPE_TAG_FILENAME:
        case GI_TYPE_TAG_UTF8:
            if (transfer == GI_TRANSFER_CONTAINER) {
                if (PyErr_WarnEx(NULL, "Invalid 'container' transfer for string", 1) < 0) {
                    break;
                }
            }
            if ((direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
                    || (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_EVERYTHING)) {
                g_free(arg->v_string);
            }
            break;
        case GI_TYPE_TAG_ARRAY:
        {
            GArray *array;
            gsize i;

            array = arg->v_pointer;

            if ((direction == GI_DIRECTION_IN && transfer != GI_TRANSFER_EVERYTHING)
                    || (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_EVERYTHING)) {
                GITypeInfo *item_type_info;
                GITransfer item_transfer;

                item_type_info = g_type_info_get_param_type(type_info, 0);
                g_assert(item_type_info != NULL);

                if (direction == GI_DIRECTION_IN) {
                    item_transfer = GI_TRANSFER_NOTHING;
                } else {
                    item_transfer = GI_TRANSFER_EVERYTHING;
                }

                /* Free the items */
                for (i = 0; i < array->len; i++) {
                    GArgument item;
                    item = _g_array_index(array, GArgument, i);
                    _pygi_argument_release(&item, item_type_info, item_transfer, direction);
                }

                g_base_info_unref((GIBaseInfo *)item_type_info);
            }

            if ((direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
                    || (direction == GI_DIRECTION_OUT && transfer != GI_TRANSFER_NOTHING)) {
                g_array_free(array, TRUE);
            }

            break;
        }
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface(type_info);
            g_assert(info != NULL);

            info_type = g_base_info_get_type(info);

            switch (info_type) {
                case GI_INFO_TYPE_STRUCT:
                {
                    GType type;

                    type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)info);

                    if (g_type_is_a(type, G_TYPE_VALUE)) {
                        GValue *value;

                        value = arg->v_pointer;

                        if ((direction == GI_DIRECTION_IN && transfer != GI_TRANSFER_EVERYTHING)
                                || (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_EVERYTHING)) {
                            g_value_unset(value);
                        }

                        if ((direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
                                || (direction == GI_DIRECTION_OUT && transfer != GI_TRANSFER_NOTHING)) {
                            g_slice_free(GValue, value);
                        }
                        break;
                    }

                    /* TODO */

                    break;
                }
                case GI_INFO_TYPE_OBJECT:
                {
                    GObject *object;
                    object = arg->v_pointer;
                    if (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_EVERYTHING) {
                        g_object_unref(object);
                    }
                    break;
                }
                default:
                    /* TODO */
                    break;
            }

            g_base_info_unref(info);
            break;
        }
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        {
            GSList *list;

            list = arg->v_pointer;

            if ((direction == GI_DIRECTION_IN && transfer != GI_TRANSFER_EVERYTHING)
                    || (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_EVERYTHING)) {
                GITypeInfo *item_type_info;
                GITransfer item_transfer;
                GSList *item;

                item_type_info = g_type_info_get_param_type(type_info, 0);
                g_assert(item_type_info != NULL);

                if (direction == GI_DIRECTION_IN) {
                    item_transfer = GI_TRANSFER_NOTHING;
                } else {
                    item_transfer = GI_TRANSFER_EVERYTHING;
                }

                /* Free the items */
                for (item = list; item != NULL; item = g_slist_next(item)) {
                    _pygi_argument_release((GArgument *)&item->data, item_type_info,
                        item_transfer, direction);
                }

                g_base_info_unref((GIBaseInfo *)item_type_info);
            }

            if ((direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
                    || (direction == GI_DIRECTION_OUT && transfer != GI_TRANSFER_NOTHING)) {
                if (type_tag == GI_TYPE_TAG_GLIST) {
                    g_list_free((GList *)list);
                } else {
                    /* type_tag == GI_TYPE_TAG_GSLIST */
                    g_slist_free(list);
                }
            }

            break;
        }
        case GI_TYPE_TAG_GHASH:
        {
            GHashTable *hash_table;

            hash_table = arg->v_pointer;

            if (direction == GI_DIRECTION_IN && transfer != GI_TRANSFER_EVERYTHING) {
                /* We created the table without a destroy function, so keys and
                 * values need to be released. */
                GITypeInfo *key_type_info;
                GITypeInfo *value_type_info;
                GITransfer item_transfer;
                GHashTableIter hash_table_iter;
                gpointer key;
                gpointer value;

                key_type_info = g_type_info_get_param_type(type_info, 0);
                g_assert(key_type_info != NULL);

                value_type_info = g_type_info_get_param_type(type_info, 1);
                g_assert(value_type_info != NULL);

                if (direction == GI_DIRECTION_IN) {
                    item_transfer = GI_TRANSFER_NOTHING;
                } else {
                    item_transfer = GI_TRANSFER_EVERYTHING;
                }

                g_hash_table_iter_init(&hash_table_iter, hash_table);
                while (g_hash_table_iter_next(&hash_table_iter, &key, &value)) {
                    _pygi_argument_release((GArgument *)&key, key_type_info,
                        item_transfer, direction);
                    _pygi_argument_release((GArgument *)&value, value_type_info,
                        item_transfer, direction);
                }

                g_base_info_unref((GIBaseInfo *)key_type_info);
                g_base_info_unref((GIBaseInfo *)value_type_info);
            } else if (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_CONTAINER) {
                /* Be careful to avoid keys and values being freed if the
                 * callee gave a destroy function. */
                g_hash_table_steal_all(hash_table);
            }

            if ((direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
                    || (direction == GI_DIRECTION_OUT && transfer != GI_TRANSFER_NOTHING)) {
                g_hash_table_unref(hash_table);
            }

            break;
        }
        case GI_TYPE_TAG_ERROR:
        {
            GError **error;

            error = arg->v_pointer;

            if (*error != NULL) {
                g_error_free(*error);
            }
            g_slice_free(GError *, error);
            break;
        }
    }
}

void
_pygi_argument_init (void)
{
    PyDateTime_IMPORT;
}


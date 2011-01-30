/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   pygi-argument.c: GIArgument - PyObject conversion functions.
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
#include <pyglib-python-compat.h>

#include "pygi-cache.h"

static void
_pygi_g_type_tag_py_bounds (GITypeTag   type_tag,
                            PyObject  **lower,
                            PyObject  **upper)
{
    switch (type_tag) {
        case GI_TYPE_TAG_INT8:
            *lower = PYGLIB_PyLong_FromLong (-128);
            *upper = PYGLIB_PyLong_FromLong (127);
            break;
        case GI_TYPE_TAG_UINT8:
            *upper = PYGLIB_PyLong_FromLong (255);
            *lower = PYGLIB_PyLong_FromLong (0);
            break;
        case GI_TYPE_TAG_INT16:
            *lower = PYGLIB_PyLong_FromLong (-32768);
            *upper = PYGLIB_PyLong_FromLong (32767);
            break;
        case GI_TYPE_TAG_UINT16:
            *upper = PYGLIB_PyLong_FromLong (65535);
            *lower = PYGLIB_PyLong_FromLong (0);
            break;
        case GI_TYPE_TAG_INT32:
            *lower = PYGLIB_PyLong_FromLong (G_MININT32);
            *upper = PYGLIB_PyLong_FromLong (G_MAXINT32);
            break;
        case GI_TYPE_TAG_UINT32:
            /* Note: On 32-bit archs, this number doesn't fit in a long. */
            *upper = PyLong_FromLongLong (G_MAXUINT32);
            *lower = PYGLIB_PyLong_FromLong (0);
            break;
        case GI_TYPE_TAG_INT64:
            /* Note: On 32-bit archs, these numbers don't fit in a long. */
            *lower = PyLong_FromLongLong (G_MININT64);
            *upper = PyLong_FromLongLong (G_MAXINT64);
            break;
        case GI_TYPE_TAG_UINT64:
            *upper = PyLong_FromUnsignedLongLong (G_MAXUINT64);
            *lower = PYGLIB_PyLong_FromLong (0);
            break;
        case GI_TYPE_TAG_FLOAT:
            *upper = PyFloat_FromDouble (G_MAXFLOAT);
            *lower = PyFloat_FromDouble (-G_MAXFLOAT);
            break;
        case GI_TYPE_TAG_DOUBLE:
            *upper = PyFloat_FromDouble (G_MAXDOUBLE);
            *lower = PyFloat_FromDouble (-G_MAXDOUBLE);
            break;
        default:
            PyErr_SetString (PyExc_TypeError, "Non-numeric type tag");
            *lower = *upper = NULL;
            return;
    }
}

gint
_pygi_g_registered_type_info_check_object (GIRegisteredTypeInfo *info,
                                           gboolean              is_instance,
                                           PyObject             *object)
{
    gint retval;

    GType g_type;
    PyObject *py_type;
    gchar *type_name_expected = NULL;
    GIInfoType interface_type;

    interface_type = g_base_info_get_type (info);
    if ( (interface_type == GI_INFO_TYPE_STRUCT) &&
            (g_struct_info_is_foreign ( (GIStructInfo*) info))) {
        /* TODO: Could we check is the correct foreign type? */
        return 1;
    }

    g_type = g_registered_type_info_get_g_type (info);
    if (g_type != G_TYPE_NONE) {
        py_type = _pygi_type_get_from_g_type (g_type);
    } else {
        py_type = _pygi_type_import_by_gi_info ( (GIBaseInfo *) info);
    }

    if (py_type == NULL) {
        return 0;
    }

    g_assert (PyType_Check (py_type));

    if (is_instance) {
        retval = PyObject_IsInstance (object, py_type);
        if (!retval) {
            type_name_expected = _pygi_g_base_info_get_fullname (
                                     (GIBaseInfo *) info);
        }
    } else {
        if (!PyObject_Type (py_type)) {
            type_name_expected = "type";
            retval = 0;
        } else if (!PyType_IsSubtype ( (PyTypeObject *) object,
                                       (PyTypeObject *) py_type)) {
            type_name_expected = _pygi_g_base_info_get_fullname (
                                     (GIBaseInfo *) info);
            retval = 0;
        } else {
            retval = 1;
        }
    }

    Py_DECREF (py_type);

    if (!retval) {
        PyTypeObject *object_type;

        if (type_name_expected == NULL) {
            return -1;
        }

        object_type = (PyTypeObject *) PyObject_Type (object);
        if (object_type == NULL) {
            return -1;
        }

        PyErr_Format (PyExc_TypeError, "Must be %s, not %s",
                      type_name_expected, object_type->tp_name);

        g_free (type_name_expected);
    }

    return retval;
}

gint
_pygi_g_type_interface_check_object (GIBaseInfo *info,
                                     PyObject   *object)
{
    gint retval = 1;
    GIInfoType info_type;

    info_type = g_base_info_get_type (info);
    switch (info_type) {
        case GI_INFO_TYPE_CALLBACK:
            if (!PyCallable_Check (object)) {
                PyErr_Format (PyExc_TypeError, "Must be callable, not %s",
                              object->ob_type->tp_name);
                retval = 0;
            }
            break;
        case GI_INFO_TYPE_ENUM:
            retval = 0;
            if (PyNumber_Check (object)) {
                PyObject *number = PYGLIB_PyNumber_Long (object);
                if (number == NULL)
                    PyErr_Clear();
                else {
                    glong value = PYGLIB_PyLong_AsLong (number);
                    int i;
                    for (i = 0; i < g_enum_info_get_n_values (info); i++) {
                        GIValueInfo *value_info = g_enum_info_get_value (info, i);
                        glong enum_value = g_value_info_get_value (value_info);
                        if (value == enum_value) {
                            retval = 1;
                            break;
                        }
                    }
                }
            }
            if (retval < 1)
                retval = _pygi_g_registered_type_info_check_object (
                             (GIRegisteredTypeInfo *) info, TRUE, object);
            break;
        case GI_INFO_TYPE_FLAGS:
            if (PyNumber_Check (object)) {
                /* Accept 0 as a valid flag value */
                PyObject *number = PYGLIB_PyNumber_Long (object);
                if (number == NULL)
                    PyErr_Clear();
                else {
                    long value = PYGLIB_PyLong_AsLong (number);
                    if (value == 0)
                        break;
                    else if (value == -1)
                        PyErr_Clear();
                }
            }
            retval = _pygi_g_registered_type_info_check_object (
                         (GIRegisteredTypeInfo *) info, TRUE, object);
            break;
        case GI_INFO_TYPE_STRUCT:
        {
            GType type;

            /* Handle special cases. */
            type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *) info);
            if (g_type_is_a (type, G_TYPE_CLOSURE)) {
                if (!PyCallable_Check (object)) {
                    PyErr_Format (PyExc_TypeError, "Must be callable, not %s",
                                  object->ob_type->tp_name);
                    retval = 0;
                }
                break;
            } else if (g_type_is_a (type, G_TYPE_VALUE)) {
                /* we can't check g_values because we don't have
                 * enough context so just pass them through */
                break;
            }

            /* Fallback. */
        }
        case GI_INFO_TYPE_BOXED:
        case GI_INFO_TYPE_INTERFACE:
        case GI_INFO_TYPE_OBJECT:
        case GI_INFO_TYPE_UNION:
            retval = _pygi_g_registered_type_info_check_object ( (GIRegisteredTypeInfo *) info, TRUE, object);
            break;
        default:
            g_assert_not_reached();
    }

    return retval;
}

gint
_pygi_g_type_info_check_object (GITypeInfo *type_info,
                                PyObject   *object,
                                gboolean   allow_none)
{
    GITypeTag type_tag;
    gint retval = 1;

    if (allow_none && object == Py_None) {
        return retval;
    }

    type_tag = g_type_info_get_tag (type_info);

    switch (type_tag) {
        case GI_TYPE_TAG_VOID:
            /* No check; VOID means undefined type */
            break;
        case GI_TYPE_TAG_BOOLEAN:
            /* No check; every Python object has a truth value. */
            break;
        case GI_TYPE_TAG_UINT8:
            /* UINT8 types can be characters */
            if (PYGLIB_PyBytes_Check(object)) {
                if (PYGLIB_PyBytes_Size(object) != 1) {
                    PyErr_Format (PyExc_TypeError, "Must be a single character");
                    retval = 0;
                    break;
                }

                break;
            }
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
        case GI_TYPE_TAG_FLOAT:
        case GI_TYPE_TAG_DOUBLE:
        {
            PyObject *number, *lower, *upper;

            if (!PyNumber_Check (object)) {
                PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                              object->ob_type->tp_name);
                retval = 0;
                break;
            }

            if (type_tag == GI_TYPE_TAG_FLOAT || type_tag == GI_TYPE_TAG_DOUBLE) {
                number = PyNumber_Float (object);
            } else {
                number = PYGLIB_PyNumber_Long (object);
            }

            _pygi_g_type_tag_py_bounds (type_tag, &lower, &upper);

            if (lower == NULL || upper == NULL || number == NULL) {
                retval = -1;
                goto check_number_release;
            }

            /* Check bounds */
            if (PyObject_RichCompareBool (lower, number, Py_GT)
                    || PyObject_RichCompareBool (upper, number, Py_LT)) {
                PyObject *lower_str;
                PyObject *upper_str;

                if (PyErr_Occurred()) {
                    retval = -1;
                    goto check_number_release;
                }

                lower_str = PyObject_Str (lower);
                upper_str = PyObject_Str (upper);
                if (lower_str == NULL || upper_str == NULL) {
                    retval = -1;
                    goto check_number_error_release;
                }

#if PY_VERSION_HEX < 0x03000000
                PyErr_Format (PyExc_ValueError, "Must range from %s to %s",
                              PyString_AS_STRING (lower_str),
                              PyString_AS_STRING (upper_str));
#else
                {
                    PyObject *lower_pybytes_obj = PyUnicode_AsUTF8String (lower_str);
                    if (!lower_pybytes_obj)
                        goto utf8_fail;

                    PyObject *upper_pybytes_obj = PyUnicode_AsUTF8String (upper_str);                    
                    if (!upper_pybytes_obj) {
                        Py_DECREF(lower_pybytes_obj);
                        goto utf8_fail;
                    }

                    PyErr_Format (PyExc_ValueError, "Must range from %s to %s",
                                  PyBytes_AsString (lower_pybytes_obj),
                                  PyBytes_AsString (upper_pybytes_obj));
                    Py_DECREF (lower_pybytes_obj);
                    Py_DECREF (upper_pybytes_obj);
                }
utf8_fail:
#endif
                retval = 0;

check_number_error_release:
                Py_XDECREF (lower_str);
                Py_XDECREF (upper_str);
            }

check_number_release:
            Py_XDECREF (number);
            Py_XDECREF (lower);
            Py_XDECREF (upper);
            break;
        }
        case GI_TYPE_TAG_GTYPE:
        {
            if (pyg_type_from_object (object) == 0) {
                PyErr_Format (PyExc_TypeError, "Must be gobject.GType, not %s",
                              object->ob_type->tp_name);
                retval = 0;
            }
            break;
        }
        case GI_TYPE_TAG_UNICHAR:
        {
            Py_ssize_t size;
            if (PyUnicode_Check (object)) {
                size = PyUnicode_GET_SIZE (object);
#if PY_VERSION_HEX < 0x03000000
            } else if (PyString_Check (object)) {
                PyObject *pyuni = PyUnicode_FromEncodedObject (object, "UTF-8", "strict");
                size = PyUnicode_GET_SIZE (pyuni);
                Py_DECREF(pyuni);
#endif
            } else {
                PyErr_Format (PyExc_TypeError, "Must be string, not %s",
                              object->ob_type->tp_name);
                retval = 0;
                break;
            }

            if (size != 1) {
                PyErr_Format (PyExc_TypeError, "Must be a one character string, not %ld characters",
                              size);
                retval = 0;
                break;
            }

            break;
        }
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
            if (!PYGLIB_PyBaseString_Check (object) ) {
                PyErr_Format (PyExc_TypeError, "Must be string, not %s",
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

            if (!PySequence_Check (object)) {
                PyErr_Format (PyExc_TypeError, "Must be sequence, not %s",
                              object->ob_type->tp_name);
                retval = 0;
                break;
            }

            length = PySequence_Length (object);
            if (length < 0) {
                retval = -1;
                break;
            }

            fixed_size = g_type_info_get_array_fixed_size (type_info);
            if (fixed_size >= 0 && length != fixed_size) {
                PyErr_Format (PyExc_ValueError, "Must contain %zd items, not %zd",
                              fixed_size, length);
                retval = 0;
                break;
            }

            item_type_info = g_type_info_get_param_type (type_info, 0);
            g_assert (item_type_info != NULL);

            /* FIXME: This is insain.  We really should only check the first
             *        object and perhaps have a debugging mode.  Large arrays
             *        will cause apps to slow to a crawl.
             */
            for (i = 0; i < length; i++) {
                PyObject *item;

                item = PySequence_GetItem (object, i);
                if (item == NULL) {
                    retval = -1;
                    break;
                }

                retval = _pygi_g_type_info_check_object (item_type_info, item, TRUE);

                Py_DECREF (item);

                if (retval < 0) {
                    break;
                }
                if (!retval) {
                    _PyGI_ERROR_PREFIX ("Item %zd: ", i);
                    break;
                }
            }

            g_base_info_unref ( (GIBaseInfo *) item_type_info);

            break;
        }
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;

            info = g_type_info_get_interface (type_info);
            g_assert (info != NULL);

            retval = _pygi_g_type_interface_check_object(info, object);

            g_base_info_unref (info);
            break;
        }
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        {
            Py_ssize_t length;
            GITypeInfo *item_type_info;
            Py_ssize_t i;

            if (!PySequence_Check (object)) {
                PyErr_Format (PyExc_TypeError, "Must be sequence, not %s",
                              object->ob_type->tp_name);
                retval = 0;
                break;
            }

            length = PySequence_Length (object);
            if (length < 0) {
                retval = -1;
                break;
            }

            item_type_info = g_type_info_get_param_type (type_info, 0);
            g_assert (item_type_info != NULL);

            for (i = 0; i < length; i++) {
                PyObject *item;

                item = PySequence_GetItem (object, i);
                if (item == NULL) {
                    retval = -1;
                    break;
                }

                retval = _pygi_g_type_info_check_object (item_type_info, item, TRUE);

                Py_DECREF (item);

                if (retval < 0) {
                    break;
                }
                if (!retval) {
                    _PyGI_ERROR_PREFIX ("Item %zd: ", i);
                    break;
                }
            }

            g_base_info_unref ( (GIBaseInfo *) item_type_info);
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

            keys = PyMapping_Keys (object);
            if (keys == NULL) {
                PyErr_Format (PyExc_TypeError, "Must be mapping, not %s",
                              object->ob_type->tp_name);
                retval = 0;
                break;
            }

            length = PyMapping_Length (object);
            if (length < 0) {
                Py_DECREF (keys);
                retval = -1;
                break;
            }

            values = PyMapping_Values (object);
            if (values == NULL) {
                retval = -1;
                Py_DECREF (keys);
                break;
            }

            key_type_info = g_type_info_get_param_type (type_info, 0);
            g_assert (key_type_info != NULL);

            value_type_info = g_type_info_get_param_type (type_info, 1);
            g_assert (value_type_info != NULL);

            for (i = 0; i < length; i++) {
                PyObject *key;
                PyObject *value;

                key = PyList_GET_ITEM (keys, i);
                value = PyList_GET_ITEM (values, i);

                retval = _pygi_g_type_info_check_object (key_type_info, key, TRUE);
                if (retval < 0) {
                    break;
                }
                if (!retval) {
                    _PyGI_ERROR_PREFIX ("Key %zd :", i);
                    break;
                }

                retval = _pygi_g_type_info_check_object (value_type_info, value, TRUE);
                if (retval < 0) {
                    break;
                }
                if (!retval) {
                    _PyGI_ERROR_PREFIX ("Value %zd :", i);
                    break;
                }
            }

            g_base_info_unref ( (GIBaseInfo *) key_type_info);
            g_base_info_unref ( (GIBaseInfo *) value_type_info);
            Py_DECREF (values);
            Py_DECREF (keys);
            break;
        }
        case GI_TYPE_TAG_ERROR:
            PyErr_SetString (PyExc_NotImplementedError, "Error marshalling is not supported yet");
            /* TODO */
            break;
    }

    return retval;
}

GArray *
_pygi_argument_to_array (GIArgument  *arg,
                         GIArgument  *args[],
                         GITypeInfo *type_info,
                         gboolean is_method)
{
    GITypeInfo *item_type_info;
    gboolean is_zero_terminated;
    gsize item_size;
    gssize length;
    GArray *g_array;

    if (arg->v_pointer == NULL) {
        return NULL;
    }

    is_zero_terminated = g_type_info_is_zero_terminated (type_info);
    item_type_info = g_type_info_get_param_type (type_info, 0);

    item_size = _pygi_g_type_info_size (item_type_info);

    g_base_info_unref ( (GIBaseInfo *) item_type_info);

    if (is_zero_terminated) {
        length = g_strv_length (arg->v_pointer);
    } else {
        length = g_type_info_get_array_fixed_size (type_info);
        if (length < 0) {
            gint length_arg_pos;

            length_arg_pos = g_type_info_get_array_length (type_info);
            g_assert (length_arg_pos >= 0);

            /* FIXME: Take into account the type of the argument. */
            length = args[length_arg_pos]->v_int;
        }
    }

    g_assert (length >= 0);

    g_array = g_array_new (is_zero_terminated, FALSE, item_size);

    g_array->data = arg->v_pointer;
    g_array->len = length;

    return g_array;
}

GIArgument
_pygi_argument_from_object (PyObject   *object,
                            GITypeInfo *type_info,
                            GITransfer  transfer)
{
    GIArgument arg;
    GITypeTag type_tag;

    memset(&arg, 0, sizeof(GIArgument));
    type_tag = g_type_info_get_tag (type_info);

    switch (type_tag) {
        case GI_TYPE_TAG_VOID:
            g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
            arg.v_pointer = object;
            break;
        case GI_TYPE_TAG_BOOLEAN:
        {
            arg.v_boolean = PyObject_IsTrue (object);
            break;
        }
        case GI_TYPE_TAG_UINT8:
            if (PYGLIB_PyBytes_Check(object)) {
                arg.v_long = (long)(PYGLIB_PyBytes_AsString(object)[0]);
                break;
            }

        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_INT32:
        {
            PyObject *int_;

            int_ = PYGLIB_PyNumber_Long (object);
            if (int_ == NULL) {
                break;
            }

            arg.v_long = PYGLIB_PyLong_AsLong (int_);

            Py_DECREF (int_);

            break;
        }
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_UINT64:
        {
            PyObject *number;
            guint64 value;

            number = PYGLIB_PyNumber_Long (object);
            if (number == NULL) {
                break;
            }

#if PY_VERSION_HEX < 0x03000000
            if (PyInt_Check (number)) {
                value = PyInt_AS_LONG (number);
            } else
#endif
            value = PyLong_AsUnsignedLongLong (number);

            arg.v_uint64 = value;

            Py_DECREF (number);

            break;
        }
        case GI_TYPE_TAG_INT64:
        {
            PyObject *number;
            gint64 value;

            number = PYGLIB_PyNumber_Long (object);
            if (number == NULL) {
                break;
            }

#if PY_VERSION_HEX < 0x03000000
            if (PyInt_Check (number)) {
                value = PyInt_AS_LONG (number);
            } else
#endif
            value = PyLong_AsLongLong (number);

            arg.v_int64 = value;

            Py_DECREF (number);

            break;
        }
        case GI_TYPE_TAG_FLOAT:
        {
            PyObject *float_;

            float_ = PyNumber_Float (object);
            if (float_ == NULL) {
                break;
            }

            arg.v_float = (float) PyFloat_AsDouble (float_);
            Py_DECREF (float_);

            break;
        }
        case GI_TYPE_TAG_DOUBLE:
        {
            PyObject *float_;

            float_ = PyNumber_Float (object);
            if (float_ == NULL) {
                break;
            }

            arg.v_double = PyFloat_AsDouble (float_);
            Py_DECREF (float_);

            break;
        }
        case GI_TYPE_TAG_GTYPE:
        {
            arg.v_long = pyg_type_from_object (object);

            break;
        }
        case GI_TYPE_TAG_UNICHAR:
        {
            gchar *string;

            if (object == Py_None) {
                arg.v_uint32 = 0;
                break;
            }

#if PY_VERSION_HEX < 0x03000000
            if (PyUnicode_Check(object)) {
                 PyObject *pystr_obj = PyUnicode_AsUTF8String (object);

                 if (!pystr_obj)
                     break;

                 string = g_strdup(PyString_AsString (pystr_obj));
                 Py_DECREF(pystr_obj);
            } else {
                 string = g_strdup(PyString_AsString (object));
            }
#else
            {
                PyObject *pybytes_obj = PyUnicode_AsUTF8String (object);
                if (!pybytes_obj)
                    break;

                string = g_strdup(PyBytes_AsString (pybytes_obj));
                Py_DECREF (pybytes_obj);
            }
#endif

            arg.v_uint32 = g_utf8_get_char (string);

            break;
        }
        case GI_TYPE_TAG_UTF8:
        {
            gchar *string;

            if (object == Py_None) {
                arg.v_string = NULL;
                break;
            }
#if PY_VERSION_HEX < 0x03000000
            if (PyUnicode_Check(object)) {
                 PyObject *pystr_obj = PyUnicode_AsUTF8String (object);
                 
                 if (!pystr_obj)
                     break;

                 string = g_strdup(PyString_AsString (pystr_obj));
                 Py_DECREF(pystr_obj);
            } else {
                 string = g_strdup(PyString_AsString (object));
            }
#else
            {
                PyObject *pybytes_obj = PyUnicode_AsUTF8String (object);
                if (!pybytes_obj)
                    break;

                string = g_strdup(PyBytes_AsString (pybytes_obj));
                Py_DECREF (pybytes_obj);
            }
#endif
            arg.v_string = string;

            break;
        }
        case GI_TYPE_TAG_FILENAME:
        {
            GError *error = NULL;
            gchar *string;

#if PY_VERSION_HEX < 0x03000000
            string = g_strdup(PyString_AsString (object));
#else
            {
                PyObject *pybytes_obj = PyUnicode_AsUTF8String (object);
                if (!pybytes_obj)
                    break;

                string = g_strdup(PyBytes_AsString (pybytes_obj));
                Py_DECREF (pybytes_obj);
            }
#endif

            if (string == NULL) {
                break;
            }

            arg.v_string = g_filename_from_utf8 (string, -1, NULL, NULL, &error);
            g_free(string);

            if (arg.v_string == NULL) {
                PyErr_SetString (PyExc_Exception, error->message);
                /* TODO: Convert the error to an exception. */
            }

            break;
        }
        case GI_TYPE_TAG_ARRAY:
        {
            Py_ssize_t length;
            gboolean is_zero_terminated;
            GITypeInfo *item_type_info;
            gsize item_size;
            GArray *array;
            GITransfer item_transfer;
            Py_ssize_t i;

            if (object == Py_None) {
                arg.v_pointer = NULL;
                break;
            }

            length = PySequence_Length (object);
            if (length < 0) {
                break;
            }

            is_zero_terminated = g_type_info_is_zero_terminated (type_info);
            item_type_info = g_type_info_get_param_type (type_info, 0);

            item_size = _pygi_g_type_info_size (item_type_info);

            array = g_array_sized_new (is_zero_terminated, FALSE, item_size, length);
            if (array == NULL) {
                g_base_info_unref ( (GIBaseInfo *) item_type_info);
                PyErr_NoMemory();
                break;
            }

            if (g_type_info_get_tag (item_type_info) == GI_TYPE_TAG_UINT8 &&
                PYGLIB_PyBytes_Check(object)) {

                memcpy(array->data, PYGLIB_PyBytes_AsString(object), length);
                array->len = length;
                goto array_success;
            }


            item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

            for (i = 0; i < length; i++) {
                PyObject *py_item;
                GIArgument item;

                py_item = PySequence_GetItem (object, i);
                if (py_item == NULL) {
                    goto array_item_error;
                }

                item = _pygi_argument_from_object (py_item, item_type_info, item_transfer);

                Py_DECREF (py_item);

                if (PyErr_Occurred()) {
                    goto array_item_error;
                }

                g_array_insert_val (array, i, item);
                continue;

array_item_error:
                /* Free everything we have converted so far. */
                _pygi_argument_release ( (GIArgument *) &array, type_info,
                                         GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                array = NULL;

                _PyGI_ERROR_PREFIX ("Item %zd: ", i);
                break;
            }

array_success:
            arg.v_pointer = array;

            g_base_info_unref ( (GIBaseInfo *) item_type_info);
            break;
        }
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface (type_info);
            info_type = g_base_info_get_type (info);

            switch (info_type) {
                case GI_INFO_TYPE_CALLBACK:
                    /* This should be handled in invoke() */
                    g_assert_not_reached();
                    break;
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_STRUCT:
                case GI_INFO_TYPE_UNION:
                {
                    GType type;

                    if (object == Py_None) {
                        arg.v_pointer = NULL;
                        break;
                    }

                    type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *) info);

                    /* Handle special cases first. */
                    if (g_type_is_a (type, G_TYPE_VALUE)) {
                        GValue *value;
                        GType object_type;
                        gint retval;

                        object_type = pyg_type_from_object_strict ( (PyObject *) object->ob_type, FALSE);
                        if (object_type == G_TYPE_INVALID) {
                            PyErr_SetString (PyExc_RuntimeError, "unable to retrieve object's GType");
                            break;
                        }

                        g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);

                        value = g_slice_new0 (GValue);
                        g_value_init (value, object_type);

                        retval = pyg_value_from_pyobject (value, object);
                        if (retval < 0) {
                            g_slice_free (GValue, value);
                            PyErr_SetString (PyExc_RuntimeError, "PyObject conversion to GValue failed");
                            break;
                        }

                        arg.v_pointer = value;
                    } else if (g_type_is_a (type, G_TYPE_CLOSURE)) {
                        GClosure *closure;

                        g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);

                        closure = pyg_closure_new (object, NULL, NULL);
                        if (closure == NULL) {
                            PyErr_SetString (PyExc_RuntimeError, "PyObject conversion to GClosure failed");
                            break;
                        }

                        arg.v_pointer = closure;
                    } else if (g_type_is_a (type, G_TYPE_BOXED)) {
                        arg.v_pointer = pyg_boxed_get (object, void);
                        if (transfer == GI_TRANSFER_EVERYTHING) {
                            arg.v_pointer = g_boxed_copy (type, arg.v_pointer);
                        }
                    } else if ( (type == G_TYPE_NONE) && (g_struct_info_is_foreign (info))) {
                        PyObject *result;
                        result = pygi_struct_foreign_convert_to_g_argument (
                                     object, type_info, transfer, &arg);
                    } else if (g_type_is_a (type, G_TYPE_POINTER) || type == G_TYPE_NONE) {
                        g_warn_if_fail (!g_type_info_is_pointer (type_info) || transfer == GI_TRANSFER_NOTHING);
                        arg.v_pointer = pyg_pointer_get (object, void);
                    } else {
                        PyErr_Format (PyExc_NotImplementedError, "structure type '%s' is not supported yet", g_type_name (type));
                    }

                    break;
                }
                case GI_INFO_TYPE_ENUM:
                case GI_INFO_TYPE_FLAGS:
                {
                    PyObject *int_;

                    int_ = PYGLIB_PyNumber_Long (object);
                    if (int_ == NULL) {
                        break;
                    }

                    arg.v_long = PYGLIB_PyLong_AsLong (int_);

                    Py_DECREF (int_);

                    break;
                }
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_OBJECT:
                    if (object == Py_None) {
                        arg.v_pointer = NULL;
                        break;
                    }

                    arg.v_pointer = pygobject_get (object);
                    if (transfer == GI_TRANSFER_EVERYTHING) {
                        g_object_ref (arg.v_pointer);
                    }

                    break;
                default:
                    g_assert_not_reached();
            }
            g_base_info_unref (info);
            break;
        }
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        {
            Py_ssize_t length;
            GITypeInfo *item_type_info;
            GSList *list = NULL;
            GITransfer item_transfer;
            Py_ssize_t i;

            if (object == Py_None) {
                arg.v_pointer = NULL;
                break;
            }

            length = PySequence_Length (object);
            if (length < 0) {
                break;
            }

            item_type_info = g_type_info_get_param_type (type_info, 0);
            g_assert (item_type_info != NULL);

            item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

            for (i = length - 1; i >= 0; i--) {
                PyObject *py_item;
                GIArgument item;

                py_item = PySequence_GetItem (object, i);
                if (py_item == NULL) {
                    goto list_item_error;
                }

                item = _pygi_argument_from_object (py_item, item_type_info, item_transfer);

                Py_DECREF (py_item);

                if (PyErr_Occurred()) {
                    goto list_item_error;
                }

                if (type_tag == GI_TYPE_TAG_GLIST) {
                    list = (GSList *) g_list_prepend ( (GList *) list, item.v_pointer);
                } else {
                    list = g_slist_prepend (list, item.v_pointer);
                }

                continue;

list_item_error:
                /* Free everything we have converted so far. */
                _pygi_argument_release ( (GIArgument *) &list, type_info,
                                         GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                list = NULL;

                _PyGI_ERROR_PREFIX ("Item %zd: ", i);
                break;
            }

            arg.v_pointer = list;

            g_base_info_unref ( (GIBaseInfo *) item_type_info);

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
            GITransfer item_transfer;
            Py_ssize_t i;


            if (object == Py_None) {
                arg.v_pointer = NULL;
                break;
            }

            length = PyMapping_Length (object);
            if (length < 0) {
                break;
            }

            keys = PyMapping_Keys (object);
            if (keys == NULL) {
                break;
            }

            values = PyMapping_Values (object);
            if (values == NULL) {
                Py_DECREF (keys);
                break;
            }

            key_type_info = g_type_info_get_param_type (type_info, 0);
            g_assert (key_type_info != NULL);

            value_type_info = g_type_info_get_param_type (type_info, 1);
            g_assert (value_type_info != NULL);

            key_type_tag = g_type_info_get_tag (key_type_info);

            switch (key_type_tag) {
                case GI_TYPE_TAG_UTF8:
                case GI_TYPE_TAG_FILENAME:
                    hash_func = g_str_hash;
                    equal_func = g_str_equal;
                    break;
                default:
                    hash_func = NULL;
                    equal_func = NULL;
            }

            hash_table = g_hash_table_new (hash_func, equal_func);
            if (hash_table == NULL) {
                PyErr_NoMemory();
                goto hash_table_release;
            }

            item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

            for (i = 0; i < length; i++) {
                PyObject *py_key;
                PyObject *py_value;
                GIArgument key;
                GIArgument value;

                py_key = PyList_GET_ITEM (keys, i);
                py_value = PyList_GET_ITEM (values, i);

                key = _pygi_argument_from_object (py_key, key_type_info, item_transfer);
                if (PyErr_Occurred()) {
                    goto hash_table_item_error;
                }

                value = _pygi_argument_from_object (py_value, value_type_info, item_transfer);
                if (PyErr_Occurred()) {
                    _pygi_argument_release (&key, type_info, GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                    goto hash_table_item_error;
                }

                g_hash_table_insert (hash_table, key.v_pointer, value.v_pointer);
                continue;

hash_table_item_error:
                /* Free everything we have converted so far. */
                _pygi_argument_release ( (GIArgument *) &hash_table, type_info,
                                         GI_TRANSFER_NOTHING, GI_DIRECTION_IN);
                hash_table = NULL;

                _PyGI_ERROR_PREFIX ("Item %zd: ", i);
                break;
            }

            arg.v_pointer = hash_table;

hash_table_release:
            g_base_info_unref ( (GIBaseInfo *) key_type_info);
            g_base_info_unref ( (GIBaseInfo *) value_type_info);
            Py_DECREF (keys);
            Py_DECREF (values);
            break;
        }
        case GI_TYPE_TAG_ERROR:
            PyErr_SetString (PyExc_NotImplementedError, "error marshalling is not supported yet");
            /* TODO */
            break;
    }

    return arg;
}

PyObject *
_pygi_argument_to_object (GIArgument  *arg,
                          GITypeInfo *type_info,
                          GITransfer transfer)
{
    GITypeTag type_tag;
    PyObject *object = NULL;

    type_tag = g_type_info_get_tag (type_info);
    switch (type_tag) {
        case GI_TYPE_TAG_VOID:
            if (g_type_info_is_pointer (type_info)) {
                /* Raw Python objects are passed to void* args */
                g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
                object = arg->v_pointer;
            } else
                object = Py_None;
            Py_XINCREF (object);
            break;
        case GI_TYPE_TAG_BOOLEAN:
        {
            object = PyBool_FromLong (arg->v_boolean);
            break;
        }
        case GI_TYPE_TAG_INT8:
        {
            object = PYGLIB_PyLong_FromLong (arg->v_int8);
            break;
        }
        case GI_TYPE_TAG_UINT8:
        {
            object = PYGLIB_PyLong_FromLong (arg->v_uint8);
            break;
        }
        case GI_TYPE_TAG_INT16:
        {
            object = PYGLIB_PyLong_FromLong (arg->v_int16);
            break;
        }
        case GI_TYPE_TAG_UINT16:
        {
            object = PYGLIB_PyLong_FromLong (arg->v_uint16);
            break;
        }
        case GI_TYPE_TAG_INT32:
        {
            object = PYGLIB_PyLong_FromLong (arg->v_int32);
            break;
        }
        case GI_TYPE_TAG_UINT32:
        {
            object = PyLong_FromLongLong (arg->v_uint32);
            break;
        }
        case GI_TYPE_TAG_INT64:
        {
            object = PyLong_FromLongLong (arg->v_int64);
            break;
        }
        case GI_TYPE_TAG_UINT64:
        {
            object = PyLong_FromUnsignedLongLong (arg->v_uint64);
            break;
        }
        case GI_TYPE_TAG_FLOAT:
        {
            object = PyFloat_FromDouble (arg->v_float);
            break;
        }
        case GI_TYPE_TAG_DOUBLE:
        {
            object = PyFloat_FromDouble (arg->v_double);
            break;
        }
        case GI_TYPE_TAG_GTYPE:
        {
            object = pyg_type_wrapper_new ( (GType) arg->v_long);
            break;
        }
        case GI_TYPE_TAG_UNICHAR:
        {
            /* Preserve the bidirectional mapping between 0 and "" */
            if (arg->v_uint32 == 0) {
                object = PYGLIB_PyUnicode_FromString ("");
            } else if (g_unichar_validate (arg->v_uint32)) {
                gchar utf8[6];
                gint bytes;

                bytes = g_unichar_to_utf8 (arg->v_uint32, utf8);
                object = PYGLIB_PyUnicode_FromStringAndSize ((char*)utf8, bytes);
            } else {
                /* TODO: Convert the error to an exception. */
                PyErr_Format (PyExc_TypeError,
                              "Invalid unicode codepoint %" G_GUINT32_FORMAT,
                              arg->v_uint32);
                object = Py_None;
                Py_INCREF (object);
            }
            break;
        }
        case GI_TYPE_TAG_UTF8:
            if (arg->v_string == NULL) {
                object = Py_None;
                Py_INCREF (object);
                break;
            }

            object = PYGLIB_PyUnicode_FromString (arg->v_string);
            break;
        case GI_TYPE_TAG_FILENAME:
        {
            GError *error = NULL;
            gchar *string;

            if (arg->v_string == NULL) {
                object = Py_None;
                Py_INCREF (object);
                break;
            }

            string = g_filename_to_utf8 (arg->v_string, -1, NULL, NULL, &error);
            if (string == NULL) {
                PyErr_SetString (PyExc_Exception, error->message);
                /* TODO: Convert the error to an exception. */
                break;
            }

            object = PYGLIB_PyUnicode_FromString (string);

            g_free (string);

            break;
        }
        case GI_TYPE_TAG_ARRAY:
        {
            GArray *array;
            GITypeInfo *item_type_info;
            GITypeTag item_type_tag;
            GITransfer item_transfer;
            gsize i, item_size;

            array = arg->v_pointer;

            item_type_info = g_type_info_get_param_type (type_info, 0);
            g_assert (item_type_info != NULL);

            item_type_tag = g_type_info_get_tag (item_type_info);
            item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

            if (item_type_tag == GI_TYPE_TAG_UINT8) {
                /* Return as a byte array */
                if (arg->v_pointer == NULL) {
                    object = PYGLIB_PyBytes_FromString ("");
                    break;
                }

                object = PYGLIB_PyBytes_FromStringAndSize(array->data, array->len);
                break;

            } else {
                if (arg->v_pointer == NULL) {
                    object = PyList_New (0);
                    break;
                }

                object = PyList_New (array->len);
                if (object == NULL) {
                    break;
                }

            }
            item_size = g_array_get_element_size (array);

            for (i = 0; i < array->len; i++) {
                GIArgument item;
                PyObject *py_item;
                gboolean is_struct = FALSE;

                if (item_type_tag == GI_TYPE_TAG_INTERFACE) {
                    GIBaseInfo *iface_info = g_type_info_get_interface (item_type_info);
                    switch (g_base_info_get_type (iface_info)) {
                        case GI_INFO_TYPE_STRUCT:
                        case GI_INFO_TYPE_BOXED:
                            is_struct = TRUE;
                        default:
                            break;
                    }
                    g_base_info_unref ( (GIBaseInfo *) iface_info);
                }

                if (is_struct) {
                    item.v_pointer = &_g_array_index (array, GIArgument, i);
                } else {
                    memcpy (&item, &_g_array_index (array, GIArgument, i), item_size);
                }

                py_item = _pygi_argument_to_object (&item, item_type_info, item_transfer);
                if (py_item == NULL) {
                    Py_CLEAR (object);
                    _PyGI_ERROR_PREFIX ("Item %zu: ", i);
                    break;
                }

                PyList_SET_ITEM (object, i, py_item);
            }

            g_base_info_unref ( (GIBaseInfo *) item_type_info);
            break;
        }
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface (type_info);
            info_type = g_base_info_get_type (info);

            switch (info_type) {
                case GI_INFO_TYPE_CALLBACK:
                {
                    /* There is no way we can support a callback return
                     * as we are never sure if the callback was set from C
                     * or Python.  API that return callbacks are broken
                     * so we print a warning and send back a None
                     */

                    g_warning ("You are trying to use an API which returns a callback."
                               "Callback returns can not be supported. Returning None instead.");
                    object = Py_None;
                    Py_INCREF (object);
                    break;
                }
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_STRUCT:
                case GI_INFO_TYPE_UNION:
                {
                    GType type;

                    if (arg->v_pointer == NULL) {
                        object = Py_None;
                        Py_INCREF (object);
                        break;
                    }

                    type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *) info);
                    if (g_type_is_a (type, G_TYPE_VALUE)) {
                        object = pyg_value_as_pyobject (arg->v_pointer, FALSE);
                    } else if (g_type_is_a (type, G_TYPE_BOXED)) {
                        PyObject *py_type;

                        py_type = _pygi_type_get_from_g_type (type);
                        if (py_type == NULL)
                            break;

                        object = _pygi_boxed_new ( (PyTypeObject *) py_type, arg->v_pointer, transfer == GI_TRANSFER_EVERYTHING);

                        Py_DECREF (py_type);
                    } else if (g_type_is_a (type, G_TYPE_POINTER)) {
                        PyObject *py_type;

                        py_type = _pygi_type_get_from_g_type (type);

                        if (py_type == NULL || !PyType_IsSubtype ( (PyTypeObject *) type, &PyGIStruct_Type)) {
                            g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
                            object = pyg_pointer_new (type, arg->v_pointer);
                        } else {
                            object = _pygi_struct_new ( (PyTypeObject *) py_type, arg->v_pointer, transfer == GI_TRANSFER_EVERYTHING);
                        }

                        Py_XDECREF (py_type);
                    } else if ( (type == G_TYPE_NONE) && (g_struct_info_is_foreign (info))) {
                        object = pygi_struct_foreign_convert_from_g_argument (type_info, arg->v_pointer);
                    } else if (type == G_TYPE_NONE) {
                        PyObject *py_type;

                        py_type = _pygi_type_import_by_gi_info (info);
                        if (py_type == NULL) {
                            break;
                        }

                        /* Only structs created in invoke can be safely marked
                         * GI_TRANSFER_EVERYTHING. Trust that invoke has
                         * filtered correctly
                         */
                        object = _pygi_struct_new ( (PyTypeObject *) py_type, arg->v_pointer,
                                                    transfer == GI_TRANSFER_EVERYTHING);

                        Py_DECREF (py_type);
                    } else {
                        PyErr_Format (PyExc_NotImplementedError, "structure type '%s' is not supported yet", g_type_name (type));
                    }

                    break;
                }
                case GI_INFO_TYPE_ENUM:
                case GI_INFO_TYPE_FLAGS:
                {
                    GType type;

                    type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *) info);

                    if (type == G_TYPE_NONE) {
                        /* An enum with a GType of None is an enum without GType */
                        PyObject *py_type = _pygi_type_import_by_gi_info (info);
                        PyObject *py_args = NULL;

                        if (!py_type)
                            return NULL;

                        py_args = PyTuple_New (1);
                        if (PyTuple_SetItem (py_args, 0, PyLong_FromLong (arg->v_long)) != 0) {
                            Py_DECREF (py_args);
                            Py_DECREF (py_type);
                            return NULL;
                        }

                        object = PyObject_CallFunction (py_type, "l", arg->v_long);

                        Py_DECREF (py_args);
                        Py_DECREF (py_type);

                    } else if (info_type == GI_INFO_TYPE_ENUM) {
                        object = pyg_enum_from_gtype (type, arg->v_long);
                    } else {
                        object = pyg_flags_from_gtype (type, arg->v_long);
                    }

                    break;
                }
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_OBJECT:
                    if (arg->v_pointer == NULL) {
                        object = Py_None;
                        Py_INCREF (object);
                        break;
                    }
                    object = pygobject_new (arg->v_pointer);
                    break;
                default:
                    g_assert_not_reached();
            }

            g_base_info_unref (info);
            break;
        }
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        {
            GSList *list;
            gsize length;
            GITypeInfo *item_type_info;
            GITransfer item_transfer;
            gsize i;

            list = arg->v_pointer;
            length = g_slist_length (list);

            object = PyList_New (length);
            if (object == NULL) {
                break;
            }

            item_type_info = g_type_info_get_param_type (type_info, 0);
            g_assert (item_type_info != NULL);

            item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

            for (i = 0; list != NULL; list = g_slist_next (list), i++) {
                GIArgument item;
                PyObject *py_item;

                item.v_pointer = list->data;

                py_item = _pygi_argument_to_object (&item, item_type_info, item_transfer);
                if (py_item == NULL) {
                    Py_CLEAR (object);
                    _PyGI_ERROR_PREFIX ("Item %zu: ", i);
                    break;
                }

                PyList_SET_ITEM (object, i, py_item);
            }

            g_base_info_unref ( (GIBaseInfo *) item_type_info);
            break;
        }
        case GI_TYPE_TAG_GHASH:
        {
            GITypeInfo *key_type_info;
            GITypeInfo *value_type_info;
            GITransfer item_transfer;
            GHashTableIter hash_table_iter;
            GIArgument key;
            GIArgument value;

            if (arg->v_pointer == NULL) {
                object = Py_None;
                Py_INCREF (object);
                break;
            }

            object = PyDict_New();
            if (object == NULL) {
                break;
            }

            key_type_info = g_type_info_get_param_type (type_info, 0);
            g_assert (key_type_info != NULL);
            g_assert (g_type_info_get_tag (key_type_info) != GI_TYPE_TAG_VOID);

            value_type_info = g_type_info_get_param_type (type_info, 1);
            g_assert (value_type_info != NULL);
            g_assert (g_type_info_get_tag (value_type_info) != GI_TYPE_TAG_VOID);

            item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;

            g_hash_table_iter_init (&hash_table_iter, (GHashTable *) arg->v_pointer);
            while (g_hash_table_iter_next (&hash_table_iter, &key.v_pointer, &value.v_pointer)) {
                PyObject *py_key;
                PyObject *py_value;
                int retval;

                py_key = _pygi_argument_to_object (&key, key_type_info, item_transfer);
                if (py_key == NULL) {
                    break;
                }

                py_value = _pygi_argument_to_object (&value, value_type_info, item_transfer);
                if (py_value == NULL) {
                    Py_DECREF (py_key);
                    break;
                }

                retval = PyDict_SetItem (object, py_key, py_value);

                Py_DECREF (py_key);
                Py_DECREF (py_value);

                if (retval < 0) {
                    Py_CLEAR (object);
                    break;
                }
            }

            g_base_info_unref ( (GIBaseInfo *) key_type_info);
            g_base_info_unref ( (GIBaseInfo *) value_type_info);
            break;
        }
        case GI_TYPE_TAG_ERROR:
            /* Errors should be handled in the invoke wrapper. */
            g_assert_not_reached();
    }

    return object;
}

void
_pygi_argument_release (GIArgument   *arg,
                        GITypeInfo  *type_info,
                        GITransfer   transfer,
                        GIDirection  direction)
{
    GITypeTag type_tag;
    gboolean is_out = (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT);

    type_tag = g_type_info_get_tag (type_info);

    switch (type_tag) {
        case GI_TYPE_TAG_VOID:
            /* Don't do anything, it's transparent to the C side */
            break;
        case GI_TYPE_TAG_BOOLEAN:
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_UINT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
        case GI_TYPE_TAG_FLOAT:
        case GI_TYPE_TAG_DOUBLE:
        case GI_TYPE_TAG_GTYPE:
        case GI_TYPE_TAG_UNICHAR:
            break;
        case GI_TYPE_TAG_FILENAME:
        case GI_TYPE_TAG_UTF8:
            /* With allow-none support the string could be NULL */
            if ((arg->v_string != NULL &&
                    (direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING))
                    || (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_EVERYTHING)) {
                g_free (arg->v_string);
            }
            break;
        case GI_TYPE_TAG_ARRAY:
        {
            GArray *array;
            gsize i;

            if (arg->v_pointer == NULL) {
                return;
            }

            array = arg->v_pointer;

            if ( (direction == GI_DIRECTION_IN && transfer != GI_TRANSFER_EVERYTHING)
                    || (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_EVERYTHING)) {
                GITypeInfo *item_type_info;
                GITransfer item_transfer;

                item_type_info = g_type_info_get_param_type (type_info, 0);

                item_transfer = direction == GI_DIRECTION_IN ? GI_TRANSFER_NOTHING : GI_TRANSFER_EVERYTHING;

                /* Free the items */
                for (i = 0; i < array->len; i++) {
                    GIArgument *item;
                    item = &_g_array_index (array, GIArgument, i);
                    _pygi_argument_release (item, item_type_info, item_transfer, direction);
                }

                g_base_info_unref ( (GIBaseInfo *) item_type_info);
            }

            if ( (direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
                    || (direction == GI_DIRECTION_OUT && transfer != GI_TRANSFER_NOTHING)) {
                g_array_free (array, TRUE);
            }

            break;
        }
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface (type_info);
            info_type = g_base_info_get_type (info);

            switch (info_type) {
                case GI_INFO_TYPE_CALLBACK:
                    /* TODO */
                    break;
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_STRUCT:
                case GI_INFO_TYPE_UNION:
                {
                    GType type;

                    if (arg->v_pointer == NULL) {
                        return;
                    }

                    type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *) info);

                    if (g_type_is_a (type, G_TYPE_VALUE)) {
                        GValue *value;

                        value = arg->v_pointer;

                        if ( (direction == GI_DIRECTION_IN && transfer != GI_TRANSFER_EVERYTHING)
                                || (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_EVERYTHING)) {
                            g_value_unset (value);
                        }

                        if ( (direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
                                || (direction == GI_DIRECTION_OUT && transfer != GI_TRANSFER_NOTHING)) {
                            g_slice_free (GValue, value);
                        }
                    } else if (g_type_is_a (type, G_TYPE_CLOSURE)) {
                        if (direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING) {
                            g_closure_unref (arg->v_pointer);
                        }
                    } else if (g_struct_info_is_foreign ( (GIStructInfo*) info)) {
                        if (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_EVERYTHING) {
                            pygi_struct_foreign_release (info, arg->v_pointer);
                        }
                    } else if (g_type_is_a (type, G_TYPE_BOXED)) {
                    } else if (g_type_is_a (type, G_TYPE_POINTER) || type == G_TYPE_NONE) {
                        g_warn_if_fail (!g_type_info_is_pointer (type_info) || transfer == GI_TRANSFER_NOTHING);
                    }

                    break;
                }
                case GI_INFO_TYPE_ENUM:
                case GI_INFO_TYPE_FLAGS:
                    break;
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_OBJECT:
                    if (arg->v_pointer == NULL) {
                        return;
                    }
                    if (is_out && transfer == GI_TRANSFER_EVERYTHING) {
                        g_object_unref (arg->v_pointer);
                    }
                    break;
                default:
                    g_assert_not_reached();
            }

            g_base_info_unref (info);
            break;
        }
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        {
            GSList *list;

            if (arg->v_pointer == NULL) {
                return;
            }

            list = arg->v_pointer;

            if ( (direction == GI_DIRECTION_IN && transfer != GI_TRANSFER_EVERYTHING)
                    || (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_EVERYTHING)) {
                GITypeInfo *item_type_info;
                GITransfer item_transfer;
                GSList *item;

                item_type_info = g_type_info_get_param_type (type_info, 0);
                g_assert (item_type_info != NULL);

                item_transfer = direction == GI_DIRECTION_IN ? GI_TRANSFER_NOTHING : GI_TRANSFER_EVERYTHING;

                /* Free the items */
                for (item = list; item != NULL; item = g_slist_next (item)) {
                    _pygi_argument_release ( (GIArgument *) &item->data, item_type_info,
                                             item_transfer, direction);
                }

                g_base_info_unref ( (GIBaseInfo *) item_type_info);
            }

            if ( (direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
                    || (direction == GI_DIRECTION_OUT && transfer != GI_TRANSFER_NOTHING)) {
                if (type_tag == GI_TYPE_TAG_GLIST) {
                    g_list_free ( (GList *) list);
                } else {
                    /* type_tag == GI_TYPE_TAG_GSLIST */
                    g_slist_free (list);
                }
            }

            break;
        }
        case GI_TYPE_TAG_GHASH:
        {
            GHashTable *hash_table;

            if (arg->v_pointer == NULL) {
                return;
            }

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

                key_type_info = g_type_info_get_param_type (type_info, 0);
                g_assert (key_type_info != NULL);

                value_type_info = g_type_info_get_param_type (type_info, 1);
                g_assert (value_type_info != NULL);

                if (direction == GI_DIRECTION_IN) {
                    item_transfer = GI_TRANSFER_NOTHING;
                } else {
                    item_transfer = GI_TRANSFER_EVERYTHING;
                }

                g_hash_table_iter_init (&hash_table_iter, hash_table);
                while (g_hash_table_iter_next (&hash_table_iter, &key, &value)) {
                    _pygi_argument_release ( (GIArgument *) &key, key_type_info,
                                             item_transfer, direction);
                    _pygi_argument_release ( (GIArgument *) &value, value_type_info,
                                             item_transfer, direction);
                }

                g_base_info_unref ( (GIBaseInfo *) key_type_info);
                g_base_info_unref ( (GIBaseInfo *) value_type_info);
            } else if (direction == GI_DIRECTION_OUT && transfer == GI_TRANSFER_CONTAINER) {
                /* Be careful to avoid keys and values being freed if the
                 * callee gave a destroy function. */
                g_hash_table_steal_all (hash_table);
            }

            if ( (direction == GI_DIRECTION_IN && transfer == GI_TRANSFER_NOTHING)
                    || (direction == GI_DIRECTION_OUT && transfer != GI_TRANSFER_NOTHING)) {
                g_hash_table_unref (hash_table);
            }

            break;
        }
        case GI_TYPE_TAG_ERROR:
        {
            GError *error;

            if (arg->v_pointer == NULL) {
                return;
            }

            error = * (GError **) arg->v_pointer;

            if (error != NULL) {
                g_error_free (error);
            }

            g_slice_free (GError *, arg->v_pointer);
            break;
        }
    }
}

void
_pygi_argument_init (void)
{
    PyDateTime_IMPORT;
    _pygobject_import();
}

/*** argument marshaling and validating routines ***/

gboolean
_pygi_marshal_in_void (PyGIInvokeState   *state,
                       PyGIFunctionCache *function_cache,
                       PyGIArgCache      *arg_cache,
                       PyObject          *py_arg,
                       GIArgument        *arg)
{
    g_warn_if_fail (arg_cache->transfer == GI_TRANSFER_NOTHING);

    arg->v_pointer = py_arg;

    return TRUE;
}

gboolean
_pygi_marshal_in_boolean (PyGIInvokeState   *state,
                          PyGIFunctionCache *function_cache,
                          PyGIArgCache      *arg_cache,
                          PyObject          *py_arg,
                          GIArgument        *arg)
{
    arg->v_boolean = PyObject_IsTrue(py_arg);

    return TRUE;
}

gboolean
_pygi_marshal_in_int8 (PyGIInvokeState   *state,
                       PyGIFunctionCache *function_cache,
                       PyGIArgCache      *arg_cache,
                       PyObject          *py_arg,
                       GIArgument        *arg)
{
    PyObject *py_long;
    long long_;

    if (!PyNumber_Check(py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long(py_arg);
    if (!py_long)
        return FALSE;

    long_ = PYGLIB_PyLong_AsLong(py_long);
    Py_DECREF(py_long);

    if (PyErr_Occurred())
        return FALSE;

    if (long_ < -128 || long_ > 127) {
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, -128, 127);
        return FALSE;
    }

    arg->v_long = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_uint8 (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        PyObject          *py_arg,
                        GIArgument        *arg)
{
    unsigned long long_;

    if (PYGLIB_PyBytes_Check(py_arg)) {

        if (PYGLIB_PyBytes_Size(py_arg) != 1) {
            PyErr_Format (PyExc_TypeError, "Must be a single character");
            return FALSE;
        }

        long_ = (unsigned char)(PYGLIB_PyBytes_AsString(py_arg)[0]);

    } else if (PyNumber_Check(py_arg)) {
        PyObject *py_long;
        py_long = PYGLIB_PyNumber_Long(py_arg);
        if (!py_long)
            return FALSE;

        long_ = PYGLIB_PyLong_AsLong(py_long);
        Py_DECREF(py_long);

        if (PyErr_Occurred())
            return FALSE;
    } else {
        PyErr_Format (PyExc_TypeError, "Must be number or single byte string, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    if (long_ < 0 || long_ > 255) {
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, 0, 255);
        return FALSE;
    }

    arg->v_long = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_int16 (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        PyObject          *py_arg,
                        GIArgument        *arg)
{
    PyObject *py_long;
    long long_;

    if (!PyNumber_Check(py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long(py_arg);
    if (!py_long)
        return FALSE;

    long_ = PYGLIB_PyLong_AsLong(py_long);
    Py_DECREF(py_long);

    if (PyErr_Occurred())
        return FALSE;

    if (long_ < -32768 || long_ > 32767) {
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, -32768, 32767);
        return FALSE;
    }

    arg->v_long = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_uint16 (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         PyObject          *py_arg,
                         GIArgument        *arg)
{
    PyObject *py_long;
    long long_;

    if (!PyNumber_Check(py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long(py_arg);
    if (!py_long)
        return FALSE;

    long_ = PYGLIB_PyLong_AsLong(py_long);
    Py_DECREF(py_long);

    if (PyErr_Occurred())
        return FALSE;

    if (long_ < 0 || long_ > 65535) {
        PyErr_Format (PyExc_ValueError, "%li not in range %d to %d", long_, 0, 65535);
        return FALSE;
    }

    arg->v_long = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_int32 (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        PyObject          *py_arg,
                        GIArgument        *arg)
{
    PyObject *py_long;
    long long_;

    if (!PyNumber_Check(py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long(py_arg);
    if (!py_long)
        return FALSE;

    long_ = PYGLIB_PyLong_AsLong(py_long);
    Py_DECREF(py_long);

    if (PyErr_Occurred())
        return FALSE;

    if (long_ < G_MININT32 || long_ > G_MAXINT32) {
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, G_MININT32, G_MAXINT32);
        return FALSE;
    }

    arg->v_long = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_uint32 (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         PyObject          *py_arg,
                         GIArgument        *arg)
{
    PyObject *py_long;
    long long long_;

    if (!PyNumber_Check(py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long(py_arg);
    if (!py_long)
        return FALSE;

#if PY_VERSION_HEX < 0x03000000
    if (PyInt_Check (py_long))
        long_ = PyInt_AsLong (py_long);
    else
#endif
        long_ = PyLong_AsLongLong(py_long);

    Py_DECREF(py_long);

    if (PyErr_Occurred())
        return FALSE;

    if (long_ < 0 || long_ > G_MAXUINT32) {
        PyErr_Format (PyExc_ValueError, "%lli not in range %i to %u", long_, 0, G_MAXUINT32);
        return FALSE;
    }

    arg->v_uint64 = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_int64 (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        PyObject          *py_arg,
                        GIArgument        *arg)
{
    PyObject *py_long;
    long long long_;

    if (!PyNumber_Check(py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long(py_arg);
    if (!py_long)
        return FALSE;

#if PY_VERSION_HEX < 0x03000000
    if (PyInt_Check (py_long))
        long_ = PyInt_AS_LONG (py_long);
    else
#endif
        long_ = PyLong_AsLongLong(py_long);

    Py_DECREF(py_long);

    if (PyErr_Occurred()) {
        /* OverflowError occured but range errors should be returned as ValueError */
        char *long_str;
        PyObject *py_str = PyObject_Str(py_long);

        if (PyUnicode_Check(py_str)) {
            PyObject *py_bytes = PyUnicode_AsUTF8String(py_str);
            if (py_bytes == NULL)
                return FALSE;

            long_str = g_strdup(PYGLIB_PyBytes_AsString(py_bytes));
            if (long_str == NULL) {
                PyErr_NoMemory();
                return FALSE;
            }

            Py_DECREF(py_bytes);
        } else {
            long_str = g_strdup(PYGLIB_PyBytes_AsString(py_str));
        }

        Py_DECREF(py_str);
        PyErr_Clear();
        PyErr_Format(PyExc_ValueError, "%s not in range %ld to %ld",
                     long_str, G_MININT64, G_MAXINT64);

        g_free(long_str);
        return FALSE;
    }

    if (long_ < G_MININT64 || long_ > G_MAXINT64) {
        PyErr_Format (PyExc_ValueError, "%lld not in range %ld to %ld", long_, G_MININT64, G_MAXINT64);
        return FALSE;
    }

    arg->v_int64 = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_uint64 (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         PyObject          *py_arg,
                         GIArgument        *arg)
{
    PyObject *py_long;
    guint64 ulong_;

    if (!PyNumber_Check(py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long(py_arg);
    if (!py_long)
        return FALSE;

#if PY_VERSION_HEX < 0x03000000
    if (PyInt_Check (py_long)) {
        long long_ = PyInt_AsLong(py_long);
        if (long_ < 0) {
            PyErr_Format (PyExc_ValueError, "%ld not in range %d to %llu",
                          long_, 0, G_MAXUINT64);
            return FALSE;
        }
        ulong_ = long_;
    } else
#endif
        ulong_ = PyLong_AsUnsignedLongLong(py_long);

    Py_DECREF(py_long);

    if (PyErr_Occurred()) {
        /* OverflowError occured but range errors should be returned as ValueError */
        char *long_str;
        PyObject *py_str = PyObject_Str(py_long);

        if (PyUnicode_Check(py_str)) {
            PyObject *py_bytes = PyUnicode_AsUTF8String(py_str);
            if (py_bytes == NULL)
                return FALSE;

            long_str = g_strdup(PYGLIB_PyBytes_AsString(py_bytes));
            if (long_str == NULL) {
                PyErr_NoMemory();
                return FALSE;
            }

            Py_DECREF(py_bytes);
        } else {
            long_str = g_strdup(PYGLIB_PyBytes_AsString(py_str));
        }

        Py_DECREF(py_str);
        PyErr_Clear();
        PyErr_Format(PyExc_ValueError, "%s not in range %d to %llu",
                     long_str, 0, G_MAXUINT64);

        g_free(long_str);
        return FALSE;
    }

    if (ulong_ > G_MAXUINT64) {
        PyErr_Format (PyExc_ValueError, "%llu not in range %d to %llu", ulong_, 0, G_MAXUINT64);
        return FALSE;
    }

    arg->v_uint64 = ulong_;

    return TRUE;
}

gboolean
_pygi_marshal_in_float (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        PyObject          *py_arg,
                        GIArgument        *arg)
{
    PyObject *py_float;
    double double_;

    if (!PyNumber_Check(py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_float = PyNumber_Float(py_arg);
    if (!py_float)
        return FALSE;

    double_ = PyFloat_AsDouble(py_float);
    Py_DECREF(py_float);

    if (PyErr_Occurred())
        return FALSE;

    if (double_ < -G_MAXFLOAT || double_ > G_MAXFLOAT) {
        PyErr_Format (PyExc_ValueError, "%f not in range %f to %f", double_, -G_MAXFLOAT, G_MAXFLOAT);
        return FALSE;
    }

    arg->v_float = double_;

    return TRUE;
}

gboolean
_pygi_marshal_in_double (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         PyObject          *py_arg,
                         GIArgument        *arg)
{
    PyObject *py_float;
    double double_;

    if (!PyNumber_Check(py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_float = PyNumber_Float(py_arg);
    if (!py_float)
        return FALSE;

    double_ = PyFloat_AsDouble(py_float);
    Py_DECREF(py_float);

    if (PyErr_Occurred())
        return FALSE;

    if (double_ < -G_MAXDOUBLE || double_ > G_MAXDOUBLE) {
        PyErr_Format (PyExc_ValueError, "%f not in range %f to %f", double_, -G_MAXDOUBLE, G_MAXDOUBLE);
        return FALSE;
    }

    arg->v_double = double_;

    return TRUE;
}

gboolean
_pygi_marshal_in_unichar (PyGIInvokeState   *state,
                          PyGIFunctionCache *function_cache,
                          PyGIArgCache      *arg_cache,
                          PyObject          *py_arg,
                          GIArgument        *arg)
{
    Py_ssize_t size;
    gchar *string_;

    if (PyUnicode_Check (py_arg)) {
       PyObject *py_bytes;

       size = PyUnicode_GET_SIZE (py_arg);
       py_bytes = PyUnicode_AsUTF8String(py_arg);
       string_ = strdup(PYGLIB_PyBytes_AsString(py_bytes));
       Py_DECREF(py_bytes);

#if PY_VERSION_HEX < 0x03000000
    } else if (PyString_Check (py_arg)) {
       PyObject *pyuni = PyUnicode_FromEncodedObject (py_arg, "UTF-8", "strict");
       if (!pyuni)
           return FALSE;

       size = PyUnicode_GET_SIZE (pyuni);
       string_ = g_strdup(PyString_AsString(py_arg));
       Py_DECREF(pyuni);
#endif
    } else {
       PyErr_Format (PyExc_TypeError, "Must be string, not %s",
                     py_arg->ob_type->tp_name);
       return FALSE;
    }

    if (size != 1) {
       PyErr_Format (PyExc_TypeError, "Must be a one character string, not %ld characters",
                     size);
       g_free(string_);
       return FALSE;
    }

    arg->v_uint32 = g_utf8_get_char(string_);
    g_free(string_);

    return TRUE;
}
gboolean
_pygi_marshal_in_gtype (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        PyObject          *py_arg,
                        GIArgument        *arg)
{
    long type_ = pyg_type_from_object (py_arg);

    if (type_ == 0) {
        PyErr_Format (PyExc_TypeError, "Must be gobject.GType, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    arg->v_long = type_;
    return TRUE;
}
gboolean
_pygi_marshal_in_utf8 (PyGIInvokeState   *state,
                       PyGIFunctionCache *function_cache,
                       PyGIArgCache      *arg_cache,
                       PyObject          *py_arg,
                       GIArgument        *arg)
{
    gchar *string_;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (PyUnicode_Check(py_arg)) {
        PyObject *pystr_obj = PyUnicode_AsUTF8String (py_arg);
        if (!pystr_obj)
            return FALSE;

        string_ = g_strdup(PYGLIB_PyBytes_AsString (pystr_obj));
        Py_DECREF(pystr_obj);
    }
#if PY_VERSION_HEX < 0x03000000
    else if (PyString_Check(py_arg)) {
        string_ = g_strdup(PyString_AsString (py_arg));
    }
#endif
    else {
        PyErr_Format (PyExc_TypeError, "Must be string, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    arg->v_string = string_;
    return TRUE;
}

gboolean
_pygi_marshal_in_filename (PyGIInvokeState   *state,
                           PyGIFunctionCache *function_cache,
                           PyGIArgCache      *arg_cache,
                           PyObject          *py_arg,
                           GIArgument        *arg)
{
    gchar *string_;
    GError *error = NULL;

    if (PyUnicode_Check(py_arg)) {
        PyObject *pystr_obj = PyUnicode_AsUTF8String (py_arg);
        if (!pystr_obj)
            return FALSE;

        string_ = g_strdup(PYGLIB_PyBytes_AsString (pystr_obj));
        Py_DECREF(pystr_obj);
    }
#if PY_VERSION_HEX < 0x03000000
    else if (PyString_Check(py_arg)) {
        string_ = g_strdup(PyString_AsString (py_arg));
    }
#endif
    else {
        PyErr_Format (PyExc_TypeError, "Must be string, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    arg->v_string = g_filename_from_utf8 (string_, -1, NULL, NULL, &error);
    g_free(string_);

    if (arg->v_string == NULL) {
        PyErr_SetString (PyExc_Exception, error->message);
        g_error_free(error);
        /* TODO: Convert the error to an exception. */
        return FALSE;
    }

    return TRUE;
}

gboolean
_pygi_marshal_in_array (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        PyObject          *py_arg,
                        GIArgument        *arg)
{
    PyGIMarshalInFunc in_marshaller;
    int i;
    Py_ssize_t length;
    GArray *array_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;


    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!PySequence_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be sequence, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    length = PySequence_Length (py_arg);
    if (length < 0)
        return FALSE;

    if (sequence_cache->fixed_size >= 0 &&
        sequence_cache->fixed_size != length) {
        PyErr_Format (PyExc_ValueError, "Must contain %zd items, not %zd",
                      sequence_cache->fixed_size, length);

        return FALSE;
    }

    array_ = g_array_sized_new (sequence_cache->is_zero_terminated,
                                FALSE,
                                sequence_cache->item_size,
                                length);

    if (array_ == NULL) {
        PyErr_NoMemory();
        return FALSE;
    }

    if (sequence_cache->item_cache->type_tag == GI_TYPE_TAG_UINT8 &&
        PYGLIB_PyBytes_Check(py_arg)) {
        memcpy(array_->data, PYGLIB_PyBytes_AsString(py_arg), length);

        goto array_success;
    }

    in_marshaller = sequence_cache->item_cache->in_marshaller;
    for (i = 0; i < length; i++) {
        GIArgument item;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL)
            goto err;

        if (!in_marshaller(state,
                           function_cache,
                           sequence_cache->item_cache,
                           py_item,
                           &item))
            goto err;

        g_array_insert_val(array_, i, item);
        continue;
err:
        if (sequence_cache->item_cache->cleanup != NULL) {
            GDestroyNotify cleanup = sequence_cache->item_cache->cleanup;
            /*for(j = 0; j < i; j++)
                cleanup((gpointer)(array_->data[j]));*/
        }

        g_array_free(array_, TRUE);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

array_success:
    if (sequence_cache->len_arg_index >= 0) {
        /* we have an aux arg to handle */
        PyGIArgCache *aux_cache =
            function_cache->args_cache[sequence_cache->len_arg_index];

        if (aux_cache->direction == GI_DIRECTION_INOUT) {
            gint *len_arg = (gint *)state->in_args[aux_cache->c_arg_index].v_pointer;
            /* if we are not setup yet just set the in arg */
            if (len_arg == NULL)
                state->in_args[aux_cache->c_arg_index].v_long = length;
            else
                *len_arg = length;
        } else {              
            state->in_args[aux_cache->c_arg_index].v_long = length;
        }
    }

    if (sequence_cache->array_type == GI_ARRAY_TYPE_C) {
        arg->v_pointer = array_->data;
        g_array_free(array_, FALSE);
    } else {
        arg->v_pointer = array_;
    }

    return TRUE;
}

gboolean
_pygi_marshal_in_glist (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        PyObject          *py_arg,
                        GIArgument        *arg)
{
    PyGIMarshalInFunc in_marshaller;
    int i;
    Py_ssize_t length;
    GList *list_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;


    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!PySequence_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be sequence, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    length = PySequence_Length(py_arg);
    if (length < 0)
        return FALSE;

    if (sequence_cache->fixed_size >= 0 &&
        sequence_cache->fixed_size != length) {
        PyErr_Format (PyExc_ValueError, "Must contain %zd items, not %zd",
                      sequence_cache->fixed_size, length);

        return FALSE;
    }

    in_marshaller = sequence_cache->item_cache->in_marshaller;
    for (i = 0; i < length; i++) {
        GIArgument item;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL)
            goto err;

        if (!in_marshaller(state,
                           function_cache,
                           sequence_cache->item_cache,
                           py_item,
                           &item))
            goto err;

        list_ = g_list_append(list_, item.v_pointer);
        continue;
err:
        if (sequence_cache->item_cache->cleanup != NULL) {
            GDestroyNotify cleanup = sequence_cache->item_cache->cleanup;
        }

        g_list_free(list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = list_;
    return TRUE;
}

gboolean
_pygi_marshal_in_gslist (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         PyObject          *py_arg,
                         GIArgument        *arg)
{
    PyGIMarshalInFunc in_marshaller;
    int i;
    Py_ssize_t length;
    GSList *list_ = NULL;
    PyGISequenceCache *sequence_cache = (PyGISequenceCache *)arg_cache;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!PySequence_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be sequence, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    length = PySequence_Length(py_arg);
    if (length < 0)
        return FALSE;

    if (sequence_cache->fixed_size >= 0 &&
        sequence_cache->fixed_size != length) {
        PyErr_Format (PyExc_ValueError, "Must contain %zd items, not %zd",
                      sequence_cache->fixed_size, length);

        return FALSE;
    }

    in_marshaller = sequence_cache->item_cache->in_marshaller;
    for (i = 0; i < length; i++) {
        GIArgument item;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL)
            goto err;

        if (!in_marshaller(state,
                           function_cache,
                           sequence_cache->item_cache,
                           py_item,
                           &item))
            goto err;

        list_ = g_slist_append(list_, item.v_pointer);
        continue;
err:
        if (sequence_cache->item_cache->cleanup != NULL) {
            GDestroyNotify cleanup = sequence_cache->item_cache->cleanup;
        }

        g_slist_free(list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = list_;
    return TRUE;
}

gboolean
_pygi_marshal_in_ghash (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        PyObject          *py_arg,
                        GIArgument        *arg)
{
    PyGIMarshalInFunc key_in_marshaller;
    PyGIMarshalInFunc value_in_marshaller;

    int i;
    Py_ssize_t length;
    PyObject *py_keys, *py_values;

    GHashFunc hash_func;
    GEqualFunc equal_func;

    GHashTable *hash_ = NULL;
    PyGIHashCache *hash_cache = (PyGIHashCache *)arg_cache;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    py_keys = PyMapping_Keys(py_arg);
    if (py_keys == NULL) {
        PyErr_Format (PyExc_TypeError, "Must be mapping, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    length = PyMapping_Length(py_arg);
    if (length < 0) {
        Py_DECREF(py_keys);
        return FALSE;
    }

    py_values = PyMapping_Values(py_arg);
    if (py_values == NULL) {
        Py_DECREF(py_keys);
        return FALSE;
    }

    key_in_marshaller = hash_cache->key_cache->in_marshaller;
    value_in_marshaller = hash_cache->value_cache->in_marshaller;

    switch (hash_cache->key_cache->type_tag) {
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
            hash_func = g_str_hash;
            equal_func = g_str_equal;
            break;
        default:
            hash_func = NULL;
            equal_func = NULL;
    }

    hash_ = g_hash_table_new (hash_func, equal_func);
    if (hash_ == NULL) {
        PyErr_NoMemory();
        Py_DECREF(py_keys);
        Py_DECREF(py_values);
        return FALSE;
    }

    for (i = 0; i < length; i++) {
        GIArgument key, value;
        PyObject *py_key = PyList_GET_ITEM (py_keys, i);
        PyObject *py_value = PyList_GET_ITEM (py_values, i);
        if (py_key == NULL || py_value == NULL)
            goto err;

        if (!key_in_marshaller(state,
                               function_cache,
                               hash_cache->key_cache,
                               py_key,
                               &key))
            goto err;

        if (!value_in_marshaller(state,
                                 function_cache,
                                 hash_cache->value_cache,
                                 py_value,
                                 &value))
            goto err;

        g_hash_table_insert(hash_, key.v_pointer, value.v_pointer);
        continue;
err:
        /* FIXME: cleanup hash keys and values */
        Py_XDECREF(py_key);
        Py_XDECREF(py_value);
        Py_DECREF(py_keys);
        Py_DECREF(py_values);
        g_hash_table_unref(hash_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = hash_;
    return TRUE;
}

gboolean
_pygi_marshal_in_gerror (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         PyObject          *py_arg,
                         GIArgument        *arg)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Marshalling for GErrors is not implemented");
    return FALSE;
}

gboolean
_pygi_marshal_in_interface_callback (PyGIInvokeState   *state,
                                     PyGIFunctionCache *function_cache,
                                     PyGIArgCache      *arg_cache,
                                     PyObject          *py_arg,
                                     GIArgument        *arg)
{
    GICallableInfo *callable_info;
    PyGICClosure *closure;
    PyGIArgCache *user_data_cache = NULL;
    PyGIArgCache *destroy_cache = NULL;
    PyGICallbackCache *callback_cache;
    PyObject *py_user_data = NULL;

    callback_cache = (PyGICallbackCache *)arg_cache;

    if (callback_cache->user_data_index > 0) {
        user_data_cache = function_cache->args_cache[callback_cache->user_data_index];
        if (user_data_cache->py_arg_index < state->n_py_in_args) {
            py_user_data = PyTuple_GetItem(state->py_in_args, user_data_cache->py_arg_index);
            if (!py_user_data)
                return FALSE;
        } else {
            py_user_data = Py_None;
            Py_INCREF(Py_None);
        }
    }

    if (py_arg == Py_None && !(py_user_data == Py_None || py_user_data == NULL)) {
        Py_DECREF(py_user_data);
        PyErr_Format(PyExc_TypeError,
                     "When passing None for a callback userdata must also be None");

        return FALSE;
    }

    if (py_arg == Py_None) {
        Py_XDECREF(py_user_data);
        return TRUE;
    }

    if (!PyCallable_Check(py_arg)) {
        Py_XDECREF(py_user_data);
        PyErr_Format(PyExc_TypeError,
                    "Callback needs to be a function or method not %s",
                    py_arg->ob_type->tp_name);

        return FALSE;
    }

    if (callback_cache->destroy_notify_index > 0)
        destroy_cache = function_cache->args_cache[callback_cache->destroy_notify_index];

    callable_info = (GICallableInfo *)callback_cache->interface_info;

    closure = _pygi_make_native_closure (callable_info, callback_cache->scope, py_arg, py_user_data);
    arg->v_pointer = closure->closure;
    if (user_data_cache != NULL) {
        state->in_args[user_data_cache->c_arg_index].v_pointer = closure;
    }

    if (destroy_cache) {
        PyGICClosure *destroy_notify = _pygi_destroy_notify_create();
        state->in_args[destroy_cache->c_arg_index].v_pointer = destroy_notify->closure;
    }

    return TRUE;
}

gboolean
_pygi_marshal_in_interface_enum (PyGIInvokeState   *state,
                                 PyGIFunctionCache *function_cache,
                                 PyGIArgCache      *arg_cache,
                                 PyObject          *py_arg,
                                 GIArgument        *arg)
{
    PyObject *int_;
    gint is_instance;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    is_instance = PyObject_IsInstance(py_arg, iface_cache->py_type);

    int_ = PYGLIB_PyNumber_Long(py_arg);
    if (int_ == NULL) {
        PyErr_Clear();
        goto err;
    }

    arg->v_long = PYGLIB_PyLong_AsLong(int_);
    Py_DECREF(int_);

    /* If this is not an instance of the Enum type that we want
     * we need to check if the value is equivilant to one of the
     * Enum's memebers */
    if (!is_instance) {
        int i;
        gboolean is_found = FALSE;

        for (i = 0; i < g_enum_info_get_n_values(iface_cache->interface_info); i++) {
            GIValueInfo *value_info =
                g_enum_info_get_value(iface_cache->interface_info, i);
            glong enum_value = g_value_info_get_value(value_info);
            g_base_info_unref( (GIBaseInfo *)value_info);
            if (arg->v_long == enum_value) {
                is_found = TRUE;
                break;
            }
        }

        if (!is_found)
            goto err;
    }

    return TRUE;

err:
    PyErr_Format(PyExc_TypeError, "Expected a %s, but got %s",
                 iface_cache->type_name, py_arg->ob_type->tp_name);
    return FALSE;
}

gboolean
_pygi_marshal_in_interface_flags (PyGIInvokeState   *state,
                                  PyGIFunctionCache *function_cache,
                                  PyGIArgCache      *arg_cache,
                                  PyObject          *py_arg,
                                  GIArgument        *arg)
{
    PyObject *int_;
    gint is_instance;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    is_instance = PyObject_IsInstance(py_arg, iface_cache->py_type);

    int_ = PYGLIB_PyNumber_Long(py_arg);
    if (int_ == NULL) {
        PyErr_Clear();
        goto err;
    }

    arg->v_long = PYGLIB_PyLong_AsLong(int_);
    Py_DECREF(int_);

    /* only 0 or argument of type Flag is allowed */
    if (!is_instance && arg->v_long != 0)
        goto err;

    return TRUE;

err:
    PyErr_Format (PyExc_TypeError, "Expected a %s, but got %s",
                  iface_cache->type_name, py_arg->ob_type->tp_name);
    return FALSE;

}

gboolean
_pygi_marshal_in_interface_struct (PyGIInvokeState   *state,
                                   PyGIFunctionCache *function_cache,
                                   PyGIArgCache      *arg_cache,
                                   PyObject          *py_arg,
                                   GIArgument        *arg)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    /* FIXME: handle this large if statement in the cache
     *        and set the correct marshaller
     */

    if (iface_cache->g_type == G_TYPE_CLOSURE) {
        GClosure *closure;
        if (!PyCallable_Check(py_arg)) {
            PyErr_Format(PyExc_TypeError, "Must be callable, not %s",
                         py_arg->ob_type->tp_name);
            return FALSE;
        }

        closure = pyg_closure_new(py_arg, NULL, NULL);
        if (closure == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "PyObject conversion to GClosure failed");
            return FALSE;
        }

        arg->v_pointer = closure;
        return TRUE;
    } else if (iface_cache->g_type == G_TYPE_VALUE) {
        GValue *value;
        GType object_type;

        object_type = pyg_type_from_object_strict( (PyObject *) py_arg->ob_type, FALSE);
        if (object_type == G_TYPE_INVALID) {
            PyErr_SetString (PyExc_RuntimeError, "unable to retrieve object's GType");
            return FALSE;
        }

        value = g_slice_new0 (GValue);
        g_value_init (value, object_type);
        if (pyg_value_from_pyobject(value, py_arg) < 0) {
            g_slice_free (GValue, value);
            PyErr_SetString (PyExc_RuntimeError, "PyObject conversion to GValue failed");
            return FALSE;
        }

        arg->v_pointer = value;
        return TRUE;
    } else if (iface_cache->is_foreign) {
        gboolean success;
        success = pygi_struct_foreign_convert_to_g_argument(py_arg,
                                                            iface_cache->interface_info,
                                                            arg_cache->transfer,
                                                            arg);

        return success;
    } else if (!PyObject_IsInstance(py_arg, iface_cache->py_type)) {
        PyErr_Format (PyExc_TypeError, "Expected %s, but got %s",
                      iface_cache->type_name,
                      iface_cache->py_type->ob_type->tp_name);
        return FALSE;
    }

    if (g_type_is_a (iface_cache->g_type, G_TYPE_BOXED)) {
        arg->v_pointer = pyg_boxed_get(py_arg, void);
        if (arg_cache->transfer == GI_TRANSFER_EVERYTHING) {
            arg->v_pointer = g_boxed_copy(iface_cache->g_type, arg->v_pointer);
        }
    } else if (g_type_is_a (iface_cache->g_type, G_TYPE_POINTER) ||
               iface_cache->g_type  == G_TYPE_NONE) {
        arg->v_pointer = pyg_pointer_get(py_arg, void);
    } else {
        PyErr_Format (PyExc_NotImplementedError, "structure type '%s' is not supported yet", g_type_name(iface_cache->g_type));
        return FALSE;
    }
    return TRUE;
}

gboolean
_pygi_marshal_in_interface_boxed (PyGIInvokeState   *state,
                                  PyGIFunctionCache *function_cache,
                                  PyGIArgCache      *arg_cache,
                                  PyObject          *py_arg,
                                  GIArgument        *arg)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Marshalling for this type is not implemented yet");
    return FALSE;
}

gboolean
_pygi_marshal_in_interface_object (PyGIInvokeState   *state,
                                   PyGIFunctionCache *function_cache,
                                   PyGIArgCache      *arg_cache,
                                   PyObject          *py_arg,
                                   GIArgument        *arg)
{
    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!PyObject_IsInstance (py_arg, ((PyGIInterfaceCache *)arg_cache)->py_type)) {
        PyErr_Format (PyExc_TypeError, "Expected %s, but got %s",
                      ((PyGIInterfaceCache *)arg_cache)->type_name,
                      ((PyGIInterfaceCache *)arg_cache)->py_type->ob_type->tp_name);
        return FALSE;
    }

    arg->v_pointer = pygobject_get (py_arg);
    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        g_object_ref (arg->v_pointer);

    return TRUE;
}

gboolean
_pygi_marshal_in_interface_union (PyGIInvokeState   *state,
                                  PyGIFunctionCache *function_cache,
                                  PyGIArgCache      *arg_cache,
                                  PyObject          *py_arg,
                                  GIArgument        *arg)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Marshalling for this type is not implemented yet");
    return FALSE;
}

PyObject *
_pygi_marshal_out_void (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    if (arg_cache->is_pointer)
        py_obj = arg->v_pointer;
    else
        py_obj = Py_None;

    Py_XINCREF(py_obj);
    return py_obj;
}

PyObject *
_pygi_marshal_out_boolean (PyGIInvokeState   *state,
                           PyGIFunctionCache *function_cache,
                           PyGIArgCache      *arg_cache,
                           GIArgument        *arg)
{
    PyObject *py_obj = PyBool_FromLong(arg->v_boolean);
    return py_obj;
}

PyObject *
_pygi_marshal_out_int8 (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        GIArgument        *arg)
{
    PyObject *py_obj = PYGLIB_PyLong_FromLong(arg->v_int8);
    return py_obj;
}

PyObject *
_pygi_marshal_out_uint8 (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj =  PYGLIB_PyLong_FromLong(arg->v_uint8);

    return py_obj;
}

PyObject *
_pygi_marshal_out_int16 (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj =  PYGLIB_PyLong_FromLong(arg->v_int16);

    return py_obj;
}

PyObject *
_pygi_marshal_out_uint16 (PyGIInvokeState   *state,
                          PyGIFunctionCache *function_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    PyObject *py_obj =  PYGLIB_PyLong_FromLong(arg->v_uint16);

    return py_obj;
}

PyObject *
_pygi_marshal_out_int32 (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj = PYGLIB_PyLong_FromLong(arg->v_int32);

    return py_obj;
}

PyObject *
_pygi_marshal_out_uint32 (PyGIInvokeState   *state,
                          PyGIFunctionCache *function_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    PyObject *py_obj = PyLong_FromLongLong(arg->v_uint32);

    return py_obj;
}

PyObject *
_pygi_marshal_out_int64 (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj = PyLong_FromLongLong(arg->v_int64);

    return py_obj;
}

PyObject *
_pygi_marshal_out_uint64 (PyGIInvokeState   *state,
                          PyGIFunctionCache *function_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    PyObject *py_obj = PyLong_FromUnsignedLongLong(arg->v_uint64);

    return py_obj;
}

PyObject *
_pygi_marshal_out_float (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj = PyFloat_FromDouble (arg->v_float);

    return py_obj;
}

PyObject *
_pygi_marshal_out_double (PyGIInvokeState   *state,
                          PyGIFunctionCache *function_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    PyObject *py_obj = PyFloat_FromDouble (arg->v_double);

    return py_obj;
}

PyObject *
_pygi_marshal_out_unichar (PyGIInvokeState   *state,
                           PyGIFunctionCache *function_cache,
                           PyGIArgCache      *arg_cache,
                           GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    /* Preserve the bidirectional mapping between 0 and "" */
    if (arg->v_uint32 == 0) {
        py_obj = PYGLIB_PyUnicode_FromString ("");
    } else if (g_unichar_validate (arg->v_uint32)) {
        gchar utf8[6];
        gint bytes;

        bytes = g_unichar_to_utf8 (arg->v_uint32, utf8);
        py_obj = PYGLIB_PyUnicode_FromStringAndSize ((char*)utf8, bytes);
    } else {
        /* TODO: Convert the error to an exception. */
        PyErr_Format (PyExc_TypeError,
                      "Invalid unicode codepoint %" G_GUINT32_FORMAT,
                      arg->v_uint32);
    }

    return py_obj;
}

PyObject *
_pygi_marshal_out_gtype (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    py_obj = pyg_type_wrapper_new( (GType)arg->v_long);
    return py_obj;
}

PyObject *
_pygi_marshal_out_utf8 (PyGIInvokeState   *state,
                        PyGIFunctionCache *function_cache,
                        PyGIArgCache      *arg_cache,
                        GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    if (arg->v_string == NULL) {
        py_obj = Py_None;
        Py_INCREF (py_obj);
        return py_obj;
     }

    py_obj = PYGLIB_PyUnicode_FromString (arg->v_string);
    return py_obj;
}

PyObject *
_pygi_marshal_out_filename (PyGIInvokeState   *state,
                            PyGIFunctionCache *function_cache,
                            PyGIArgCache      *arg_cache,
                            GIArgument        *arg)
{
    gchar *string;
    PyObject *py_obj = NULL;
    GError *error = NULL;

    if (arg->v_string == NULL) {
        py_obj = Py_None;
        Py_INCREF (py_obj);
        return py_obj;
    }

    string = g_filename_to_utf8 (arg->v_string, -1, NULL, NULL, &error);
    if (string == NULL) {
        PyErr_SetString (PyExc_Exception, error->message);
        /* TODO: Convert the error to an exception. */
        return NULL;
    }

    py_obj = PYGLIB_PyUnicode_FromString (string);
    g_free (string);

    return py_obj;
}

PyObject *
_pygi_marshal_out_array (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    GArray *array_;
    PyObject *py_obj = NULL;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;

    array_ = arg->v_pointer;

     /* GArrays make it easier to iterate over arrays
      * with different element sizes but requires that
      * we allocate a GArray if the argument was a C array
      */
    if (seq_cache->array_type == GI_ARRAY_TYPE_C) {
        gsize len;
        if (seq_cache->fixed_size >= 0) {
            len = seq_cache->fixed_size;
        } else if (seq_cache->is_zero_terminated) {
            len = g_strv_length(arg->v_string);
        } else {
            GIArgument *len_arg = state->args[seq_cache->len_arg_index];
            len = len_arg->v_long;
        }

        array_ = g_array_new(seq_cache->is_zero_terminated,
                             FALSE,
                             seq_cache->item_size);
        if (array_ == NULL) {
            PyErr_NoMemory();
            return NULL;
        }

        array_->data = arg->v_pointer;
        array_->len = len;
    }

    if (seq_cache->item_cache->type_tag == GI_TYPE_TAG_UINT8) {
        if (arg->v_pointer == NULL) {
            py_obj = PYGLIB_PyBytes_FromString ("");
        } else {
            py_obj = PYGLIB_PyBytes_FromStringAndSize(array_->data, array_->len);
        }
    } else {
        if (arg->v_pointer == NULL) {
            py_obj = PyList_New (0);
        } else {
            int i;
            gboolean is_struct;
            gsize item_size;
            PyGIMarshalOutFunc item_out_marshaller;
            PyGIArgCache *item_arg_cache;

            py_obj = PyList_New(array_->len);
            if (py_obj == NULL) {
                if (seq_cache->array_type == GI_ARRAY_TYPE_C)
                    g_array_unref(array_);

                return NULL;
            }

            item_arg_cache = seq_cache->item_cache;
            item_out_marshaller = item_arg_cache->out_marshaller;
            is_struct = FALSE;
            if (item_arg_cache->type_tag == GI_TYPE_TAG_INTERFACE) {
                PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)item_arg_cache;
                switch (g_base_info_get_type(iface_cache->interface_info)) {
                    case GI_INFO_TYPE_STRUCT:
                        case GI_INFO_TYPE_BOXED:
                            is_struct = TRUE;
                        default:
                            break;
                }
            }

            item_size = g_array_get_element_size(array_);
            for (i = 0; i < array_->len; i++) {
                GIArgument item_arg;
                PyObject *py_item;

                if (is_struct) {
                    item_arg.v_pointer = &_g_array_index(array_, GIArgument, i);
                } else {
                    memcpy (&item_arg, &_g_array_index (array_, GIArgument, i), item_size);
                }

                py_item = item_out_marshaller(state,
                                              function_cache,
                                              item_arg_cache,
                                              &item_arg);

                if (py_item == NULL) {
                    Py_CLEAR(py_obj);

                    if (seq_cache->array_type == GI_ARRAY_TYPE_C)
                        g_array_unref(array_);

                    return NULL;
                }
                PyList_SET_ITEM(py_obj, i, py_item);
            }
        }
    }

    if (seq_cache->array_type == GI_ARRAY_TYPE_C)
        g_array_free(array_, FALSE);

    return py_obj;
}

PyObject *
_pygi_marshal_out_glist (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    GList *list_;
    gsize length;
    gsize i;

    PyGIMarshalOutFunc item_out_marshaller;
    PyGIArgCache *item_arg_cache;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;

    PyObject *py_obj = NULL;

    list_ = arg->v_pointer;
    length = g_list_length(list_);

    py_obj = PyList_New(length);
    if (py_obj == NULL)
        return NULL;

    item_arg_cache = seq_cache->item_cache;
    item_out_marshaller = item_arg_cache->out_marshaller;

    for (i = 0; list_ != NULL; list_ = g_list_next (list_), i++) {
        GIArgument item_arg;
        PyObject *py_item;

        item_arg.v_pointer = list_->data;
        py_item = item_out_marshaller(state,
                                      function_cache,
                                      item_arg_cache,
                                      &item_arg);

        if (py_item == NULL) {
            Py_CLEAR (py_obj);
            _PyGI_ERROR_PREFIX ("Item %zu: ", i);
            return NULL;
        }

        PyList_SET_ITEM (py_obj, i, py_item);
    }

    return py_obj;
}

PyObject *
_pygi_marshal_out_gslist (PyGIInvokeState   *state,
                          PyGIFunctionCache *function_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    GSList *list_;
    gsize length;
    gsize i;

    PyGIMarshalOutFunc item_out_marshaller;
    PyGIArgCache *item_arg_cache;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;

    PyObject *py_obj = NULL;

    list_ = arg->v_pointer;
    length = g_slist_length(list_);

    py_obj = PyList_New(length);
    if (py_obj == NULL)
        return NULL;

    item_arg_cache = seq_cache->item_cache;
    item_out_marshaller = item_arg_cache->out_marshaller;

    for (i = 0; list_ != NULL; list_ = g_slist_next (list_), i++) {
        GIArgument item_arg;
        PyObject *py_item;

        item_arg.v_pointer = list_->data;
        py_item = item_out_marshaller(state,
                                      function_cache,
                                      item_arg_cache,
                                      &item_arg);

        if (py_item == NULL) {
            Py_CLEAR (py_obj);
            _PyGI_ERROR_PREFIX ("Item %zu: ", i);
            return NULL;
        }

        PyList_SET_ITEM (py_obj, i, py_item);
    }

    return py_obj;
}

PyObject *
_pygi_marshal_out_ghash (PyGIInvokeState   *state,
                         PyGIFunctionCache *function_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    GHashTable *hash_;
    GHashTableIter hash_table_iter;

    PyGIMarshalOutFunc key_out_marshaller;
    PyGIMarshalOutFunc value_out_marshaller;

    PyGIArgCache *key_arg_cache;
    PyGIArgCache *value_arg_cache;
    PyGIHashCache *hash_cache = (PyGIHashCache *)arg_cache;

    GIArgument key_arg;
    GIArgument value_arg;

    PyObject *py_obj = NULL;

    hash_ = arg->v_pointer;

    py_obj = PyDict_New();
    if (py_obj == NULL)
        return NULL;

    key_arg_cache = hash_cache->key_cache;
    key_out_marshaller = key_arg_cache->out_marshaller;

    value_arg_cache = hash_cache->value_cache;
    value_out_marshaller = value_arg_cache->out_marshaller;

    g_hash_table_iter_init(&hash_table_iter, hash_);
    while (g_hash_table_iter_next(&hash_table_iter,
                                  &key_arg.v_pointer,
                                  &value_arg.v_pointer)) {
        PyObject *py_key;
        PyObject *py_value;
        int retval;

        py_key = key_out_marshaller(state,
                                    function_cache,
                                    key_arg_cache,
                                    &key_arg);

        if (py_key == NULL) {
            Py_CLEAR (py_obj);
            return NULL;
        }

        py_value = value_out_marshaller(state,
                                        function_cache,
                                        value_arg_cache,
                                        &value_arg);

        if (py_value == NULL) {
            Py_CLEAR (py_obj);
            Py_DECREF(py_key);
            return NULL;
        }

        retval = PyDict_SetItem (py_obj, py_key, py_value);

        Py_DECREF(py_key);
        Py_DECREF(py_value);

        if (retval < 0) {
            Py_CLEAR(py_obj);
            return NULL;
        }
    }

    return py_obj;
}

PyObject *
_pygi_marshal_out_gerror (PyGIInvokeState   *state,
                          PyGIFunctionCache *function_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format(PyExc_NotImplementedError,
                 "Marshalling for gerror out is not implemented");
    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_callback(PyGIInvokeState   *state,
                                     PyGIFunctionCache *function_cache,
                                     PyGIArgCache      *arg_cache,
                                     GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format(PyExc_NotImplementedError,
                 "Callback out values are not supported");
    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_enum (PyGIInvokeState   *state,
                                  PyGIFunctionCache *function_cache,
                                  PyGIArgCache      *arg_cache,
                                  GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (iface_cache->g_type == G_TYPE_NONE) {
        py_obj = PyObject_CallFunction (iface_cache->py_type, "l", arg->v_long);
    } else {
        py_obj = pyg_enum_from_gtype(iface_cache->g_type, arg->v_long);
    }
    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_flags (PyGIInvokeState   *state,
                                   PyGIFunctionCache *function_cache,
                                   PyGIArgCache      *arg_cache,
                                   GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    py_obj = pyg_flags_from_gtype(iface_cache->g_type, arg->v_long);

    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_struct (PyGIInvokeState   *state,
                                    PyGIFunctionCache *function_cache,
                                    PyGIArgCache      *arg_cache,
                                    GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GType type = iface_cache->g_type;

    if (g_type_is_a (type, G_TYPE_VALUE)) {
        py_obj = pyg_value_as_pyobject (arg->v_pointer, FALSE);
    } else if (g_type_is_a (type, G_TYPE_BOXED)) {
        py_obj = _pygi_boxed_new ( (PyTypeObject *)iface_cache->py_type, arg->v_pointer, 
                                  arg_cache->transfer == GI_TRANSFER_EVERYTHING);
    } else if (g_type_is_a (type, G_TYPE_POINTER)) {
        if (iface_cache->py_type == NULL ||
                !PyType_IsSubtype( (PyTypeObject *)iface_cache->py_type, &PyGIStruct_Type)) {
            g_warn_if_fail(arg_cache->transfer == GI_TRANSFER_NOTHING);
            py_obj = pyg_pointer_new(type, arg->v_pointer);
        } else {
            py_obj = _pygi_struct_new ( (PyTypeObject *)iface_cache->py_type, arg->v_pointer, 
                                      arg_cache->transfer == GI_TRANSFER_EVERYTHING);
        }
    } else if (type == G_TYPE_NONE && iface_cache->is_foreign) {
        py_obj = pygi_struct_foreign_convert_from_g_argument (iface_cache->interface_info, arg->v_pointer);
    } else if (type == G_TYPE_NONE) {
        py_obj = _pygi_struct_new((PyTypeObject *) iface_cache->py_type, arg->v_pointer, 
                                  arg_cache->transfer == GI_TRANSFER_EVERYTHING);
    } else {
        PyErr_Format (PyExc_NotImplementedError, "structure type '%s' is not supported yet", g_type_name (type));
    }

    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_interface (PyGIInvokeState   *state,
                                       PyGIFunctionCache *function_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format(PyExc_NotImplementedError,
                 "Marshalling for this type is not implemented yet");
    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_boxed (PyGIInvokeState   *state,
                                   PyGIFunctionCache *function_cache,
                                   PyGIArgCache      *arg_cache,
                                   GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format(PyExc_NotImplementedError,
                 "Marshalling for this type is not implemented yet");
    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_object (PyGIInvokeState   *state,
                                    PyGIFunctionCache *function_cache,
                                    PyGIArgCache      *arg_cache,
                                    GIArgument        *arg)
{
    PyObject *py_obj = pygobject_new (arg->v_pointer);

    /* The new wrapper increased the reference count, so decrease it. */
    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        g_object_unref (arg->v_pointer);

    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_union  (PyGIInvokeState   *state,
                                    PyGIFunctionCache *function_cache,
                                    PyGIArgCache      *arg_cache,
                                    GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format(PyExc_NotImplementedError,
                 "Marshalling for this type is not implemented yet");
    return py_obj;
}


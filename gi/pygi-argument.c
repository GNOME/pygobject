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
                        g_base_info_unref (value_info);
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
                if (!(PyCallable_Check (object) ||
                      pyg_type_from_object_strict (object, FALSE) == G_TYPE_CLOSURE)) {
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
            retval = _pygi_g_registered_type_info_check_object ( (GIRegisteredTypeInfo *) info, TRUE, object);
            break;
        case GI_INFO_TYPE_UNION:


            retval = _pygi_g_registered_type_info_check_object ( (GIRegisteredTypeInfo *) info, TRUE, object);

            /* If not the same type then check to see if the object's type
             * is the same as one of the union's members
             */
            if (retval == 0) {
                gint i;
                gint n_fields;

                n_fields = g_union_info_get_n_fields ( (GIUnionInfo *) info);

                for (i = 0; i < n_fields; i++) {
                    gint member_retval;
                    GIFieldInfo *field_info;
                    GITypeInfo *field_type_info;

                    field_info =
                        g_union_info_get_field ( (GIUnionInfo *) info, i);
                    field_type_info = g_field_info_get_type (field_info);

                    member_retval = _pygi_g_type_info_check_object(
                        field_type_info,
                        object,
                        TRUE);

                    g_base_info_unref ( ( GIBaseInfo *) field_type_info);
                    g_base_info_unref ( ( GIBaseInfo *) field_info);

                    if (member_retval == 1) {
                        retval = member_retval;
                        break;
                    }
                }
            }

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

                        /* if already a gvalue, copy, else marshal into gvalue */
                        if (object_type == G_TYPE_VALUE) {
                            /* src GValue's lifecycle is handled by Python
                             * so we have to copy it into the destination's
                             * GValue which is freed during the cleanup of
                             * invoke.
                             */
                            GValue *src = (GValue *)((PyGObject *) object)->obj;
                            g_value_init (value, G_VALUE_TYPE (src));
                            g_value_copy(src, value);
                        } else {
                            g_value_init (value, object_type);
                            retval = pyg_value_from_pyobject (value, object);
                            if (retval < 0) {
                                g_slice_free (GValue, value);
                                PyErr_SetString (PyExc_RuntimeError, "PyObject conversion to GValue failed");
                                break;
                            }
                        }

                        arg.v_pointer = value;
                    } else if (g_type_is_a (type, G_TYPE_CLOSURE)) {
                        GClosure *closure;

                        if (pyg_type_from_object_strict (object, FALSE) == G_TYPE_CLOSURE) {
                            closure = (GClosure *)pyg_boxed_get (object, void);
                        } else {
                            closure = pyg_closure_new (object, NULL, NULL);
                            if (closure == NULL) {
                                PyErr_SetString (PyExc_RuntimeError, "PyObject conversion to GClosure failed");
                                break;
                            }
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
                    } else if (g_struct_info_is_foreign (info)) {
                        object = pygi_struct_foreign_convert_from_g_argument (type_info, arg->v_pointer);
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


GIArgument
_pygi_argument_from_g_value(const GValue *value,
                            GITypeInfo *type_info)
{
    GIArgument arg = { 0, };

    GITypeTag type_tag = g_type_info_get_tag (type_info);
    switch (type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
            arg.v_boolean = g_value_get_boolean (value);
            break;
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_INT64:
            arg.v_int = g_value_get_int (value);
            break;
        case GI_TYPE_TAG_UINT8:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_UINT64:
            arg.v_uint = g_value_get_uint (value);
            break;
        case GI_TYPE_TAG_UNICHAR:
            arg.v_uint32 = g_value_get_char (value);
            break;
        case GI_TYPE_TAG_FLOAT:
            arg.v_float = g_value_get_float (value);
            break;
        case GI_TYPE_TAG_DOUBLE:
            arg.v_double = g_value_get_double (value);
            break;
        case GI_TYPE_TAG_GTYPE:
            arg.v_long = g_value_get_gtype (value);
            break;
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
            arg.v_string = g_value_dup_string (value);
            break;
        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
            arg.v_pointer = g_value_get_pointer (value);
            break;
        case GI_TYPE_TAG_ARRAY:
        case GI_TYPE_TAG_GHASH:
            arg.v_pointer = g_value_get_boxed (value);
            break;
        case GI_TYPE_TAG_INTERFACE:
        {
            GIBaseInfo *info;
            GIInfoType info_type;

            info = g_type_info_get_interface (type_info);
            info_type = g_base_info_get_type (info);

            g_base_info_unref (info);

            switch (info_type) {
                case GI_INFO_TYPE_FLAGS:
                case GI_INFO_TYPE_ENUM:
                    arg.v_long = g_value_get_enum (value);
                    break;
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_OBJECT:
                    arg.v_pointer = g_value_get_object (value);
                    break;
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_STRUCT:
                case GI_INFO_TYPE_UNION:
                    if (G_VALUE_HOLDS(value, G_TYPE_BOXED)) {
                        arg.v_pointer = g_value_get_boxed (value);
                    } else {
                        arg.v_pointer = g_value_get_pointer (value);
                    }
                    break;
                default:
                    g_warning("Converting of type '%s' is not implemented", g_info_type_to_string(info_type));
                    g_assert_not_reached();
            }
            break;
        }
        case GI_TYPE_TAG_ERROR:
        case GI_TYPE_TAG_VOID:
            g_critical("Converting of type '%s' is not implemented", g_type_tag_to_string(type_tag));
            g_assert_not_reached();
    }

    return arg;
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


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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "pygi-private.h"
#include "pygobject-private.h"

#include <string.h>
#include <time.h>

#include <datetime.h>
#include <pyglib-python-compat.h>
#include <pyglib.h>

#include "pygi-value.h"
#include "pygi-basictype.h"
#include "pygi-object.h"
#include "pygi-struct-marshal.h"
#include "pygi-error.h"

gboolean
pygi_argument_to_gssize (GIArgument *arg_in,
                         GITypeTag  type_tag,
                         gssize *gssize_out)
{
    switch (type_tag) {
      case GI_TYPE_TAG_INT8:
          *gssize_out = arg_in->v_int8;
          return TRUE;
      case GI_TYPE_TAG_UINT8:
          *gssize_out = arg_in->v_uint8;
          return TRUE;
      case GI_TYPE_TAG_INT16:
          *gssize_out = arg_in->v_int16;
          return TRUE;
      case GI_TYPE_TAG_UINT16:
          *gssize_out = arg_in->v_uint16;
          return TRUE;
      case GI_TYPE_TAG_INT32:
          *gssize_out = arg_in->v_int32;
          return TRUE;
      case GI_TYPE_TAG_UINT32:
          *gssize_out = arg_in->v_uint32;
          return TRUE;
      case GI_TYPE_TAG_INT64:
          *gssize_out = arg_in->v_int64;
          return TRUE;
      case GI_TYPE_TAG_UINT64:
          *gssize_out = arg_in->v_uint64;
          return TRUE;
      default:
          PyErr_Format (PyExc_TypeError,
                        "Unable to marshal %s to gssize",
                        g_type_tag_to_string(type_tag));
          return FALSE;
    }
}

void
_pygi_hash_pointer_to_arg (GIArgument *arg,
                           GITypeTag  type_tag)
{
    switch (type_tag) {
        case GI_TYPE_TAG_INT8:
            arg->v_int8 = GPOINTER_TO_INT (arg->v_pointer);
            break;
        case GI_TYPE_TAG_INT16:
            arg->v_int16 = GPOINTER_TO_INT (arg->v_pointer);
            break;
        case GI_TYPE_TAG_INT32:
            arg->v_int32 = GPOINTER_TO_INT (arg->v_pointer);
            break;
        case GI_TYPE_TAG_UINT8:
            arg->v_uint8 = GPOINTER_TO_UINT (arg->v_pointer);
            break;
        case GI_TYPE_TAG_UINT16:
            arg->v_uint16 = GPOINTER_TO_UINT (arg->v_pointer);
            break;
        case GI_TYPE_TAG_UINT32:
            arg->v_uint32 = GPOINTER_TO_UINT (arg->v_pointer);
            break;
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
        case GI_TYPE_TAG_INTERFACE:
            break;
        default:
            g_critical ("Unsupported type %s", g_type_tag_to_string(type_tag));
    }
}

gpointer
_pygi_arg_to_hash_pointer (const GIArgument *arg,
                           GITypeTag        type_tag)
{
    switch (type_tag) {
        case GI_TYPE_TAG_INT8:
            return GINT_TO_POINTER (arg->v_int8);
        case GI_TYPE_TAG_UINT8:
            return GINT_TO_POINTER (arg->v_uint8);
        case GI_TYPE_TAG_INT16:
            return GINT_TO_POINTER (arg->v_int16);
        case GI_TYPE_TAG_UINT16:
            return GINT_TO_POINTER (arg->v_uint16);
        case GI_TYPE_TAG_INT32:
            return GINT_TO_POINTER (arg->v_int32);
        case GI_TYPE_TAG_UINT32:
            return GINT_TO_POINTER (arg->v_uint32);
        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
        case GI_TYPE_TAG_INTERFACE:
            return arg->v_pointer;
        default:
            g_critical ("Unsupported type %s", g_type_tag_to_string(type_tag));
            return arg->v_pointer;
    }
}

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
        case GI_TYPE_TAG_INT8:
            /* (U)INT8 types can be characters */
            if (PYGLIB_PyBytes_Check(object)) {
                if (PYGLIB_PyBytes_Size(object) != 1) {
                    PyErr_Format (PyExc_TypeError, "Must be a single character");
                    retval = 0;
                    break;
                }

                break;
            }
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
                    PyObject *lower_pybytes_obj;
                    PyObject *upper_pybytes_obj;

                    lower_pybytes_obj = PyUnicode_AsUTF8String (lower_str);
                    if (!lower_pybytes_obj) {
                        goto utf8_fail;
                    }

                    upper_pybytes_obj = PyUnicode_AsUTF8String (upper_str);
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
                PyErr_Format (PyExc_TypeError, "Must be a one character string, not %" G_GINT64_FORMAT " characters",
                              (gint64)size);
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


/**
 * _pygi_argument_array_length_marshal:
 * @length_arg_index: Index of length argument in the callables args list.
 * @user_data1: (type Array(GValue)): Array of GValue arguments to retrieve length
 * @user_data2: (type GICallableInfo): Callable info to get the argument from.
 *
 * Generic marshalling policy for array length arguments in callables.
 *
 * Returns: The length of the array or -1 on failure.
 */
gssize
_pygi_argument_array_length_marshal (gsize length_arg_index,
                                     void *user_data1,
                                     void *user_data2)
{
    GIArgInfo length_arg_info;
    GITypeInfo length_type_info;
    GIArgument length_arg;
    gssize array_len = -1;
    GValue *values = (GValue *)user_data1;
    GICallableInfo *callable_info = (GICallableInfo *)user_data2;

    g_callable_info_load_arg (callable_info, length_arg_index, &length_arg_info);
    g_arg_info_load_type (&length_arg_info, &length_type_info);

    length_arg = _pygi_argument_from_g_value (&(values[length_arg_index]),
                                              &length_type_info);
    if (!pygi_argument_to_gssize (&length_arg,
                                  g_type_info_get_tag (&length_type_info),
                                  &array_len)) {
        return -1;
    }

    return array_len;
}

/**
 * _pygi_argument_to_array
 * @arg: The argument to convert
 * @array_length_policy: Closure for marshalling the array length argument when needed.
 * @user_data1: Generic user data passed to the array_length_policy.
 * @user_data2: Generic user data passed to the array_length_policy.
 * @type_info: The type info for @arg
 * @out_free_array: A return location for a gboolean that indicates whether
 *                  or not the wrapped GArray should be freed
 *
 * Make sure an array type argument is wrapped in a GArray.
 *
 * Note: This method can *not* be folded into _pygi_argument_to_object() because
 * arrays are special in the sense that they might require access to @args in
 * order to get the length.
 *
 * Returns: A GArray wrapping @arg. If @out_free_array has been set to TRUE then
 *          free the array with g_array_free() without freeing the data members.
 *          Otherwise don't free the array.
 */
GArray *
_pygi_argument_to_array (GIArgument  *arg,
                         PyGIArgArrayLengthPolicy array_length_policy,
                         void        *user_data1,
                         void        *user_data2,
                         GITypeInfo  *type_info,
                         gboolean    *out_free_array)
{
    GITypeInfo *item_type_info;
    gboolean is_zero_terminated;
    gsize item_size;
    gssize length;
    GArray *g_array;
    
    g_return_val_if_fail (g_type_info_get_tag (type_info) == GI_TYPE_TAG_ARRAY, NULL);

    if (arg->v_pointer == NULL) {
        return NULL;
    }
    
    switch (g_type_info_get_array_type (type_info)) {
        case GI_ARRAY_TYPE_C:
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

                    if (G_UNLIKELY (array_length_policy == NULL)) {
                        g_critical ("Unable to determine array length for %p",
                                    arg->v_pointer);
                        g_array = g_array_new (is_zero_terminated, FALSE, item_size);
                        *out_free_array = TRUE;
                        return g_array;
                    }

                    length_arg_pos = g_type_info_get_array_length (type_info);
                    g_assert (length_arg_pos >= 0);

                    length = array_length_policy (length_arg_pos, user_data1, user_data2);
                    if (length < 0) {
                        return NULL;
                    }
                }
            }

            g_assert (length >= 0);

            g_array = g_array_new (is_zero_terminated, FALSE, item_size);

            g_free (g_array->data);
            g_array->data = arg->v_pointer;
            g_array->len = length;
            *out_free_array = TRUE;
            break;
        case GI_ARRAY_TYPE_ARRAY:
        case GI_ARRAY_TYPE_BYTE_ARRAY:
            /* Note: GByteArray is really just a GArray */
            g_array = arg->v_pointer;
            *out_free_array = FALSE;
            break;
        case GI_ARRAY_TYPE_PTR_ARRAY:
        {
            GPtrArray *ptr_array = (GPtrArray*) arg->v_pointer;
            g_array = g_array_sized_new (FALSE, FALSE,
                                         sizeof(gpointer),
                                         ptr_array->len);
             g_array->data = (char*) ptr_array->pdata;
             g_array->len = ptr_array->len;
             *out_free_array = TRUE;
             break;
        }
        default:
            g_critical ("Unexpected array type %u",
                        g_type_info_get_array_type (type_info));
            g_array = NULL;
            break;
    }

    return g_array;
}

GIArgument
_pygi_argument_from_object (PyObject   *object,
                            GITypeInfo *type_info,
                            GITransfer  transfer)
{
    GIArgument arg;
    GITypeTag type_tag;
    gpointer cleanup_data = NULL;

    memset(&arg, 0, sizeof(GIArgument));
    type_tag = g_type_info_get_tag (type_info);

    /* Ignores cleanup data for now. */
    if (_pygi_marshal_from_py_basic_type (object, &arg, type_tag, transfer, &cleanup_data) ||
            PyErr_Occurred()) {
        return arg;
    }

    switch (type_tag) {
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

            /* Note, strings are sequences, but we cannot accept them here */
            if (!PySequence_Check (object) || 
#if PY_VERSION_HEX < 0x03000000
                PyString_Check (object) || 
#endif
                PyUnicode_Check (object)) {
                PyErr_SetString (PyExc_TypeError, "expected sequence");
                break;
            }

            length = PySequence_Length (object);
            if (length < 0) {
                break;
            }

            is_zero_terminated = g_type_info_is_zero_terminated (type_info);
            item_type_info = g_type_info_get_param_type (type_info, 0);

            /* we handle arrays that are really strings specially, see below */
            if (g_type_info_get_tag (item_type_info) == GI_TYPE_TAG_UINT8)
               item_size = 1;
            else
               item_size = sizeof (GIArgument);

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
                    GType g_type;
                    PyObject *py_type;
                    gboolean is_foreign = (info_type == GI_INFO_TYPE_STRUCT) &&
                                          (g_struct_info_is_foreign ((GIStructInfo *) info));

                    g_type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *) info);
                    py_type = _pygi_type_import_by_gi_info ( (GIBaseInfo *) info);

                    /* Note for G_TYPE_VALUE g_type:
                     * This will currently leak the GValue that is allocated and
                     * stashed in arg.v_pointer. Out argument marshaling for caller
                     * allocated GValues already pass in memory for the GValue.
                     * Further re-factoring is needed to fix this leak.
                     * See: https://bugzilla.gnome.org/show_bug.cgi?id=693405
                     */
                    pygi_arg_struct_from_py_marshal (object,
                                                     &arg,
                                                     NULL, /*arg_name*/
                                                     info, /*interface_info*/
                                                     g_type,
                                                     py_type,
                                                     transfer,
                                                     FALSE, /*copy_reference*/
                                                     is_foreign,
                                                     g_type_info_is_pointer (type_info));

                    Py_DECREF (py_type);
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

                    arg.v_int = PYGLIB_PyLong_AsLong (int_);

                    Py_DECREF (int_);

                    break;
                }
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_OBJECT:
                    /* An error within this call will result in a NULL arg */
                    pygi_arg_gobject_out_arg_from_py (object, &arg, transfer);
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

                g_hash_table_insert (hash_table, key.v_pointer,
                                     _pygi_arg_to_hash_pointer (&value, g_type_info_get_tag (value_type_info)));
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
        default:
            g_assert_not_reached ();
    }

    return arg;
}

/**
 * _pygi_argument_to_object:
 * @arg: The argument to convert to an object.
 * @type_info: Type info for @arg
 * @transfer:
 *
 * If the argument is of type array, it must be encoded in a GArray, by calling
 * _pygi_argument_to_array(). This logic can not be folded into this method
 * as determining array lengths may require access to method call arguments.
 *
 * Returns: A PyObject representing @arg
 */
PyObject *
_pygi_argument_to_object (GIArgument  *arg,
                          GITypeInfo *type_info,
                          GITransfer transfer)
{
    GITypeTag type_tag;
    PyObject *object = NULL;

    type_tag = g_type_info_get_tag (type_info);
    object = _pygi_marshal_to_py_basic_type (arg, type_tag, transfer);
    if (object)
        return object;

    switch (type_tag) {
        case GI_TYPE_TAG_VOID:
        {
            if (g_type_info_is_pointer (type_info)) {
                g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
                object = PyLong_FromVoidPtr (arg->v_pointer);
            }
            break;
        }
        case GI_TYPE_TAG_ARRAY:
        {
            /* Arrays are assumed to be packed in a GArray */
            GArray *array;
            GITypeInfo *item_type_info;
            GITypeTag item_type_tag;
            GITransfer item_transfer;
            gsize i, item_size;

            if (arg->v_pointer == NULL)
                return PyList_New (0);
            
            item_type_info = g_type_info_get_param_type (type_info, 0);
            g_assert (item_type_info != NULL);

            item_type_tag = g_type_info_get_tag (item_type_info);
            item_transfer = transfer == GI_TRANSFER_CONTAINER ? GI_TRANSFER_NOTHING : transfer;
            
            array = arg->v_pointer;
            item_size = g_array_get_element_size (array);
            
            if (G_UNLIKELY (item_size > sizeof(GIArgument))) {
                g_critical ("Stack overflow protection. "
                            "Can't copy array element into GIArgument.");
                return PyList_New (0);
            }

            if (item_type_tag == GI_TYPE_TAG_UINT8) {
                /* Return as a byte array */
                object = PYGLIB_PyBytes_FromStringAndSize (array->data, array->len);
            } else {
                object = PyList_New (array->len);
                if (object == NULL) {
                    g_critical ("Failure to allocate array for %u items", array->len);
                    g_base_info_unref ( (GIBaseInfo *) item_type_info);
                    break;
                }

                for (i = 0; i < array->len; i++) {
                    GIArgument item = { 0 };
                    PyObject *py_item;
                    
                    memcpy (&item, array->data + i * item_size, item_size);

                    py_item = _pygi_argument_to_object (&item, item_type_info, item_transfer);
                    if (py_item == NULL) {
                        Py_CLEAR (object);
                        _PyGI_ERROR_PREFIX ("Item %zu: ", i);
                        break;
                    }

                    PyList_SET_ITEM (object, i, py_item);
                }
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
                    g_assert_not_reached();
                }
                case GI_INFO_TYPE_BOXED:
                case GI_INFO_TYPE_STRUCT:
                case GI_INFO_TYPE_UNION:
                {
                    PyObject *py_type;
                    GType g_type = g_registered_type_info_get_g_type ( (GIRegisteredTypeInfo *) info);
                    gboolean is_foreign = (info_type == GI_INFO_TYPE_STRUCT) &&
                                          (g_struct_info_is_foreign ((GIStructInfo *) info));

                    /* Special case variant and none to force loading from py module. */
                    if (g_type == G_TYPE_VARIANT || g_type == G_TYPE_NONE) {
                        py_type = _pygi_type_import_by_gi_info (info);
                    } else {
                        py_type = _pygi_type_get_from_g_type (g_type);
                    }

                    object = pygi_arg_struct_to_py_marshal (arg,
                                                            info, /*interface_info*/
                                                            g_type,
                                                            py_type,
                                                            transfer,
                                                            FALSE, /*is_allocated*/
                                                            is_foreign);

                    Py_XDECREF (py_type);
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
                        if (PyTuple_SetItem (py_args, 0, PyLong_FromLong (arg->v_int)) != 0) {
                            Py_DECREF (py_args);
                            Py_DECREF (py_type);
                            return NULL;
                        }

                        object = PyObject_CallFunction (py_type, "i", arg->v_int);

                        Py_DECREF (py_args);
                        Py_DECREF (py_type);

                    } else if (info_type == GI_INFO_TYPE_ENUM) {
                        object = pyg_enum_from_gtype (type, arg->v_int);
                    } else {
                        object = pyg_flags_from_gtype (type, arg->v_uint);
                    }

                    break;
                }
                case GI_INFO_TYPE_INTERFACE:
                case GI_INFO_TYPE_OBJECT:
                    object = pygi_arg_gobject_to_py_called_from_c (arg, transfer);

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

                _pygi_hash_pointer_to_arg (&value, g_type_info_get_tag (value_type_info));
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
        {
            GError *error = (GError *) arg->v_pointer;
            if (error != NULL && transfer == GI_TRANSFER_NOTHING) {
                /* If we have not been transferred the ownership we must copy
                 * the error, because pygi_error_check() is going to free it.
                 */
                error = g_error_copy (error);
            }

            if (pygi_error_check (&error)) {
                PyObject *err_type;
                PyObject *err_value;
                PyObject *err_trace;
                PyErr_Fetch (&err_type, &err_value, &err_trace);
                Py_XDECREF (err_type);
                Py_XDECREF (err_trace);
                object = err_value;
            } else {
                object = Py_None;
                Py_INCREF (object);
                break;
            }
            break;
        }
        default:
        {
            g_assert_not_reached();
        }
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
                    } else if (info_type == GI_INFO_TYPE_STRUCT &&
                               g_struct_info_is_foreign ((GIStructInfo*) info)) {
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
}


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
gi_argument_from_py_ssize_t (GIArgument   *arg_out,
                             Py_ssize_t    size_in,
                             GITypeTag     type_tag)                             
{
    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
    case GI_TYPE_TAG_BOOLEAN:
        goto unhandled_type;

    case GI_TYPE_TAG_INT8:
        if (size_in >= G_MININT8 && size_in <= G_MAXINT8) {
            arg_out->v_int8 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_UINT8:
        if (size_in >= 0 && size_in <= G_MAXUINT8) {
            arg_out->v_uint8 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_INT16:
        if (size_in >= G_MININT16 && size_in <= G_MAXINT16) {
            arg_out->v_int16 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_UINT16:
        if (size_in >= 0 && size_in <= G_MAXUINT16) {
            arg_out->v_uint16 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

        /* Ranges assume two's complement */
    case GI_TYPE_TAG_INT32:
        if (size_in >= G_MININT32 && size_in <= G_MAXINT32) {
            arg_out->v_int32 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_UINT32:
        if (size_in >= 0 && size_in <= G_MAXUINT32) {
            arg_out->v_uint32 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }

    case GI_TYPE_TAG_INT64:
        arg_out->v_int64 = size_in;
        return TRUE;

    case GI_TYPE_TAG_UINT64:
        if (size_in >= 0) {
            arg_out->v_uint64 = size_in;
            return TRUE;
        } else {
            goto overflow;
        }
            
    case GI_TYPE_TAG_FLOAT:
    case GI_TYPE_TAG_DOUBLE:
    case GI_TYPE_TAG_GTYPE:
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_ARRAY:
    case GI_TYPE_TAG_INTERFACE:
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
    case GI_TYPE_TAG_GHASH:
    case GI_TYPE_TAG_ERROR:
    case GI_TYPE_TAG_UNICHAR:
    default:
        goto unhandled_type;
    }

 overflow:
    PyErr_Format (PyExc_OverflowError,
                  "Unable to marshal C Py_ssize_t %zd to %s",
                  size_in,
                  g_type_tag_to_string (type_tag));
    return FALSE;

 unhandled_type:
    PyErr_Format (PyExc_TypeError,
                  "Unable to marshal C Py_ssize_t %zd to %s",
                  size_in,
                  g_type_tag_to_string (type_tag));
    return FALSE;
}

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
_pygi_marshal_from_py_void (PyGIInvokeState   *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache      *arg_cache,
                            PyObject          *py_arg,
                            GIArgument        *arg,
                            gpointer          *cleanup_data)
{
    g_warn_if_fail (arg_cache->transfer == GI_TRANSFER_NOTHING);

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
    } else if (PYGLIB_CPointer_Check(py_arg)) {
        arg->v_pointer = PYGLIB_CPointer_GetPointer (py_arg, NULL);
    } else if (PYGLIB_PyLong_Check(py_arg) || PyLong_Check(py_arg)) {
        arg->v_pointer = PyLong_AsVoidPtr (py_arg);
    } else {
        PyErr_SetString(PyExc_ValueError,
                        "Pointer arguments are restricted to integers, capsules, and None. "
                        "See: https://bugzilla.gnome.org/show_bug.cgi?id=683599");
        return FALSE;
    }

    *cleanup_data = arg->v_pointer;
    return TRUE;
}

static gboolean
check_valid_double (double x, double min, double max)
{
    char buf[100];

    if ((x < min || x > max) && x != INFINITY && x != -INFINITY && x != NAN) {
        if (PyErr_Occurred())
            PyErr_Clear ();

        /* we need this as PyErr_Format() does not support float types */
        snprintf (buf, sizeof (buf), "%g not in range %g to %g", x, min, max);
        PyErr_SetString (PyExc_OverflowError, buf);
        return FALSE;
    }
    return TRUE;
}

static gboolean
_pygi_py_arg_to_double (PyObject *py_arg, double *double_)
{
    PyObject *py_float;

    if (!PyNumber_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_float = PyNumber_Float (py_arg);
    if (!py_float)
        return FALSE;

    *double_ = PyFloat_AsDouble (py_float);
    Py_DECREF (py_float);


    return TRUE;
}

static gboolean
_pygi_marshal_from_py_float (PyObject          *py_arg,
                             GIArgument        *arg)
{
    double double_;

    if (!_pygi_py_arg_to_double (py_arg, &double_))
        return FALSE;

    if (PyErr_Occurred () || !check_valid_double (double_, -G_MAXFLOAT, G_MAXFLOAT))
        return FALSE;

    arg->v_float = double_;
    return TRUE;
}

static gboolean
_pygi_marshal_from_py_double (PyObject          *py_arg,
                              GIArgument        *arg)
{
    double double_;

    if (!_pygi_py_arg_to_double (py_arg, &double_))
        return FALSE;

    if (PyErr_Occurred () || !check_valid_double (double_, -G_MAXDOUBLE, G_MAXDOUBLE))
        return FALSE;

    arg->v_double = double_;
    return TRUE;
}

static gboolean
_pygi_marshal_from_py_unichar (PyObject          *py_arg,
                               GIArgument        *arg)
{
    Py_ssize_t size;
    gchar *string_;

    if (py_arg == Py_None) {
        arg->v_uint32 = 0;
        return FALSE;
    }

    if (PyUnicode_Check (py_arg)) {
       PyObject *py_bytes;

       size = PyUnicode_GET_SIZE (py_arg);
       py_bytes = PyUnicode_AsUTF8String (py_arg);
       if (!py_bytes)
           return FALSE;

       string_ = g_strdup(PYGLIB_PyBytes_AsString (py_bytes));
       Py_DECREF (py_bytes);

#if PY_VERSION_HEX < 0x03000000
    } else if (PyString_Check (py_arg)) {
       PyObject *pyuni = PyUnicode_FromEncodedObject (py_arg, "UTF-8", "strict");
       if (!pyuni)
           return FALSE;

       size = PyUnicode_GET_SIZE (pyuni);
       string_ = g_strdup (PyString_AsString(py_arg));
       Py_DECREF (pyuni);
#endif
    } else {
       PyErr_Format (PyExc_TypeError, "Must be string, not %s",
                     py_arg->ob_type->tp_name);
       return FALSE;
    }

    if (size != 1) {
       PyErr_Format (PyExc_TypeError, "Must be a one character string, not %lld characters",
                     (long long) size);
       g_free (string_);
       return FALSE;
    }

    arg->v_uint32 = g_utf8_get_char (string_);
    g_free (string_);

    return TRUE;
}

static gboolean
_pygi_marshal_from_py_gtype (PyObject          *py_arg,
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

static gboolean
_pygi_marshal_from_py_utf8 (PyObject          *py_arg,
                            GIArgument        *arg,
                            gpointer          *cleanup_data)
{
    gchar *string_;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (PyUnicode_Check (py_arg)) {
        PyObject *pystr_obj = PyUnicode_AsUTF8String (py_arg);
        if (!pystr_obj)
            return FALSE;

        string_ = g_strdup (PYGLIB_PyBytes_AsString (pystr_obj));
        Py_DECREF (pystr_obj);
    }
#if PY_VERSION_HEX < 0x03000000
    else if (PyString_Check (py_arg)) {
        string_ = g_strdup (PyString_AsString (py_arg));
    }
#endif
    else {
        PyErr_Format (PyExc_TypeError, "Must be string, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    arg->v_string = string_;
    *cleanup_data = arg->v_string;
    return TRUE;
}

static gboolean
_pygi_marshal_from_py_filename (PyObject          *py_arg,
                                GIArgument        *arg,
                                gpointer          *cleanup_data)
{
    gchar *string_;
    GError *error = NULL;

    if (PyUnicode_Check (py_arg)) {
        PyObject *pystr_obj = PyUnicode_AsUTF8String (py_arg);
        if (!pystr_obj)
            return FALSE;

        string_ = g_strdup (PYGLIB_PyBytes_AsString (pystr_obj));
        Py_DECREF (pystr_obj);
    }
#if PY_VERSION_HEX < 0x03000000
    else if (PyString_Check (py_arg)) {
        string_ = g_strdup (PyString_AsString (py_arg));
    }
#endif
    else {
        PyErr_Format (PyExc_TypeError, "Must be string, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    arg->v_string = g_filename_from_utf8 (string_, -1, NULL, NULL, &error);
    g_free (string_);

    if (arg->v_string == NULL) {
        PyErr_SetString (PyExc_Exception, error->message);
        g_error_free (error);
        /* TODO: Convert the error to an exception. */
        return FALSE;
    }

    *cleanup_data = arg->v_string;
    return TRUE;
}

static gboolean
_pygi_marshal_from_py_long (PyObject   *object,   /* in */
                            GIArgument *arg,      /* out */
                            GITypeTag   type_tag,
                            GITransfer  transfer)
{
    PyObject *number;

    if (!PyNumber_Check (object)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      object->ob_type->tp_name);
        return FALSE;
    }

#if PY_MAJOR_VERSION < 3
    {
        PyObject *tmp = PyNumber_Int (object);
        if (tmp) {
            number = PyNumber_Long (tmp);
            Py_DECREF (tmp);
        } else {
            number = PyNumber_Long (object);
        }
    }
#else
    number = PyNumber_Long (object);
#endif

    if (number == NULL) {
        PyErr_SetString (PyExc_TypeError, "expected int argument");
        return FALSE;
    }

    switch (type_tag) {
        case GI_TYPE_TAG_INT8:
        {
            long long_value = PyLong_AsLong (number);
            if (PyErr_Occurred()) {
                break;
            } else if (long_value < G_MININT8 || long_value > G_MAXINT8) {
                PyErr_Format (PyExc_OverflowError, "%ld not in range %ld to %ld",
                              long_value, (long)G_MININT8, (long)G_MAXINT8);
            } else {
                arg->v_int8 = long_value;
            }
            break;
        }

        case GI_TYPE_TAG_UINT8:
        {
            long long_value = PyLong_AsLong (number);
            if (PyErr_Occurred()) {
                break;
            } else if (long_value < 0 || long_value > G_MAXUINT8) {
                PyErr_Format (PyExc_OverflowError, "%ld not in range %ld to %ld",
                              long_value, (long)0, (long)G_MAXUINT8);
            } else {
                arg->v_uint8 = long_value;
            }
            break;
        }

        case GI_TYPE_TAG_INT16:
        {
            long long_value = PyLong_AsLong (number);
            if (PyErr_Occurred()) {
                break;
            } else if (long_value < G_MININT16 || long_value > G_MAXINT16) {
                PyErr_Format (PyExc_OverflowError, "%ld not in range %ld to %ld",
                              long_value, (long)G_MININT16, (long)G_MAXINT16);
            } else {
                arg->v_int16 = long_value;
            }
            break;
        }

        case GI_TYPE_TAG_UINT16:
        {
            long long_value = PyLong_AsLong (number);
            if (PyErr_Occurred()) {
                break;
            } else if (long_value < 0 || long_value > G_MAXUINT16) {
                PyErr_Format (PyExc_OverflowError, "%ld not in range %ld to %ld",
                              long_value, (long)0, (long)G_MAXUINT16);
            } else {
                arg->v_uint16 = long_value;
            }
            break;
        }

        case GI_TYPE_TAG_INT32:
        {
            long long_value = PyLong_AsLong (number);
            if (PyErr_Occurred()) {
                break;
            } else if (long_value < G_MININT32 || long_value > G_MAXINT32) {
                PyErr_Format (PyExc_OverflowError, "%ld not in range %ld to %ld",
                              long_value, (long)G_MININT32, (long)G_MAXINT32);
            } else {
                arg->v_int32 = long_value;
            }
            break;
        }

        case GI_TYPE_TAG_UINT32:
        {
            PY_LONG_LONG long_value = PyLong_AsLongLong (number);
            if (PyErr_Occurred()) {
                break;
            } else if (long_value < 0 || long_value > G_MAXUINT32) {
                PyErr_Format (PyExc_OverflowError, "%lld not in range %ld to %lu",
                              long_value, (long)0, (unsigned long)G_MAXUINT32);
            } else {
                arg->v_uint32 = long_value;
            }
            break;
        }

        case GI_TYPE_TAG_INT64:
        {
            /* Rely on Python overflow error and convert to ValueError for 64 bit values */
            arg->v_int64 = PyLong_AsLongLong (number);
            break;
        }

        case GI_TYPE_TAG_UINT64:
        {
            /* Rely on Python overflow error and convert to ValueError for 64 bit values */
            arg->v_uint64 = PyLong_AsUnsignedLongLong (number);
            break;
        }

        default:
            g_assert_not_reached ();
    }

    Py_DECREF (number);

    if (PyErr_Occurred())
        return FALSE;
    return TRUE;
}

gboolean
_pygi_marshal_from_py_basic_type (PyObject   *object,   /* in */
                                  GIArgument *arg,      /* out */
                                  GITypeTag   type_tag,
                                  GITransfer  transfer,
                                  gpointer   *cleanup_data /* out */)
{
    switch (type_tag) {
        case GI_TYPE_TAG_VOID:
            g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
            if (object == Py_None) {
                arg->v_pointer = NULL;
            } else if (!PYGLIB_PyLong_Check(object)  && !PyLong_Check(object)) {
                PyErr_SetString(PyExc_TypeError,
                    "Pointer assignment is restricted to integer values. "
                    "See: https://bugzilla.gnome.org/show_bug.cgi?id=683599");
            } else {
                arg->v_pointer = PyLong_AsVoidPtr (object);
                *cleanup_data = arg->v_pointer;
            }
            break;
        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_UINT8:
            if (PYGLIB_PyBytes_Check (object)) {
                if (PYGLIB_PyBytes_Size (object) != 1) {
                    PyErr_Format (PyExc_TypeError, "Must be a single character");
                    return FALSE;
                }
                if (type_tag == GI_TYPE_TAG_INT8) {
                    arg->v_int8 = (gint8)(PYGLIB_PyBytes_AsString (object)[0]);
                } else {
                    arg->v_uint8 = (guint8)(PYGLIB_PyBytes_AsString (object)[0]);
                }
            } else {
                return _pygi_marshal_from_py_long (object, arg, type_tag, transfer);
            }
            break;
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
            return _pygi_marshal_from_py_long (object, arg, type_tag, transfer);

        case GI_TYPE_TAG_BOOLEAN:
            arg->v_boolean = PyObject_IsTrue (object);
            break;

        case GI_TYPE_TAG_FLOAT:
            return _pygi_marshal_from_py_float (object, arg);

        case GI_TYPE_TAG_DOUBLE:
            return _pygi_marshal_from_py_double (object, arg);

        case GI_TYPE_TAG_GTYPE:
            return _pygi_marshal_from_py_gtype (object, arg);

        case GI_TYPE_TAG_UNICHAR:
            return _pygi_marshal_from_py_unichar (object, arg);

        case GI_TYPE_TAG_UTF8:
            return _pygi_marshal_from_py_utf8 (object, arg, cleanup_data);

        case GI_TYPE_TAG_FILENAME:
            return _pygi_marshal_from_py_filename (object, arg, cleanup_data);

        default:
            return FALSE;
    }

    if (PyErr_Occurred())
        return FALSE;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_basic_type_cache_adapter (PyGIInvokeState   *state,
                                                PyGICallableCache *callable_cache,
                                                PyGIArgCache      *arg_cache,
                                                PyObject          *py_arg,
                                                GIArgument        *arg,
                                                gpointer          *cleanup_data)
{
    return _pygi_marshal_from_py_basic_type (py_arg,
                                             arg,
                                             arg_cache->type_tag,
                                             arg_cache->transfer,
                                             cleanup_data);
}

gboolean
_pygi_marshal_from_py_array (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg,
                             gpointer          *cleanup_data)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
    int i = 0;
    int success_count = 0;
    Py_ssize_t length;
    gssize item_size;
    gboolean is_ptr_array;
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

    item_size = sequence_cache->item_size;
    is_ptr_array = (sequence_cache->array_type == GI_ARRAY_TYPE_PTR_ARRAY);
    if (is_ptr_array) {
        array_ = (GArray *)g_ptr_array_sized_new (length);
    } else {
        array_ = g_array_sized_new (sequence_cache->is_zero_terminated,
                                    TRUE,
                                    item_size,
                                    length);
    }

    if (array_ == NULL) {
        PyErr_NoMemory ();
        return FALSE;
    }

    if (sequence_cache->item_cache->type_tag == GI_TYPE_TAG_UINT8 &&
        PYGLIB_PyBytes_Check (py_arg)) {
        memcpy(array_->data, PYGLIB_PyBytes_AsString (py_arg), length);
        array_->len = length;
        if (sequence_cache->is_zero_terminated) {
            /* If array_ has been created with zero_termination, space for the
             * terminator is properly allocated, so we're not off-by-one here. */
            array_->data[length] = '\0';
        }
        goto array_success;
    }

    from_py_marshaller = sequence_cache->item_cache->from_py_marshaller;
    for (i = 0, success_count = 0; i < length; i++) {
        GIArgument item = {0};
        gpointer item_cleanup_data = NULL;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL)
            goto err;

        if (!from_py_marshaller ( state,
                                  callable_cache,
                                  sequence_cache->item_cache,
                                  py_item,
                                 &item,
                                 &item_cleanup_data)) {
            Py_DECREF (py_item);
            goto err;
        }
        Py_DECREF (py_item);

        if (item_cleanup_data != NULL && item_cleanup_data != item.v_pointer) {
            /* We only support one level of data discrepancy between an items
             * data and its cleanup data. This is because we only track a single
             * extra cleanup data pointer per-argument and cannot track the entire
             * array of items differing data and cleanup_data.
             * For example, this would fail if trying to marshal an array of
             * callback closures marked with SCOPE call type where the cleanup data
             * is different from the items v_pointer, likewise an array of arrays.
             */
            PyErr_SetString(PyExc_RuntimeError, "Cannot cleanup item data for array due to "
                                                "the items data its cleanup data being different.");
            goto err;
        }

        /* FIXME: it is much more efficent to have seperate marshaller
         *        for ptr arrays than doing the evaluation
         *        and casting each loop iteration
         */
        if (is_ptr_array) {
            g_ptr_array_add((GPtrArray *)array_, item.v_pointer);
        } else if (sequence_cache->item_cache->is_pointer) {
            /* if the item is a pointer, simply copy the pointer */
            g_assert (item_size == sizeof (item.v_pointer));
            g_array_insert_val (array_, i, item);
        } else if (sequence_cache->item_cache->type_tag == GI_TYPE_TAG_INTERFACE) {
            /* Special case handling of flat arrays of gvalue/boxed/struct */
            PyGIInterfaceCache *item_iface_cache = (PyGIInterfaceCache *) sequence_cache->item_cache;
            GIBaseInfo *base_info = (GIBaseInfo *) item_iface_cache->interface_info;
            GIInfoType info_type = g_base_info_get_type (base_info);

            switch (info_type) {
                case GI_INFO_TYPE_UNION:
                case GI_INFO_TYPE_STRUCT:
                {
                    PyGIArgCache *item_arg_cache = (PyGIArgCache *)item_iface_cache;
                    PyGIMarshalCleanupFunc from_py_cleanup = item_arg_cache->from_py_cleanup;

                    if (g_type_is_a (item_iface_cache->g_type, G_TYPE_VALUE)) {
                        /* Special case GValue flat arrays to properly init and copy the contents. */
                        GValue* dest = (GValue*) (array_->data + (i * item_size));
                        if (item.v_pointer != NULL) {
                            memset (dest, 0, item_size);
                            g_value_init (dest, G_VALUE_TYPE ((GValue*) item.v_pointer));
                            g_value_copy ((GValue*) item.v_pointer, dest);
                        }
                        /* Manually increment the length because we are manually setting the memory. */
                        array_->len++;

                    } else {
                        /* Handles flat arrays of boxed or struct types. */
                        g_array_insert_vals (array_, i, item.v_pointer, 1);
                    }

                    /* Cleanup any memory left by the per-item marshaler because
                     * _pygi_marshal_cleanup_from_py_array will not know about this
                     * due to "item" being a temporarily marshaled value done on the stack.
                     */
                    if (from_py_cleanup)
                        from_py_cleanup (state, item_arg_cache, py_item, item_cleanup_data, TRUE);

                    break;
                }
                default:
                    g_array_insert_val (array_, i, item);
            }
        } else {
            /* default value copy of a simple type */
            g_array_insert_val (array_, i, item);
        }

        success_count++;
        continue;
err:
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            gsize j;
            PyGIMarshalCleanupFunc cleanup_func =
                sequence_cache->item_cache->from_py_cleanup;

            /* Only attempt per item cleanup on pointer items */
            if (sequence_cache->item_cache->is_pointer) {
                for(j = 0; j < success_count; j++) {
                    PyObject *py_item = PySequence_GetItem (py_arg, j);
                    cleanup_func (state,
                                  sequence_cache->item_cache,
                                  py_item,
                                  is_ptr_array ?
                                          g_ptr_array_index ((GPtrArray *)array_, j) :
                                          g_array_index (array_, gpointer, j),
                                  TRUE);
                    Py_DECREF (py_item);
                }
            }
        }

        if (is_ptr_array)
            g_ptr_array_free ( ( GPtrArray *)array_, TRUE);
        else
            g_array_free (array_, TRUE);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

array_success:
    if (sequence_cache->len_arg_index >= 0) {
        /* we have an child arg to handle */
        PyGIArgCache *child_cache =
            _pygi_callable_cache_get_arg (callable_cache, sequence_cache->len_arg_index);

        if (child_cache->direction == PYGI_DIRECTION_BIDIRECTIONAL) {
            gint *len_arg = (gint *)state->in_args[child_cache->c_arg_index].v_pointer;
            /* if we are not setup yet just set the in arg */
            if (len_arg == NULL) {
                if (!gi_argument_from_py_ssize_t (&state->in_args[child_cache->c_arg_index],
                                                  length,
                                                  child_cache->type_tag)) {
                    goto err;
                }
            } else {
                *len_arg = length;
            }
        } else {
            if (!gi_argument_from_py_ssize_t (&state->in_args[child_cache->c_arg_index],
                                              length,
                                              child_cache->type_tag)) {
                goto err;
            }
        }
    }

    if (sequence_cache->array_type == GI_ARRAY_TYPE_C) {
        /* In the case of GI_ARRAY_C, we give the data directly as the argument
         * but keep the array_ wrapper as cleanup data so we don't have to find
         * it's length again.
         */
        arg->v_pointer = array_->data;

        if (arg_cache->transfer == GI_TRANSFER_EVERYTHING) {
            g_array_free (array_, FALSE);
            *cleanup_data = NULL;
        } else {
            *cleanup_data = array_;
        }
    } else {
        arg->v_pointer = array_;

        if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
            /* Free everything in cleanup. */
            *cleanup_data = array_;
        } else if (arg_cache->transfer == GI_TRANSFER_CONTAINER) {
            /* Make a shallow copy so we can free the elements later in cleanup
             * because it is possible invoke will free the list before our cleanup. */
            *cleanup_data = is_ptr_array ?
                    (gpointer)g_ptr_array_ref ((GPtrArray *)array_) :
                    (gpointer)g_array_ref (array_);
        } else { /* GI_TRANSFER_EVERYTHING */
            /* No cleanup, everything is given to the callee. */
            *cleanup_data = NULL;
        }
    }

    return TRUE;
}

gboolean
_pygi_marshal_from_py_glist (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg,
                             gpointer          *cleanup_data)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
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

    length = PySequence_Length (py_arg);
    if (length < 0)
        return FALSE;

    if (sequence_cache->fixed_size >= 0 &&
        sequence_cache->fixed_size != length) {
        PyErr_Format (PyExc_ValueError, "Must contain %zd items, not %zd",
                      sequence_cache->fixed_size, length);

        return FALSE;
    }

    from_py_marshaller = sequence_cache->item_cache->from_py_marshaller;
    for (i = 0; i < length; i++) {
        GIArgument item = {0};
        gpointer item_cleanup_data = NULL;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL)
            goto err;

        if (!from_py_marshaller ( state,
                                  callable_cache,
                                  sequence_cache->item_cache,
                                  py_item,
                                 &item,
                                 &item_cleanup_data))
            goto err;

        Py_DECREF (py_item);
        list_ = g_list_prepend (list_, _pygi_arg_to_hash_pointer (&item, sequence_cache->item_cache->type_tag));
        continue;
err:
        /* FIXME: clean up list
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            PyGIMarshalCleanupFunc cleanup = sequence_cache->item_cache->from_py_cleanup;
        }
        */
        Py_DECREF (py_item);
        g_list_free (list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = g_list_reverse (list_);

    if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
        /* Free everything in cleanup. */
        *cleanup_data = arg->v_pointer;
    } else if (arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        /* Make a shallow copy so we can free the elements later in cleanup
         * because it is possible invoke will free the list before our cleanup. */
        *cleanup_data = g_list_copy (arg->v_pointer);
    } else { /* GI_TRANSFER_EVERYTHING */
        /* No cleanup, everything is given to the callee. */
        *cleanup_data = NULL;
    }
    return TRUE;
}

gboolean
_pygi_marshal_from_py_gslist (PyGIInvokeState   *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache      *arg_cache,
                              PyObject          *py_arg,
                              GIArgument        *arg,
                              gpointer          *cleanup_data)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
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

    length = PySequence_Length (py_arg);
    if (length < 0)
        return FALSE;

    if (sequence_cache->fixed_size >= 0 &&
        sequence_cache->fixed_size != length) {
        PyErr_Format (PyExc_ValueError, "Must contain %zd items, not %zd",
                      sequence_cache->fixed_size, length);

        return FALSE;
    }

    from_py_marshaller = sequence_cache->item_cache->from_py_marshaller;
    for (i = 0; i < length; i++) {
        GIArgument item = {0};
        gpointer item_cleanup_data = NULL;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL)
            goto err;

        if (!from_py_marshaller ( state,
                             callable_cache,
                             sequence_cache->item_cache,
                             py_item,
                            &item,
                            &item_cleanup_data))
            goto err;

        Py_DECREF (py_item);
        list_ = g_slist_prepend (list_, _pygi_arg_to_hash_pointer (&item, sequence_cache->item_cache->type_tag));
        continue;
err:
        /* FIXME: Clean up list
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            PyGIMarshalCleanupFunc cleanup = sequence_cache->item_cache->from_py_cleanup;
        }
        */

        Py_DECREF (py_item);
        g_slist_free (list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = g_slist_reverse (list_);

    if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
        /* Free everything in cleanup. */
        *cleanup_data = arg->v_pointer;
    } else if (arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        /* Make a shallow copy so we can free the elements later in cleanup
         * because it is possible invoke will free the list before our cleanup. */
        *cleanup_data = g_slist_copy (arg->v_pointer);
    } else { /* GI_TRANSFER_EVERYTHING */
        /* No cleanup, everything is given to the callee. */
        *cleanup_data = NULL;
    }

    return TRUE;
}

gboolean
_pygi_marshal_from_py_ghash (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg,
                             gpointer          *cleanup_data)
{
    PyGIMarshalFromPyFunc key_from_py_marshaller;
    PyGIMarshalFromPyFunc value_from_py_marshaller;

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

    py_keys = PyMapping_Keys (py_arg);
    if (py_keys == NULL) {
        PyErr_Format (PyExc_TypeError, "Must be mapping, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    length = PyMapping_Length (py_arg);
    if (length < 0) {
        Py_DECREF (py_keys);
        return FALSE;
    }

    py_values = PyMapping_Values (py_arg);
    if (py_values == NULL) {
        Py_DECREF (py_keys);
        return FALSE;
    }

    key_from_py_marshaller = hash_cache->key_cache->from_py_marshaller;
    value_from_py_marshaller = hash_cache->value_cache->from_py_marshaller;

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
        PyErr_NoMemory ();
        Py_DECREF (py_keys);
        Py_DECREF (py_values);
        return FALSE;
    }

    for (i = 0; i < length; i++) {
        GIArgument key, value;
        gpointer key_cleanup_data = NULL;
        gpointer value_cleanup_data = NULL;
        PyObject *py_key = PyList_GET_ITEM (py_keys, i);
        PyObject *py_value = PyList_GET_ITEM (py_values, i);
        if (py_key == NULL || py_value == NULL)
            goto err;

        if (!key_from_py_marshaller ( state,
                                      callable_cache,
                                      hash_cache->key_cache,
                                      py_key,
                                     &key,
                                     &key_cleanup_data))
            goto err;

        if (!value_from_py_marshaller ( state,
                                        callable_cache,
                                        hash_cache->value_cache,
                                        py_value,
                                       &value,
                                       &value_cleanup_data))
            goto err;

        g_hash_table_insert (hash_,
                             _pygi_arg_to_hash_pointer (&key, hash_cache->key_cache->type_tag),
                             _pygi_arg_to_hash_pointer (&value, hash_cache->value_cache->type_tag));
        continue;
err:
        /* FIXME: cleanup hash keys and values */
        Py_XDECREF (py_key);
        Py_XDECREF (py_value);
        Py_DECREF (py_keys);
        Py_DECREF (py_values);
        g_hash_table_unref (hash_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = hash_;

    if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
        /* Free everything in cleanup. */
        *cleanup_data = arg->v_pointer;
    } else if (arg_cache->transfer == GI_TRANSFER_CONTAINER) {
        /* Make a shallow copy so we can free the elements later in cleanup
         * because it is possible invoke will free the list before our cleanup. */
        *cleanup_data = g_hash_table_ref (arg->v_pointer);
    } else { /* GI_TRANSFER_EVERYTHING */
        /* No cleanup, everything is given to the callee.
         * Note that the keys and values will leak for transfer everything because
         * we do not use g_hash_table_new_full and set key/value_destroy_func. */
        *cleanup_data = NULL;
    }

    return TRUE;
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
            if (py_user_data == _PyGIDefaultArgPlaceholder)
                py_user_data = NULL;
        }
    }

    if (py_arg == Py_None && !(py_user_data == Py_None || py_user_data == NULL)) {
        PyErr_Format (PyExc_TypeError,
                      "When passing None for a callback userdata must also be None");

        return FALSE;
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

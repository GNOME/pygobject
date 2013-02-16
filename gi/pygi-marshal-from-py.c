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
_is_union_member (PyGIInterfaceCache *iface_cache, PyObject *py_arg) {
    gint i;
    gint n_fields;
    GIUnionInfo *union_info;
    GIInfoType info_type;
    gboolean is_member = FALSE;

    info_type = g_base_info_get_type (iface_cache->interface_info);

    if (info_type != GI_INFO_TYPE_UNION)
        return FALSE;

    union_info = (GIUnionInfo *) iface_cache->interface_info;
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
                            GIArgument        *arg)
{
    g_warn_if_fail (arg_cache->transfer == GI_TRANSFER_NOTHING);

    if (PYGLIB_CPointer_Check(py_arg)) {
        arg->v_pointer = PYGLIB_CPointer_GetPointer (py_arg, NULL);
    } else {
        arg->v_pointer = py_arg;
    }

    return TRUE;
}

gboolean
_pygi_marshal_from_py_boolean (PyGIInvokeState   *state,
                               PyGICallableCache *callable_cache,
                               PyGIArgCache      *arg_cache,
                               PyObject          *py_arg,
                               GIArgument        *arg)
{
    arg->v_boolean = PyObject_IsTrue (py_arg);

    return TRUE;
}

gboolean
_pygi_marshal_from_py_int8 (PyGIInvokeState   *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache      *arg_cache,
                            PyObject          *py_arg,
                            GIArgument        *arg)
{
    PyObject *py_long;
    long long_;

    if (PYGLIB_PyBytes_Check (py_arg)) {

        if (PYGLIB_PyBytes_Size (py_arg) != 1) {
            PyErr_Format (PyExc_TypeError, "Must be a single character");
            return FALSE;
        }

        long_ = (char)(PYGLIB_PyBytes_AsString (py_arg)[0]);
    } else if (PyNumber_Check (py_arg)) {
        py_long = PYGLIB_PyNumber_Long (py_arg);
        if (!py_long)
            return FALSE;

        long_ = PYGLIB_PyLong_AsLong (py_long);
        Py_DECREF (py_long);

        if (PyErr_Occurred ()) {
            PyErr_Clear ();
            PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, -128, 127);
            return FALSE;
        }
    } else {
        PyErr_Format (PyExc_TypeError, "Must be number or single byte string, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    if (long_ < -128 || long_ > 127) {
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, -128, 127);
        return FALSE;
    }

    arg->v_int8 = long_;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_uint8 (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg)
{
    unsigned long long_;

    if (PYGLIB_PyBytes_Check (py_arg)) {

        if (PYGLIB_PyBytes_Size (py_arg) != 1) {
            PyErr_Format (PyExc_TypeError, "Must be a single character");
            return FALSE;
        }

        long_ = (unsigned char)(PYGLIB_PyBytes_AsString (py_arg)[0]);

    } else if (PyNumber_Check (py_arg)) {
        PyObject *py_long;
        py_long = PYGLIB_PyNumber_Long (py_arg);
        if (!py_long)
            return FALSE;

        long_ = PYGLIB_PyLong_AsLong (py_long);
        Py_DECREF (py_long);

        if (PyErr_Occurred ()) {
            PyErr_Clear();

            PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, 0, 255);
            return FALSE;
        }
    } else {
        PyErr_Format (PyExc_TypeError, "Must be number or single byte string, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    if (long_ < 0 || long_ > 255) {
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, 0, 255);
        return FALSE;
    }

    arg->v_uint8 = long_;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_int16 (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg)
{
    PyObject *py_long;
    long long_;

    if (!PyNumber_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long (py_arg);
    if (!py_long)
        return FALSE;

    long_ = PYGLIB_PyLong_AsLong (py_long);
    Py_DECREF (py_long);

    if (PyErr_Occurred ()) {
        PyErr_Clear ();
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, -32768, 32767);
        return FALSE;
    }

    if (long_ < -32768 || long_ > 32767) {
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, -32768, 32767);
        return FALSE;
    }

    arg->v_int16 = long_;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_uint16 (PyGIInvokeState   *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache      *arg_cache,
                              PyObject          *py_arg,
                              GIArgument        *arg)
{
    PyObject *py_long;
    long long_;

    if (!PyNumber_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long (py_arg);
    if (!py_long)
        return FALSE;

    long_ = PYGLIB_PyLong_AsLong (py_long);
    Py_DECREF (py_long);

    if (PyErr_Occurred ()) {
        PyErr_Clear ();
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, 0, 65535);
        return FALSE;
    }

    if (long_ < 0 || long_ > 65535) {
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, 0, 65535);
        return FALSE;
    }

    arg->v_uint16 = long_;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_int32 (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg)
{
    PyObject *py_long;
    long long_;

    if (!PyNumber_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long (py_arg);
    if (!py_long)
        return FALSE;

    long_ = PYGLIB_PyLong_AsLong (py_long);
    Py_DECREF (py_long);

    if (PyErr_Occurred ()) {
        PyErr_Clear();
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, G_MININT32, G_MAXINT32);
        return FALSE;
    }

    if (long_ < G_MININT32 || long_ > G_MAXINT32) {
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, G_MININT32, G_MAXINT32);
        return FALSE;
    }

    arg->v_int32 = long_;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_uint32 (PyGIInvokeState   *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache      *arg_cache,
                              PyObject          *py_arg,
                              GIArgument        *arg)
{
    PyObject *py_long;
    long long long_;

    if (!PyNumber_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long (py_arg);
    if (!py_long)
        return FALSE;

#if PY_VERSION_HEX < 0x03000000
    if (PyInt_Check (py_long))
        long_ = PyInt_AsLong (py_long);
    else
#endif
        long_ = PyLong_AsLongLong (py_long);

    Py_DECREF (py_long);

    if (PyErr_Occurred ()) {
        PyErr_Clear ();
        PyErr_Format (PyExc_ValueError, "%lld not in range %i to %u", long_, 0, G_MAXUINT32);
        return FALSE;
    }

    if (long_ < 0 || long_ > G_MAXUINT32) {
        PyErr_Format (PyExc_ValueError, "%lld not in range %i to %u", long_, 0, G_MAXUINT32);
        return FALSE;
    }

    arg->v_uint32 = long_;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_int64 (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg)
{
    PyObject *py_long;
    gint64 long_;

    if (!PyNumber_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long (py_arg);
    if (!py_long)
        return FALSE;

#if PY_VERSION_HEX < 0x03000000
    if (PyInt_Check (py_long))
        long_ = (gint64) PyInt_AS_LONG (py_long);
    else
#endif
        long_ = (gint64) PyLong_AsLongLong (py_long);

    Py_DECREF (py_long);

    if (PyErr_Occurred ()) {
        /* OverflowError occured but range errors should be returned as ValueError */
        char *long_str;
        PyObject *py_str;

        PyErr_Clear ();

        py_str = PyObject_Str (py_long);

        if (PyUnicode_Check (py_str)) {
            PyObject *py_bytes = PyUnicode_AsUTF8String (py_str);
            Py_DECREF (py_str);

            if (py_bytes == NULL)
                return FALSE;

            long_str = g_strdup (PYGLIB_PyBytes_AsString (py_bytes));
            if (long_str == NULL) {
                PyErr_NoMemory ();
                return FALSE;
            }

            Py_DECREF (py_bytes);
        } else {
            long_str = g_strdup (PYGLIB_PyBytes_AsString(py_str));
            Py_DECREF (py_str);
        }

        PyErr_Format (PyExc_ValueError, "%s not in range %lld to %lld",
                      long_str, (long long) G_MININT64, (long long) G_MAXINT64);

        g_free (long_str);
        return FALSE;
    }

    arg->v_int64 = long_;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_uint64 (PyGIInvokeState   *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache      *arg_cache,
                              PyObject          *py_arg,
                              GIArgument        *arg)
{
    PyObject *py_long;
    guint64 ulong_;

    if (!PyNumber_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_long = PYGLIB_PyNumber_Long (py_arg);
    if (!py_long)
        return FALSE;

#if PY_VERSION_HEX < 0x03000000
    if (PyInt_Check (py_long)) {
        long long_ =  PyInt_AsLong (py_long);
        if (long_ < 0 || long_ > G_MAXUINT64) {
            PyErr_Format (PyExc_ValueError, "%" G_GUINT64_FORMAT " not in range %d to %" G_GUINT64_FORMAT,
                          (gint64) long_, 0, G_MAXUINT64);
            return FALSE;
        }
        ulong_ = (guint64) long_;
    } else
#endif
        ulong_ = PyLong_AsUnsignedLongLong (py_long);

    Py_DECREF (py_long);

    if (PyErr_Occurred ()) {
        /* OverflowError occured but range errors should be returned as ValueError */
        char *long_str;
        PyObject *py_str;

        PyErr_Clear ();

        py_str = PyObject_Str (py_long);

        if (PyUnicode_Check (py_str)) {
            PyObject *py_bytes = PyUnicode_AsUTF8String (py_str);
            Py_DECREF (py_str);

            if (py_bytes == NULL)
                return FALSE;

            long_str = g_strdup (PYGLIB_PyBytes_AsString (py_bytes));
            if (long_str == NULL) {
                PyErr_NoMemory ();
                return FALSE;
            }

            Py_DECREF (py_bytes);
        } else {
            long_str = g_strdup (PYGLIB_PyBytes_AsString (py_str));
            Py_DECREF (py_str);
        }

        PyErr_Format (PyExc_ValueError, "%s not in range %d to %" G_GUINT64_FORMAT,
                      long_str, 0, G_MAXUINT64);

        g_free (long_str);
        return FALSE;
    }

    if (ulong_ > G_MAXUINT64) {
        PyErr_Format (PyExc_ValueError, "%" G_GUINT64_FORMAT " not in range %d to %" G_GUINT64_FORMAT, ulong_, 0, G_MAXUINT64);
        return FALSE;
    }

    arg->v_uint64 = ulong_;

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
        PyErr_SetString (PyExc_ValueError, buf);
        return FALSE;
    }
    return TRUE;
}

gboolean
_pygi_marshal_from_py_float (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg)
{
    PyObject *py_float;
    double double_;

    if (!PyNumber_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_float = PyNumber_Float (py_arg);
    if (!py_float)
        return FALSE;

    double_ = PyFloat_AsDouble (py_float);
    Py_DECREF (py_float);

    if (PyErr_Occurred () || !check_valid_double (double_, -G_MAXFLOAT, G_MAXFLOAT))
        return FALSE;

    arg->v_float = double_;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_double (PyGIInvokeState   *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache      *arg_cache,
                              PyObject          *py_arg,
                              GIArgument        *arg)
{
    PyObject *py_float;
    double double_;

    if (!PyNumber_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_float = PyNumber_Float (py_arg);
    if (!py_float)
        return FALSE;

    double_ = PyFloat_AsDouble (py_float);
    Py_DECREF (py_float);

    if (PyErr_Occurred () || !check_valid_double (double_, -G_MAXDOUBLE, G_MAXDOUBLE))
        return FALSE;

    arg->v_double = double_;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_unichar (PyGIInvokeState   *state,
                               PyGICallableCache *callable_cache,
                               PyGIArgCache      *arg_cache,
                               PyObject          *py_arg,
                               GIArgument        *arg)
{
    Py_ssize_t size;
    gchar *string_;

    if (PyUnicode_Check (py_arg)) {
       PyObject *py_bytes;

       size = PyUnicode_GET_SIZE (py_arg);
       py_bytes = PyUnicode_AsUTF8String (py_arg);
       string_ = strdup(PYGLIB_PyBytes_AsString (py_bytes));
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
gboolean
_pygi_marshal_from_py_gtype (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
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
_pygi_marshal_from_py_utf8 (PyGIInvokeState   *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache      *arg_cache,
                            PyObject          *py_arg,
                            GIArgument        *arg)
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
    return TRUE;
}

gboolean
_pygi_marshal_from_py_filename (PyGIInvokeState   *state,
                                PyGICallableCache *callable_cache,
                                PyGIArgCache      *arg_cache,
                                PyObject          *py_arg,
                                GIArgument        *arg)
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

    return TRUE;
}

gboolean
_pygi_marshal_from_py_array (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg)
{
    PyGIMarshalFromPyFunc from_py_marshaller;
    int i = 0;
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
        array_ = (GArray *)g_ptr_array_new ();
    } else {
        array_ = g_array_sized_new (sequence_cache->is_zero_terminated,
                                    FALSE,
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
    for (i = 0; i < length; i++) {
        GIArgument item;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL)
            goto err;

        if (!from_py_marshaller ( state,
                                  callable_cache,
                                  sequence_cache->item_cache,
                                  py_item,
                                 &item))
            goto err;

        /* FIXME: it is much more efficent to have seperate marshaller
         *        for ptr arrays than doing the evaluation
         *        and casting each loop iteration
         */
        if (is_ptr_array) {
            g_ptr_array_add((GPtrArray *)array_, item.v_pointer);
        } else if (sequence_cache->item_cache->type_tag == GI_TYPE_TAG_INTERFACE) {
            PyGIInterfaceCache *item_iface_cache = (PyGIInterfaceCache *) sequence_cache->item_cache;
            GIBaseInfo *base_info = (GIBaseInfo *) item_iface_cache->interface_info;
            GIInfoType info_type = g_base_info_get_type (base_info);

            switch (info_type) {
                case GI_INFO_TYPE_UNION:
                case GI_INFO_TYPE_STRUCT:
                {
                    PyGIArgCache *item_arg_cache = (PyGIArgCache *)item_iface_cache;
                    PyGIMarshalCleanupFunc from_py_cleanup = item_arg_cache->from_py_cleanup;
                    gboolean is_boxed = g_type_is_a (item_iface_cache->g_type, G_TYPE_BOXED);
                    gboolean is_gvalue = item_iface_cache->g_type == G_TYPE_VALUE;
                    gboolean is_gvariant = item_iface_cache->g_type == G_TYPE_VARIANT;
                    
                    if (is_gvariant) {
                        /* Item size will always be that of a pointer,
                         * since GVariants are opaque hence always passed by ref */
                        g_assert (item_size == sizeof (item.v_pointer));
                        g_array_insert_val (array_, i, item.v_pointer);
                    } else if (is_gvalue) {
                        GValue* dest = (GValue*) (array_->data + (i * item_size));
                        memset (dest, 0, item_size);
                        if (item.v_pointer != NULL) {
                            g_value_init (dest, G_VALUE_TYPE ((GValue*) item.v_pointer));
                            g_value_copy ((GValue*) item.v_pointer, dest);
                        }

                        if (from_py_cleanup) {
                            from_py_cleanup (state, item_arg_cache, item.v_pointer, TRUE);
                            /* we freed the original copy already, the new one is a 
                             * struct in an array. _pygi_marshal_cleanup_from_py_array()
                             * must not free it again */
                            item_arg_cache->from_py_cleanup = NULL;
                        }
                    } else if (!is_boxed) {
                        /* HACK: Gdk.Atom is merely an integer wrapped in a pointer,
                         * so we must not dereference it; just copy the pointer
                         * value, and don't attempt to free it. TODO: find out
                         * if there are other data types with similar behaviour
                         * and generalize. */
                        if (g_strcmp0 (item_iface_cache->type_name, "Gdk.Atom") == 0) {
                            g_assert (item_size == sizeof (item.v_pointer));
                            memcpy (array_->data + (i * item_size), &item.v_pointer, item_size);
                        } else {
                            memcpy (array_->data + (i * item_size), item.v_pointer, item_size);

                            if (from_py_cleanup)
                                from_py_cleanup (state, item_arg_cache, item.v_pointer, TRUE);
                        }
                    } else if (is_boxed && !item_iface_cache->arg_cache.is_pointer) {
                        /* The array elements are not expected to be pointers, but the
                         * elements obtained are boxed pointers themselves, so insert
                         * the pointed to data.
                         */
                        g_array_insert_vals (array_, i, item.v_pointer, 1);
                    } else {
                        g_array_insert_val (array_, i, item);
                    }
                    break;
                }
                default:
                    g_array_insert_val (array_, i, item);
            }
        } else {
            g_array_insert_val (array_, i, item);
        }
        continue;
err:
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            gsize j;
            PyGIMarshalCleanupFunc cleanup_func =
                sequence_cache->item_cache->from_py_cleanup;

            for(j = 0; j < i; j++) {
                cleanup_func (state,
                              sequence_cache->item_cache,
                              g_array_index (array_, gpointer, j),
                              TRUE);
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
            callable_cache->args_cache[sequence_cache->len_arg_index];

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
        arg->v_pointer = array_->data;
        g_array_free (array_, FALSE);
    } else {
        arg->v_pointer = array_;
    }

    return TRUE;
}

gboolean
_pygi_marshal_from_py_glist (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg)
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
        GIArgument item;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL)
            goto err;

        if (!from_py_marshaller ( state,
                                  callable_cache,
                                  sequence_cache->item_cache,
                                  py_item,
                                 &item))
            goto err;

        list_ = g_list_prepend (list_, _pygi_arg_to_hash_pointer (&item, sequence_cache->item_cache->type_tag));
        continue;
err:
        /* FIXME: clean up list
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            PyGIMarshalCleanupFunc cleanup = sequence_cache->item_cache->from_py_cleanup;
        }
        */
        g_list_free (list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = g_list_reverse (list_);
    return TRUE;
}

gboolean
_pygi_marshal_from_py_gslist (PyGIInvokeState   *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache      *arg_cache,
                              PyObject          *py_arg,
                              GIArgument        *arg)
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
        GIArgument item;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL)
            goto err;

        if (!from_py_marshaller ( state,
                             callable_cache,
                             sequence_cache->item_cache,
                             py_item,
                            &item))
            goto err;

        list_ = g_slist_prepend (list_, _pygi_arg_to_hash_pointer (&item, sequence_cache->item_cache->type_tag));
        continue;
err:
        /* FIXME: Clean up list
        if (sequence_cache->item_cache->from_py_cleanup != NULL) {
            PyGIMarshalCleanupFunc cleanup = sequence_cache->item_cache->from_py_cleanup;
        }
        */

        g_slist_free (list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = g_slist_reverse (list_);
    return TRUE;
}

gboolean
_pygi_marshal_from_py_ghash (PyGIInvokeState   *state,
                             PyGICallableCache *callable_cache,
                             PyGIArgCache      *arg_cache,
                             PyObject          *py_arg,
                             GIArgument        *arg)
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
        PyObject *py_key = PyList_GET_ITEM (py_keys, i);
        PyObject *py_value = PyList_GET_ITEM (py_values, i);
        if (py_key == NULL || py_value == NULL)
            goto err;

        if (!key_from_py_marshaller ( state,
                                      callable_cache,
                                      hash_cache->key_cache,
                                      py_key,
                                     &key))
            goto err;

        if (!value_from_py_marshaller ( state,
                                        callable_cache,
                                        hash_cache->value_cache,
                                        py_value,
                                       &value))
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
    return TRUE;
}

gboolean
_pygi_marshal_from_py_gerror (PyGIInvokeState   *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache      *arg_cache,
                              PyObject          *py_arg,
                              GIArgument        *arg)
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

        g_assert (destroy_notify);

        GIBaseInfo* glib_destroy_notify = g_irepository_find_by_name (NULL, "GLib", "DestroyNotify");
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
        user_data_cache = callable_cache->args_cache[callback_cache->user_data_index];
        if (user_data_cache->py_arg_index < state->n_py_in_args) {
            /* py_user_data is a borrowed reference. */
            py_user_data = PyTuple_GetItem (state->py_in_args, user_data_cache->py_arg_index);
            if (!py_user_data)
                return FALSE;
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
        destroy_cache = callable_cache->args_cache[callback_cache->destroy_notify_index];
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

    /* Store the PyGIClosure as extra args data so _pygi_marshal_cleanup_from_py_interface_callback
     * can clean it up later for GI_SCOPE_TYPE_CALL based closures.
     */
    state->args_data[arg_cache->c_arg_index] = closure;

    return TRUE;
}

gboolean
_pygi_marshal_from_py_interface_enum (PyGIInvokeState   *state,
                                      PyGICallableCache *callable_cache,
                                      PyGIArgCache      *arg_cache,
                                      PyObject          *py_arg,
                                      GIArgument        *arg)
{
    PyObject *py_long;
    long c_long;
    gint is_instance;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GIBaseInfo *interface;

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

    return TRUE;

err:
    PyErr_Format (PyExc_TypeError, "Expected a %s, but got %s",
                  iface_cache->type_name, py_arg->ob_type->tp_name);
    return FALSE;
}

gboolean
_pygi_marshal_from_py_interface_flags (PyGIInvokeState   *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg)
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
        return FALSE;
    }

    return TRUE;

err:
    PyErr_Format (PyExc_TypeError, "Expected a %s, but got %s",
                  iface_cache->type_name, py_arg->ob_type->tp_name);
    return FALSE;

}

gboolean
_pygi_marshal_from_py_interface_struct (PyGIInvokeState   *state,
                                        PyGICallableCache *callable_cache,
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
    } else if (iface_cache->g_type == G_TYPE_VALUE) {
        GValue *value;
        GType object_type;

        object_type = pyg_type_from_object_strict ( (PyObject *) py_arg->ob_type, FALSE);
        if (object_type == G_TYPE_INVALID) {
            PyErr_SetString (PyExc_RuntimeError, "unable to retrieve object's GType");
            return FALSE;
        }

        /* if already a gvalue, use that, else marshal into gvalue */
        if (object_type == G_TYPE_VALUE) {
            value = (GValue *)( (PyGObject *)py_arg)->obj;
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
    } else if (iface_cache->is_foreign) {
        PyObject *success;
        success = pygi_struct_foreign_convert_to_g_argument (py_arg,
                                                             iface_cache->interface_info,
                                                             arg_cache->transfer,
                                                             arg);

        return (success == Py_None);
    } else if (!PyObject_IsInstance (py_arg, iface_cache->py_type)) {
        /* first check to see if this is a member of the expected union */
        if (!_is_union_member (iface_cache, py_arg)) {
            if (!PyErr_Occurred()) {
                PyObject *module = PyObject_GetAttrString(py_arg, "__module__");

                PyErr_Format (PyExc_TypeError, "argument %s: Expected %s, but got %s%s%s",
                              arg_cache->arg_name ? arg_cache->arg_name : "self",
                              iface_cache->type_name,
                              module ? PYGLIB_PyUnicode_AsString(module) : "",
                              module ? "." : "",
                              py_arg->ob_type->tp_name);
                if (module)
                    Py_DECREF (module);
            }

            return FALSE;
        }
    }

    if (g_type_is_a (iface_cache->g_type, G_TYPE_BOXED)) {
        arg->v_pointer = pyg_boxed_get (py_arg, void);
        if (arg_cache->transfer == GI_TRANSFER_EVERYTHING) {
            arg->v_pointer = g_boxed_copy (iface_cache->g_type, arg->v_pointer);
        }
    } else if (g_type_is_a (iface_cache->g_type, G_TYPE_POINTER) ||
                   g_type_is_a (iface_cache->g_type, G_TYPE_VARIANT) ||
                       iface_cache->g_type  == G_TYPE_NONE) {
        arg->v_pointer = pyg_pointer_get (py_arg, void);
    } else {
        PyErr_Format (PyExc_NotImplementedError,
                      "structure type '%s' is not supported yet",
                      g_type_name(iface_cache->g_type));
        return FALSE;
    }
    return TRUE;
}

gboolean
_pygi_marshal_from_py_interface_boxed (PyGIInvokeState   *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg)
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
                                        GIArgument        *arg)
{
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

    return pygi_marshal_from_py_gobject (py_arg, arg, arg_cache->transfer);
}

gboolean
_pygi_marshal_from_py_interface_union (PyGIInvokeState   *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache      *arg_cache,
                                       PyObject          *py_arg,
                                       GIArgument        *arg)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Marshalling for this type is not implemented yet");
    return FALSE;
}

gboolean _pygi_marshal_from_py_interface_instance (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg)
{
    GIInfoType info_type;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    info_type = g_base_info_get_type (iface_cache->interface_info);
    switch (info_type) {
        case GI_INFO_TYPE_UNION:
        case GI_INFO_TYPE_STRUCT:
        {
            GType type = iface_cache->g_type;

            if (!PyObject_IsInstance (py_arg, iface_cache->py_type)) {
                /* wait, we might be a member of a union so manually check */
                if (!_is_union_member (iface_cache, py_arg)) {
                    if (!PyErr_Occurred()) {
                        PyObject *module = PyObject_GetAttrString(py_arg, "__module__");
                        PyErr_Format (PyExc_TypeError,
                                      "argument %s: Expected a %s, but got %s%s%s",
                                      arg_cache->arg_name ? arg_cache->arg_name : "self",
                                      iface_cache->type_name,
                                      module ? PYGLIB_PyUnicode_AsString(module) : "",
                                      module ? "." : "",
                                      py_arg->ob_type->tp_name);
                        if (module)
                            Py_DECREF (module);
                    }
                    return FALSE;
                }
            }

            if (g_type_is_a (type, G_TYPE_BOXED)) {
                arg->v_pointer = pyg_boxed_get (py_arg, void);
            } else if (g_type_is_a (type, G_TYPE_POINTER) ||
                           g_type_is_a (type, G_TYPE_VARIANT) ||
                               type == G_TYPE_NONE) {
                arg->v_pointer = pyg_pointer_get (py_arg, void);
            } else {
                 PyErr_Format (PyExc_TypeError, "unable to convert an instance of '%s'", g_type_name (type));
                 return FALSE;
            }

            break;
        }
        case GI_INFO_TYPE_OBJECT:
        case GI_INFO_TYPE_INTERFACE:
            arg->v_pointer = pygobject_get (py_arg);
            if (arg->v_pointer != NULL) {
                GType obj_type = G_OBJECT_TYPE (( GObject *)arg->v_pointer);
                GType expected_type = iface_cache->g_type;

                if (!g_type_is_a (obj_type, expected_type)) {
                    PyObject *module = PyObject_GetAttrString(py_arg, "__module__");
                    PyErr_Format (PyExc_TypeError, "argument %s: Expected %s, but got %s%s%s",
                                  arg_cache->arg_name ? arg_cache->arg_name : "self",
                                  iface_cache->type_name,
                                  module ? PYGLIB_PyUnicode_AsString(module) : "",
                                  module ? "." : "",
                                  py_arg->ob_type->tp_name);
                    if (module)
                        Py_DECREF (module);
                    return FALSE;
                }
            }
            break;
        default:
            /* Other types don't have methods. */
            g_assert_not_reached ();
   }

   return TRUE;
}

/* pygi_marshal_from_py_gobject:
 * py_arg: (in):
 * arg: (out):
 */
gboolean
pygi_marshal_from_py_gobject (PyObject *py_arg, /*in*/
                              GIArgument *arg,  /*out*/
                              GITransfer transfer) {
    GObject *gobj;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    gobj = pygobject_get (py_arg);
    if (transfer == GI_TRANSFER_EVERYTHING) {
        /* An easy case of adding a new ref that the caller will take ownership of.
         * Pythons existing ref to the GObject will be managed normally with the wrapper.
         */
        g_object_ref (gobj);

    } else if (py_arg->ob_refcnt == 1 && gobj->ref_count == 1) {
        /* If both object ref counts are only 1 at this point (the reference held
         * in a return tuple), we assume the GObject will be free'd before reaching
         * its target and become invalid. So instead of getting invalid object errors
         * we add a new GObject ref.
         */
        g_object_ref (gobj);

        if (((PyGObject *)py_arg)->private_flags.flags & PYGOBJECT_GOBJECT_WAS_FLOATING) {
            /* HACK:
             * We want to re-float instances that were floating and the Python
             * wrapper assumed ownership. With the additional caveat that there
             * are not any strong references beyond the return tuple.
             * This should be removed once the following ticket is fixed:
             * https://bugzilla.gnome.org/show_bug.cgi?id=693393
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

    arg->v_pointer = gobj;
    return TRUE;
}

/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2014 Simon Feltman <sfeltman@gnome.org>
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

#include <Python.h>
#include <pyglib-python-compat.h>

#include "pygi-basictype.h"
#include "pygi-argument.h"
#include "pygi-private.h"

#ifdef G_OS_WIN32
#include <math.h>

#ifndef NAN
static const unsigned long __nan[2] = {0xffffffff, 0x7fffffff};
#define NAN (*(const float *) __nan)
#endif

#ifndef INFINITY
#define INFINITY HUGE_VAL
#endif

#endif


/*
 * From Python Marshaling
 */

static gboolean
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
    PyObject *tmp = NULL;

    if (PyUnicode_Check (py_arg)) {
        tmp = PyUnicode_AsUTF8String (py_arg);
        if (!tmp)
            return FALSE;

        string_ = PYGLIB_PyBytes_AsString (tmp);
    }
#if PY_VERSION_HEX < 0x03000000
    else if (PyString_Check (py_arg)) {
        string_ = PyString_AsString (py_arg);
    }
#endif
    else {
        PyErr_Format (PyExc_TypeError, "Must be string, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    arg->v_string = g_filename_from_utf8 (string_, -1, NULL, NULL, &error);
    Py_XDECREF (tmp);

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

static void
_pygi_marshal_cleanup_from_py_utf8 (PyGIInvokeState *state,
                                    PyGIArgCache    *arg_cache,
                                    PyObject        *py_arg,
                                    gpointer         data,
                                    gboolean         was_processed)
{
    /* We strdup strings so free unless ownership is transferred to C. */
    if (was_processed && arg_cache->transfer == GI_TRANSFER_NOTHING)
        g_free (data);
}

static void
_arg_cache_from_py_void_setup (PyGIArgCache *arg_cache)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_void;
}


static void
_arg_cache_from_py_basic_type_setup (PyGIArgCache *arg_cache)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_basic_type_cache_adapter;
}

static void
_arg_cache_from_py_utf8_setup (PyGIArgCache *arg_cache,
                               GITransfer transfer)
{
    arg_cache->from_py_marshaller = _pygi_marshal_from_py_basic_type_cache_adapter;
    arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_utf8;
}


/*
 * To Python Marshaling
 */


static PyObject *
_pygi_marshal_to_py_void (PyGIInvokeState   *state,
                          PyGICallableCache *callable_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    if (arg_cache->is_pointer) {
        return PyLong_FromVoidPtr (arg->v_pointer);
    }
    Py_RETURN_NONE;
}

static PyObject *
_pygi_marshal_to_py_unichar (GIArgument *arg)
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

static PyObject *
_pygi_marshal_to_py_utf8 (GIArgument *arg)
{
    PyObject *py_obj = NULL;
    if (arg->v_string == NULL) {
        Py_RETURN_NONE;
     }

    py_obj = PYGLIB_PyUnicode_FromString (arg->v_string);
    return py_obj;
}

static PyObject *
_pygi_marshal_to_py_filename (GIArgument *arg)
{
    gchar *string = NULL;
    PyObject *py_obj = NULL;
    GError *error = NULL;

    if (arg->v_string == NULL) {
        Py_RETURN_NONE;
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


/**
 * _pygi_marshal_to_py_basic_type:
 * @arg: The argument to convert to an object.
 * @type_tag: Type tag for @arg
 * @transfer: Transfer annotation
 *
 * Convert the given argument to a Python object. This function
 * is restricted to simple types that only require the GITypeTag
 * and GITransfer. For a more complete conversion routine, use:
 * _pygi_argument_to_object.
 *
 * Returns: A PyObject representing @arg or NULL if it cannot convert
 *          the argument.
 */
PyObject *
_pygi_marshal_to_py_basic_type (GIArgument  *arg,
                                GITypeTag type_tag,
                                GITransfer transfer)
{
    switch (type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
            return PyBool_FromLong (arg->v_boolean);

        case GI_TYPE_TAG_INT8:
            return PYGLIB_PyLong_FromLong (arg->v_int8);

        case GI_TYPE_TAG_UINT8:
            return PYGLIB_PyLong_FromLong (arg->v_uint8);

        case GI_TYPE_TAG_INT16:
            return PYGLIB_PyLong_FromLong (arg->v_int16);

        case GI_TYPE_TAG_UINT16:
            return PYGLIB_PyLong_FromLong (arg->v_uint16);

        case GI_TYPE_TAG_INT32:
            return PYGLIB_PyLong_FromLong (arg->v_int32);

        case GI_TYPE_TAG_UINT32:
            return PyLong_FromLongLong (arg->v_uint32);

        case GI_TYPE_TAG_INT64:
            return PyLong_FromLongLong (arg->v_int64);

        case GI_TYPE_TAG_UINT64:
            return PyLong_FromUnsignedLongLong (arg->v_uint64);

        case GI_TYPE_TAG_FLOAT:
            return PyFloat_FromDouble (arg->v_float);

        case GI_TYPE_TAG_DOUBLE:
            return PyFloat_FromDouble (arg->v_double);

        case GI_TYPE_TAG_GTYPE:
            return pyg_type_wrapper_new ( (GType) arg->v_long);

        case GI_TYPE_TAG_UNICHAR:
            return _pygi_marshal_to_py_unichar (arg);

        case GI_TYPE_TAG_UTF8:
            return _pygi_marshal_to_py_utf8 (arg);

        case GI_TYPE_TAG_FILENAME:
            return _pygi_marshal_to_py_filename (arg);

        default:
            return NULL;
    }
    return NULL;
}

PyObject *
_pygi_marshal_to_py_basic_type_cache_adapter (PyGIInvokeState   *state,
                                              PyGICallableCache *callable_cache,
                                              PyGIArgCache      *arg_cache,
                                              GIArgument        *arg)
{
    return _pygi_marshal_to_py_basic_type (arg,
                                            arg_cache->type_tag,
                                            arg_cache->transfer);
}

static void
_pygi_marshal_cleanup_to_py_utf8 (PyGIInvokeState *state,
                                  PyGIArgCache    *arg_cache,
                                  PyObject        *dummy,
                                  gpointer         data,
                                  gboolean         was_processed)
{
    /* Python copies the string so we need to free it
       if the interface is transfering ownership, 
       whether or not it has been processed yet */
    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        g_free (data);
}



static void
_arg_cache_to_py_basic_type_setup (PyGIArgCache *arg_cache)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_basic_type_cache_adapter;
}

static void
_arg_cache_to_py_void_setup (PyGIArgCache *arg_cache)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_void;
}

static void
_arg_cache_to_py_utf8_setup (PyGIArgCache *arg_cache,
                               GITransfer transfer)
{
    arg_cache->to_py_marshaller = _pygi_marshal_to_py_basic_type_cache_adapter;
    arg_cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_utf8;
}

/*
 * Basic Type Interface
 */

static gboolean
pygi_arg_basic_type_setup_from_info (PyGIArgCache  *arg_cache,
                                     GITypeInfo    *type_info,
                                     GIArgInfo     *arg_info,
                                     GITransfer     transfer,
                                     PyGIDirection  direction)
{
    GITypeTag type_tag = g_type_info_get_tag (type_info);

    if (!pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction))
        return FALSE;

    switch (type_tag) {
       case GI_TYPE_TAG_VOID:
           if (direction & PYGI_DIRECTION_FROM_PYTHON)
               _arg_cache_from_py_void_setup (arg_cache);

           if (direction & PYGI_DIRECTION_TO_PYTHON)
               _arg_cache_to_py_void_setup (arg_cache);

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
       case GI_TYPE_TAG_UNICHAR:
       case GI_TYPE_TAG_GTYPE:
           if (direction & PYGI_DIRECTION_FROM_PYTHON)
               _arg_cache_from_py_basic_type_setup (arg_cache);

           if (direction & PYGI_DIRECTION_TO_PYTHON)
               _arg_cache_to_py_basic_type_setup (arg_cache);

           break;
       case GI_TYPE_TAG_UTF8:
       case GI_TYPE_TAG_FILENAME:
           if (direction & PYGI_DIRECTION_FROM_PYTHON)
               _arg_cache_from_py_utf8_setup (arg_cache, transfer);

           if (direction & PYGI_DIRECTION_TO_PYTHON)
               _arg_cache_to_py_utf8_setup (arg_cache, transfer);

           break;
       default:
           g_assert_not_reached ();
    }

    return TRUE;
}

PyGIArgCache *
pygi_arg_basic_type_new_from_info (GITypeInfo   *type_info,
                                   GIArgInfo    *arg_info,
                                   GITransfer    transfer,
                                   PyGIDirection direction)
{
    gboolean res = FALSE;
    PyGIArgCache *arg_cache = pygi_arg_cache_alloc ();
    if (arg_cache == NULL)
        return NULL;

    res = pygi_arg_basic_type_setup_from_info (arg_cache,
                                               type_info,
                                               arg_info,
                                               transfer,
                                               direction);
    if (res) {
        return arg_cache;
    } else {
        pygi_arg_cache_free (arg_cache);
        return NULL;
    }
}

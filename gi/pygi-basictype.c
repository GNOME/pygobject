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
#include "pygi-python-compat.h"
#include "pygi-type.h"
#include "pygi-basictype.h"
#include "pygi-argument.h"
#include "pygi-util.h"

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

static gboolean
pygi_gpointer_from_py (PyObject *py_arg, gpointer *result)
{
    void* temp;

    if (py_arg == Py_None) {
        *result = NULL;
        return TRUE;
    } else if (PYGLIB_CPointer_Check(py_arg)) {
        temp = PYGLIB_CPointer_GetPointer (py_arg, NULL);
        if (temp == NULL)
            return FALSE;
        *result = temp;
        return TRUE;
    } else if (PYGLIB_PyLong_Check(py_arg) || PyLong_Check(py_arg)) {
        temp = PyLong_AsVoidPtr (py_arg);
        if (PyErr_Occurred ())
            return FALSE;
        *result = temp;
        return TRUE;
    } else {
        PyErr_SetString(PyExc_ValueError,
                        "Pointer arguments are restricted to integers, capsules, and None. "
                        "See: https://bugzilla.gnome.org/show_bug.cgi?id=683599");
        return FALSE;
    }
}

static gboolean
marshal_from_py_void (PyGIInvokeState   *state,
                      PyGICallableCache *callable_cache,
                      PyGIArgCache      *arg_cache,
                      PyObject          *py_arg,
                      GIArgument        *arg,
                      gpointer          *cleanup_data)
{
    g_warn_if_fail (arg_cache->transfer == GI_TRANSFER_NOTHING);

    if (pygi_gpointer_from_py (py_arg, &(arg->v_pointer))) {
        *cleanup_data = arg->v_pointer;
        return TRUE;
    }

    return FALSE;
}

PyObject *
pygi_gsize_to_py (gsize value)
{
    return PYGLIB_PyLong_FromSize_t (value);
}

PyObject *
pygi_gssize_to_py (gssize value)
{
    return PYGLIB_PyLong_FromSsize_t (value);
}

gboolean
pygi_gdouble_from_py (PyObject *py_arg, gdouble *result)
{
    PyObject *py_float;
    gdouble temp;

    if (!PyNumber_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    py_float = PyNumber_Float (py_arg);
    if (py_float == NULL)
        return FALSE;

    temp = PyFloat_AsDouble (py_float);
    Py_DECREF (py_float);

    if (PyErr_Occurred ())
        return FALSE;

    *result = temp;

    return TRUE;
}

PyObject *
pygi_gdouble_to_py (gdouble value)
{
    return PyFloat_FromDouble (value);
}

gboolean
pygi_gfloat_from_py (PyObject *py_arg, gfloat *result)
{
    gdouble double_;

    if (!pygi_gdouble_from_py (py_arg, &double_))
        return FALSE;

    if ((double_ < -G_MAXFLOAT || double_ > G_MAXFLOAT) &&
            double_ != INFINITY && double_ != -INFINITY && double_ != NAN) {
        char buf[100];

        /* we need this as PyErr_Format() does not support float types */
        snprintf (buf, sizeof (buf), "%g not in range %g to %g",
                  double_, -G_MAXFLOAT, G_MAXFLOAT);
        PyErr_SetString (PyExc_OverflowError, buf);
        return FALSE;
    }

    *result = (gfloat)double_;

    return TRUE;
}

PyObject *
pygi_gfloat_to_py (gfloat value)
{
    return PyFloat_FromDouble (value);
}

gboolean
pygi_gunichar_from_py (PyObject *py_arg, gunichar *result)
{
    Py_ssize_t size;
    gchar *string_;

    if (py_arg == Py_None) {
        *result = 0;
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

    *result = g_utf8_get_char (string_);
    g_free (string_);

    return TRUE;
}

static PyObject *
pygi_gunichar_to_py (gunichar value)
{
    PyObject *py_obj = NULL;

    /* Preserve the bidirectional mapping between 0 and "" */
    if (value == 0) {
        py_obj = PYGLIB_PyUnicode_FromString ("");
    } else if (g_unichar_validate (value)) {
        gchar utf8[6];
        gint bytes;

        bytes = g_unichar_to_utf8 (value, utf8);
        py_obj = PYGLIB_PyUnicode_FromStringAndSize ((char*)utf8, bytes);
    } else {
        /* TODO: Convert the error to an exception. */
        PyErr_Format (PyExc_TypeError,
                      "Invalid unicode codepoint %" G_GUINT32_FORMAT,
                      value);
    }

    return py_obj;
}

static gboolean
pygi_gtype_from_py (PyObject *py_arg, GType *type)
{
    GType temp = pyg_type_from_object (py_arg);

    if (temp == 0) {
        PyErr_Format (PyExc_TypeError, "Must be gobject.GType, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    *type = temp;

    return TRUE;
}

gboolean
pygi_utf8_from_py (PyObject *py_arg, gchar **result)
{
    gchar *string_;

    if (py_arg == Py_None) {
        *result = NULL;
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

    *result = string_;
    return TRUE;
}

G_GNUC_UNUSED
static gboolean
filename_from_py_unix (PyObject *py_arg, gchar **result)
{
    gchar *filename;

    if (py_arg == Py_None) {
        *result = NULL;
        return TRUE;
    }

    if (PYGLIB_PyBytes_Check (py_arg)) {
        char *buffer;

        if (PYGLIB_PyBytes_AsStringAndSize (py_arg, &buffer, NULL) == -1)
            return FALSE;

        filename = g_strdup (buffer);
    } else if (PyUnicode_Check (py_arg)) {
        PyObject *bytes;
        char *buffer;

#if PY_VERSION_HEX < 0x03000000
        bytes = PyUnicode_AsEncodedString (py_arg, Py_FileSystemDefaultEncoding,
                                           NULL);
#else
        bytes = PyUnicode_EncodeFSDefault (py_arg);
#endif

        if (!bytes)
            return FALSE;

        if (PYGLIB_PyBytes_AsStringAndSize (bytes, &buffer, NULL) == -1) {
            Py_DECREF (bytes);
            return FALSE;
        }

        filename = g_strdup (buffer);
        Py_DECREF (bytes);
    } else {
        PyErr_Format (PyExc_TypeError, "Must be bytes, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }

    *result = filename;
    return TRUE;
}

G_GNUC_UNUSED
static gboolean
filename_from_py_win32 (PyObject *py_arg, gchar **result)
{
    gchar *filename;

    if (py_arg == Py_None) {
        *result = NULL;
        return TRUE;
    }

#if PY_VERSION_HEX < 0x03000000
    if (PYGLIB_PyBytes_Check (py_arg)) {
        char *buffer;

        if (PYGLIB_PyBytes_AsStringAndSize (py_arg, &buffer, NULL) == -1)
            return FALSE;

        filename = g_strdup (buffer);
    } else if (PyUnicode_Check (py_arg)) {
        PyObject *bytes;
        char *buffer;

        bytes = PyUnicode_AsUTF8String (py_arg);
        if (!bytes)
            return FALSE;

        if (PYGLIB_PyBytes_AsStringAndSize (bytes, &buffer, NULL) == -1) {
            Py_DECREF (bytes);
            return FALSE;
        }

        filename = g_strdup (buffer);
        Py_DECREF (bytes);
    } else {
        PyErr_Format (PyExc_TypeError, "Must be unicode, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }
#else
    if (PYGLIB_PyBytes_Check (py_arg)) {
        PyObject *uni_arg;
        gboolean temp_result;
        char *buffer;

        if (PYGLIB_PyBytes_AsStringAndSize (py_arg, &buffer, NULL) == -1)
            return FALSE;

        uni_arg = PyUnicode_DecodeFSDefault (buffer);
        if (!uni_arg)
            return FALSE;
        temp_result = filename_from_py_win32 (uni_arg, result);
        Py_DECREF (uni_arg);
        return temp_result;
    } else if (PyUnicode_Check (py_arg)) {
        PyObject *bytes, *temp_uni;
        char *buffer;

        /* The roundtrip merges lone surrogates, so we get the same output as
         * with Py 2. Requires 3.4+ because of https://bugs.python.org/issue27971
         * Separated lone surrogates can occur when concatenating two paths.
         */
        bytes = PyUnicode_AsEncodedString (py_arg, "utf-16-le", "surrogatepass");
        if (!bytes)
            return FALSE;
        temp_uni = PyUnicode_FromEncodedObject (bytes, "utf-16-le", "surrogatepass");
        Py_DECREF (bytes);
        if (!temp_uni)
            return FALSE;
        /* glib uses utf-8, so encode to that and allow surrogates so we can
         * represent all possible path values
         */
        bytes = PyUnicode_AsEncodedString (temp_uni, "utf-8", "surrogatepass");
        Py_DECREF (temp_uni);
        if (!bytes)
            return FALSE;

        if (PYGLIB_PyBytes_AsStringAndSize (bytes, &buffer, NULL) == -1) {
            Py_DECREF (bytes);
            return FALSE;
        }

        filename = g_strdup (buffer);
        Py_DECREF (bytes);
    } else {
        PyErr_Format (PyExc_TypeError, "Must be str, not %s",
                      py_arg->ob_type->tp_name);
        return FALSE;
    }
#endif

    *result = filename;
    return TRUE;
}

static gboolean
pygi_filename_from_py (PyObject *py_arg, gchar **result)
{
#ifdef G_OS_WIN32
    return filename_from_py_win32 (py_arg, result);
#else
    return filename_from_py_unix (py_arg, result);
#endif
}

static PyObject *
base_number_checks (PyObject *object)
{
    PyObject *number;

    if (!PyNumber_Check (object)) {
        PyErr_Format (PyExc_TypeError, "Must be number, not %s",
                      object->ob_type->tp_name);
        return NULL;
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
        return NULL;
    }

    return number;
}

gboolean
pygi_gboolean_from_py (PyObject *object, gboolean *result)
{
    int value = PyObject_IsTrue (object);
    if (value == -1)
        return FALSE;
    *result = (gboolean)value;
    return TRUE;
}

PyObject *
pygi_gboolean_to_py (gboolean value)
{
    return PyBool_FromLong (value);
}

gboolean
pygi_gint_from_py (PyObject *object, gint *result)
{
    long long_value;
    PyObject *number;

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PYGLIB_PyLong_AsLong (number);
    if (PyErr_Occurred ()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    } else if (long_value < G_MININT || long_value > G_MAXINT) {
        goto overflow;
    }

    Py_DECREF (number);
    *result = (gint)long_value;
    return TRUE;

overflow:
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %d to %d",
        number, (int)G_MININT, (int)G_MAXINT);
    Py_DECREF (number);
    return FALSE;
}

PyObject *
pygi_gint_to_py (gint value)
{
    return PYGLIB_PyLong_FromLong (value);
}

gboolean
pygi_guint_from_py (PyObject *object, guint *result)
{
    unsigned long long_value;
    PyObject *number;

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PyLong_AsUnsignedLong (number);
    if (PyErr_Occurred ()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    } else if (long_value > G_MAXUINT) {
        goto overflow;
    }

    Py_DECREF (number);
    *result = (gint)long_value;
    return TRUE;

overflow:
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %ld to %lu",
        number, (long)0, (unsigned long)G_MAXUINT);
    Py_DECREF (number);
    return FALSE;
}

PyObject *
pygi_guint_to_py (guint value)
{
#if (G_MAXUINT <= LONG_MAX)
    return PYGLIB_PyLong_FromLong ((long) value);
#else
    if (value <= LONG_MAX)
        return PYGLIB_PyLong_FromLong ((long) value);
    return PyLong_FromUnsignedLong (value);
#endif
}

gboolean
pygi_glong_from_py (PyObject *object, glong *result)
{
    long long_value;
    PyObject *number;

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PyLong_AsLong (number);
    if (long_value == -1 && PyErr_Occurred ()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    }

    Py_DECREF (number);
    *result = (glong)long_value;
    return TRUE;

overflow:
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %ld to %ld",
        number, (long)G_MINLONG, (long)G_MAXLONG);
    Py_DECREF (number);
    return FALSE;
}

PyObject *
pygi_glong_to_py (glong value)
{
    return PYGLIB_PyLong_FromLong (value);
}

gboolean
pygi_gulong_from_py (PyObject *object, gulong *result)
{
    unsigned long long_value;
    PyObject *number;

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PyLong_AsUnsignedLong (number);
    if (PyErr_Occurred ()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    }

    Py_DECREF (number);
    *result = (gulong)long_value;
    return TRUE;

overflow:
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %ld to %lu",
        number, (long)0, (unsigned long)G_MAXULONG);
    Py_DECREF (number);
    return FALSE;
}

PyObject *
pygi_gulong_to_py (gulong value)
{
    if (value <= LONG_MAX)
        return PYGLIB_PyLong_FromLong ((long) value);
    else
        return PyLong_FromUnsignedLong (value);
}

static gboolean
pygi_gint8_from_py (PyObject *object, gint8 *result)
{
    long long_value;
    PyObject *number;

    if (PYGLIB_PyBytes_Check (object)) {
        if (PYGLIB_PyBytes_Size (object) != 1) {
            PyErr_Format (PyExc_TypeError, "Must be a single character");
            return FALSE;
        }

        *result = (gint8)(PYGLIB_PyBytes_AsString (object)[0]);
        return TRUE;
    }

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PyLong_AsLong (number);
    if (long_value == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    } else if (long_value < G_MININT8 || long_value > G_MAXINT8) {
        goto overflow;
    }

    Py_DECREF (number);
    *result = (gint8)long_value;
    return TRUE;

overflow:
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %ld to %ld",
        number, (long)G_MININT8, (long)G_MAXINT8);
    Py_DECREF (number);
    return FALSE;
}

PyObject *
pygi_gint8_to_py (gint8 value)
{
    return PYGLIB_PyLong_FromLong (value);
}

static gboolean
pygi_guint8_from_py (PyObject *object, guint8 *result)
{
    long long_value;
    PyObject *number;

    if (PYGLIB_PyBytes_Check (object)) {
        if (PYGLIB_PyBytes_Size (object) != 1) {
            PyErr_Format (PyExc_TypeError, "Must be a single character");
            return FALSE;
        }

        *result = (guint8)(PYGLIB_PyBytes_AsString (object)[0]);
        return TRUE;
    }

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PyLong_AsLong (number);
    if (long_value == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    } else if (long_value < 0 || long_value > G_MAXUINT8) {
        goto overflow;
    }

    Py_DECREF (number);
    *result = (guint8)long_value;
    return TRUE;

overflow:
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %ld to %ld",
        number, (long)0, (long)G_MAXUINT8);
    Py_DECREF (number);
    return FALSE;
}

PyObject *
pygi_guint8_to_py (guint8 value)
{
    return PYGLIB_PyLong_FromLong (value);
}

static gboolean
pygi_gint16_from_py (PyObject *object, gint16 *result)
{
    long long_value;
    PyObject *number;

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PyLong_AsLong (number);
    if (long_value == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    } else if (long_value < G_MININT16 || long_value > G_MAXINT16) {
        goto overflow;
    }

    Py_DECREF (number);
    *result = (gint16)long_value;
    return TRUE;

overflow:
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %ld to %ld",
        number, (long)G_MININT16, (long)G_MAXINT16);
    Py_DECREF (number);
    return FALSE;
}

static PyObject *
pygi_gint16_to_py (gint16 value)
{
    return PYGLIB_PyLong_FromLong (value);
}

static gboolean
pygi_guint16_from_py (PyObject *object, guint16 *result)
{
    long long_value;
    PyObject *number;

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PyLong_AsLong (number);
    if (long_value == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    } else if (long_value < 0 || long_value > G_MAXUINT16) {
        goto overflow;
    }

    Py_DECREF (number);
    *result = (guint16)long_value;
    return TRUE;

overflow:
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %ld to %ld",
        number, (long)0, (long)G_MAXUINT16);
    Py_DECREF (number);
    return FALSE;
}

static PyObject *
pygi_guint16_to_py (guint16 value)
{
    return PYGLIB_PyLong_FromLong (value);
}

static gboolean
pygi_gint32_from_py (PyObject *object, gint32 *result)
{
    long long_value;
    PyObject *number;

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PyLong_AsLong (number);
    if (long_value == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    } else if (long_value < G_MININT32 || long_value > G_MAXINT32) {
        goto overflow;
    }

    Py_DECREF (number);
    *result = (gint32)long_value;
    return TRUE;

overflow:
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %ld to %ld",
        number, (long)G_MININT32, (long)G_MAXINT32);
    Py_DECREF (number);
    return FALSE;
}

static PyObject *
pygi_gint32_to_py (gint32 value)
{
    return PYGLIB_PyLong_FromLong (value);
}

static gboolean
pygi_guint32_from_py (PyObject *object, guint32 *result)
{
    long long long_value;
    PyObject *number;

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PyLong_AsLongLong (number);
    if (PyErr_Occurred ()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    } else if (long_value < 0 || long_value > G_MAXUINT32) {
        goto overflow;
    }

    Py_DECREF (number);
    *result = (guint32)long_value;
    return TRUE;

overflow:
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %ld to %lu",
        number, (long)0, (unsigned long)G_MAXUINT32);
    Py_DECREF (number);
    return FALSE;
}

static PyObject *
pygi_guint32_to_py (guint32 value)
{
#if (G_MAXUINT <= LONG_MAX)
    return PYGLIB_PyLong_FromLong (value);
#else
    if (value <= LONG_MAX)
        return PYGLIB_PyLong_FromLong((long) value);
    else
        return PyLong_FromLongLong (value);
#endif
}

gboolean
pygi_gint64_from_py (PyObject *object, gint64 *result)
{
    long long long_value;
    PyObject *number, *min, *max;

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PyLong_AsLongLong (number);
    if (PyErr_Occurred ()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    } else if (long_value < G_MININT64 || long_value > G_MAXINT64) {
        goto overflow;
    }

    Py_DECREF (number);
    *result = (gint64)long_value;
    return TRUE;

overflow:
    min = pygi_gint64_to_py (G_MININT64);
    max = pygi_gint64_to_py (G_MAXINT64);
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %S to %S",
        number, min, max);
    Py_DECREF (number);
    Py_DECREF (min);
    Py_DECREF (max);
    return FALSE;
}

PyObject *
pygi_gint64_to_py (gint64 value)
{
    if (LONG_MIN <= value && value <= LONG_MAX)
        return PYGLIB_PyLong_FromLong((long) value);
    else
        return PyLong_FromLongLong (value);
}

gboolean
pygi_guint64_from_py (PyObject *object, guint64 *result)
{
    unsigned long long long_value;
    PyObject *number, *max;

    number = base_number_checks (object);
    if (number == NULL)
        return FALSE;

    long_value = PyLong_AsUnsignedLongLong (number);
    if (PyErr_Occurred ()) {
        if (PyErr_ExceptionMatches (PyExc_OverflowError))
            goto overflow;
        Py_DECREF (number);
        return FALSE;
    } else if (long_value > G_MAXUINT64) {
        goto overflow;
    }

    Py_DECREF (number);
    *result = (guint64)long_value;
    return TRUE;

overflow:
    max = pygi_guint64_to_py (G_MAXUINT64);
    pygi_pyerr_format (
        PyExc_OverflowError, "%S not in range %ld to %S",
        number, (long)0, max);
    Py_DECREF (number);
    Py_DECREF (max);
    return FALSE;
}

PyObject *
pygi_guint64_to_py (guint64 value)
{
    if (value <= LONG_MAX)
        return PYGLIB_PyLong_FromLong((long) value);
    else
        return PyLong_FromUnsignedLongLong (value);
}

gboolean
pygi_marshal_from_py_basic_type (PyObject   *object,   /* in */
                                 GIArgument *arg,      /* out */
                                 GITypeTag   type_tag,
                                 GITransfer  transfer,
                                 gpointer   *cleanup_data /* out */)
{
    switch (type_tag) {
        case GI_TYPE_TAG_VOID:
            g_warn_if_fail (transfer == GI_TRANSFER_NOTHING);
            if (pygi_gpointer_from_py (object, &(arg->v_pointer))) {
                *cleanup_data = arg->v_pointer;
                return TRUE;
            }
            return FALSE;

        case GI_TYPE_TAG_INT8:
            return pygi_gint8_from_py (object, &(arg->v_int8));

        case GI_TYPE_TAG_UINT8:
            return pygi_guint8_from_py (object, &(arg->v_uint8));

        case GI_TYPE_TAG_INT16:
            return pygi_gint16_from_py (object, &(arg->v_int16));

        case GI_TYPE_TAG_UINT16:
            return pygi_guint16_from_py (object, &(arg->v_uint16));

        case GI_TYPE_TAG_INT32:
            return pygi_gint32_from_py (object, &(arg->v_int32));

        case GI_TYPE_TAG_UINT32:
            return pygi_guint32_from_py (object, &(arg->v_uint32));

        case GI_TYPE_TAG_INT64:
            return pygi_gint64_from_py (object, &(arg->v_int64));

        case GI_TYPE_TAG_UINT64:
            return pygi_guint64_from_py (object, &(arg->v_uint64));

        case GI_TYPE_TAG_BOOLEAN:
            return pygi_gboolean_from_py (object, &(arg->v_boolean));

        case GI_TYPE_TAG_FLOAT:
            return pygi_gfloat_from_py (object, &(arg->v_float));

        case GI_TYPE_TAG_DOUBLE:
            return pygi_gdouble_from_py (object, &(arg->v_double));

        case GI_TYPE_TAG_GTYPE:
            return pygi_gtype_from_py (object, &(arg->v_size));

        case GI_TYPE_TAG_UNICHAR:
            return pygi_gunichar_from_py (object, &(arg->v_uint32));

        case GI_TYPE_TAG_UTF8:
            if (pygi_utf8_from_py (object, &(arg->v_string))) {
                *cleanup_data = arg->v_string;
                return TRUE;
            }
            return FALSE;

        case GI_TYPE_TAG_FILENAME:
            if (pygi_filename_from_py (object, &(arg->v_string))) {
                *cleanup_data = arg->v_string;
                return TRUE;
            }
            return FALSE;

        default:
            PyErr_Format (PyExc_TypeError, "Type tag %d not supported",
                          type_tag);
            return FALSE;
    }

    return TRUE;
}

gboolean
pygi_marshal_from_py_basic_type_cache_adapter (PyGIInvokeState   *state,
                                               PyGICallableCache *callable_cache,
                                               PyGIArgCache      *arg_cache,
                                               PyObject          *py_arg,
                                               GIArgument        *arg,
                                               gpointer          *cleanup_data)
{
    return pygi_marshal_from_py_basic_type (py_arg,
                                            arg,
                                            arg_cache->type_tag,
                                            arg_cache->transfer,
                                            cleanup_data);
}

static void
marshal_cleanup_from_py_utf8 (PyGIInvokeState *state,
                              PyGIArgCache    *arg_cache,
                              PyObject        *py_arg,
                              gpointer         data,
                              gboolean         was_processed)
{
    /* We strdup strings so free unless ownership is transferred to C. */
    if (was_processed && arg_cache->transfer == GI_TRANSFER_NOTHING)
        g_free (data);
}

static PyObject *
marshal_to_py_void (PyGIInvokeState   *state,
                    PyGICallableCache *callable_cache,
                    PyGIArgCache      *arg_cache,
                    GIArgument        *arg,
                    gpointer          *cleanup_data)
{
    if (arg_cache->is_pointer) {
        return PyLong_FromVoidPtr (arg->v_pointer);
    }
    Py_RETURN_NONE;
}

PyObject *
pygi_utf8_to_py (gchar *value)
{
    if (value == NULL) {
        Py_RETURN_NONE;
     }

    return PYGLIB_PyUnicode_FromString (value);
}

PyObject *
pygi_filename_to_py (gchar *value)
{
    PyObject *py_obj;

    if (value == NULL) {
        Py_RETURN_NONE;
    }

#if PY_VERSION_HEX < 0x03000000
    /* On PY2 we return str as is */
    py_obj = PyString_FromString (value);
#else
#ifdef G_OS_WIN32
    py_obj = PyUnicode_DecodeUTF8 (value, strlen(value),
                                   "surrogatepass");
#else
    py_obj = PyUnicode_DecodeFSDefault (value);
#endif
#endif

    return py_obj;
}

/**
 * pygi_marshal_to_py_basic_type:
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
pygi_marshal_to_py_basic_type (GIArgument  *arg,
                               GITypeTag type_tag,
                               GITransfer transfer)
{
    switch (type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
            return pygi_gboolean_to_py (arg->v_boolean);

        case GI_TYPE_TAG_INT8:
            return pygi_gint8_to_py (arg->v_int8);

        case GI_TYPE_TAG_UINT8:
            return pygi_guint8_to_py (arg->v_uint8);

        case GI_TYPE_TAG_INT16:
            return pygi_gint16_to_py (arg->v_int16);

        case GI_TYPE_TAG_UINT16:
            return pygi_guint16_to_py (arg->v_uint16);

        case GI_TYPE_TAG_INT32:
            return pygi_gint32_to_py (arg->v_int32);

        case GI_TYPE_TAG_UINT32:
            return pygi_guint32_to_py (arg->v_uint32);

        case GI_TYPE_TAG_INT64:
            return pygi_gint64_to_py (arg->v_int64);

        case GI_TYPE_TAG_UINT64:
            return pygi_guint64_to_py (arg->v_uint64);

        case GI_TYPE_TAG_FLOAT:
            return pygi_gfloat_to_py (arg->v_float);

        case GI_TYPE_TAG_DOUBLE:
            return pygi_gdouble_to_py (arg->v_double);

        case GI_TYPE_TAG_GTYPE:
            return pyg_type_wrapper_new ( (GType) arg->v_size);

        case GI_TYPE_TAG_UNICHAR:
            return pygi_gunichar_to_py (arg->v_uint32);

        case GI_TYPE_TAG_UTF8:
            return pygi_utf8_to_py (arg->v_string);

        case GI_TYPE_TAG_FILENAME:
            return pygi_filename_to_py (arg->v_string);

        default:
            PyErr_Format (PyExc_TypeError, "Type tag %d not supported",
                          type_tag);
            return NULL;
    }
}

PyObject *
pygi_marshal_to_py_basic_type_cache_adapter (PyGIInvokeState   *state,
                                             PyGICallableCache *callable_cache,
                                             PyGIArgCache      *arg_cache,
                                             GIArgument        *arg,
                                             gpointer          *cleanup_data)
{
    return pygi_marshal_to_py_basic_type (arg,
                                          arg_cache->type_tag,
                                          arg_cache->transfer);
}

static void
marshal_cleanup_to_py_utf8 (PyGIInvokeState *state,
                            PyGIArgCache    *arg_cache,
                            gpointer         cleanup_data,
                            gpointer         data,
                            gboolean         was_processed)
{
    /* Python copies the string so we need to free it
       if the interface is transfering ownership, 
       whether or not it has been processed yet */
    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        g_free (data);
}

static gboolean
arg_basic_type_setup_from_info (PyGIArgCache  *arg_cache,
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
                arg_cache->from_py_marshaller = marshal_from_py_void;

           if (direction & PYGI_DIRECTION_TO_PYTHON)
                arg_cache->to_py_marshaller = marshal_to_py_void;

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
               arg_cache->from_py_marshaller = pygi_marshal_from_py_basic_type_cache_adapter;

           if (direction & PYGI_DIRECTION_TO_PYTHON)
               arg_cache->to_py_marshaller = pygi_marshal_to_py_basic_type_cache_adapter;

           break;
       case GI_TYPE_TAG_UTF8:
       case GI_TYPE_TAG_FILENAME:
           if (direction & PYGI_DIRECTION_FROM_PYTHON) {
                arg_cache->from_py_marshaller = pygi_marshal_from_py_basic_type_cache_adapter;
                arg_cache->from_py_cleanup = marshal_cleanup_from_py_utf8;
           }

           if (direction & PYGI_DIRECTION_TO_PYTHON) {
                arg_cache->to_py_marshaller = pygi_marshal_to_py_basic_type_cache_adapter;
                arg_cache->to_py_cleanup = marshal_cleanup_to_py_utf8;
           }

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

    res = arg_basic_type_setup_from_info (arg_cache,
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

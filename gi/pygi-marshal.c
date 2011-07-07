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
#include "pygi-marshal-cleanup.h"

/*** argument marshaling and validating routines ***/

gboolean
_pygi_marshal_in_void (PyGIInvokeState   *state,
                       PyGICallableCache *callable_cache,
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
                          PyGICallableCache *callable_cache,
                          PyGIArgCache      *arg_cache,
                          PyObject          *py_arg,
                          GIArgument        *arg)
{
    arg->v_boolean = PyObject_IsTrue (py_arg);

    return TRUE;
}

gboolean
_pygi_marshal_in_int8 (PyGIInvokeState   *state,
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
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, -128, 127);
        return FALSE;
    }

    if (long_ < -128 || long_ > 127) {
        PyErr_Format (PyExc_ValueError, "%ld not in range %d to %d", long_, -128, 127);
        return FALSE;
    }

    arg->v_long = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_uint8 (PyGIInvokeState   *state,
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

    arg->v_long = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_int16 (PyGIInvokeState   *state,
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

    arg->v_long = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_uint16 (PyGIInvokeState   *state,
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
        PyErr_Format (PyExc_ValueError, "%li not in range %d to %d", long_, 0, 65535);
        return FALSE;
    }

    if (long_ < 0 || long_ > 65535) {
        PyErr_Format (PyExc_ValueError, "%li not in range %d to %d", long_, 0, 65535);
        return FALSE;
    }

    arg->v_long = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_int32 (PyGIInvokeState   *state,
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

    arg->v_long = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_uint32 (PyGIInvokeState   *state,
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
        PyErr_Format (PyExc_ValueError, "%lli not in range %i to %u", long_, 0, G_MAXUINT32);
        return FALSE;
    }

    if (long_ < 0 || long_ > G_MAXUINT32) {
        PyErr_Format (PyExc_ValueError, "%lli not in range %i to %u", long_, 0, G_MAXUINT32);
        return FALSE;
    }

    arg->v_uint64 = long_;

    return TRUE;
}

gboolean
_pygi_marshal_in_int64 (PyGIInvokeState   *state,
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
        long_ = PyInt_AS_LONG (py_long);
    else
#endif
        long_ = PyLong_AsLongLong (py_long);

    Py_DECREF (py_long);

    if (PyErr_Occurred ()) {
        /* OverflowError occured but range errors should be returned as ValueError */
        char *long_str;
        PyObject *py_str;

        PyErr_Clear ();

        py_str = PyObject_Str (py_long);

        if (PyUnicode_Check (py_str)) {
            PyObject *py_bytes = PyUnicode_AsUTF8String (py_str);
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
        }

        Py_DECREF (py_str);
        PyErr_Format (PyExc_ValueError, "%s not in range %ld to %ld",
                      long_str, G_MININT64, G_MAXINT64);

        g_free (long_str);
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
        long long_ = PyInt_AsLong (py_long);
        if (long_ < 0) {
            PyErr_Format (PyExc_ValueError, "%ld not in range %d to %llu",
                          long_, 0, G_MAXUINT64);
            return FALSE;
        }
        ulong_ = long_;
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
        }

        Py_DECREF (py_str);

        PyErr_Format (PyExc_ValueError, "%s not in range %d to %llu",
                      long_str, 0, G_MAXUINT64);

        g_free (long_str);
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

    if (PyErr_Occurred ()) {
        PyErr_Clear ();
        PyErr_Format (PyExc_ValueError, "%f not in range %f to %f", double_, -G_MAXFLOAT, G_MAXFLOAT);
        return FALSE;
    }

    if (double_ < -G_MAXFLOAT || double_ > G_MAXFLOAT) {
        PyErr_Format (PyExc_ValueError, "%f not in range %f to %f", double_, -G_MAXFLOAT, G_MAXFLOAT);
        return FALSE;
    }

    arg->v_float = double_;

    return TRUE;
}

gboolean
_pygi_marshal_in_double (PyGIInvokeState   *state,
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

    if (PyErr_Occurred ()) {
        PyErr_Clear ();
        PyErr_Format (PyExc_ValueError, "%f not in range %f to %f", double_, -G_MAXDOUBLE, G_MAXDOUBLE);
        return FALSE;
    }

    if (double_ < -G_MAXDOUBLE || double_ > G_MAXDOUBLE) {
        PyErr_Format (PyExc_ValueError, "%f not in range %f to %f", double_, -G_MAXDOUBLE, G_MAXDOUBLE);
        return FALSE;
    }

    arg->v_double = double_;

    return TRUE;
}

gboolean
_pygi_marshal_in_unichar (PyGIInvokeState   *state,
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
       PyErr_Format (PyExc_TypeError, "Must be a one character string, not %ld characters",
                     size);
       g_free (string_);
       return FALSE;
    }

    arg->v_uint32 = g_utf8_get_char (string_);
    g_free (string_);

    return TRUE;
}
gboolean
_pygi_marshal_in_gtype (PyGIInvokeState   *state,
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
_pygi_marshal_in_utf8 (PyGIInvokeState   *state,
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
_pygi_marshal_in_filename (PyGIInvokeState   *state,
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
_pygi_marshal_in_array (PyGIInvokeState   *state,
                        PyGICallableCache *callable_cache,
                        PyGIArgCache      *arg_cache,
                        PyObject          *py_arg,
                        GIArgument        *arg)
{
    PyGIMarshalInFunc in_marshaller;
    int i;
    Py_ssize_t length;
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

    is_ptr_array = (sequence_cache->array_type == GI_ARRAY_TYPE_PTR_ARRAY);
    if (is_ptr_array) {
        array_ = (GArray *)g_ptr_array_new ();
    } else {
        array_ = g_array_sized_new (sequence_cache->is_zero_terminated,
                                    FALSE,
                                    sequence_cache->item_size,
                                    length);
    }

    if (array_ == NULL) {
        PyErr_NoMemory ();
        return FALSE;
    }

    if (sequence_cache->item_cache->type_tag == GI_TYPE_TAG_UINT8 &&
        PYGLIB_PyBytes_Check (py_arg)) {
        memcpy(array_->data, PYGLIB_PyBytes_AsString (py_arg), length);

        goto array_success;
    }

    in_marshaller = sequence_cache->item_cache->in_marshaller;
    for (i = 0; i < length; i++) {
        GIArgument item;
        PyObject *py_item = PySequence_GetItem (py_arg, i);
        if (py_item == NULL)
            goto err;

        if (!in_marshaller ( state,
                             callable_cache,
                             sequence_cache->item_cache,
                             py_item,
                            &item))
            goto err;

        /* FIXME: it is much more efficent to have seperate marshaller
         *        for ptr arrays than doing the evaluation
         *        and casting each loop iteration
         */
        if (is_ptr_array)
            g_ptr_array_add((GPtrArray *)array_, item.v_pointer);
        else
            g_array_insert_val (array_, i, item);
        continue;
err:
        if (sequence_cache->item_cache->in_cleanup != NULL) {
            gsize j;
            PyGIMarshalCleanupFunc cleanup_func =
                sequence_cache->item_cache->in_cleanup;

            for(j = 0; j < i; j++) {
                cleanup_func (state,
                              sequence_cache->item_cache,
                              g_array_index (array_, gpointer, j),
                              TRUE);
            }
        }

        if (is_ptr_array)
            g_ptr_array_free (array_, TRUE);
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

        if (child_cache->direction == GI_DIRECTION_INOUT) {
            gint *len_arg = (gint *)state->in_args[child_cache->c_arg_index].v_pointer;
            /* if we are not setup yet just set the in arg */
            if (len_arg == NULL)
                state->in_args[child_cache->c_arg_index].v_long = length;
            else
                *len_arg = length;
        } else {
            state->in_args[child_cache->c_arg_index].v_long = length;
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
_pygi_marshal_in_glist (PyGIInvokeState   *state,
                        PyGICallableCache *callable_cache,
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

    length = PySequence_Length (py_arg);
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

        if (!in_marshaller ( state,
                             callable_cache,
                             sequence_cache->item_cache,
                             py_item,
                            &item))
            goto err;

        list_ = g_list_append (list_, item.v_pointer);
        continue;
err:
        if (sequence_cache->item_cache->in_cleanup != NULL) {
            GDestroyNotify cleanup = sequence_cache->item_cache->in_cleanup;
        }

        g_list_free (list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = list_;
    return TRUE;
}

gboolean
_pygi_marshal_in_gslist (PyGIInvokeState   *state,
                         PyGICallableCache *callable_cache,
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

    length = PySequence_Length (py_arg);
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

        if (!in_marshaller ( state,
                             callable_cache,
                             sequence_cache->item_cache,
                             py_item,
                            &item))
            goto err;

        list_ = g_slist_append (list_, item.v_pointer);
        continue;
err:
        if (sequence_cache->item_cache->in_cleanup != NULL) {
            GDestroyNotify cleanup = sequence_cache->item_cache->in_cleanup;
        }

        g_slist_free (list_);
        _PyGI_ERROR_PREFIX ("Item %i: ", i);
        return FALSE;
    }

    arg->v_pointer = list_;
    return TRUE;
}

gboolean
_pygi_marshal_in_ghash (PyGIInvokeState   *state,
                        PyGICallableCache *callable_cache,
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

        if (!key_in_marshaller ( state,
                                 callable_cache,
                                 hash_cache->key_cache,
                                 py_key,
                                &key))
            goto err;

        if (!value_in_marshaller ( state,
                                   callable_cache,
                                   hash_cache->value_cache,
                                   py_value,
                                  &value))
            goto err;

        g_hash_table_insert (hash_, key.v_pointer, value.v_pointer);
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
_pygi_marshal_in_gerror (PyGIInvokeState   *state,
                         PyGICallableCache *callable_cache,
                         PyGIArgCache      *arg_cache,
                         PyObject          *py_arg,
                         GIArgument        *arg)
{
    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for GErrors is not implemented");
    return FALSE;
}

gboolean
_pygi_marshal_in_interface_callback (PyGIInvokeState   *state,
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
            py_user_data = PyTuple_GetItem (state->py_in_args, user_data_cache->py_arg_index);
            if (!py_user_data)
                return FALSE;
        } else {
            py_user_data = Py_None;
            Py_INCREF (Py_None);
        }
    }

    if (py_arg == Py_None && !(py_user_data == Py_None || py_user_data == NULL)) {
        Py_DECREF (py_user_data);
        PyErr_Format (PyExc_TypeError,
                      "When passing None for a callback userdata must also be None");

        return FALSE;
    }

    if (py_arg == Py_None) {
        Py_XDECREF (py_user_data);
        return TRUE;
    }

    if (!PyCallable_Check (py_arg)) {
        Py_XDECREF (py_user_data);
        PyErr_Format (PyExc_TypeError,
                      "Callback needs to be a function or method not %s",
                      py_arg->ob_type->tp_name);

        return FALSE;
    }

    if (callback_cache->destroy_notify_index > 0)
        destroy_cache = callable_cache->args_cache[callback_cache->destroy_notify_index];

    callable_info = (GICallableInfo *)callback_cache->interface_info;

    closure = _pygi_make_native_closure (callable_info, callback_cache->scope, py_arg, py_user_data);
    arg->v_pointer = closure->closure;
    if (user_data_cache != NULL) {
        state->in_args[user_data_cache->c_arg_index].v_pointer = closure;
    }

    if (destroy_cache) {
        PyGICClosure *destroy_notify = _pygi_destroy_notify_create ();
        state->in_args[destroy_cache->c_arg_index].v_pointer = destroy_notify->closure;
    }

    return TRUE;
}

gboolean
_pygi_marshal_in_interface_enum (PyGIInvokeState   *state,
                                 PyGICallableCache *callable_cache,
                                 PyGIArgCache      *arg_cache,
                                 PyObject          *py_arg,
                                 GIArgument        *arg)
{
    PyObject *int_;
    gint is_instance;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    is_instance = PyObject_IsInstance (py_arg, iface_cache->py_type);

    int_ = PYGLIB_PyNumber_Long (py_arg);
    if (int_ == NULL) {
        PyErr_Clear();
        goto err;
    }

    arg->v_long = PYGLIB_PyLong_AsLong (int_);
    Py_DECREF (int_);

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
    PyErr_Format (PyExc_TypeError, "Expected a %s, but got %s",
                  iface_cache->type_name, py_arg->ob_type->tp_name);
    return FALSE;
}

gboolean
_pygi_marshal_in_interface_flags (PyGIInvokeState   *state,
                                  PyGICallableCache *callable_cache,
                                  PyGIArgCache      *arg_cache,
                                  PyObject          *py_arg,
                                  GIArgument        *arg)
{
    PyObject *int_;
    gint is_instance;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    is_instance = PyObject_IsInstance (py_arg, iface_cache->py_type);

    int_ = PYGLIB_PyNumber_Long (py_arg);
    if (int_ == NULL) {
        PyErr_Clear ();
        goto err;
    }

    arg->v_long = PYGLIB_PyLong_AsLong (int_);
    Py_DECREF (int_);

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
        gboolean success;
        success = pygi_struct_foreign_convert_to_g_argument (py_arg,
                                                             iface_cache->interface_info,
                                                             arg_cache->transfer,
                                                             arg);

        return success;
    } else if (!PyObject_IsInstance (py_arg, iface_cache->py_type)) {
        PyErr_Format (PyExc_TypeError, "Expected %s, but got %s",
                      iface_cache->type_name,
                      iface_cache->py_type->ob_type->tp_name);
        return FALSE;
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
_pygi_marshal_in_interface_boxed (PyGIInvokeState   *state,
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
_pygi_marshal_in_interface_object (PyGIInvokeState   *state,
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
        PyErr_Format (PyExc_TypeError, "Expected %s, but got %s",
                      ( (PyGIInterfaceCache *)arg_cache)->type_name,
                      ( (PyGIInterfaceCache *)arg_cache)->py_type->ob_type->tp_name);
        return FALSE;
    }

    arg->v_pointer = pygobject_get(py_arg);
    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        g_object_ref (arg->v_pointer);

    return TRUE;
}

gboolean
_pygi_marshal_in_interface_union (PyGIInvokeState   *state,
                                  PyGICallableCache *callable_cache,
                                  PyGIArgCache      *arg_cache,
                                  PyObject          *py_arg,
                                  GIArgument        *arg)
{
    PyErr_Format(PyExc_NotImplementedError,
                 "Marshalling for this type is not implemented yet");
    return FALSE;
}

gboolean _pygi_marshal_in_interface_instance (PyGIInvokeState   *state,
                                              PyGICallableCache *callable_cache,
                                              PyGIArgCache      *arg_cache,
                                              PyObject          *py_arg,
                                              GIArgument        *arg)
{
    GIInfoType info_type;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    /* FIXME: add instance checks */

    info_type = g_base_info_get_type (iface_cache->interface_info);
    switch (info_type) {
        case GI_INFO_TYPE_UNION:
        case GI_INFO_TYPE_STRUCT:
        {
            GType type = iface_cache->g_type;
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
            break;
        default:
            /* Other types don't have methods. */
            g_assert_not_reached ();
   }

   return TRUE;
}

PyObject *
_pygi_marshal_out_void (PyGIInvokeState   *state,
                        PyGICallableCache *callable_cache,
                        PyGIArgCache      *arg_cache,
                        GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    if (arg_cache->is_pointer)
        py_obj = arg->v_pointer;
    else
        py_obj = Py_None;

    Py_XINCREF (py_obj);
    return py_obj;
}

PyObject *
_pygi_marshal_out_boolean (PyGIInvokeState   *state,
                           PyGICallableCache *callable_cache,
                           PyGIArgCache      *arg_cache,
                           GIArgument        *arg)
{
    PyObject *py_obj = PyBool_FromLong (arg->v_boolean);
    return py_obj;
}

PyObject *
_pygi_marshal_out_int8 (PyGIInvokeState   *state,
                        PyGICallableCache *callable_cache,
                        PyGIArgCache      *arg_cache,
                        GIArgument        *arg)
{
    PyObject *py_obj = PYGLIB_PyLong_FromLong (arg->v_int8);
    return py_obj;
}

PyObject *
_pygi_marshal_out_uint8 (PyGIInvokeState   *state,
                         PyGICallableCache *callable_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj =  PYGLIB_PyLong_FromLong (arg->v_uint8);

    return py_obj;
}

PyObject *
_pygi_marshal_out_int16 (PyGIInvokeState   *state,
                         PyGICallableCache *callable_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj =  PYGLIB_PyLong_FromLong (arg->v_int16);

    return py_obj;
}

PyObject *
_pygi_marshal_out_uint16 (PyGIInvokeState   *state,
                          PyGICallableCache *callable_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    PyObject *py_obj =  PYGLIB_PyLong_FromLong (arg->v_uint16);

    return py_obj;
}

PyObject *
_pygi_marshal_out_int32 (PyGIInvokeState   *state,
                         PyGICallableCache *callable_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj = PYGLIB_PyLong_FromLong (arg->v_int32);

    return py_obj;
}

PyObject *
_pygi_marshal_out_uint32 (PyGIInvokeState   *state,
                          PyGICallableCache *callable_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    PyObject *py_obj = PyLong_FromLongLong (arg->v_uint32);

    return py_obj;
}

PyObject *
_pygi_marshal_out_int64 (PyGIInvokeState   *state,
                         PyGICallableCache *callable_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj = PyLong_FromLongLong (arg->v_int64);

    return py_obj;
}

PyObject *
_pygi_marshal_out_uint64 (PyGIInvokeState   *state,
                          PyGICallableCache *callable_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    PyObject *py_obj = PyLong_FromUnsignedLongLong (arg->v_uint64);

    return py_obj;
}

PyObject *
_pygi_marshal_out_float (PyGIInvokeState   *state,
                         PyGICallableCache *callable_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj = PyFloat_FromDouble (arg->v_float);

    return py_obj;
}

PyObject *
_pygi_marshal_out_double (PyGIInvokeState   *state,
                          PyGICallableCache *callable_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    PyObject *py_obj = PyFloat_FromDouble (arg->v_double);

    return py_obj;
}

PyObject *
_pygi_marshal_out_unichar (PyGIInvokeState   *state,
                           PyGICallableCache *callable_cache,
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
                         PyGICallableCache *callable_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    py_obj = pyg_type_wrapper_new ( (GType)arg->v_long);
    return py_obj;
}

PyObject *
_pygi_marshal_out_utf8 (PyGIInvokeState   *state,
                        PyGICallableCache *callable_cache,
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
                            PyGICallableCache *callable_cache,
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
                         PyGICallableCache *callable_cache,
                         PyGIArgCache      *arg_cache,
                         GIArgument        *arg)
{
    GArray *array_;
    PyObject *py_obj = NULL;
    PyGISequenceCache *seq_cache = (PyGISequenceCache *)arg_cache;
    gsize processed_items = 0;

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
            len = g_strv_length (arg->v_string);
        } else {
            GIArgument *len_arg = state->args[seq_cache->len_arg_index];
            len = len_arg->v_long;
        }

        array_ = g_array_new (FALSE,
                              FALSE,
                              seq_cache->item_size);
        if (array_ == NULL) {
            PyErr_NoMemory ();

            if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
                g_free (arg->v_pointer);

            return NULL;
        }

        array_->data = arg->v_pointer;
        array_->len = len;
    }

    if (seq_cache->item_cache->type_tag == GI_TYPE_TAG_UINT8) {
        if (arg->v_pointer == NULL) {
            py_obj = PYGLIB_PyBytes_FromString ("");
        } else {
            py_obj = PYGLIB_PyBytes_FromStringAndSize (array_->data, array_->len);
        }
    } else {
        if (arg->v_pointer == NULL) {
            py_obj = PyList_New (0);
        } else {
            int i;

            gsize item_size;
            PyGIMarshalOutFunc item_out_marshaller;
            PyGIArgCache *item_arg_cache;

            py_obj = PyList_New (array_->len);
            if (py_obj == NULL)
                goto err;


            item_arg_cache = seq_cache->item_cache;
            item_out_marshaller = item_arg_cache->out_marshaller;

            item_size = g_array_get_element_size (array_);

            for (i = 0; i < array_->len; i++) {
                GIArgument item_arg;
                PyObject *py_item;

                if (seq_cache->array_type == GI_ARRAY_TYPE_PTR_ARRAY) {
                    item_arg.v_pointer = g_ptr_array_index ( ( GPtrArray *)array_, i);
                } else if (item_arg_cache->type_tag == GI_TYPE_TAG_INTERFACE) {
                    item_arg.v_pointer = array_->data + i * item_size;
                    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING) {
                       PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *) item_arg_cache;
                       switch (g_base_info_get_type (iface_cache->interface_info)) {
                           case GI_INFO_TYPE_STRUCT:
                           {
                               gpointer *_struct = g_malloc (item_size);
                               memcpy (_struct, item_arg.v_pointer, item_size);
                               item_arg.v_pointer = _struct;
                               break;
                           }
                           default:
                               break;
                       }
                    }
                } else {
                    memcpy (&item_arg, array_->data + i * item_size, item_size);
                }

                py_item = item_out_marshaller ( state,
                                                callable_cache,
                                                item_arg_cache,
                                                &item_arg);

                if (py_item == NULL) {
                    Py_CLEAR (py_obj);

                    if (seq_cache->array_type == GI_ARRAY_TYPE_C)
                        g_array_unref (array_);

                    goto err;
                }
                PyList_SET_ITEM (py_obj, i, py_item);
                processed_items++;
            }
        }
    }

    if (seq_cache->array_type == GI_ARRAY_TYPE_C)
        g_array_free (array_, FALSE);

    return py_obj;

err:
    if (seq_cache->array_type == GI_ARRAY_TYPE_C) {
        g_array_free (array_, arg_cache->transfer == GI_TRANSFER_EVERYTHING);
    } else {
        /* clean up unprocessed items */
        if (seq_cache->item_cache->out_cleanup != NULL) {
            int j;
            PyGIMarshalCleanupFunc cleanup_func = seq_cache->item_cache->out_cleanup;
            for (j = processed_items; j < array_->len; j++) {
                cleanup_func (state,
                              seq_cache->item_cache,
                              g_array_index (array_, gpointer, j),
                              FALSE);
            }
        }

        if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
            g_array_free (array_, TRUE);
    }

    return NULL;
}

PyObject *
_pygi_marshal_out_glist (PyGIInvokeState   *state,
                         PyGICallableCache *callable_cache,
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
    length = g_list_length (list_);

    py_obj = PyList_New (length);
    if (py_obj == NULL)
        return NULL;

    item_arg_cache = seq_cache->item_cache;
    item_out_marshaller = item_arg_cache->out_marshaller;

    for (i = 0; list_ != NULL; list_ = g_list_next (list_), i++) {
        GIArgument item_arg;
        PyObject *py_item;

        item_arg.v_pointer = list_->data;
        py_item = item_out_marshaller ( state,
                                        callable_cache,
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
                          PyGICallableCache *callable_cache,
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
    length = g_slist_length (list_);

    py_obj = PyList_New (length);
    if (py_obj == NULL)
        return NULL;

    item_arg_cache = seq_cache->item_cache;
    item_out_marshaller = item_arg_cache->out_marshaller;

    for (i = 0; list_ != NULL; list_ = g_slist_next (list_), i++) {
        GIArgument item_arg;
        PyObject *py_item;

        item_arg.v_pointer = list_->data;
        py_item = item_out_marshaller ( state,
                                        callable_cache,
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
                         PyGICallableCache *callable_cache,
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

    if (hash_ == NULL) {
        py_obj = Py_None;
        Py_INCREF (py_obj);
        return py_obj;
    }

    py_obj = PyDict_New ();
    if (py_obj == NULL)
        return NULL;

    key_arg_cache = hash_cache->key_cache;
    key_out_marshaller = key_arg_cache->out_marshaller;

    value_arg_cache = hash_cache->value_cache;
    value_out_marshaller = value_arg_cache->out_marshaller;

    g_hash_table_iter_init (&hash_table_iter, hash_);
    while (g_hash_table_iter_next (&hash_table_iter,
                                   &key_arg.v_pointer,
                                   &value_arg.v_pointer)) {
        PyObject *py_key;
        PyObject *py_value;
        int retval;

        py_key = key_out_marshaller ( state,
                                      callable_cache,
                                      key_arg_cache,
                                     &key_arg);

        if (py_key == NULL) {
            Py_CLEAR (py_obj);
            return NULL;
        }

        py_value = value_out_marshaller ( state,
                                          callable_cache,
                                          value_arg_cache,
                                         &value_arg);

        if (py_value == NULL) {
            Py_CLEAR (py_obj);
            Py_DECREF(py_key);
            return NULL;
        }

        retval = PyDict_SetItem (py_obj, py_key, py_value);

        Py_DECREF (py_key);
        Py_DECREF (py_value);

        if (retval < 0) {
            Py_CLEAR (py_obj);
            return NULL;
        }
    }

    return py_obj;
}

PyObject *
_pygi_marshal_out_gerror (PyGIInvokeState   *state,
                          PyGICallableCache *callable_cache,
                          PyGIArgCache      *arg_cache,
                          GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for gerror out is not implemented");
    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_callback (PyGIInvokeState   *state,
                                      PyGICallableCache *callable_cache,
                                      PyGIArgCache      *arg_cache,
                                      GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Callback out values are not supported");
    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_enum (PyGIInvokeState   *state,
                                  PyGICallableCache *callable_cache,
                                  PyGIArgCache      *arg_cache,
                                  GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (iface_cache->g_type == G_TYPE_NONE) {
        py_obj = PyObject_CallFunction (iface_cache->py_type, "l", arg->v_long);
    } else {
        py_obj = pyg_enum_from_gtype (iface_cache->g_type, arg->v_long);
    }
    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_flags (PyGIInvokeState   *state,
                                   PyGICallableCache *callable_cache,
                                   PyGIArgCache      *arg_cache,
                                   GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (iface_cache->g_type == G_TYPE_NONE) {
        /* An enum with a GType of None is an enum without GType */

        PyObject *py_type = _pygi_type_import_by_gi_info (iface_cache->interface_info);
        PyObject *py_args = NULL;

        if (!py_type)
            return NULL;

        py_args = PyTuple_New (1);
        if (PyTuple_SetItem (py_args, 0, PyLong_FromLong (arg->v_long)) != 0) {
            Py_DECREF (py_args);
            Py_DECREF (py_type);
            return NULL;
        }

        py_obj = PyObject_CallFunction (py_type, "l", arg->v_long);

        Py_DECREF (py_args);
        Py_DECREF (py_type);
    } else {
        py_obj = pyg_flags_from_gtype (iface_cache->g_type, arg->v_long);
    }

    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_struct (PyGIInvokeState   *state,
                                    PyGICallableCache *callable_cache,
                                    PyGIArgCache      *arg_cache,
                                    GIArgument        *arg)
{
    PyObject *py_obj = NULL;
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
    GType type = iface_cache->g_type;

    if (arg->v_pointer == NULL) {
        py_obj = Py_None;
        Py_INCREF (py_obj);
        return py_obj;
    }

    if (g_type_is_a (type, G_TYPE_VALUE)) {
        py_obj = pyg_value_as_pyobject (arg->v_pointer, FALSE);
    } else if (iface_cache->is_foreign) {
        py_obj = pygi_struct_foreign_convert_from_g_argument (iface_cache->interface_info,
                                                              arg->v_pointer);
    } else if (g_type_is_a (type, G_TYPE_BOXED)) {
        py_obj = _pygi_boxed_new ( (PyTypeObject *)iface_cache->py_type, arg->v_pointer, 
                                  arg_cache->transfer == GI_TRANSFER_EVERYTHING);
    } else if (g_type_is_a (type, G_TYPE_POINTER)) {
        if (iface_cache->py_type == NULL ||
                !PyType_IsSubtype ( (PyTypeObject *)iface_cache->py_type, &PyGIStruct_Type)) {
            g_warn_if_fail(arg_cache->transfer == GI_TRANSFER_NOTHING);
            py_obj = pyg_pointer_new (type, arg->v_pointer);
        } else {
            py_obj = _pygi_struct_new ( (PyTypeObject *)iface_cache->py_type, arg->v_pointer, 
                                      arg_cache->transfer == GI_TRANSFER_EVERYTHING);
        }
    } else if (g_type_is_a (type, G_TYPE_VARIANT)) {
         g_variant_ref_sink (arg->v_pointer);
         py_obj = _pygi_struct_new ( (PyTypeObject *)iface_cache->py_type, arg->v_pointer, 
                                      FALSE);
    } else if (type == G_TYPE_NONE && iface_cache->is_foreign) {
        py_obj = pygi_struct_foreign_convert_from_g_argument (iface_cache->interface_info, arg->v_pointer);
    } else if (type == G_TYPE_NONE) {
        py_obj = _pygi_struct_new ( (PyTypeObject *) iface_cache->py_type, arg->v_pointer, 
                                   arg_cache->transfer == GI_TRANSFER_EVERYTHING);
    } else {
        PyErr_Format (PyExc_NotImplementedError,
                      "structure type '%s' is not supported yet",
                      g_type_name (type));
    }

    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_interface (PyGIInvokeState   *state,
                                       PyGICallableCache *callable_cache,
                                       PyGIArgCache      *arg_cache,
                                       GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for this type is not implemented yet");
    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_boxed (PyGIInvokeState   *state,
                                   PyGICallableCache *callable_cache,
                                   PyGIArgCache      *arg_cache,
                                   GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for this type is not implemented yet");
    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_object (PyGIInvokeState   *state,
                                    PyGICallableCache *callable_cache,
                                    PyGIArgCache      *arg_cache,
                                    GIArgument        *arg)
{
    PyObject *py_obj;

    if (arg->v_pointer == NULL) {
        py_obj = Py_None;
        Py_INCREF (py_obj);
        return py_obj;
    }

    py_obj = pygobject_new (arg->v_pointer);

    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        g_object_unref (arg->v_pointer);

    return py_obj;
}

PyObject *
_pygi_marshal_out_interface_union  (PyGIInvokeState   *state,
                                    PyGICallableCache *callable_cache,
                                    PyGIArgCache      *arg_cache,
                                    GIArgument        *arg)
{
    PyObject *py_obj = NULL;

    PyErr_Format (PyExc_NotImplementedError,
                  "Marshalling for this type is not implemented yet");
    return py_obj;
}

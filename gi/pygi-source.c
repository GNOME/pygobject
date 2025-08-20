/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 1998-2003  James Henstridge
 * Copyright (C) 2005       Oracle
 * Copyright (c) 2012       Canonical Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <pythoncapi_compat.h>

#include "pygi-type.h"

#include "pygboxed.h"
#include "pygi-basictype.h"
#include "pygi-boxed.h"
#include "pygi-info.h"
#include "pygi-source.h"

typedef struct {
    GSource source;
    PyObject *obj;
} PyGRealSource;

static gboolean
source_prepare (GSource *source, gint *timeout)
{
    PyGRealSource *pysource = (PyGRealSource *)source;
    PyObject *t;
    gboolean ret = FALSE;
    gboolean got_err = TRUE;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();

    t = PyObject_CallMethod (pysource->obj, "prepare", NULL);

    if (t == NULL) {
        goto bail;
    } else if (!PyObject_IsTrue (t)) {
        got_err = FALSE;
        goto bail;
    } else if (!PyTuple_Check (t)) {
        PyErr_SetString (
            PyExc_TypeError,
            "source prepare function must return a tuple or False");
        goto bail;
    } else if (PyTuple_Size (t) != 2) {
        PyErr_SetString (
            PyExc_TypeError,
            "source prepare function return tuple must be exactly "
            "2 elements long");
        goto bail;
    }

    if (!pygi_gboolean_from_py (PyTuple_GET_ITEM (t, 0), &ret)) {
        ret = FALSE;
        goto bail;
    }

    if (!pygi_gint_from_py (PyTuple_GET_ITEM (t, 1), timeout)) {
        ret = FALSE;
        goto bail;
    }

    got_err = FALSE;

bail:
    if (got_err) PyErr_Print ();

    Py_XDECREF (t);

    PyGILState_Release (state);

    return ret;
}

static gboolean
source_check (GSource *source)
{
    PyGRealSource *pysource = (PyGRealSource *)source;
    PyObject *t;
    gboolean ret;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();

    t = PyObject_CallMethod (pysource->obj, "check", NULL);

    if (t == NULL) {
        PyErr_Print ();
        ret = FALSE;
    } else {
        ret = PyObject_IsTrue (t);
        Py_DECREF (t);
    }

    PyGILState_Release (state);

    return ret;
}

static gboolean
source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
{
    PyGRealSource *pysource = (PyGRealSource *)source;
    PyObject *func, *args, *tuple, *t;
    gboolean ret;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();

    if (callback) {
        tuple = user_data;

        func = PyTuple_GetItem (tuple, 0);
        args = PyTuple_GetItem (tuple, 1);
    } else {
        func = Py_None;
        args = Py_None;
    }

    t = PyObject_CallMethod (pysource->obj, "dispatch", "OO", func, args);

    if (t == NULL) {
        PyErr_Print ();
        ret = FALSE;
    } else {
        ret = PyObject_IsTrue (t);
        Py_DECREF (t);
    }

    PyGILState_Release (state);

    return ret;
}

static GSourceFuncs pyg_source_funcs = {
    source_prepare,
    source_check,
    source_dispatch,
    NULL,
};

/**
 * _pyglib_destroy_notify:
 * @user_data: a PyObject pointer.
 *
 * A function that can be used as a GDestroyNotify callback that will
 * call Py_DECREF on the data.
 */
static void
destroy_notify (gpointer user_data)
{
    PyObject *obj = (PyObject *)user_data;
    PyGILState_STATE state;

    state = PyGILState_Ensure ();
    Py_DECREF (obj);
    PyGILState_Release (state);
}

static gboolean
handler_marshal (gpointer user_data)
{
    PyObject *tuple, *ret;
    gboolean res;
    PyGILState_STATE state;

    g_return_val_if_fail (user_data != NULL, FALSE);

    state = PyGILState_Ensure ();

    tuple = (PyObject *)user_data;
    ret = PyObject_CallObject (PyTuple_GetItem (tuple, 0),
                               PyTuple_GetItem (tuple, 1));
    if (!ret) {
        PyErr_Print ();
        res = FALSE;
    } else {
        res = PyObject_IsTrue (ret);
        Py_DECREF (ret);
    }

    PyGILState_Release (state);

    return res;
}

PyObject *
pygi_source_set_callback (PyGObject *self_module, PyObject *args)
{
    PyObject *self, *first, *callback, *cbargs = NULL, *data;
    Py_ssize_t len;

    len = PyTuple_Size (args);
    if (len < 2) {
        PyErr_SetString (PyExc_TypeError,
                         "set_callback requires at least 2 arguments");
        return NULL;
    }

    first = PySequence_GetSlice (args, 0, 2);
    if (!PyArg_ParseTuple (first, "OO:set_callback", &self, &callback)) {
        Py_DECREF (first);
        return NULL;
    }
    Py_DECREF (first);

    if (!pyg_boxed_check (self, G_TYPE_SOURCE)) {
        PyErr_SetString (PyExc_TypeError,
                         "first argument is not a GLib.Source");
        return NULL;
    }

    if (!PyCallable_Check (callback)) {
        PyErr_SetString (PyExc_TypeError, "second argument not callable");
        return NULL;
    }

    cbargs = PySequence_GetSlice (args, 2, len);
    if (cbargs == NULL) return NULL;

    data = Py_BuildValue ("(ON)", callback, cbargs);
    if (data == NULL) return NULL;

    g_source_set_callback (pyg_boxed_get (self, GSource), handler_marshal,
                           data, destroy_notify);

    Py_RETURN_NONE;
}

/**
 * pygi_source_new:
 *
 * Wrap the un-bindable g_source_new() and provide wrapper callbacks in the
 * GSourceFuncs which call back to Python.
 *
 * Returns NULL on error and sets an exception.
 */
PyObject *
pygi_source_new (PyObject *self, PyObject *args)
{
    PyGRealSource *source;
    PyObject *py_type, *boxed;

    g_assert (args == NULL);

    py_type = pygi_type_import_by_name ("GLib", "Source");
    if (!py_type) return NULL;

    source = (PyGRealSource *)g_source_new (&pyg_source_funcs,
                                            sizeof (PyGRealSource));
    /* g_source_new uses malloc, not slices */
    boxed = pygi_boxed_new ((PyTypeObject *)py_type, source, TRUE, 0);
    Py_DECREF (py_type);
    if (!boxed) {
        g_source_unref ((GSource *)source);
        return NULL;
    }
    source->obj = boxed;

    return source->obj;
}

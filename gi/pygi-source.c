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

#include "pygobject-private.h"

#include "pygi-private.h"
#include "pyglib.h"
#include "pyglib-private.h"
#include "pygi-source.h"

typedef struct
{
    GSource source;
    PyObject *obj;
} PyGRealSource;

static gboolean
pyg_source_prepare(GSource *source, gint *timeout)
{
    PyGRealSource *pysource = (PyGRealSource *)source;
    PyObject *t;
    gboolean ret = FALSE;
    gboolean got_err = TRUE;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();

    t = PyObject_CallMethod(pysource->obj, "prepare", NULL);

    if (t == NULL) {
	goto bail;
    } else if (!PyObject_IsTrue(t)) {
	got_err = FALSE;
	goto bail;
    } else if (!PyTuple_Check(t)) {
	PyErr_SetString(PyExc_TypeError,
			"source prepare function must return a tuple or False");
	goto bail;
    } else if (PyTuple_Size(t) != 2) {
	PyErr_SetString(PyExc_TypeError,
			"source prepare function return tuple must be exactly "
			"2 elements long");
	goto bail;
    }

    ret = PyObject_IsTrue(PyTuple_GET_ITEM(t, 0));
	*timeout = PYGLIB_PyLong_AsLong(PyTuple_GET_ITEM(t, 1));

	if (*timeout == -1 && PyErr_Occurred()) {
	    ret = FALSE;
	    goto bail;
	}

    got_err = FALSE;

bail:
    if (got_err)
	PyErr_Print();

    Py_XDECREF(t);

    pyglib_gil_state_release(state);

    return ret;
}

static gboolean
pyg_source_check(GSource *source)
{
    PyGRealSource *pysource = (PyGRealSource *)source;
    PyObject *t;
    gboolean ret;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();

    t = PyObject_CallMethod(pysource->obj, "check", NULL);

    if (t == NULL) {
	PyErr_Print();
	ret = FALSE;
    } else {
	ret = PyObject_IsTrue(t);
	Py_DECREF(t);
    }

    pyglib_gil_state_release(state);

    return ret;
}

static gboolean
pyg_source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
    PyGRealSource *pysource = (PyGRealSource *)source;
    PyObject *func, *args, *tuple, *t;
    gboolean ret;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();

    if (callback) {
	tuple = user_data;

	func = PyTuple_GetItem(tuple, 0);
        args = PyTuple_GetItem(tuple, 1);
    } else {
	func = Py_None;
	args = Py_None;
    }

    t = PyObject_CallMethod(pysource->obj, "dispatch", "OO", func, args);

    if (t == NULL) {
	PyErr_Print();
	ret = FALSE;
    } else {
	ret = PyObject_IsTrue(t);
	Py_DECREF(t);
    }

    pyglib_gil_state_release(state);

    return ret;
}

static void
pyg_source_finalize(GSource *source)
{
    PyGRealSource *pysource = (PyGRealSource *)source;
    PyObject *func, *t;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();

    func = PyObject_GetAttrString(pysource->obj, "finalize");
    if (func) {
	t = PyObject_CallObject(func, NULL);
	Py_DECREF(func);

	if (t == NULL) {
	    PyErr_Print();
	} else {
	    Py_DECREF(t);
	}
    }

    pyglib_gil_state_release(state);
}

static GSourceFuncs pyg_source_funcs =
{
    pyg_source_prepare,
    pyg_source_check,
    pyg_source_dispatch,
    pyg_source_finalize
};

PyObject *
pyg_source_set_callback(PyGObject *self_module, PyObject *args)
{
    PyObject *self, *first, *callback, *cbargs = NULL, *data;
    gint len;

    len = PyTuple_Size (args);
    if (len < 2) {
	PyErr_SetString(PyExc_TypeError,
			"set_callback requires at least 2 arguments");
	return NULL;
    }

    first = PySequence_GetSlice(args, 0, 2);
    if (!PyArg_ParseTuple(first, "OO:set_callback", &self, &callback)) {
	Py_DECREF (first);
	return NULL;
    }
    Py_DECREF(first);
    
    if (!pyg_boxed_check (self, G_TYPE_SOURCE)) {
	PyErr_SetString(PyExc_TypeError, "first argument is not a GLib.Source");
	return NULL;
    }

    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "second argument not callable");
	return NULL;
    }

    cbargs = PySequence_GetSlice(args, 2, len);
    if (cbargs == NULL)
	return NULL;

    data = Py_BuildValue("(ON)", callback, cbargs);
    if (data == NULL)
	return NULL;

    g_source_set_callback(pyg_boxed_get (self, GSource),
			  _pyglib_handler_marshal, data,
			  _pyglib_destroy_notify);

    Py_INCREF(Py_None);
    return Py_None;
}

/**
 * pyg_source_new:
 *
 * Wrap the un-bindable g_source_new() and provide wrapper callbacks in the
 * GSourceFuncs which call back to Python.
 */
PyObject*
pyg_source_new (void)
{
    PyGRealSource *source = NULL;
    PyObject      *py_type;

    source = (PyGRealSource*) g_source_new (&pyg_source_funcs, sizeof (PyGRealSource));

    py_type = _pygi_type_import_by_name ("GLib", "Source");
    /* Full ownership transfer of the source, this will be free'd with g_boxed_free. */
    source->obj = _pygi_boxed_new ( (PyTypeObject *) py_type, source,
                                    FALSE, /* copy_boxed */
                                    0);    /* slice_allocated */

    return source->obj;
}

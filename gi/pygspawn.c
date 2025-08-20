/* -*- Mode: C; c-basic-offset: 4 -*-
 * pyglib - Python bindings for GLib toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *               2004-2008  Johan Dahlin
 *
 *   pygspawn.c: wrapper for the glib library.
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

#include <glib.h>
#include <pythoncapi_compat.h>

#include "pygi-basictype.h"
#include "pygi-error.h"
#include "pygi-util.h"
#include "pygspawn.h"

struct _PyGChildSetupData {
    PyObject *func;
    PyObject *data;
};

typedef struct {
    PyLongObject obj;
    /*
     * This exists for two reasons:
     *  1. We must only close once (ideally we would not have "close" in python)
     *  2. In PyPy we cannot use PyLong_AsLong during free
     */
    GPid unclosed;
} PyGPid;

PYGI_DEFINE_TYPE ("gi._gi.Pid", PyGPid_Type, PyGPid)

static PyObject *
pyg_pid_close (PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyGPid *gpid = (PyGPid *)self;

    if (gpid->unclosed) g_spawn_close_pid (gpid->unclosed);
#ifdef G_OS_WIN32
    gpid->unclosed = NULL;
#else
    gpid->unclosed = 0;
#endif

    Py_RETURN_NONE;
}

static PyMethodDef pyg_pid_methods[] = {
    { "close", (PyCFunction)pyg_pid_close, METH_NOARGS },
    { NULL, NULL, 0 },
};

static void
pyg_pid_free (PyObject *self)
{
    PyGPid *gpid = (PyGPid *)self;

    if (gpid->unclosed) g_spawn_close_pid (gpid->unclosed);

    PyLong_Type.tp_free ((void *)self);
}

static int
pyg_pid_tp_init (PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyErr_SetString (PyExc_TypeError,
                     "gi._gi.Pid cannot be manually instantiated");
    return -1;
}

PyObject *
pyg_pid_new (GPid pid)
{
    PyObject *long_val;
#ifdef G_OS_WIN32
    long_val = PyLong_FromVoidPtr (pid);
#else
    long_val = pygi_gint_to_py (pid);
#endif
    return PyObject_CallMethod ((PyObject *)&PyGPid_Type, "__new__", "ON",
                                &PyGPid_Type, long_val);
}

static void
_pyg_spawn_async_callback (gpointer user_data)
{
    struct _PyGChildSetupData *data;
    PyObject *retval;
    PyGILState_STATE gil;

    data = (struct _PyGChildSetupData *)user_data;
    gil = PyGILState_Ensure ();
    if (data->data)
        retval = PyObject_CallFunction (data->func, "O", data->data);
    else
        retval = PyObject_CallFunction (data->func, NULL);
    if (retval)
        Py_DECREF (retval);
    else
        PyErr_Print ();
    Py_DECREF (data->func);
    Py_XDECREF (data->data);
    g_slice_free (struct _PyGChildSetupData, data);
    PyGILState_Release (gil);
}

PyObject *
pyglib_spawn_async (PyObject *object, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {
        "argv",           "envp",      "working_directory", "flags",
        "child_setup",    "user_data", "standard_input",    "standard_output",
        "standard_error", NULL
    };
    PyObject *pyargv, *pyenvp = NULL;
    char **argv, **envp = NULL;
    PyObject *func = Py_None, *user_data = NULL;
    char *working_directory = NULL;
    int flags = 0, _stdin = -1, _stdout = -1, _stderr = -1;
    PyObject *pystdin = NULL, *pystdout = NULL, *pystderr = NULL;
    gint *standard_input, *standard_output, *standard_error;
    struct _PyGChildSetupData *callback_data = NULL;
    GError *error = NULL;
    GPid child_pid = 0;
    Py_ssize_t len, i;

    if (!PyArg_ParseTupleAndKeywords (
            args, kwargs, "O|OsiOOOOO:gi._gi.spawn_async", kwlist, &pyargv,
            &pyenvp, &working_directory, &flags, &func, &user_data, &pystdin,
            &pystdout, &pystderr))
        return NULL;

    if (pystdin && PyObject_IsTrue (pystdin))
        standard_input = &_stdin;
    else
        standard_input = NULL;

    if (pystdout && PyObject_IsTrue (pystdout))
        standard_output = &_stdout;
    else
        standard_output = NULL;

    if (pystderr && PyObject_IsTrue (pystderr))
        standard_error = &_stderr;
    else
        standard_error = NULL;

    /* parse argv */
    if (!PySequence_Check (pyargv)) {
        PyErr_SetString (PyExc_TypeError,
                         "gi._gi.spawn_async: "
                         "first argument must be a sequence of strings");
        return NULL;
    }
    len = PySequence_Length (pyargv);
    argv = g_new0 (char *, len + 1);
    for (i = 0; i < len; ++i) {
        PyObject *tmp = PySequence_ITEM (pyargv, i);
        if (tmp == NULL || !PyUnicode_Check (tmp)) {
            PyErr_SetString (PyExc_TypeError,
                             "gi._gi.spawn_async: "
                             "first argument must be a sequence of strings");
            g_free (argv);
            Py_XDECREF (tmp);
            return NULL;
        }
        argv[i] = PyUnicode_AsUTF8 (tmp);
        Py_DECREF (tmp);
    }

    /* parse envp */
    if (pyenvp) {
        if (!PySequence_Check (pyenvp)) {
            PyErr_SetString (PyExc_TypeError,
                             "gi._gi.spawn_async: "
                             "second argument must be a sequence of strings");
            g_free (argv);
            return NULL;
        }
        len = PySequence_Length (pyenvp);
        envp = g_new0 (char *, len + 1);
        for (i = 0; i < len; ++i) {
            PyObject *tmp = PySequence_ITEM (pyenvp, i);
            if (tmp == NULL || !PyUnicode_Check (tmp)) {
                PyErr_SetString (
                    PyExc_TypeError,
                    "gi._gi.spawn_async: "
                    "second argument must be a sequence of strings");
                g_free (envp);
                Py_XDECREF (tmp);
                g_free (argv);
                return NULL;
            }
            envp[i] = PyUnicode_AsUTF8 (tmp);
            Py_DECREF (tmp);
        }
    }

    if (!Py_IsNone (func)) {
        if (!PyCallable_Check (func)) {
            PyErr_SetString (PyExc_TypeError,
                             "child_setup parameter must be callable or None");
            g_free (argv);
            if (envp) g_free (envp);
            return NULL;
        }
        callback_data = g_slice_new (struct _PyGChildSetupData);
        callback_data->func = func;
        callback_data->data = user_data;
        Py_INCREF (callback_data->func);
        if (callback_data->data) Py_INCREF (callback_data->data);
    }

    if (!g_spawn_async_with_pipes (
            working_directory, argv, envp, flags,
            !Py_IsNone (func) ? _pyg_spawn_async_callback : NULL,
            callback_data, &child_pid, standard_input, standard_output,
            standard_error, &error))


    {
        g_free (argv);
        if (envp) g_free (envp);
        if (callback_data) {
            Py_DECREF (callback_data->func);
            Py_XDECREF (callback_data->data);
            g_slice_free (struct _PyGChildSetupData, callback_data);
        }
        pygi_error_check (&error);
        return NULL;
    }
    g_free (argv);
    if (envp) g_free (envp);

    if (standard_input)
        pystdin = pygi_gint_to_py (*standard_input);
    else {
        pystdin = Py_NewRef (Py_None);
    }

    if (standard_output)
        pystdout = pygi_gint_to_py (*standard_output);
    else {
        pystdout = Py_NewRef (Py_None);
    }

    if (standard_error)
        pystderr = pygi_gint_to_py (*standard_error);
    else {
        pystderr = Py_NewRef (Py_None);
    }

    return Py_BuildValue ("NNNN", pyg_pid_new (child_pid), pystdin, pystdout,
                          pystderr);
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_spawn_register_types (PyObject *d)
{
    PyGPid_Type.tp_base = &PyLong_Type;
    PyGPid_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGPid_Type.tp_methods = pyg_pid_methods;
    PyGPid_Type.tp_init = pyg_pid_tp_init;
    PyGPid_Type.tp_free = (freefunc)pyg_pid_free;
    PyGPid_Type.tp_new = PyLong_Type.tp_new;
    PyGPid_Type.tp_alloc = PyType_GenericAlloc;

    if (PyType_Ready (&PyGPid_Type)) return -1;

    PyDict_SetItemString (d, "Pid", (PyObject *)&PyGPid_Type);

    return 0;
}

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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <Python.h>
#include <glib/gspawn.h>

#include "pyglib.h"
#include "pyglib-private.h"

struct _PyGChildSetupData {
    PyObject *func;
    PyObject *data;
};

static PyObject *
pyg_pid_close(PyIntObject *self, PyObject *args, PyObject *kwargs)
{
    g_spawn_close_pid((GPid) self->ob_ival);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pyg_pid_methods[] = {
    { "close", (PyCFunction)pyg_pid_close, METH_NOARGS },
    { NULL, NULL, 0 }
};

static void
pyg_pid_free(PyIntObject *gpid)
{
    g_spawn_close_pid((GPid) gpid->ob_ival);
    PyInt_Type.tp_free((void *) gpid);
}

static int
pyg_pid_tp_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyErr_SetString(PyExc_TypeError, "glib.Pid cannot be manually instantiated");
    return -1;
}

static PyTypeObject PyGPid_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "glib.Pid",
    sizeof(PyIntObject),
    0,
    0,					  /* tp_dealloc */
    0,                			  /* tp_print */
    0,					  /* tp_getattr */
    0,					  /* tp_setattr */
    0,					  /* tp_compare */
    0,		  			  /* tp_repr */
    0,                   		  /* tp_as_number */
    0,					  /* tp_as_sequence */
    0,					  /* tp_as_mapping */
    0,					  /* tp_hash */
    0,					  /* tp_call */
    0,		  			  /* tp_str */
    0,					  /* tp_getattro */
    0,					  /* tp_setattro */
    0,				  	  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT, 		  /* tp_flags */
    0,      				  /* tp_doc */
    0,					  /* tp_traverse */
    0,					  /* tp_clear */
    0,					  /* tp_richcompare */
    0,					  /* tp_weaklistoffset */
    0,					  /* tp_iter */
    0,					  /* tp_iternext */
    pyg_pid_methods,			  /* tp_methods */
    0,					  /* tp_members */
    0,					  /* tp_getset */
    0,	  				  /* tp_base */
    0,					  /* tp_dict */
    0,					  /* tp_descr_get */
    0,					  /* tp_descr_set */
    0,					  /* tp_dictoffset */
    pyg_pid_tp_init,		  	  /* tp_init */
    0,					  /* tp_alloc */
    0,					  /* tp_new */
    (freefunc)pyg_pid_free,		  /* tp_free */
    0,					  /* tp_is_gc */
};

PyObject *
pyg_pid_new(GPid pid)
{
    PyIntObject *pygpid;
    pygpid = PyObject_NEW(PyIntObject, &PyGPid_Type);

    pygpid->ob_ival = pid;
    return (PyObject *) pygpid;
}

static void
_pyg_spawn_async_callback(gpointer user_data)
{
    struct _PyGChildSetupData *data;
    PyObject *retval;
    PyGILState_STATE gil;

    data = (struct _PyGChildSetupData *) user_data;
    gil = pyglib_gil_state_ensure();
    if (data->data)
        retval = PyObject_CallFunction(data->func, "O", data->data);
    else
        retval = PyObject_CallFunction(data->func, NULL);
    if (retval)
	Py_DECREF(retval);
    else
	PyErr_Print();
    Py_DECREF(data->func);
    Py_XDECREF(data->data);
    g_free(data);
    pyglib_gil_state_release(gil);
}

PyObject *
pyglib_spawn_async(PyObject *object, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "argv", "envp", "working_directory", "flags",
                              "child_setup", "user_data", "standard_input",
                              "standard_output", "standard_error", NULL };
    PyObject *pyargv, *pyenvp = NULL;
    char **argv, **envp = NULL;
    PyObject *func = Py_None, *user_data = NULL;
    char *working_directory = NULL;
    int flags = 0, _stdin = -1, _stdout = -1, _stderr = -1;
    PyObject *pystdin = NULL, *pystdout = NULL, *pystderr = NULL;
    gint *standard_input, *standard_output, *standard_error;
    struct _PyGChildSetupData *callback_data = NULL;
    GError *error = NULL;
    GPid child_pid = -1;
    Py_ssize_t len, i;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OsiOOOOO:glib.spawn_async",
                                     kwlist,
                                     &pyargv, &pyenvp, &working_directory, &flags,
                                     &func, &user_data,
                                     &pystdin, &pystdout, &pystderr))
        return NULL;

    if (pystdin && PyObject_IsTrue(pystdin))
        standard_input = &_stdin;
    else
        standard_input = NULL;

    if (pystdout && PyObject_IsTrue(pystdout))
        standard_output = &_stdout;
    else
        standard_output = NULL;

    if (pystderr && PyObject_IsTrue(pystderr))
        standard_error = &_stderr;
    else
        standard_error = NULL;

      /* parse argv */
    if (!PySequence_Check(pyargv)) {
        PyErr_SetString(PyExc_TypeError,
                        "glib.spawn_async: "
			"first argument must be a sequence of strings");
        return NULL;
    }
    len = PySequence_Length(pyargv);
    argv = g_new0(char *, len + 1);
    for (i = 0; i < len; ++i) {
        PyObject *tmp = PySequence_ITEM(pyargv, i);
        if (!PyString_Check(tmp)) {
            PyErr_SetString(PyExc_TypeError,
                            "glib.spawn_async: "
			    "first argument must be a sequence of strings");
            g_free(argv);
            Py_XDECREF(tmp);
            return NULL;
        }
        argv[i] = PyString_AsString(tmp);
        Py_DECREF(tmp);
    }

      /* parse envp */
    if (pyenvp) {
        if (!PySequence_Check(pyenvp)) {
            PyErr_SetString(PyExc_TypeError,
                            "glib.spawn_async: "
			    "second argument must be a sequence of strings");
            g_free(argv);
            return NULL;
        }
        len = PySequence_Length(pyenvp);
        envp = g_new0(char *, len + 1);
        for (i = 0; i < len; ++i) {
            PyObject *tmp = PySequence_ITEM(pyenvp, i);
            if (!PyString_Check(tmp)) {
                PyErr_SetString(PyExc_TypeError,
                                "glib.spawn_async: "
				"second argument must be a sequence of strings");
                g_free(envp);
                Py_XDECREF(tmp);
		g_free(argv);
                return NULL;
            }
            envp[i] = PyString_AsString(tmp);
            Py_DECREF(tmp);
        }
    }

    if (func != Py_None) {
        if (!PyCallable_Check(func)) {
            PyErr_SetString(PyExc_TypeError, "child_setup parameter must be callable or None");
            g_free(argv);
            if (envp)
                g_free(envp);
            return NULL;
        }
        callback_data = g_new(struct _PyGChildSetupData, 1);
        callback_data->func = func;
        callback_data->data = user_data;
        Py_INCREF(callback_data->func);
        if (callback_data->data)
            Py_INCREF(callback_data->data);
    }

    if (!g_spawn_async_with_pipes(working_directory, argv, envp, flags,
                                  (func != Py_None ? _pyg_spawn_async_callback : NULL),
                                  callback_data, &child_pid,
                                  standard_input,
                                  standard_output,
                                  standard_error,
                                  &error))


    {
        g_free(argv);
        if (envp) g_free(envp);
        if (callback_data) {
            Py_DECREF(callback_data->func);
            Py_XDECREF(callback_data->data);
            g_free(callback_data);
        }
        pyglib_error_check(&error);
        return NULL;
    }
    g_free(argv);
    if (envp) g_free(envp);

    if (standard_input)
        pystdin = PyInt_FromLong(*standard_input);
    else {
        Py_INCREF(Py_None);
        pystdin = Py_None;
    }

    if (standard_output)
        pystdout = PyInt_FromLong(*standard_output);
    else {
        Py_INCREF(Py_None);
        pystdout = Py_None;
    }

    if (standard_error)
        pystderr = PyInt_FromLong(*standard_error);
    else {
        Py_INCREF(Py_None);
        pystderr = Py_None;
    }

    return Py_BuildValue("NNNN", pyg_pid_new(child_pid), pystdin, pystdout, pystderr);
}

void
pyglib_spawn_register_types(PyObject *d)
{
    PyGPid_Type.tp_base = &PyInt_Type;
    PYGLIB_REGISTER_TYPE(d, PyGPid_Type, "Pid");
}

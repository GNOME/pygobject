/* -*- Mode: C; c-set-style: python; c-basic-offset: 4  -*-
 * pyglib - Python bindings for GLib toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *               2004-2008  Johan Dahlin
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Python.h>
#include <pythread.h>
#include "pyglib.h"
#include "pyglib-private.h"
#include "pygmaincontext.h"

static struct _PyGLib_Functions *_PyGLib_API;
static int pyglib_thread_state_tls_key;

static PyTypeObject *_PyGMainContext_Type;
#define PyGMainContext_Type (*_PyGMainContext_Type)

void
pyglib_init(void)
{
    PyObject *glib, *cobject;
    
    glib = PyImport_ImportModule("glib");
    if (!glib) {
	if (PyErr_Occurred()) {
	    PyObject *type, *value, *traceback;
	    PyObject *py_orig_exc;
	    PyErr_Fetch(&type, &value, &traceback);
	    py_orig_exc = PyObject_Repr(value);
	    Py_XDECREF(type);
	    Py_XDECREF(value);
	    Py_XDECREF(traceback);
	    PyErr_Format(PyExc_ImportError,
			 "could not import glib (error was: %s)",
			 PyString_AsString(py_orig_exc));
	    Py_DECREF(py_orig_exc);
        } else
	    PyErr_SetString(PyExc_ImportError,
			    "could not import glib (no error given)");
    }
    
    cobject = PyObject_GetAttrString(glib, "_PyGLib_API");
    if (cobject && PyCObject_Check(cobject))
	_PyGLib_API = (struct _PyGLib_Functions *) PyCObject_AsVoidPtr(cobject);
    else {
	PyErr_SetString(PyExc_ImportError,
			"could not import glib (could not find _PyGLib_API object)");
	Py_DECREF(glib);
    }

    _PyGMainContext_Type = (PyTypeObject*)PyObject_GetAttrString(glib, "MainContext");
}

void
pyglib_init_internal(PyObject *api)
{
    _PyGLib_API = (struct _PyGLib_Functions *) PyCObject_AsVoidPtr(api);
}

gboolean
pyglib_threads_enabled(void)
{
    return _PyGLib_API->threads_enabled;
}

PyGILState_STATE
pyglib_gil_state_ensure(void)
{
    if (!_PyGLib_API->threads_enabled)
	return PyGILState_LOCKED;

    return PyGILState_Ensure();
}

void
pyglib_gil_state_release(PyGILState_STATE state)
{
    if (!_PyGLib_API->threads_enabled)
	return;

    PyGILState_Release(state);
}

#ifdef DISABLE_THREADING
gboolean
pyglib_enable_threads(void)
{
    PyErr_SetString(PyExc_RuntimeError,
		    "pyglib threading disabled at compile time");
    return FALSE;
}
#else
/* Enable threading; note that the GIL must be held by the current
 * thread when this function is called
 */
gboolean
pyglib_enable_threads(void)
{
    if (_PyGLib_API->threads_enabled)
	return TRUE;
  
    PyEval_InitThreads();
    if (!g_threads_got_initialized)
	g_thread_init(NULL);
    
    _PyGLib_API->threads_enabled = TRUE;
    pyglib_thread_state_tls_key = PyThread_create_key();
    
    return TRUE;
}
#endif

int
pyglib_gil_state_ensure_py23 (void)
{
    return PyGILState_Ensure();
}

void
pyglib_gil_state_release_py23 (int flag)
{
    PyGILState_Release(flag);
}


/**
 * pyglib_error_check:
 * @error: a pointer to the GError.
 *
 * Checks to see if the GError has been set.  If the error has been
 * set, then the glib.GError Python exception will be raised, and
 * the GError cleared.
 *
 * Returns: True if an error was set.
 */
gboolean
pyglib_error_check(GError **error)
{
    PyGILState_STATE state;
    PyObject *exc_instance;
    PyObject *d;

    g_return_val_if_fail(error != NULL, FALSE);

    if (*error == NULL)
	return FALSE;
    
    state = pyglib_gil_state_ensure();
	
    exc_instance = PyObject_CallFunction(_PyGLib_API->gerror_exception, "z",
					 (*error)->message);
    PyObject_SetAttrString(exc_instance, "domain",
			   d=PyString_FromString(g_quark_to_string((*error)->domain)));
    Py_DECREF(d);

    PyObject_SetAttrString(exc_instance, "code",
			   d=PyInt_FromLong((*error)->code));
    Py_DECREF(d);

    if ((*error)->message) {
	PyObject_SetAttrString(exc_instance, "message",
			       d=PyString_FromString((*error)->message));
	Py_DECREF(d);
    } else {
	PyObject_SetAttrString(exc_instance, "message", Py_None);
    }
    
    PyErr_SetObject(_PyGLib_API->gerror_exception, exc_instance);
    Py_DECREF(exc_instance);
    g_clear_error(error);
    
    pyglib_gil_state_release(state);
    
    return TRUE;
}

/**
 * pyglib_gerror_exception_check:
 * @error: a standard GLib GError ** output parameter
 *
 * Checks to see if a GError exception has been raised, and if so
 * translates the python exception to a standard GLib GError.  If the
 * raised exception is not a GError then PyErr_Print() is called.
 *
 * Returns: 0 if no exception has been raised, -1 if it is a
 * valid gobject.GError, -2 otherwise.
 */
gboolean
pyglib_gerror_exception_check(GError **error)
{
    PyObject *type, *value, *traceback;
    PyObject *py_message, *py_domain, *py_code;
    const char *bad_gerror_message;

    PyErr_Fetch(&type, &value, &traceback);
    if (type == NULL)
        return 0;
    PyErr_NormalizeException(&type, &value, &traceback);
    if (value == NULL) {
        PyErr_Restore(type, value, traceback);
        PyErr_Print();
        return -2;
    }
    if (!value ||
	!PyErr_GivenExceptionMatches(type,
				     (PyObject *) _PyGLib_API->gerror_exception)) {
        PyErr_Restore(type, value, traceback);
        PyErr_Print();
        return -2;
    }
    Py_DECREF(type);
    Py_XDECREF(traceback);

    py_message = PyObject_GetAttrString(value, "message");
    if (!py_message || !PyString_Check(py_message)) {
        bad_gerror_message = "gobject.GError instances must have a 'message' string attribute";
        goto bad_gerror;
    }

    py_domain = PyObject_GetAttrString(value, "domain");
    if (!py_domain || !PyString_Check(py_domain)) {
        bad_gerror_message = "gobject.GError instances must have a 'domain' string attribute";
        Py_DECREF(py_message);
        goto bad_gerror;
    }

    py_code = PyObject_GetAttrString(value, "code");
    if (!py_code || !PyInt_Check(py_code)) {
        bad_gerror_message = "gobject.GError instances must have a 'code' int attribute";
        Py_DECREF(py_message);
        Py_DECREF(py_domain);
        goto bad_gerror;
    }

    g_set_error(error, g_quark_from_string(PyString_AsString(py_domain)),
                PyInt_AsLong(py_code), PyString_AsString(py_message));

    Py_DECREF(py_message);
    Py_DECREF(py_code);
    Py_DECREF(py_domain);
    return -1;

bad_gerror:
    Py_DECREF(value);
    g_set_error(error, g_quark_from_static_string("pygobject"), 0, bad_gerror_message);
    PyErr_SetString(PyExc_ValueError, bad_gerror_message);
    PyErr_Print();
    return -2;
}

/**
 * pyglib_main_context_new:
 * @context: a GMainContext.
 *
 * Creates a wrapper for a GMainContext.
 *
 * Returns: the GMainContext wrapper.
 */
PyObject *
pyglib_main_context_new(GMainContext *context)
{
    PyGMainContext *self;

    self = (PyGMainContext *)PyObject_NEW(PyGMainContext,
					  &PyGMainContext_Type);
    if (self == NULL)
	return NULL;

    self->context = g_main_context_ref(context);
    return (PyObject *)self;
}

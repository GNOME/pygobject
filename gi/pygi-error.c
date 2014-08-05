/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 1998-2003  James Henstridge
 *               2004-2008  Johan Dahlin
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

#include "pyglib.h"
#include "pygi-private.h"
#include "pygi-error.h"


static PyObject *PyGError = NULL;
static PyObject *exception_table = NULL;

/**
 * pygi_error_marshal:
 * @error: a pointer to the GError.
 *
 * Checks to see if @error has been set.  If @error has been set, then a
 * GLib.GError Python exception object is returned (but not raised).
 *
 * Returns: a GLib.GError Python exception object, or NULL.
 */
PyObject *
pygi_error_marshal (GError **error)
{
    PyGILState_STATE state;
    PyObject *exc_type;
    PyObject *exc_instance;
    const char *domain = NULL;

    g_return_val_if_fail(error != NULL, NULL);

    if (*error == NULL)
        return NULL;

    state = pyglib_gil_state_ensure();

    exc_type = PyGError;
    if (exception_table != NULL)
    {
        PyObject *item;
        item = PyDict_GetItem(exception_table, PYGLIB_PyLong_FromLong((*error)->domain));
        if (item != NULL)
            exc_type = item;
    }

    if ((*error)->domain) {
        domain = g_quark_to_string ((*error)->domain);
    }

    exc_instance = PyObject_CallFunction (exc_type, "ssi",
                                          (*error)->message,
                                          domain,
                                          (*error)->code);

    pyglib_gil_state_release(state);

    return exc_instance;
}

/**
 * pygi_error_check:
 * @error: a pointer to the GError.
 *
 * Checks to see if the GError has been set.  If the error has been
 * set, then the glib.GError Python exception will be raised, and
 * the GError cleared.
 *
 * Returns: True if an error was set.
 */
gboolean
pygi_error_check (GError **error)
{
    PyGILState_STATE state;
    PyObject *exc_instance;

    g_return_val_if_fail(error != NULL, FALSE);
    if (*error == NULL)
        return FALSE;

    state = pyglib_gil_state_ensure();

    exc_instance = pygi_error_marshal (error);
    PyErr_SetObject(PyGError, exc_instance);
    Py_DECREF(exc_instance);
    g_clear_error(error);

    pyglib_gil_state_release(state);

    return TRUE;
}

/**
 * pygi_gerror_exception_check:
 * @error: a standard GLib GError ** output parameter
 *
 * Checks to see if a GError exception has been raised, and if so
 * translates the python exception to a standard GLib GError.  If the
 * raised exception is not a GError then PyErr_Print() is called.
 *
 * Returns: 0 if no exception has been raised, -1 if it is a
 * valid glib.GError, -2 otherwise.
 */
gboolean
pygi_gerror_exception_check (GError **error)
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
                                     (PyObject *) PyGError)) {
        PyErr_Restore(type, value, traceback);
        PyErr_Print();
        return -2;
    }
    Py_DECREF(type);
    Py_XDECREF(traceback);

    py_message = PyObject_GetAttrString(value, "message");
    if (!py_message || !PYGLIB_PyUnicode_Check(py_message)) {
        bad_gerror_message = "GLib.Error instances must have a 'message' string attribute";
        Py_XDECREF(py_message);
        goto bad_gerror;
    }

    py_domain = PyObject_GetAttrString(value, "domain");
    if (!py_domain || !PYGLIB_PyUnicode_Check(py_domain)) {
        bad_gerror_message = "GLib.Error instances must have a 'domain' string attribute";
        Py_DECREF(py_message);
        Py_XDECREF(py_domain);
        goto bad_gerror;
    }

    py_code = PyObject_GetAttrString(value, "code");
    if (!py_code || !PYGLIB_PyLong_Check(py_code)) {
        bad_gerror_message = "GLib.Error instances must have a 'code' int attribute";
        Py_DECREF(py_message);
        Py_DECREF(py_domain);
        Py_XDECREF(py_code);
        goto bad_gerror;
    }

    g_set_error(error, g_quark_from_string(PYGLIB_PyUnicode_AsString(py_domain)),
                PYGLIB_PyLong_AsLong(py_code), "%s", PYGLIB_PyUnicode_AsString(py_message));

    Py_DECREF(py_message);
    Py_DECREF(py_code);
    Py_DECREF(py_domain);
    return -1;

bad_gerror:
    Py_DECREF(value);
    g_set_error(error, g_quark_from_static_string("pygi"), 0, "%s", bad_gerror_message);
    PyErr_SetString(PyExc_ValueError, bad_gerror_message);
    PyErr_Print();
    return -2;
}

/**
 * pygi_register_exception_for_domain:
 * @name: name of the exception
 * @error_domain: error domain
 *
 * Registers a new GLib.Error exception subclass called #name for
 * a specific #domain. This exception will be raised when a GError
 * of the same domain is passed in to pygi_error_check().
 *
 * Returns: the new exception
 */
PyObject *
pygi_register_exception_for_domain (gchar *name,
                                    gint error_domain)
{
    PyObject *exception;

    exception = PyErr_NewException(name, PyGError, NULL);

    if (exception_table == NULL)
        exception_table = PyDict_New();

    PyDict_SetItem(exception_table,
                   PYGLIB_PyLong_FromLong(error_domain),
                   exception);

    return exception;
}

static gboolean
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

static PyObject *
_pygi_marshal_to_py_gerror (PyGIInvokeState   *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache      *arg_cache,
                            GIArgument        *arg)
{
    GError *error = arg->v_pointer;
    PyObject *py_obj = NULL;

    py_obj = pygi_error_marshal (&error);

    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING && error != NULL) {
        g_error_free (error);
    }

    if (py_obj != NULL) {
        return py_obj;
    } else {
        Py_RETURN_NONE;
    }
}

static gboolean
pygi_arg_gerror_setup_from_info (PyGIArgCache  *arg_cache,
                                 GITypeInfo    *type_info,
                                 GIArgInfo     *arg_info,
                                 GITransfer     transfer,
                                 PyGIDirection  direction)
{
    if (!pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction)) {
        return FALSE;
    }

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_gerror;
        arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_gerror;
        arg_cache->meta_type = PYGI_META_ARG_TYPE_PARENT;
    }

    return TRUE;
}

PyGIArgCache *
pygi_arg_gerror_new_from_info (GITypeInfo   *type_info,
                               GIArgInfo    *arg_info,
                               GITransfer    transfer,
                               PyGIDirection direction)
{
    gboolean res = FALSE;
    PyGIArgCache *arg_cache = NULL;

    arg_cache = pygi_arg_cache_alloc ();
    if (arg_cache == NULL)
        return NULL;

    res = pygi_arg_gerror_setup_from_info (arg_cache,
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

void
pygi_error_register_types (PyObject *module)
{
    PyObject *error_module = PyImport_ImportModule ("gi._error");
    if (!error_module) {
        return;
    }

    /* Stash a reference to the Python implemented gi._error.GError. */
    PyGError = PyObject_GetAttrString (error_module, "GError");
}


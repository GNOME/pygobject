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

#include <pythoncapi_compat.h>

#include "pygi-basictype.h"
#include "pygi-error.h"
#include "pygi-type.h"
#include "pygi-util.h"

PyObject *PyGError = NULL;

/**
 * pygi_error_marshal_to_py:
 * @error: a pointer to the GError.
 *
 * Checks to see if @error has been set.  If @error has been set, then a
 * GLib.GError Python exception object is returned (but not raised).
 * If not error is set returns Py_None.
 *
 * Returns: a GLib.GError Python exception object, or Py_None,
 *     or NULL and sets an error if creating the exception object fails.
 */
PyObject *
pygi_error_marshal_to_py (GError **error)
{
    PyGILState_STATE state;
    PyObject *exc_type;
    PyObject *exc_instance;
    const char *domain = NULL;

    g_return_val_if_fail (error != NULL, NULL);

    if (*error == NULL) Py_RETURN_NONE;

    state = PyGILState_Ensure ();

    exc_type = PyGError;

    if ((*error)->domain) {
        domain = g_quark_to_string ((*error)->domain);
    }

    exc_instance = PyObject_CallFunction (exc_type, "ssi", (*error)->message,
                                          domain, (*error)->code);

    PyGILState_Release (state);

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

    g_return_val_if_fail (error != NULL, FALSE);
    if (*error == NULL) return FALSE;

    state = PyGILState_Ensure ();

    exc_instance = pygi_error_marshal_to_py (error);
    if (exc_instance != NULL) {
        PyErr_SetObject (PyGError, exc_instance);
        Py_DECREF (exc_instance);
    } else {
        PyErr_Print ();
        PyErr_SetString (PyExc_RuntimeError, "Converting the GError failed");
    }
    g_clear_error (error);

    PyGILState_Release (state);

    return TRUE;
}

/**
 * pygi_error_marshal_from_py:
 * @pyerr: A Python exception instance.
 * @error: a standard GLib GError ** output parameter
 *
 * Converts from a Python implemented GError into a GError.
 *
 * Returns: TRUE if the conversion was successful, otherwise a Python exception
 *          is set and FALSE is returned.
 */
gboolean
pygi_error_marshal_from_py (PyObject *pyerr, GError **error)
{
    gint code;
    gchar *message = NULL;
    gchar *domain = NULL;
    gboolean res = FALSE;
    PyObject *py_message = NULL, *py_domain = NULL, *py_code = NULL;

    if (PyObject_IsInstance (pyerr, PyGError) != 1) {
        PyErr_Format (PyExc_TypeError, "Must be GLib.Error, not %s",
                      Py_TYPE (pyerr)->tp_name);
        return FALSE;
    }

    py_message = PyObject_GetAttrString (pyerr, "message");
    if (!py_message) {
        PyErr_SetString (
            PyExc_ValueError,
            "GLib.Error instances must have a 'message' string attribute");
        goto cleanup;
    }

    if (!pygi_utf8_from_py (py_message, &message)) goto cleanup;

    py_domain = PyObject_GetAttrString (pyerr, "domain");
    if (!py_domain) {
        PyErr_SetString (
            PyExc_ValueError,
            "GLib.Error instances must have a 'domain' string attribute");
        goto cleanup;
    }

    if (!pygi_utf8_from_py (py_domain, &domain)) goto cleanup;

    py_code = PyObject_GetAttrString (pyerr, "code");
    if (!py_code) {
        PyErr_SetString (
            PyExc_ValueError,
            "GLib.Error instances must have a 'code' int attribute");
        goto cleanup;
    }

    if (!pygi_gint_from_py (py_code, &code)) goto cleanup;

    res = TRUE;
    g_set_error_literal (error, g_quark_from_string (domain), code, message);

cleanup:
    g_free (message);
    g_free (domain);
    Py_XDECREF (py_message);
    Py_XDECREF (py_code);
    Py_XDECREF (py_domain);
    return res;
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
    int res = -1;
    PyObject *type, *value, *traceback;
    PyErr_Fetch (&type, &value, &traceback);
    if (type == NULL) return 0;
    PyErr_NormalizeException (&type, &value, &traceback);
    if (value == NULL) {
        PyErr_Restore (type, value, traceback);
        PyErr_Print ();
        return -2;
    }
    if (!value || !PyErr_GivenExceptionMatches (type, (PyObject *)PyGError)) {
        PyErr_Restore (type, value, traceback);
        PyErr_Print ();
        return -2;
    }
    Py_DECREF (type);
    Py_XDECREF (traceback);

    if (!pygi_error_marshal_from_py (value, error)) {
        PyErr_Print ();
        res = -2;
    }

    Py_DECREF (value);
    return res;
}

static gboolean
_pygi_marshal_from_py_gerror (PyGIInvokeState *state,
                              PyGICallableCache *callable_cache,
                              PyGIArgCache *arg_cache, PyObject *py_arg,
                              GIArgument *arg, gpointer *cleanup_data)
{
    GError *error = NULL;
    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        *cleanup_data = NULL;
        return TRUE;
    } else if (pygi_error_marshal_from_py (py_arg, &error)) {
        arg->v_pointer = error;
        *cleanup_data = error;
        return TRUE;
    } else {
        return FALSE;
    }
}


static void
_pygi_marshal_from_py_gerror_cleanup (PyGIInvokeState *state,
                                      PyGIArgCache *arg_cache,
                                      PyObject *py_arg, gpointer data,
                                      gboolean was_processed)
{
    if (was_processed) {
        g_error_free ((GError *)data);
    }
}

static PyObject *
_pygi_marshal_to_py_gerror (PyGIInvokeState *state,
                            PyGICallableCache *callable_cache,
                            PyGIArgCache *arg_cache, GIArgument *arg,
                            gpointer *cleanup_data)
{
    GError *error = arg->v_pointer;
    PyObject *py_obj = NULL;

    py_obj = pygi_error_marshal_to_py (&error);

    if (arg_cache->transfer == GI_TRANSFER_EVERYTHING && error != NULL) {
        g_error_free (error);
    }

    return py_obj;
}

PyGIArgCache *
pygi_arg_gerror_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                               GITransfer transfer, PyGIDirection direction)
{
    PyGIArgCache *arg_cache;

    arg_cache = pygi_arg_cache_alloc ();

    pygi_arg_base_setup (arg_cache, type_info, arg_info, transfer, direction);

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_gerror;

        /* Assign cleanup function if we manage memory after call completion. */
        if (arg_cache->transfer == GI_TRANSFER_NOTHING) {
            arg_cache->from_py_cleanup = _pygi_marshal_from_py_gerror_cleanup;
        }
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_gerror;
        arg_cache->meta_type = PYGI_META_ARG_TYPE_PARENT;
    }

    return arg_cache;
}

static PyObject *
pygerror_from_gvalue (const GValue *value)
{
    GError *gerror = (GError *)g_value_get_boxed (value);
    PyObject *pyerr = pygi_error_marshal_to_py (&gerror);
    return pyerr;
}

static int
pygerror_to_gvalue (GValue *value, PyObject *pyerror)
{
    GError *gerror = NULL;

    if (pygi_error_marshal_from_py (pyerror, &gerror)) {
        g_value_take_boxed (value, gerror);
        return 0;
    }

    return -1;
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_error_register_types (PyObject *module)
{
    PyObject *error_module = PyImport_ImportModule ("gi._error");
    if (!error_module) {
        return -1;
    }

    /* Stash a reference to the Python implemented gi._error.GError. */
    PyGError = PyObject_GetAttrString (error_module, "GError");
    Py_DECREF (error_module);
    if (PyGError == NULL) return -1;

    pyg_register_gtype_custom (G_TYPE_ERROR, pygerror_from_gvalue,
                               pygerror_to_gvalue);

    return 0;
}

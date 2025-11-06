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

#include "pygi-object.h"
#include "pygobject-object.h"
#include "pygi-cache-private.h"

/*
 * GObject from Python
 */

typedef gboolean (*PyGIObjectMarshalFromPyFunc) (PyObject *py_arg,
                                                 GIArgument *arg,
                                                 GITransfer transfer);

static gboolean
_pygi_marshal_from_py_interface_object (PyGIInvokeState *state,
                                        PyGICallableCache *callable_cache,
                                        PyGIArgCache *arg_cache,
                                        PyObject *py_arg, GIArgument *arg,
                                        gpointer *cleanup_data,
                                        PyGIObjectMarshalFromPyFunc func)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (PyObject_IsInstance (py_arg, iface_cache->py_type)
        || (pygobject_check (py_arg, &PyGObject_Type)
            && g_type_is_a (G_OBJECT_TYPE (pygobject_get (py_arg)),
                            iface_cache->g_type))) {
        gboolean res;
        res = func (py_arg, arg, arg_cache->transfer);
        *cleanup_data = arg->v_pointer;
        return res;

    } else {
        PyObject *module = PyObject_GetAttrString (py_arg, "__module__");
        const char *arg_name = pygi_arg_cache_get_name (arg_cache);
        PyErr_Format (PyExc_TypeError,
                      "argument %s: Expected %s, but got %s%s%s",
                      arg_name ? arg_name : "self",
                      ((PyGIInterfaceCache *)arg_cache)->type_name,
                      module ? PyUnicode_AsUTF8 (module) : "",
                      module ? "." : "", Py_TYPE (py_arg)->tp_name);
        if (module) Py_DECREF (module);
        return FALSE;
    }
}

static gboolean
_pygi_marshal_from_py_called_from_c_interface_object (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, PyObject *py_arg, GIArgument *arg,
    gpointer *cleanup_data)
{
    return _pygi_marshal_from_py_interface_object (
        state, callable_cache, arg_cache, py_arg, arg, cleanup_data,
        pygi_arg_gobject_out_arg_from_py);
}

static gboolean
_pygi_marshal_from_py_called_from_py_interface_object (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, PyObject *py_arg, GIArgument *arg,
    gpointer *cleanup_data)
{
    return _pygi_marshal_from_py_interface_object (
        state, callable_cache, arg_cache, py_arg, arg, cleanup_data,
        pygi_marshal_from_py_object);
}

static PyObject *
_pygi_marshal_to_py_called_from_c_interface_object_cache_adapter (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, GIArgument *arg, gpointer *cleanup_data)
{
    return pygi_arg_object_to_py_called_from_c (arg, arg_cache->transfer);
}

static PyObject *
_pygi_marshal_to_py_called_from_py_interface_object_cache_adapter (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, GIArgument *arg, gpointer *cleanup_data)
{
    return pygi_arg_object_to_py (arg, arg_cache->transfer);
}

static void
_pygi_marshal_cleanup_to_py_interface_object (PyGIInvokeState *state,
                                              PyGIArgCache *arg_cache,
                                              gpointer cleanup_data,
                                              gpointer data,
                                              gboolean was_processed)
{
    if (was_processed && state->failed && data != NULL
        && arg_cache->transfer == GI_TRANSFER_EVERYTHING) {
        if (G_IS_OBJECT (data)) {
            g_object_unref (G_OBJECT (data));
        } else {
            PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;
            GIObjectInfoUnrefFunction unref_func;

            unref_func = gi_object_info_get_unref_function_pointer (
                (GIObjectInfo *)iface_cache->interface_info);
            if (unref_func) unref_func (data);
        }
    }
}

static void
_pygi_marshal_cleanup_from_py_interface_object (PyGIInvokeState *state,
                                                PyGIArgCache *arg_cache,
                                                PyObject *py_arg,
                                                gpointer data,
                                                gboolean was_processed)
{
    /* If we processed the parameter but fail before invoking the method,
       we need to remove the ref we added */
    if (was_processed && state->failed && data != NULL
        && arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        g_object_unref (G_OBJECT (data));
}

PyGIArgCache *
pygi_arg_gobject_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                                GITransfer transfer, PyGIDirection direction,
                                GIRegisteredTypeInfo *iface_info,
                                PyGICallableCache *callable_cache)
{
    PyGIArgCache *cache = NULL;

    cache = pygi_arg_interface_new_from_info (type_info, arg_info, transfer,
                                              direction, iface_info);
    if (cache == NULL) return NULL;

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        if (callable_cache->calling_context
            == PYGI_CALLING_CONTEXT_IS_FROM_C) {
            cache->from_py_marshaller =
                _pygi_marshal_from_py_called_from_c_interface_object;
        } else {
            cache->from_py_marshaller =
                _pygi_marshal_from_py_called_from_py_interface_object;
        }

        cache->from_py_cleanup =
            _pygi_marshal_cleanup_from_py_interface_object;
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        if (callable_cache->calling_context
            == PYGI_CALLING_CONTEXT_IS_FROM_C) {
            cache->to_py_marshaller =
                _pygi_marshal_to_py_called_from_c_interface_object_cache_adapter;
        } else {
            cache->to_py_marshaller =
                _pygi_marshal_to_py_called_from_py_interface_object_cache_adapter;
        }

        cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_interface_object;
    }

    return cache;
}

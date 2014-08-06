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

#include <glib.h>
#include <Python.h>
#include <pyglib-python-compat.h>

#include "pygi-object.h"
#include "pygi-private.h"
#include "pygparamspec.h"

/*
 * GObject from Python
 */

typedef gboolean (*PyGIObjectMarshalFromPyFunc) (PyObject *py_arg,
                                                 GIArgument *arg,
                                                 GITransfer transfer);

/* _pygi_marshal_from_py_gobject:
 * py_arg: (in):
 * arg: (out):
 */
static gboolean
_pygi_marshal_from_py_gobject (PyObject *py_arg, /*in*/
                               GIArgument *arg,  /*out*/
                               GITransfer transfer) {
    GObject *gobj;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (!pygobject_check (py_arg, &PyGObject_Type)) {
        PyObject *repr = PyObject_Repr (py_arg);
        PyErr_Format(PyExc_TypeError, "expected GObject but got %s",
                     PYGLIB_PyUnicode_AsString (repr));
        Py_DECREF (repr);
        return FALSE;
    }

    gobj = pygobject_get (py_arg);
    if (transfer == GI_TRANSFER_EVERYTHING) {
        /* For transfer everything, add a new ref that the callee will take ownership of.
         * Pythons existing ref to the GObject will be managed with the PyGObject wrapper.
         */
        g_object_ref (gobj);
    }

    arg->v_pointer = gobj;
    return TRUE;
}

/* pygi_arg_gobject_out_arg_from_py:
 * py_arg: (in):
 * arg: (out):
 *
 * A specialization for marshaling Python GObjects used for out/return values
 * from a Python implemented vfuncs, signals, or an assignment to a GObject property.
 */
gboolean
pygi_arg_gobject_out_arg_from_py (PyObject *py_arg, /*in*/
                                  GIArgument *arg,  /*out*/
                                  GITransfer transfer) {
    GObject *gobj;
    if (!_pygi_marshal_from_py_gobject (py_arg, arg, transfer)) {
        return FALSE;
    }

    /* HACK: At this point the basic marshaling of the GObject was successful
     * but we add some special case hacks for vfunc returns due to buggy APIs:
     * https://bugzilla.gnome.org/show_bug.cgi?id=693393
     */
    gobj = arg->v_pointer;
    if (py_arg->ob_refcnt == 1 && gobj->ref_count == 1) {
        /* If both object ref counts are only 1 at this point (the reference held
         * in a return tuple), we assume the GObject will be free'd before reaching
         * its target and become invalid. So instead of getting invalid object errors
         * we add a new GObject ref.
         */
        g_object_ref (gobj);

        if (((PyGObject *)py_arg)->private_flags.flags & PYGOBJECT_GOBJECT_WAS_FLOATING) {
            /*
             * We want to re-float instances that were floating and the Python
             * wrapper assumed ownership. With the additional caveat that there
             * are not any strong references beyond the return tuple.
             */
            g_object_force_floating (gobj);

        } else {
            PyObject *repr = PyObject_Repr (py_arg);
            gchar *msg = g_strdup_printf ("Expecting to marshal a borrowed reference for %s, "
                                          "but nothing in Python is holding a reference to this object. "
                                          "See: https://bugzilla.gnome.org/show_bug.cgi?id=687522",
                                          PYGLIB_PyUnicode_AsString(repr));
            Py_DECREF (repr);
            if (PyErr_WarnEx (PyExc_RuntimeWarning, msg, 2)) {
                g_free (msg);
                return FALSE;
            }
            g_free (msg);
        }
    }

    return TRUE;
}

static gboolean
_pygi_marshal_from_py_interface_object (PyGIInvokeState             *state,
                                        PyGICallableCache           *callable_cache,
                                        PyGIArgCache                *arg_cache,
                                        PyObject                    *py_arg,
                                        GIArgument                  *arg,
                                        gpointer                    *cleanup_data,
                                        PyGIObjectMarshalFromPyFunc  func)
{
    PyGIInterfaceCache *iface_cache = (PyGIInterfaceCache *)arg_cache;

    if (py_arg == Py_None) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (PyObject_IsInstance (py_arg, iface_cache->py_type) ||
            (pygobject_check (py_arg, &PyGObject_Type) &&
             g_type_is_a (G_OBJECT_TYPE (pygobject_get (py_arg)), iface_cache->g_type))) {

        gboolean res;
        res = func (py_arg, arg, arg_cache->transfer);
        *cleanup_data = arg->v_pointer;
        return res;

    } else {
        PyObject *module = PyObject_GetAttrString(py_arg, "__module__");

        PyErr_Format (PyExc_TypeError, "argument %s: Expected %s, but got %s%s%s",
                      arg_cache->arg_name ? arg_cache->arg_name : "self",
                      ( (PyGIInterfaceCache *)arg_cache)->type_name,
                      module ? PYGLIB_PyUnicode_AsString(module) : "",
                      module ? "." : "",
                      py_arg->ob_type->tp_name);
        if (module)
            Py_DECREF (module);
        return FALSE;
    }
}

static gboolean
_pygi_marshal_from_py_called_from_c_interface_object (PyGIInvokeState   *state,
                                                      PyGICallableCache *callable_cache,
                                                      PyGIArgCache      *arg_cache,
                                                      PyObject          *py_arg,
                                                      GIArgument        *arg,
                                                      gpointer          *cleanup_data)
{
    return _pygi_marshal_from_py_interface_object (state,
                                                   callable_cache,
                                                   arg_cache,
                                                   py_arg,
                                                   arg,
                                                   cleanup_data,
                                                   pygi_arg_gobject_out_arg_from_py);
}

static gboolean
_pygi_marshal_from_py_called_from_py_interface_object (PyGIInvokeState   *state,
                                                       PyGICallableCache *callable_cache,
                                                       PyGIArgCache      *arg_cache,
                                                       PyObject          *py_arg,
                                                       GIArgument        *arg,
                                                       gpointer          *cleanup_data)
{
    return _pygi_marshal_from_py_interface_object (state,
                                                   callable_cache,
                                                   arg_cache,
                                                   py_arg,
                                                   arg,
                                                   cleanup_data,
                                                   _pygi_marshal_from_py_gobject);
}

static void
_pygi_marshal_cleanup_from_py_interface_object (PyGIInvokeState *state,
                                                PyGIArgCache    *arg_cache,
                                                PyObject        *py_arg,
                                                gpointer         data,
                                                gboolean         was_processed)
{
    /* If we processed the parameter but fail before invoking the method,
       we need to remove the ref we added */
    if (was_processed && state->failed && data != NULL &&
            arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        g_object_unref (G_OBJECT(data));
}


/*
 * GObject to Python
 */

PyObject *
pygi_arg_gobject_to_py (GIArgument *arg, GITransfer transfer) {
    PyObject *pyobj;

    if (arg->v_pointer == NULL) {
        pyobj = Py_None;
        Py_INCREF (pyobj);

    } else if (G_IS_PARAM_SPEC(arg->v_pointer)) {
        pyobj = pyg_param_spec_new (arg->v_pointer);
        if (transfer == GI_TRANSFER_EVERYTHING)
            g_param_spec_unref (arg->v_pointer);

    } else {
         pyobj = pygobject_new_full (arg->v_pointer,
                                     /*steal=*/ transfer == GI_TRANSFER_EVERYTHING,
                                     /*type=*/  NULL);
    }

    return pyobj;
}

PyObject *
pygi_arg_gobject_to_py_called_from_c (GIArgument *arg,
                                      GITransfer  transfer)
{
    PyObject *object;

    /* HACK:
     * The following hack is to work around GTK+ sending signals which
     * contain floating widgets in them. This assumes control of how
     * references are added by the PyGObject wrapper and avoids the sink
     * behavior by explicitly passing GI_TRANSFER_EVERYTHING as the transfer
     * mode and then re-forcing the object as floating afterwards.
     *
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=693400
     */
    if (arg->v_pointer != NULL &&
            transfer == GI_TRANSFER_NOTHING &&
            !G_IS_PARAM_SPEC (arg->v_pointer) &&
            g_object_is_floating (arg->v_pointer)) {

        g_object_ref (arg->v_pointer);
        object = pygi_arg_gobject_to_py (arg, GI_TRANSFER_EVERYTHING);
        g_object_force_floating (arg->v_pointer);
    } else {
        object = pygi_arg_gobject_to_py (arg, transfer);
    }

    return object;
}

static PyObject *
_pygi_marshal_to_py_called_from_c_interface_object_cache_adapter (PyGIInvokeState   *state,
                                                                  PyGICallableCache *callable_cache,
                                                                  PyGIArgCache      *arg_cache,
                                                                  GIArgument        *arg)
{
    return pygi_arg_gobject_to_py_called_from_c (arg, arg_cache->transfer);
}

static PyObject *
_pygi_marshal_to_py_called_from_py_interface_object_cache_adapter (PyGIInvokeState   *state,
                                                                   PyGICallableCache *callable_cache,
                                                                   PyGIArgCache      *arg_cache,
                                                                   GIArgument        *arg)
{
    return pygi_arg_gobject_to_py (arg, arg_cache->transfer);
}

static void
_pygi_marshal_cleanup_to_py_interface_object (PyGIInvokeState *state,
                                              PyGIArgCache    *arg_cache,
                                              PyObject        *dummy,
                                              gpointer         data,
                                              gboolean         was_processed)
{
    /* If we error out and the object is not marshalled into a PyGObject
       we must take care of removing the ref */
    if (!was_processed && arg_cache->transfer == GI_TRANSFER_EVERYTHING)
        g_object_unref (G_OBJECT(data));
}

static gboolean
pygi_arg_gobject_setup_from_info (PyGIArgCache      *arg_cache,
                                  GITypeInfo        *type_info,
                                  GIArgInfo         *arg_info,
                                  GITransfer         transfer,
                                  PyGIDirection      direction,
                                  PyGICallableCache *callable_cache)
{
    /* NOTE: usage of pygi_arg_interface_new_from_info already calls
     * pygi_arg_interface_setup so no need to do it here.
     */

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        if (callable_cache->calling_context == PYGI_CALLING_CONTEXT_IS_FROM_C) {
            arg_cache->from_py_marshaller = _pygi_marshal_from_py_called_from_c_interface_object;
        } else {
            arg_cache->from_py_marshaller = _pygi_marshal_from_py_called_from_py_interface_object;
        }

        arg_cache->from_py_cleanup = _pygi_marshal_cleanup_from_py_interface_object;
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        if (callable_cache->calling_context == PYGI_CALLING_CONTEXT_IS_FROM_C) {
            arg_cache->to_py_marshaller = _pygi_marshal_to_py_called_from_c_interface_object_cache_adapter;
        } else {
            arg_cache->to_py_marshaller = _pygi_marshal_to_py_called_from_py_interface_object_cache_adapter;
        }

        arg_cache->to_py_cleanup = _pygi_marshal_cleanup_to_py_interface_object;
    }

    return TRUE;
}

PyGIArgCache *
pygi_arg_gobject_new_from_info (GITypeInfo        *type_info,
                                GIArgInfo         *arg_info,
                                GITransfer         transfer,
                                PyGIDirection      direction,
                                GIInterfaceInfo   *iface_info,
                                PyGICallableCache *callable_cache)
{
    gboolean res = FALSE;
    PyGIArgCache *cache = NULL;

    cache = pygi_arg_interface_new_from_info (type_info,
                                              arg_info,
                                              transfer,
                                              direction,
                                              iface_info);
    if (cache == NULL)
        return NULL;

    res = pygi_arg_gobject_setup_from_info (cache,
                                            type_info,
                                            arg_info,
                                            transfer,
                                            direction,
                                            callable_cache);
    if (res) {
        return cache;
    } else {
        pygi_arg_cache_free (cache);
        return NULL;
    }
}

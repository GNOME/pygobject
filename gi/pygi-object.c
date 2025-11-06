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

#include "pygi-fundamental.h"
#include "pygi-object.h"
#include "pygobject-object.h"

/* pygi_marshal_from_py_object:
 * py_arg: (in):
 * arg: (out):
 */
gboolean
pygi_marshal_from_py_object (PyObject *py_arg, /*in*/
                             GIArgument *arg,  /*out*/
                             GITransfer transfer)
{
    GObject *gobj;

    if (Py_IsNone (py_arg)) {
        arg->v_pointer = NULL;
        return TRUE;
    }

    if (PyObject_TypeCheck (py_arg, &PyGIFundamental_Type)) {
        arg->v_pointer = pygi_fundamental_get (py_arg);
        if (transfer == GI_TRANSFER_EVERYTHING) {
            pygi_fundamental_ref ((PyGIFundamental *)py_arg);
        }
        return TRUE;
    }

    if (!pygobject_check (py_arg, &PyGObject_Type)) {
        PyObject *repr = PyObject_Repr (py_arg);
        PyErr_Format (PyExc_TypeError, "expected GObject but got %s",
                      PyUnicode_AsUTF8 (repr));
        Py_DECREF (repr);
        return FALSE;
    }

    gobj = pygobject_get (py_arg);
    if (gobj == NULL) {
        PyErr_Format (PyExc_RuntimeError,
                      "object at %p of type %s is not initialized", py_arg,
                      Py_TYPE (py_arg)->tp_name);
        return FALSE;
    }

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
pygi_arg_gobject_out_arg_from_py (PyObject *py_arg, /* in */
                                  GIArgument *arg,  /* out */
                                  GITransfer transfer)
{
    GObject *gobj;
    if (!pygi_marshal_from_py_object (py_arg, arg, transfer)) {
        return FALSE;
    }

    /* HACK: At this point the basic marshaling of the GObject was successful
     * but we add some special case hacks for vfunc returns due to buggy APIs:
     * https://bugzilla.gnome.org/show_bug.cgi?id=693393
     */
    gobj = arg->v_pointer;
    // In Python 3.13 the py_arg refcount is 3, in 3.14 only 1
    if (Py_REFCNT (py_arg) == 1 && gobj->ref_count == 1) {
        /* If both object ref counts are only 1 at this point (the reference held
         * in a return tuple), we assume the GObject will be free'd before reaching
         * its target and become invalid. So instead of getting invalid object errors
         * we add a new GObject ref.
         *
         * Note that this check does not hold up for Python 3.14
         */
        PyObject *repr = PyObject_Repr (py_arg);
        gchar *msg = g_strdup_printf (
            "Adding extra reference for %s, "
            "for nothing is holding a reference to this object. "
            "This could potentially lead to a memory leak. "
            "See: https://bugzilla.gnome.org/show_bug.cgi?id=687522",
            PyUnicode_AsUTF8 (repr));
        Py_DECREF (repr);
        if (PyErr_WarnEx (PyExc_RuntimeWarning, msg, 2)) {
            g_free (msg);
            return FALSE;
        }
        g_free (msg);

        g_object_ref (gobj);
    }

    return TRUE;
}


/*
 * GObject to Python
 */

PyObject *
pygi_arg_object_to_py (GIArgument *arg, GITransfer transfer)
{
    PyObject *pyobj;

    if (arg->v_pointer == NULL) {
        pyobj = Py_None;
        Py_INCREF (pyobj);
    } else if (G_IS_OBJECT (arg->v_pointer)) {
        pyobj =
            pygobject_new_full (arg->v_pointer,
                                /*steal=*/transfer == GI_TRANSFER_EVERYTHING,
                                /*type=*/NULL);
    } else {
        pyobj = pygi_fundamental_new (arg->v_pointer);
        if (pyobj && transfer == GI_TRANSFER_EVERYTHING)
            pygi_fundamental_unref ((PyGIFundamental *)pyobj);
    }

    return pyobj;
}

PyObject *
pygi_arg_object_to_py_called_from_c (GIArgument *arg, GITransfer transfer)
{
    /* HACK:
     * The following hack is to work around GTK sending signals which
     * contain floating widgets in them. This assumes control of how
     * references are added by the PyGObject wrapper and avoids the sink
     * behavior by explicitly passing GI_TRANSFER_EVERYTHING as the transfer
     * mode and then re-forcing the object as floating afterwards.
     *
     * See: https://bugzilla.gnome.org/show_bug.cgi?id=693400
     *
     * (a GtkCellRendererText issue that has since been fixed)
     * In modern bindings this should no longer be needed. We sink the object
     * as soon as it's created and that's that.
     */
    if (arg->v_pointer != NULL && transfer == GI_TRANSFER_NOTHING
        && G_IS_OBJECT (arg->v_pointer)
        && g_object_is_floating (arg->v_pointer)) {
        g_object_ref_sink (arg->v_pointer);
    }

    return pygi_arg_object_to_py (arg, transfer);
}

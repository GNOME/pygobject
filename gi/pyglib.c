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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Python.h>
#include <pythread.h>
#include "pyglib.h"
#include "pyglib-private.h"
#include "pygoptioncontext.h"
#include "pygoptiongroup.h"


/**
 * pyg_option_group_transfer_group:
 * @group: a GOptionGroup wrapper
 *
 * This is used to transfer the GOptionGroup to a GOptionContext. After this
 * is called, the calle must handle the release of the GOptionGroup.
 *
 * When #NULL is returned, the GOptionGroup was already transfered.
 *
 * Returns: Either #NULL or the wrapped GOptionGroup.
 */
GOptionGroup *
pyglib_option_group_transfer_group(PyObject *obj)
{
    PyGOptionGroup *self = (PyGOptionGroup*)obj;
    
    if (self->is_in_context)
	return NULL;

    self->is_in_context = TRUE;
    
    /* Here we increase the reference count of the PyGOptionGroup, because now
     * the GOptionContext holds an reference to us (it is the userdata passed
     * to g_option_group_new().
     *
     * The GOptionGroup is freed with the GOptionContext.
     *
     * We set it here because if we would do this in the init method we would
     * hold two references and the PyGOptionGroup would never be freed.
     */
    Py_INCREF(self);
    
    return self->group;
}


/****** Private *****/

/**
 * _pyglib_destroy_notify:
 * @user_data: a PyObject pointer.
 *
 * A function that can be used as a GDestroyNotify callback that will
 * call Py_DECREF on the data.
 */
void
_pyglib_destroy_notify(gpointer user_data)
{
    PyObject *obj = (PyObject *)user_data;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();
    Py_DECREF(obj);
    pyglib_gil_state_release(state);
}

gboolean
_pyglib_handler_marshal(gpointer user_data)
{
    PyObject *tuple, *ret;
    gboolean res;
    PyGILState_STATE state;

    g_return_val_if_fail(user_data != NULL, FALSE);

    state = pyglib_gil_state_ensure();

    tuple = (PyObject *)user_data;
    ret = PyObject_CallObject(PyTuple_GetItem(tuple, 0),
			      PyTuple_GetItem(tuple, 1));
    if (!ret) {
	PyErr_Print();
	res = FALSE;
    } else {
	res = PyObject_IsTrue(ret);
	Py_DECREF(ret);
    }
    
    pyglib_gil_state_release(state);

    return res;
}

PyObject*
_pyglib_generic_ptr_richcompare(void* a, void *b, int op)
{
    PyObject *res;

    switch (op) {

      case Py_EQ:
        res = (a == b) ? Py_True : Py_False;
        break;

      case Py_NE:
        res = (a != b) ? Py_True : Py_False;
        break;

      case Py_LT:
        res = (a < b) ? Py_True : Py_False;
        break;

      case Py_LE:
        res = (a <= b) ? Py_True : Py_False;
        break;

      case Py_GT:
        res = (a > b) ? Py_True : Py_False;
        break;

      case Py_GE:
        res = (a >= b) ? Py_True : Py_False;
        break;

      default:
        res = Py_NotImplemented;
        break;
    }

    Py_INCREF(res);
    return res;
}

PyObject*
_pyglib_generic_long_richcompare(long a, long b, int op)
{
    PyObject *res;

    switch (op) {

      case Py_EQ:
        res = (a == b) ? Py_True : Py_False;
        Py_INCREF(res);
        break;

      case Py_NE:
        res = (a != b) ? Py_True : Py_False;
        Py_INCREF(res);
        break;


      case Py_LT:
        res = (a < b) ? Py_True : Py_False;
        Py_INCREF(res);
        break;

      case Py_LE:
        res = (a <= b) ? Py_True : Py_False;
        Py_INCREF(res);
        break;

      case Py_GT:
        res = (a > b) ? Py_True : Py_False;
        Py_INCREF(res);
        break;

      case Py_GE:
        res = (a >= b) ? Py_True : Py_False;
        Py_INCREF(res);
        break;

      default:
        res = Py_NotImplemented;
        Py_INCREF(res);
        break;
    }

    return res;
}


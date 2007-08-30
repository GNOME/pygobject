/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   pygmaincontext.c: GMainContext wrapper
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

#include "pygobject-private.h"

static int
pyg_main_context_init(PyGMainContext *self)
{
    self->context = g_main_context_new();
    return 0;
}

static void
pyg_main_context_dealloc(PyGMainContext *self)
{
    if (self->context != NULL) {
	g_main_context_unref(self->context);
	self->context = NULL;
    }

    PyObject_Del(self);
}

static int
pyg_main_context_compare(PyGMainContext *self, PyGMainContext *v)
{
    if (self->context == v->context) return 0;
    if (self->context > v->context) return -1;
    return 1;
}

static PyObject *
_wrap_g_main_context_iteration (PyGMainContext *self, PyObject *args)
{
    gboolean ret, may_block = TRUE;
    
    if (!PyArg_ParseTuple(args, "|i:GMainContext.iteration",
			  &may_block))
	return NULL;

    pyg_begin_allow_threads;
    ret = g_main_context_iteration(self->context, may_block);
    pyg_end_allow_threads;
    
    return PyBool_FromLong(ret);
}

static PyObject *
_wrap_g_main_context_pending (PyGMainContext *self)
{
    return PyBool_FromLong(g_main_context_pending(self->context));
}

static PyMethodDef _PyGMainContext_methods[] = {
    { "iteration", (PyCFunction)_wrap_g_main_context_iteration, METH_VARARGS },
    { "pending", (PyCFunction)_wrap_g_main_context_pending, METH_NOARGS },
    { NULL, NULL, 0 }
};

PyTypeObject PyGMainContext_Type = {
    PyVarObject_HEAD_INIT(0, 0)
    "gobject.MainContext",
    sizeof(PyGMainContext),
    0,
    /* methods */
    (destructor)pyg_main_context_dealloc,
    (printfunc)0,
    (getattrfunc)0,
    (setattrfunc)0,
    (cmpfunc)pyg_main_context_compare,
    (reprfunc)0,
    0,
    0,
    0,
    (hashfunc)0,
    (ternaryfunc)0,
    (reprfunc)0,
    (getattrofunc)0,
    (setattrofunc)0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    NULL,
    (traverseproc)0,
    (inquiry)0,
    (richcmpfunc)0,
    0,
    (getiterfunc)0,
    (iternextfunc)0,
    _PyGMainContext_methods,
    0,
    0,
    NULL,
    NULL,
    (descrgetfunc)0,
    (descrsetfunc)0,
    0,
    (initproc)pyg_main_context_init,
};

/**
 * pyg_main_context_new:
 * @context: a GMainContext.
 *
 * Creates a wrapper for a GMainContext.
 *
 * Returns: the GMainContext wrapper.
 */
PyObject *
pyg_main_context_new(GMainContext *context)
{
    PyGMainContext *self;

    self = (PyGMainContext *)PyObject_NEW(PyGMainContext,
					  &PyGMainContext_Type);
    if (self == NULL)
	return NULL;

    self->context = g_main_context_ref(context);
    return (PyObject *)self;
}

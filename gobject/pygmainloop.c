/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 * Copyright (C) 2004       Johan Dahlin
 *
 *   pygmainloop.c: GMainLoop wrapper
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
pyg_main_loop_new(PyGMainLoop *self, PyObject *args, PyObject *kwargs)
{

    static char *kwlist[] = { "context", "is_running", NULL };
    PyObject *py_context = Py_None;
    int is_running;
    GMainContext *context;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "|Ob:GMainLoop.__init__",
				     kwlist, &py_context, &is_running))
        return -1;

    if (!PyObject_TypeCheck(py_context, &PyGMainContext_Type) &&
	py_context != Py_None) {
	PyErr_SetString(PyExc_TypeError,
			"context must be a gobject.GMainContext or None");
	return -1;
    }

    if (py_context != Py_None) {
	context = ((PyGMainContext*)py_context)->context;
    } else {
	context = NULL;
    }

    self->loop = g_main_loop_new(context, is_running);
    return 0;
}

static int
pyg_main_loop_compare(PyGMainLoop *self, PyGMainLoop *v)
{
    if (self->loop == v->loop) return 0;
    if (self->loop > v->loop) return -1;
    return 1;
}

static PyObject *
_wrap_g_main_loop_get_context (PyGMainLoop *loop)
{
    PyGMainContext *self;

    self = (PyGMainContext *)PyObject_NEW(PyGMainContext,
					  &PyGMainContext_Type);
    
    self->context = g_main_loop_get_context(loop->loop);
    
    if (self->context == NULL)
	return NULL;

    return (PyObject *)self;
}

static PyObject *
_wrap_g_main_loop_is_running (PyGMainLoop *self)
{
    return PyBool_FromLong(g_main_loop_is_running(self->loop));
}

static PyObject *
_wrap_g_main_loop_quit (PyGMainLoop *self)
{
    g_main_loop_quit(self->loop);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_g_main_loop_run (PyGMainLoop *self)
{
    Py_BEGIN_ALLOW_THREADS;
    g_main_loop_run(self->loop);
    Py_END_ALLOW_THREADS;
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef _PyGMainLoop_methods[] = {
    { "get_context", (PyCFunction)_wrap_g_main_loop_get_context, METH_NOARGS },
    { "is_running", (PyCFunction)_wrap_g_main_loop_is_running, METH_NOARGS },
    { "quit", (PyCFunction)_wrap_g_main_loop_quit, METH_NOARGS },
    { "run", (PyCFunction)_wrap_g_main_loop_run, METH_NOARGS },
    { NULL, NULL, 0 }
};

PyTypeObject PyGMainLoop_Type = {
    PyObject_HEAD_INIT(NULL)
    0,			
    "gobject.MainLoop",	
    sizeof(PyGMainLoop),	
    0,			
    /* methods */
    (destructor)0,	
    (printfunc)0,	
    (getattrfunc)0,	
    (setattrfunc)0,	
    (cmpfunc)pyg_main_loop_compare,
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
    _PyGMainLoop_methods,
    0,				
    0,		       	
    NULL,		
    NULL,		
    (descrgetfunc)0,	
    (descrsetfunc)0,	
    0,                 
    (initproc)pyg_main_loop_new,
};

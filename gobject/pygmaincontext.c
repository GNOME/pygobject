/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   pygmainloop.c: GMainContext wrapper
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
pyg_main_context_new(PyGMainContext *self)
{
    self->context = g_main_context_new();
    return 0;
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
    
    if (!PyArg_ParseTuple(args, "|b:GMainContext.iteration",
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
    PyObject_HEAD_INIT(NULL)
    0,			
    "gobject.MainContext",	
    sizeof(PyGMainContext),	
    0,			
    /* methods */
    (destructor)0,	
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
    (initproc)pyg_main_context_new,
};

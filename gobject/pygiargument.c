/* -*- Mode: C; c-basic-offset: 4 -*- */
/* 
 * Copyright (C) 2005  Johan Dahlin <johan@gnome.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "Python.h"
#include "pygobject-private.h"

int
pyg_argument_from_pyobject(PyObject *object, GIArgInfo *info, GArgument *arg)
{
    GITypeInfo *type_info;
    GITypeTag type_tag;
    int rv = 0;
	
    type_info = g_arg_info_get_type(info);
    type_tag = g_type_info_get_tag((GITypeInfo*)type_info);
    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
	/* Nothing to do */
	break;
    case GI_TYPE_TAG_UTF8:
	arg->v_pointer = PyString_AsString(object);
	break;
    case GI_TYPE_TAG_INT:
	arg->v_int = PyInt_AsLong(object);
	break;
    case GI_TYPE_TAG_UINT32:
	arg->v_uint32 = PyLong_AsUnsignedLong(object);
	break;

    case GI_TYPE_TAG_INTERFACE:
        arg->v_pointer = pygobject_get(object);
	break;
    default:
        PyErr_Format(PyExc_TypeError, "<PyO->GArg> GITypeTag %d is unhandled\n", type_tag);
        rv = -1;
        break;
    }
    g_base_info_unref((GIBaseInfo*)type_info);
    
    return rv;
}

PyObject *
pyg_argument_to_pyobject(GArgument *arg, GITypeInfo *type_info)
{
    GITypeTag type_tag;
    PyObject *obj;
    
    g_return_val_if_fail(type_info != NULL, NULL);
    type_tag = g_type_info_get_tag((GITypeInfo*)type_info);

    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
    case GI_TYPE_TAG_GLIST:
        Py_INCREF(Py_None);
	obj = Py_None;
	break;
    case GI_TYPE_TAG_INT:
	obj = PyInt_FromLong(arg->v_int);
	break;
    case GI_TYPE_TAG_DOUBLE:
	obj = PyFloat_FromDouble(arg->v_double);
	break;
    case GI_TYPE_TAG_UTF8:
	obj = PyString_FromString(arg->v_pointer);
	break;
    case GI_TYPE_TAG_INTERFACE:
        obj = pygobject_new(arg->v_pointer);
	break;
    default:
        PyErr_Format(PyExc_TypeError, "<GArg->PyO> GITypeTag %d is unhandled\n", type_tag);
        return NULL;
    }
    return obj;
}


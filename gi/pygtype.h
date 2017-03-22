/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *               2004-2008  Johan Dahlin
 *   pyginterface.c: wrapper for the gobject library.
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

#ifndef __PYGOBJECT_TYPE_H__ 
#define __PYGOBJECT_TYPE_H__

#include <Python.h>
#include <glib-object.h>
#include "pygobject-internal.h"

#define PYGOBJECT_REGISTER_GTYPE(d, type, name, gtype)      \
  {                                                         \
    PyObject *o;					    \
    PYGLIB_REGISTER_TYPE(d, type, name);                    \
    PyDict_SetItemString(type.tp_dict, "__gtype__",         \
			 o=pyg_type_wrapper_new(gtype));    \
    Py_DECREF(o);                                           \
}

extern PyTypeObject PyGTypeWrapper_Type;

typedef PyObject *(* fromvaluefunc)(const GValue *value);
typedef int (*tovaluefunc)(GValue *value, PyObject *obj);

typedef struct {
    fromvaluefunc fromvalue;
    tovaluefunc tovalue;
} PyGTypeMarshal;

PyGTypeMarshal *pyg_type_lookup(GType type);

gboolean pyg_gtype_is_custom (GType gtype);

void pyg_register_gtype_custom(GType gtype,
                               fromvaluefunc from_func,
                               tovaluefunc to_func);

void pygobject_type_register_types(PyObject *d);

PyObject *pyg_object_descr_doc_get(void);
PyObject *pyg_type_wrapper_new (GType type);
GType     pyg_type_from_object_strict (PyObject *obj, gboolean strict);
GType     pyg_type_from_object (PyObject *obj);

int pyg_pyobj_to_unichar_conv (PyObject* py_obj, void* ptr);

GClosure *pyg_closure_new(PyObject *callback, PyObject *extra_args, PyObject *swap_data);
GClosure *pyg_signal_class_closure_get(void);
void      pyg_closure_set_exception_handler(GClosure *closure,
                                            PyClosureExceptionHandler handler);
#endif /* __PYGOBJECT_TYPE_H__ */

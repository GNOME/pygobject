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

#ifndef __PYGI_TYPE_H__
#define __PYGI_TYPE_H__

#include <girepository/girepository.h>
#include <Python.h>

#include "pygobject-types.h"

extern PyTypeObject PyGTypeWrapper_Type;

typedef PyObject *(*fromvaluefunc) (const GValue *value);
typedef int (*tovaluefunc) (GValue *value, PyObject *obj);

typedef struct {
    fromvaluefunc fromvalue;
    tovaluefunc tovalue;
} PyGTypeMarshal;

typedef enum {
    PYGI_INTERFACE_TYPE_TAG_FLAGS,
    PYGI_INTERFACE_TYPE_TAG_ENUM,
    PYGI_INTERFACE_TYPE_TAG_INTERFACE,
    PYGI_INTERFACE_TYPE_TAG_OBJECT,
    PYGI_INTERFACE_TYPE_TAG_STRUCT,
    PYGI_INTERFACE_TYPE_TAG_UNION,
    PYGI_INTERFACE_TYPE_TAG_CALLBACK
} PyGIInterfaceTypeTag;

PyGTypeMarshal *pyg_type_lookup (GType type);

gboolean pyg_gtype_is_custom (GType gtype);

void pyg_register_gtype_custom (GType gtype, fromvaluefunc from_func,
                                tovaluefunc to_func);

int pygi_type_register_types (PyObject *d);

PyObject *pyg_object_descr_doc_get (void);
PyObject *pyg_type_wrapper_new (GType type);
GType pyg_type_from_object_strict (PyObject *obj, gboolean strict);
GType pyg_type_from_object (PyObject *obj);

GClosure *pyg_closure_new (PyObject *callback, PyObject *extra_args,
                           PyObject *swap_data);
GClosure *pyg_signal_class_closure_get (void);

PyObject *pygi_type_import_by_g_type (GType g_type);
PyObject *pygi_type_import_by_name (const char *namespace_, const char *name);
PyObject *pygi_type_import_by_gi_info (GIBaseInfo *info);
PyObject *pygi_type_get_from_g_type (GType g_type);

PyGIInterfaceTypeTag pygi_interface_type_tag (GIBaseInfo *info);

#endif /* __PYGI_TYPE_H__ */

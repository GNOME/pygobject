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

#include <Python.h>
#include <girepository.h>

/* This was added in Python 2.4 */
#ifndef Py_CLEAR
#define Py_CLEAR(op)                            \
        do {                                    \
                if (op) {                       \
                        PyObject *tmp = (PyObject *)(op);       \
                        (op) = NULL;            \
                        Py_DECREF(tmp);         \
                }                               \
        } while (0)
#endif /* Py_CLEAR */

typedef struct {
    PyObject_HEAD
    GIRepository *repo;
} PyGIRepository;

extern PyTypeObject PyGIRepository_Type;

PyObject * pyg_info_new(gpointer info);

typedef struct {
    PyObject_HEAD
    GIBaseInfo *info;
    PyObject *instance_dict;
    PyObject *weakreflist;
} PyGIBaseInfo;

extern PyTypeObject PyGIBaseInfo_Type;
extern PyTypeObject PyGICallableInfo_Type;
extern PyTypeObject PyGIFunctionInfo_Type;
extern PyTypeObject PyGICallbackInfo_Type;
extern PyTypeObject PyGIRegisteredTypeInfo_Type;
extern PyTypeObject PyGIStructInfo_Type;
extern PyTypeObject PyGIUnionInfo_Type;
extern PyTypeObject PyGIEnumInfo_Type;
extern PyTypeObject PyGIObjectInfo_Type;
extern PyTypeObject PyGIBoxedInfo_Type;
extern PyTypeObject PyGIInterfaceInfo_Type;
extern PyTypeObject PyGIConstantInfo_Type;
extern PyTypeObject PyGIValueInfo_Type;
extern PyTypeObject PyGISignalInfo_Type;
extern PyTypeObject PyGIVFuncInfo_Type;
extern PyTypeObject PyGIPropertyInfo_Type;
extern PyTypeObject PyGIFieldInfo_Type;
extern PyTypeObject PyGIArgInfo_Type;
extern PyTypeObject PyGITypeInfo_Type;
#if 0
extern PyTypeObject PyGIErrorDomainInfo_Type;
#endif
extern PyTypeObject PyGIUnresolvedInfo_Type;

GArgument pyg_argument_from_pyobject(PyObject *object,
				     GITypeInfo *info);
PyObject*  pyg_argument_to_pyobject(GArgument *arg,
				    GITypeInfo *info);
PyObject*  pyarray_to_pyobject(gpointer array, int length, GITypeInfo *info);

#define PyG_ARGUMENT_FROM_PYOBJECT_ERROR pyg_argument_from_pyobject_error_quark()
GQuark pyg_argument_from_pyobject_error_quark(void);

typedef enum {
    PyG_ARGUMENT_FROM_PYOBJECT_ERROR_TYPE,
    PyG_ARGUMENT_FROM_PYOBJECT_ERROR_VALUE
} PyGArgumentFromPyObjectError;

gboolean pyg_argument_from_pyobject_check(PyObject *object, GITypeInfo *type_info, GError **error);

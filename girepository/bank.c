/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 sw=4 noet ai cindent :
 *
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

#include "bank.h"
#include <pygobject.h>

#define REGISTER_TYPE(d, type, name) \
    type.ob_type = &PyType_Type; \
    type.tp_alloc = PyType_GenericAlloc; \
    type.tp_new = PyType_GenericNew; \
    if (PyType_Ready(&type)) \
        return; \
    PyDict_SetItemString(d, name, (PyObject *)&type); \
    Py_INCREF(&type);

#define REGISTER_SUBTYPE(d, type, name, base) \
    type.tp_base = &base;                     \
    REGISTER_TYPE(d, type, name)

static PyObject *
_wrap_set_object_has_new_constructor(PyGIBaseInfo *self, PyObject *args)
{
    PyObject *pygtype;

    if (!PyArg_ParseTuple(args, "O:setObjectHasNewConstructor", &pygtype))
        return NULL;

    pyg_set_object_has_new_constructor(pyg_type_from_object(pygtype));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pybank_functions[] = {
    { "setObjectHasNewConstructor", (PyCFunction)_wrap_set_object_has_new_constructor, METH_VARARGS },
    { NULL, NULL, 0 }
};

static void
register_types(PyObject *d)
{
    REGISTER_TYPE(d, PyGIRepository_Type, "Repository");
    REGISTER_TYPE(d, PyGIBaseInfo_Type, "BaseInfo");
    REGISTER_SUBTYPE(d, PyGIUnresolvedInfo_Type,
                     "UnresolvedInfo", PyGIBaseInfo_Type);
    REGISTER_SUBTYPE(d, PyGICallableInfo_Type,
                     "CallableInfo", PyGIBaseInfo_Type);
    REGISTER_SUBTYPE(d, PyGIFunctionInfo_Type,
                     "FunctionInfo", PyGICallableInfo_Type);
    REGISTER_SUBTYPE(d, PyGICallbackInfo_Type,
                     "CallbackInfo", PyGICallableInfo_Type);
    REGISTER_SUBTYPE(d, PyGIRegisteredTypeInfo_Type,
                     "RegisteredTypeInfo", PyGIBaseInfo_Type);
    REGISTER_SUBTYPE(d, PyGIStructInfo_Type,
                     "StructInfo", PyGIRegisteredTypeInfo_Type);
    REGISTER_SUBTYPE(d, PyGIEnumInfo_Type,
                     "EnumInfo", PyGIRegisteredTypeInfo_Type);
    REGISTER_SUBTYPE(d, PyGIObjectInfo_Type,
                     "ObjectInfo", PyGIRegisteredTypeInfo_Type);
    REGISTER_SUBTYPE(d, PyGIBoxedInfo_Type,
                     "BoxedInfo", PyGIRegisteredTypeInfo_Type);
    REGISTER_SUBTYPE(d, PyGIInterfaceInfo_Type,
                     "InterfaceInfo", PyGIRegisteredTypeInfo_Type);
    REGISTER_SUBTYPE(d, PyGIConstantInfo_Type,
                     "ConstantInfo", PyGIBaseInfo_Type);
    REGISTER_SUBTYPE(d, PyGIValueInfo_Type,
                     "ValueInfo", PyGIBaseInfo_Type);
    REGISTER_SUBTYPE(d, PyGISignalInfo_Type,
                     "SignalInfo", PyGICallableInfo_Type);
    REGISTER_SUBTYPE(d, PyGIVFuncInfo_Type,
                     "VFuncInfo", PyGICallableInfo_Type);
    REGISTER_SUBTYPE(d, PyGIPropertyInfo_Type,
                     "PropertyInfo", PyGIBaseInfo_Type);
    REGISTER_SUBTYPE(d, PyGIFieldInfo_Type,
                     "FieldInfo", PyGIBaseInfo_Type);
    REGISTER_SUBTYPE(d, PyGIArgInfo_Type,
                     "ArgInfo", PyGIBaseInfo_Type);
    REGISTER_SUBTYPE(d, PyGITypeInfo_Type,
                     "TypeInfo", PyGIBaseInfo_Type);
    REGISTER_SUBTYPE(d, PyGIUnionInfo_Type,
                     "UnionInfo", PyGIRegisteredTypeInfo_Type);
}

static void
register_constants(PyObject *m)
{
    PyModule_AddIntConstant(m, "TYPE_TAG_VOID", GI_TYPE_TAG_VOID);
    PyModule_AddIntConstant(m, "TYPE_TAG_BOOLEAN", GI_TYPE_TAG_BOOLEAN);
    PyModule_AddIntConstant(m, "TYPE_TAG_INT8", GI_TYPE_TAG_INT8);
    PyModule_AddIntConstant(m, "TYPE_TAG_UINT8", GI_TYPE_TAG_UINT8);
    PyModule_AddIntConstant(m, "TYPE_TAG_INT16", GI_TYPE_TAG_INT16);
    PyModule_AddIntConstant(m, "TYPE_TAG_UINT16", GI_TYPE_TAG_UINT16);
    PyModule_AddIntConstant(m, "TYPE_TAG_INT32", GI_TYPE_TAG_INT32);
    PyModule_AddIntConstant(m, "TYPE_TAG_UINT32", GI_TYPE_TAG_UINT32);
    PyModule_AddIntConstant(m, "TYPE_TAG_INT64", GI_TYPE_TAG_INT64);
    PyModule_AddIntConstant(m, "TYPE_TAG_UINT64", GI_TYPE_TAG_UINT64);
    /* FIXME: Removed from metadata format, fix properly by introducing
       special-case struct */
/*     PyModule_AddIntConstant(m, "TYPE_TAG_GSTRING", GI_TYPE_TAG_GSTRING); */
    PyModule_AddIntConstant(m, "TYPE_TAG_INT", GI_TYPE_TAG_INT);
    PyModule_AddIntConstant(m, "TYPE_TAG_UINT", GI_TYPE_TAG_UINT);
    PyModule_AddIntConstant(m, "TYPE_TAG_LONG", GI_TYPE_TAG_LONG);
    PyModule_AddIntConstant(m, "TYPE_TAG_ULONG", GI_TYPE_TAG_ULONG);
    PyModule_AddIntConstant(m, "TYPE_TAG_SSIZE", GI_TYPE_TAG_SSIZE);
    PyModule_AddIntConstant(m, "TYPE_TAG_SIZE", GI_TYPE_TAG_SIZE);
    PyModule_AddIntConstant(m, "TYPE_TAG_FLOAT", GI_TYPE_TAG_FLOAT);
    PyModule_AddIntConstant(m, "TYPE_TAG_DOUBLE", GI_TYPE_TAG_DOUBLE);
    PyModule_AddIntConstant(m, "TYPE_TAG_TIME_T", GI_TYPE_TAG_TIME_T);
    PyModule_AddIntConstant(m, "TYPE_TAG_GTYPE", GI_TYPE_TAG_GTYPE);
    PyModule_AddIntConstant(m, "TYPE_TAG_UTF8", GI_TYPE_TAG_UTF8);
    PyModule_AddIntConstant(m, "TYPE_TAG_FILENAME", GI_TYPE_TAG_FILENAME);
    PyModule_AddIntConstant(m, "TYPE_TAG_ARRAY", GI_TYPE_TAG_ARRAY);
    PyModule_AddIntConstant(m, "TYPE_TAG_INTERFACE", GI_TYPE_TAG_INTERFACE);
    PyModule_AddIntConstant(m, "TYPE_TAG_GLIST", GI_TYPE_TAG_GLIST);
    PyModule_AddIntConstant(m, "TYPE_TAG_GSLIST", GI_TYPE_TAG_GSLIST);
    PyModule_AddIntConstant(m, "TYPE_TAG_GHASH", GI_TYPE_TAG_GHASH);
    PyModule_AddIntConstant(m, "TYPE_TAG_ERROR", GI_TYPE_TAG_ERROR);

    PyModule_AddIntConstant(m, "DIRECTION_IN", GI_DIRECTION_IN);
    PyModule_AddIntConstant(m, "DIRECTION_OUT", GI_DIRECTION_OUT);
    PyModule_AddIntConstant(m, "DIRECTION_INOUT", GI_DIRECTION_INOUT);
}

void
initrepo(void)
{
    PyObject *d, *m;

    m = Py_InitModule("girepository.repo", pybank_functions);
    d = PyModule_GetDict(m);

    g_type_init();

    pygobject_init(-1, -1, -1);
    register_types(d);
    register_constants(m);
}


/* -*- Mode: C; c-basic-offset: 4 -*-
 * pyglib - Python bindings for GLib toolkit.
 * Copyright (C) 2008  Johan Dahlin
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

#ifndef __PYGLIB_PYTHON_COMPAT_H__
#define __PYGLIB_PYTHON_COMPAT_H__

# define PYGLIB_CPointer_Check PyCapsule_CheckExact
# define PYGLIB_CPointer_WrapPointer(ptr, typename) \
    PyCapsule_New(ptr, typename, NULL)
# define PYGLIB_CPointer_GetPointer(obj, typename) \
    PyCapsule_GetPointer(obj, typename)
# define PYGLIB_CPointer_Import(module, symbol) \
    PyCapsule_Import(##module##.##symbol##, FALSE)


#define PYGLIB_MODULE_ERROR_RETURN NULL

/* Compilation on Python 2.x */
#if PY_VERSION_HEX < 0x03000000

#define RO READONLY

#define PYGLIB_PyBaseString_Check(ob) (PyString_Check(ob) || PyUnicode_Check(ob))

#define PYGLIB_PyUnicode_Check PyString_Check
#define PYGLIB_PyUnicode_AsString PyString_AsString
#define PYGLIB_PyUnicode_AsStringAndSize PyString_AsStringAndSize
#define PYGLIB_PyUnicode_FromString PyString_FromString
#define PYGLIB_PyUnicode_FromStringAndSize PyString_FromStringAndSize
#define PYGLIB_PyUnicode_FromFormat PyString_FromFormat
#define PYGLIB_PyUnicode_AS_STRING PyString_AS_STRING
#define PYGLIB_PyUnicode_GET_SIZE PyString_GET_SIZE
#define PYGLIB_PyUnicode_Type PyString_Type
#define PYGLIB_PyUnicode_InternFromString PyString_InternFromString
#define PYGLIB_PyUnicode_InternInPlace PyString_InternInPlace

#define PYGLIB_PyBytes_FromString PyString_FromString
#define PYGLIB_PyBytes_FromStringAndSize PyString_FromStringAndSize
#define PYGLIB_PyBytes_Resize _PyString_Resize
#define PYGLIB_PyBytes_AsString PyString_AsString
#define PYGLIB_PyBytes_Size PyString_Size
#define PYGLIB_PyBytes_Check PyString_Check

#define PYGLIB_PyLong_Check PyInt_Check
#define PYGLIB_PyLong_FromLong PyInt_FromLong
#define PYGLIB_PyLong_FromSsize_t PyInt_FromSsize_t
#define PYGLIB_PyLong_FromSize_t PyInt_FromSize_t
#define PYGLIB_PyLong_AsLong  PyInt_AsLong
#define PYGLIB_PyLongObject PyIntObject
#define PYGLIB_PyLong_Type PyInt_Type
#define PYGLIB_PyLong_AS_LONG PyInt_AS_LONG
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)

/* Python 2.7 lacks a PyInt_FromUnsignedLong function; use signed longs, and
 * rely on PyInt_AsUnsignedLong() to interpret them correctly */
#define PYGLIB_PyLong_FromUnsignedLong PyInt_FromLong
#define PYGLIB_PyLong_AsUnsignedLong(o) PyInt_AsUnsignedLongMask((PyObject*)(o))

#define PYGLIB_PyNumber_Long PyNumber_Int

#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(base, size) \
  PyObject_HEAD_INIT(base) \
  size,
#endif

#define PYGLIB_MODULE_START(symbol, modname)	        \
PyObject * pyglib_##symbol##_module_create(void);       \
DL_EXPORT(void) init##symbol(void);                     \
DL_EXPORT(void) init##symbol(void) {                    \
    pyglib_##symbol##_module_create();                  \
};                                                      \
PyObject * pyglib_##symbol##_module_create(void)        \
{                                                       \
    PyObject *module;                                   \
    module = Py_InitModule(modname, symbol##_functions);

#define PYGLIB_MODULE_END return module; }

#define PYGLIB_DEFINE_TYPE(typename, symbol, csymbol)	\
PyTypeObject symbol = {                                 \
    PyObject_HEAD_INIT(NULL)                            \
    0,                                                  \
    typename,						\
    sizeof(csymbol),                                    \
    0,                                                  \
};

#define PYGLIB_REGISTER_TYPE(d, type, name)	        \
    if (!type.tp_alloc)                                 \
	type.tp_alloc = PyType_GenericAlloc;            \
    if (!type.tp_new)                                   \
	type.tp_new = PyType_GenericNew;                \
    if (PyType_Ready(&type))                            \
	return;                                         \
    PyDict_SetItemString(d, name, (PyObject *)&type);

#else

#define PYGLIB_MODULE_START(symbol, modname)	        \
    static struct PyModuleDef _##symbol##module = {     \
    PyModuleDef_HEAD_INIT,                              \
    modname,                                            \
    NULL,                                               \
    -1,                                                 \
    symbol##_functions,                                 \
    NULL,                                               \
    NULL,                                               \
    NULL,                                               \
    NULL                                                \
};                                                      \
PyObject * pyglib_##symbol##_module_create(void);       \
PyMODINIT_FUNC PyInit_##symbol(void);                   \
PyMODINIT_FUNC PyInit_##symbol(void) {                  \
    return pyglib_##symbol##_module_create();           \
};                                                      \
PyObject * pyglib_##symbol##_module_create(void)        \
{                                                       \
    PyObject *module;                                   \
    module = PyModule_Create(&_##symbol##module);

#define PYGLIB_MODULE_END return module; }

#define PYGLIB_DEFINE_TYPE(typename, symbol, csymbol)	\
PyTypeObject symbol = {                                 \
    PyVarObject_HEAD_INIT(NULL, 0)                      \
    typename,                                           \
    sizeof(csymbol)                                     \
};

#define PYGLIB_REGISTER_TYPE(d, type, name)	            \
    if (!type.tp_alloc)                                 \
	    type.tp_alloc = PyType_GenericAlloc;            \
    if (!type.tp_new)                                   \
	    type.tp_new = PyType_GenericNew;                \
    if (PyType_Ready(&type))                            \
	    return;                                         \
    PyDict_SetItemString(d, name, (PyObject *)&type);

#define PYGLIB_PyBaseString_Check PyUnicode_Check

#define PYGLIB_PyUnicode_Check PyUnicode_Check
#define PYGLIB_PyUnicode_AsString _PyUnicode_AsString
#define PYGLIB_PyUnicode_AsStringAndSize(obj, buf, size) \
    (((*(buf) = _PyUnicode_AsStringAndSize(obj, size)) != NULL) ? 0 : -1) 
#define PYGLIB_PyUnicode_FromString PyUnicode_FromString
#define PYGLIB_PyUnicode_FromStringAndSize PyUnicode_FromStringAndSize
#define PYGLIB_PyUnicode_FromFormat PyUnicode_FromFormat
#define PYGLIB_PyUnicode_GET_SIZE PyUnicode_GET_SIZE
#define PYGLIB_PyUnicode_Resize PyUnicode_Resize
#define PYGLIB_PyUnicode_Type PyUnicode_Type
#define PYGLIB_PyUnicode_InternFromString PyUnicode_InternFromString
#define PYGLIB_PyUnicode_InternInPlace PyUnicode_InternInPlace

#define PYGLIB_PyLong_Check PyLong_Check
#define PYGLIB_PyLong_FromLong PyLong_FromLong
#define PYGLIB_PyLong_FromSize_t PyLong_FromSize_t
#define PYGLIB_PyLong_AsLong PyLong_AsLong
#define PYGLIB_PyLong_AS_LONG(o) PyLong_AS_LONG((PyObject*)(o))
#define PYGLIB_PyLongObject PyLongObject
#define PYGLIB_PyLong_Type PyLong_Type

#define PYGLIB_PyLong_FromUnsignedLong PyLong_FromUnsignedLong
#define PYGLIB_PyLong_AsUnsignedLong(o) PyLong_AsUnsignedLongMask((PyObject*)(o))

#define PYGLIB_PyBytes_FromString PyBytes_FromString
#define PYGLIB_PyBytes_FromStringAndSize PyBytes_FromStringAndSize
#define PYGLIB_PyBytes_Resize(o, len) _PyBytes_Resize(o, len)
#define PYGLIB_PyBytes_AsString PyBytes_AsString
#define PYGLIB_PyBytes_Size PyBytes_Size
#define PYGLIB_PyBytes_Check PyBytes_Check

#define PYGLIB_PyNumber_Long PyNumber_Long

#endif

#endif /* __PYGLIB_PYTHON_COMPAT_H__ */

/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 */
#ifndef __PYGI_PRIVATE_H__
#define __PYGI_PRIVATE_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Python.h>

#include "pygi.h"

#include "pygi-repository.h"
#include "pygi-info.h"
#include "pygi-argument.h"

extern PyTypeObject PyGIRepository_Type;

extern PyTypeObject PyGIBaseInfo_Type;
extern PyTypeObject PyGICallableInfo_Type;
extern PyTypeObject PyGIFunctionInfo_Type;
extern PyTypeObject PyGIRegisteredTypeInfo_Type;
extern PyTypeObject PyGIStructInfo_Type;
extern PyTypeObject PyGIEnumInfo_Type;
extern PyTypeObject PyGIObjectInfo_Type;
extern PyTypeObject PyGIInterfaceInfo_Type;
extern PyTypeObject PyGIValueInfo_Type;
extern PyTypeObject PyGIFieldInfo_Type;
extern PyTypeObject PyGIUnresolvedInfo_Type;

#define PyErr_PREFIX_FROM_FORMAT(format, ...) G_STMT_START { \
    PyObject *py_error_prefix; \
    py_error_prefix = PyString_FromFormat(format, ## __VA_ARGS__); \
    if (py_error_prefix != NULL) { \
        PyObject *py_error_type, *py_error_value, *py_error_traceback; \
        PyErr_Fetch(&py_error_type, &py_error_value, &py_error_traceback); \
        if (PyString_Check(py_error_value)) { \
            PyString_ConcatAndDel(&py_error_prefix, py_error_value); \
            if (py_error_prefix != NULL) { \
                py_error_value = py_error_prefix; \
            } \
        } \
        PyErr_Restore(py_error_type, py_error_value, py_error_traceback); \
    } \
} G_STMT_END

PyObject * pygi_py_type_find_by_name(const char *namespace_,
                                     const char *name);

#define pygi_py_type_find_by_gi_info(info) \
    pygi_py_type_find_by_name(g_base_info_get_namespace(info), g_base_info_get_name(info))

gpointer pygi_py_object_get_buffer(PyObject *object, gsize *size);

#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 22
#define g_array_get_element_size(a) \
    *(guint *)((gpointer)(a) + sizeof(guint8 *) + sizeof(guint) * 2)
#endif


/* GArray */

/* Redefine g_array_index because we want it to return the i-th element, casted
 * to the type t, of the array a, and not the i-th element of the array a casted to the type t. */
#define _g_array_index(a,t,i) \
    *(t *)((a)->data + g_array_get_element_size(a) * (i))


#endif /* __PYGI_PRIVATE_H__ */

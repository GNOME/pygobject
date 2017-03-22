#ifndef __PYGI_UTIL_H__
#define __PYGI_UTIL_H__

#include <Python.h>
#include <glib.h>
#include "pygobject-internal.h"
#include <pyglib-python-compat.h>

G_BEGIN_DECLS

PyObject * pyg_integer_richcompare(PyObject *v, PyObject *w, int op);

#if PY_VERSION_HEX >= 0x03000000

#define _PyGI_ERROR_PREFIX(format, ...) G_STMT_START { \
    PyObject *py_error_prefix; \
    py_error_prefix = PyUnicode_FromFormat(format, ## __VA_ARGS__); \
    if (py_error_prefix != NULL) { \
        PyObject *py_error_type, *py_error_value, *py_error_traceback; \
        PyErr_Fetch(&py_error_type, &py_error_value, &py_error_traceback); \
        if (PyUnicode_Check(py_error_value)) { \
            PyObject *new; \
            new = PyUnicode_Concat(py_error_prefix, py_error_value); \
            Py_DECREF(py_error_value); \
            if (new != NULL) { \
                py_error_value = new; \
            } \
        } \
        PyErr_Restore(py_error_type, py_error_value, py_error_traceback); \
        Py_DECREF(py_error_prefix); \
    } \
} G_STMT_END

#else

#define _PyGI_ERROR_PREFIX(format, ...) G_STMT_START { \
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

#endif

G_END_DECLS

#endif /* __PYGI_UTIL_H__ */

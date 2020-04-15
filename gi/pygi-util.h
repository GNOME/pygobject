#ifndef __PYGI_UTIL_H__
#define __PYGI_UTIL_H__

#include <Python.h>
#include <glib.h>

G_BEGIN_DECLS

PyObject * pyg_integer_richcompare(PyObject *v, PyObject *w, int op);
PyObject * pyg_ptr_richcompare(void* a, void *b, int op);
const gchar * pyg_constant_strip_prefix(const gchar *name, const gchar *strip_prefix);

gboolean pygi_guint_from_pyssize (Py_ssize_t pyval, guint *result);

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
	    return -1;                                         \
    PyDict_SetItemString(d, name, (PyObject *)&type);

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


G_END_DECLS

#endif /* __PYGI_UTIL_H__ */

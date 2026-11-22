#pragma once

#include <pythoncapi_compat.h>
#include <glib.h>

G_BEGIN_DECLS

PyObject *pyg_ptr_richcompare (void *a, void *b, int op);
const gchar *pyg_constant_strip_prefix (const gchar *name,
                                        const gchar *strip_prefix);

gboolean pyg_guint_from_pyssize (Py_ssize_t pyval, guint *result);

PyObject *pyg_is_python_keyword (const gchar *name);

void pyg_destroy_notify (gpointer user_data);

#define PYG_DEFINE_TYPE(typename, symbol, csymbol)                            \
    PyTypeObject symbol = { PyVarObject_HEAD_INIT (NULL, 0) typename,         \
                            sizeof (csymbol) };

#define PYG_ERROR_PREFIX(format, ...)                                         \
    G_STMT_START                                                              \
    {                                                                         \
        PyObject *py_error_prefix;                                            \
        py_error_prefix = PyUnicode_FromFormat (format, ##__VA_ARGS__);       \
        if (py_error_prefix != NULL) {                                        \
            PyObject *py_error_type, *py_error_value, *py_error_traceback;    \
            PyErr_Fetch (&py_error_type, &py_error_value,                     \
                         &py_error_traceback);                                \
            if (PyUnicode_Check (py_error_value)) {                           \
                PyObject *new;                                                \
                new = PyUnicode_Concat (py_error_prefix, py_error_value);     \
                Py_DECREF (py_error_value);                                   \
                if (new != NULL) {                                            \
                    py_error_value = new;                                     \
                }                                                             \
            }                                                                 \
            PyErr_Restore (py_error_type, py_error_value,                     \
                           py_error_traceback);                               \
            Py_DECREF (py_error_prefix);                                      \
        }                                                                     \
    }                                                                         \
    G_STMT_END


G_END_DECLS

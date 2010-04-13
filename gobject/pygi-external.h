/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 */

#ifndef _PYGI_EXTERNAL_H_
#define _PYGI_EXTERNAL_H_

#include <Python.h>
#include <glib.h>

struct PyGI_API {
    PyObject* (*type_import_by_g_type) (GType g_type);
};

static struct PyGI_API *PyGI_API = NULL;

static int
_pygi_import (void)
{
#if ENABLE_PYGI
    PyObject *module;
    PyObject *api;

    if (PyGI_API != NULL) {
        return 1;
    }

    module = PyImport_ImportModule("gi");
    if (module == NULL) {
        return -1;
    }

    api = PyObject_GetAttrString(module, "_API");
    if (api == NULL) {
        Py_DECREF(module);
        return -1;
    }
    if (!PyCObject_Check(api)) {
        Py_DECREF(module);
        Py_DECREF(api);
        PyErr_Format(PyExc_TypeError, "gi._API must be cobject, not %s",
            Py_TYPE(api)->tp_name);
        return -1;
    }

    PyGI_API = (struct PyGI_API *)PyCObject_AsVoidPtr(api);

    Py_DECREF(module);

    return 0;
#else
    return -1;
#endif /* ENABLE_PYGI */
}

static inline PyObject *
pygi_type_import_by_g_type (GType g_type)
{
   if (_pygi_import() < 0) {
       return NULL;
   }
   return PyGI_API->type_import_by_g_type(g_type);
}

#endif /* _PYGI_EXTERNAL_H_ */

/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
 *
 *   gimodule.c: wrapper for the gobject-introspection library.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include "pygi-private.h"

#include <pygobject.h>

static PyObject *
_wrap_pyg_enum_add (PyObject *self,
                    PyObject *args,
                    PyObject *kwargs)
{
    static char *kwlist[] = { "g_type", NULL };
    PyObject *py_g_type;
    GType g_type;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                "O!:enum_add",
                kwlist, &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object(py_g_type);
    if (g_type == G_TYPE_INVALID) {
        return NULL;
    }

    return pyg_enum_add(NULL, g_type_name(g_type), NULL, g_type);
}

static PyObject *
_wrap_pyg_flags_add (PyObject *self,
                     PyObject *args,
                     PyObject *kwargs)
{
    static char *kwlist[] = { "g_type", NULL };
    PyObject *py_g_type;
    GType g_type;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                "O!:flags_add",
                kwlist, &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object(py_g_type);
    if (g_type == G_TYPE_INVALID) {
        return NULL;
    }

    return pyg_flags_add(NULL, g_type_name(g_type), NULL, g_type);
}

static PyObject *
_wrap_pyg_set_object_has_new_constructor (PyObject *self,
                                          PyObject *args,
                                          PyObject *kwargs)
{
    static char *kwlist[] = { "g_type", NULL };
    PyObject *py_g_type;
    GType g_type;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                "O!:set_object_has_new_constructor",
                kwlist, &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object(py_g_type);
    if (!g_type_is_a(g_type, G_TYPE_OBJECT)) {
        PyErr_SetString(PyExc_TypeError, "must be a subtype of GObject");
        return NULL;
    }

    pyg_set_object_has_new_constructor(g_type);

    Py_RETURN_NONE;
}

static void
initialize_interface (GTypeInterface *iface, PyTypeObject *pytype)
{
    // pygobject prints a warning if interface_init is NULL
}

static PyObject *
_wrap_pyg_register_interface_info(PyObject *self, PyObject *args)
{
    PyObject *py_g_type;
    GType g_type;
    GInterfaceInfo *info;

    if (!PyArg_ParseTuple(args, "O!:register_interface_info",
                          &PyGTypeWrapper_Type, &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object(py_g_type);
    if (!g_type_is_a(g_type, G_TYPE_INTERFACE)) {
        PyErr_SetString(PyExc_TypeError, "must be an interface");
        return NULL;
    }

    info = g_new0(GInterfaceInfo, 1);
    info->interface_init = (GInterfaceInitFunc) initialize_interface;

    pyg_register_interface_info(g_type, info);

    Py_RETURN_NONE;
}


static PyMethodDef _pygi_functions[] = {
    { "enum_add", (PyCFunction)_wrap_pyg_enum_add, METH_VARARGS | METH_KEYWORDS },
    { "flags_add", (PyCFunction)_wrap_pyg_flags_add, METH_VARARGS | METH_KEYWORDS },

    { "set_object_has_new_constructor", (PyCFunction)_wrap_pyg_set_object_has_new_constructor, METH_VARARGS | METH_KEYWORDS },
    { "register_interface_info", (PyCFunction)_wrap_pyg_register_interface_info, METH_VARARGS },
    { NULL, NULL, 0 }
};

struct PyGI_API PyGI_API = {
    pygi_type_import_by_g_type
};


PyMODINIT_FUNC
init_gi(void)
{
    PyObject *m;
    PyObject *api;

    m = Py_InitModule("_gi", _pygi_functions);
    if (m == NULL) {
        return;
    }

    if (pygobject_init(-1, -1, -1) == NULL) {
        return;
    }

    if (_pygobject_import() < 0) {
        return;
    }

    _pygi_repository_register_types(m);
    _pygi_info_register_types(m);
    _pygi_struct_register_types(m);
    _pygi_argument_init();

    api = PyCObject_FromVoidPtr((void *)&PyGI_API, NULL);
    if (api == NULL) {
        return;
    }
    PyModule_AddObject(m, "_API", api);
}


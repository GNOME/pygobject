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

PyObject *
pygi_type_find_by_name (const char *namespace_,
                        const char *name)
{
    PyObject *py_module;
    gchar *module_name;

    module_name = g_strconcat("gi.repository.", namespace_, NULL);

    py_module = PyImport_ImportModule(module_name);

    g_free(module_name);

    if (name != NULL) {
        PyObject *py_object;

        if (py_module == NULL) {
            return NULL;
        }

        if (strcmp(namespace_, "GObject") == 0 &&
                (strcmp(name, "Object") == 0 || strcmp(name, "InitiallyUnowned") == 0)) {
            /* Special case for GObject.(Object|InitiallyUnowned) which actually is
             * gobject.GObject. */
             py_object = (PyObject *)&PyGObject_Type;
             Py_INCREF(py_object);
        } else {
            py_object = PyObject_GetAttrString(py_module, name);
        }

        Py_DECREF(py_module);

        return py_object;
    }

    return py_module;
}

PyObject *
pygi_type_find_by_gi_info (GIBaseInfo *info)
{
    const char *namespace_;
    const char *name;

    namespace_ = g_base_info_get_namespace(info);
    name = g_base_info_get_name(info);

    return pygi_type_find_by_name(namespace_, name);
}

GIBaseInfo *
pygi_object_get_gi_info (PyObject     *object,
                          PyTypeObject *type)
{
    PyObject *py_info;
    GIBaseInfo *info = NULL;

    py_info = PyObject_GetAttrString(object, "__info__");
    if (py_info == NULL) {
        goto out;
    }
    if (!PyObject_TypeCheck(py_info, type)) {
        PyErr_Format(PyExc_TypeError, "attribute '__info__' must be %s, not %s",
            type->tp_name, py_info->ob_type->tp_name);
        goto out;
    }

    info = PyGIBaseInfo_GET_GI_INFO(py_info);

out:
    Py_XDECREF(py_info);

    return info;
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


static PyMethodDef _pygi_functions[] = {
    { "set_object_has_new_constructor", (PyCFunction)_wrap_pyg_set_object_has_new_constructor, METH_VARARGS | METH_KEYWORDS },
    { NULL, NULL, 0 }
};

struct PyGI_API PyGI_API = {
    &PyGIStructInfo_Type,
    pygi_g_struct_info_is_simple,
    pygi_type_find_by_gi_info,
    pygi_object_get_gi_info
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

    _pygi_repository_register_types(m);
    _pygi_info_register_types(m);
    _pygi_argument_init();

    api = PyCObject_FromVoidPtr((void *)&PyGI_API, NULL);
    if (api == NULL) {
        return;
    }
    PyModule_AddObject(m, "_API", api);
}


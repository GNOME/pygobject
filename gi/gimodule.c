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
pygi_py_type_find_by_name(const char *namespace_, const char *name)
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

gpointer
pygi_py_object_get_buffer(PyObject *object, gsize *size)
{
    PyBufferProcs *py_buffer_procs;
    PyObject *py_buffer;
    gpointer buffer;

    py_buffer = PyObject_GetAttrString(object, "__buffer__");
    if (py_buffer == NULL) {
        return NULL;
    }

    if(!PyBuffer_Check(py_buffer)) {
        PyErr_Format(PyExc_TypeError, "Must be buffer, not %s",
                object->ob_type->tp_name);
    }

    /* We don't need to keep a reference. */
    Py_DECREF(py_buffer);

    py_buffer_procs = py_buffer->ob_type->tp_as_buffer;

    *size = (*py_buffer_procs->bf_getreadbuffer)(py_buffer, 0, &buffer);
    if (*size < 0) {
        return NULL;
    }

    return buffer;
}

static PyObject *
_wrap_set_object_has_new_constructor(PyObject *self, PyObject *args)
{
    PyObject *py_g_type;
    GType g_type;

    if (!PyArg_ParseTuple(args, "O:setObjectHasNewConstructor", &py_g_type)) {
        return NULL;
    }

    g_type = pyg_type_from_object(py_g_type);
    pyg_set_object_has_new_constructor(g_type);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef _pygi_functions[] = {
    { "setObjectHasNewConstructor", (PyCFunction)_wrap_set_object_has_new_constructor, METH_VARARGS },
    { NULL, NULL, 0 }
};

PyMODINIT_FUNC
init_gi(void)
{
    PyObject *m;

    m = Py_InitModule("_gi", _pygi_functions);
    if (m == NULL) {
        return;
    }

    if (pygobject_init(-1, -1, -1) == NULL) {
        return;
    }

    pygi_repository_register_types(m);
    pygi_info_register_types(m);
}


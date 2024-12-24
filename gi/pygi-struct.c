/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2009 Simon van der Linden <svdlinden@src.gnome.org>
 *
 *   pygi-struct.c: wrapper to handle non-registered structures.
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

#include "pygi-struct.h"

#include "pygi-foreign.h"
#include "pygi-info.h"
#include "pygi-type.h"
#include "pygi-util.h"
#include "pygpointer.h"

#include <girepository/girepository.h>


static GIBaseInfo *
struct_get_info (PyTypeObject *type)
{
    PyObject *py_info;
    GIBaseInfo *info = NULL;

    py_info = PyObject_GetAttrString ((PyObject *)type, "__info__");
    if (py_info == NULL) {
        return NULL;
    }
    if (!PyObject_TypeCheck (py_info, &PyGIStructInfo_Type)
        && !PyObject_TypeCheck (py_info, &PyGIUnionInfo_Type)) {
        PyErr_Format (PyExc_TypeError,
                      "attribute '__info__' must be %s or %s, not %s",
                      PyGIStructInfo_Type.tp_name, PyGIUnionInfo_Type.tp_name,
                      Py_TYPE (py_info)->tp_name);
        goto out;
    }

    info = ((PyGIBaseInfo *)py_info)->info;
    gi_base_info_ref (info);

out:
    Py_DECREF (py_info);

    return info;
}

static void
struct_dealloc (PyGIStruct *self)
{
    GIBaseInfo *info;
    PyObject *error_type, *error_value, *error_traceback;
    gboolean have_error = !!PyErr_Occurred ();

    if (have_error) PyErr_Fetch (&error_type, &error_value, &error_traceback);

    info = struct_get_info (Py_TYPE (self));

    if (info != NULL && gi_struct_info_is_foreign ((GIStructInfo *)info)) {
        pygi_struct_foreign_release (info, pyg_pointer_get_ptr (self));
    } else if (self->free_on_dealloc) {
        g_free (pyg_pointer_get_ptr (self));
    }

    if (info != NULL) {
        gi_base_info_unref (info);
    }

    if (have_error) PyErr_Restore (error_type, error_value, error_traceback);

    Py_TYPE (self)->tp_free ((PyObject *)self);
}

static PyObject *
struct_new (PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };

    GIBaseInfo *info;
    gsize size;
    gpointer pointer;
    PyObject *self = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "", kwlist)) {
        return NULL;
    }

    info = struct_get_info (type);
    if (info == NULL) {
        if (PyErr_ExceptionMatches (PyExc_AttributeError)) {
            PyErr_Format (PyExc_TypeError,
                          "missing introspection information");
        }
        return NULL;
    }

    size = gi_struct_info_get_size ((GIStructInfo *)info);
    if (size == 0) {
        PyErr_Format (PyExc_TypeError,
                      "struct cannot be created directly; try using a "
                      "constructor, see: help(%s.%s)",
                      gi_base_info_get_namespace (info),
                      gi_base_info_get_name (info));
        goto out;
    }
    pointer = g_try_malloc0 (size);
    if (pointer == NULL) {
        PyErr_NoMemory ();
        goto out;
    }

    self = pygi_struct_new (type, pointer, TRUE);
    if (self == NULL) {
        g_free (pointer);
    }

out:
    gi_base_info_unref (info);

    return (PyObject *)self;
}

static int
struct_init (PyObject *self, PyObject *args, PyObject *kwargs)
{
    /* Don't call PyGPointer's init, which raises an exception. */
    return 0;
}

PYGI_DEFINE_TYPE ("gi.Struct", PyGIStruct_Type, PyGIStruct);


PyObject *
pygi_struct_new_from_g_type (GType g_type, gpointer pointer,
                             gboolean free_on_dealloc)
{
    PyGIStruct *self;
    PyTypeObject *type;

    type = (PyTypeObject *)pygi_type_import_by_g_type (g_type);

    if (!type) type = (PyTypeObject *)&PyGIStruct_Type; /* fallback */

    if (!PyType_IsSubtype (type, &PyGIStruct_Type)) {
        PyErr_SetString (PyExc_TypeError, "must be a subtype of gi.Struct");
        return NULL;
    }

    self = (PyGIStruct *)type->tp_alloc (type, 0);
    if (self == NULL) {
        return NULL;
    }

    pyg_pointer_set_ptr (self, pointer);
    ((PyGPointer *)self)->gtype = g_type;
    self->free_on_dealloc = free_on_dealloc;

    return (PyObject *)self;
}


PyObject *
pygi_struct_new (PyTypeObject *type, gpointer pointer,
                 gboolean free_on_dealloc)
{
    PyGIStruct *self;
    GType g_type;

    if (!PyType_IsSubtype (type, &PyGIStruct_Type)) {
        PyErr_SetString (PyExc_TypeError, "must be a subtype of gi.Struct");
        return NULL;
    }

    self = (PyGIStruct *)type->tp_alloc (type, 0);
    if (self == NULL) {
        return NULL;
    }

    g_type = pyg_type_from_object ((PyObject *)type);

    pyg_pointer_set_ptr (self, pointer);
    ((PyGPointer *)self)->gtype = g_type;
    self->free_on_dealloc = free_on_dealloc;

    return (PyObject *)self;
}

static PyObject *
struct_repr (PyGIStruct *self)
{
    PyObject *repr;
    GIBaseInfo *info;
    PyGPointer *pointer = (PyGPointer *)self;

    info = struct_get_info (Py_TYPE (self));
    if (info == NULL) return NULL;

    repr = PyUnicode_FromFormat (
        "<%s.%s object at %p (%s at %p)>", gi_base_info_get_namespace (info),
        gi_base_info_get_name (info), self, g_type_name (pointer->gtype),
        pointer->pointer);

    gi_base_info_unref (info);

    return repr;
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_struct_register_types (PyObject *m)
{
    Py_SET_TYPE (&PyGIStruct_Type, &PyType_Type);
    g_assert (Py_TYPE (&PyGPointer_Type) != NULL);
    PyGIStruct_Type.tp_base = &PyGPointer_Type;
    PyGIStruct_Type.tp_new = (newfunc)struct_new;
    PyGIStruct_Type.tp_init = (initproc)struct_init;
    PyGIStruct_Type.tp_dealloc = (destructor)struct_dealloc;
    PyGIStruct_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    PyGIStruct_Type.tp_repr = (reprfunc)struct_repr;

    if (PyType_Ready (&PyGIStruct_Type) < 0) return -1;
    Py_INCREF ((PyObject *)&PyGIStruct_Type);
    if (PyModule_AddObject (m, "Struct", (PyObject *)&PyGIStruct_Type) < 0) {
        Py_DECREF ((PyObject *)&PyGIStruct_Type);
        return -1;
    }

    return 0;
}

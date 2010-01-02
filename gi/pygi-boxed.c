/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2009 Simon van der Linden <svdlinden@src.gnome.org>
 *
 *   pygi-boxed.c: wrapper to handle registered structures.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#include "pygi-private.h"

#include <pygobject.h>
#include <girepository.h>

static void
_boxed_dealloc (PyGIBoxed *self)
{
    GType g_type;

    PyObject_GC_UnTrack((PyObject *)self);

    PyObject_ClearWeakRefs((PyObject *)self);

    if (((PyGBoxed *)self)->free_on_dealloc) {
        if (self->slice_allocated) {
            g_slice_free1(self->size, ((PyGBoxed *)self)->boxed);
        } else {
            g_type = pyg_type_from_object((PyObject *)self);
            g_boxed_free (g_type, ((PyGBoxed *)self)->boxed);
        }
    }

    ((PyGObject *)self)->ob_type->tp_free((PyObject *)self);
}

static PyObject *
_boxed_new (PyTypeObject *type,
            PyObject     *args,
            PyObject     *kwargs)
{
    static char *kwlist[] = { NULL };

    GIBaseInfo *info;
    gsize size;
    gpointer boxed;
    PyGIBoxed *self = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
        return NULL;
    }

    info = _pygi_object_get_gi_info((PyObject *)type, &PyGIBaseInfo_Type);
    if (info == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Format(PyExc_TypeError, "missing introspection information");
        }
        return NULL;
    }

    switch (g_base_info_get_type(info)) {
        case GI_INFO_TYPE_UNION:
            size = g_union_info_get_size((GIUnionInfo *)info);
            break;
        case GI_INFO_TYPE_BOXED:
        case GI_INFO_TYPE_STRUCT:
            size = g_struct_info_get_size((GIStructInfo *)info);
            break;
        default:
            PyErr_Format(PyExc_TypeError,
                         "info should be Boxed or Union, not '%d'",
                         g_base_info_get_type(info));
            return NULL;
    }

    boxed = g_slice_alloc0(size);
    if (boxed == NULL) {
        PyErr_NoMemory();
        goto out;
    }

    self = (PyGIBoxed *)_pygi_boxed_new(type, boxed, TRUE);
    if (self == NULL) {
        g_slice_free1(size, boxed);
        goto out;
    }

    self->size = size;
    self->slice_allocated = TRUE;

out:
    g_base_info_unref(info);

    return (PyObject *)self;
}

static int
_boxed_init (PyObject *self,
              PyObject *args,
              PyObject *kwargs)
{
    /* Don't call PyGBoxed's init, which raises an exception. */
    return 0;
}


PyTypeObject PyGIBoxed_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "gi.Boxed",                                /* tp_name */
    sizeof(PyGIBoxed),                         /* tp_basicsize */
    0,                                         /* tp_itemsize */
    (destructor)_boxed_dealloc,                /* tp_dealloc */
    (printfunc)NULL,                           /* tp_print */
    (getattrfunc)NULL,                         /* tp_getattr */
    (setattrfunc)NULL,                         /* tp_setattr */
    (cmpfunc)NULL,                             /* tp_compare */
    (reprfunc)NULL,                            /* tp_repr */
    NULL,                                      /* tp_as_number */
    NULL,                                      /* tp_as_sequence */
    NULL,                                      /* tp_as_mapping */
    (hashfunc)NULL,                            /* tp_hash */
    (ternaryfunc)NULL,                         /* tp_call */
    (reprfunc)NULL,                            /* tp_str */
    (getattrofunc)NULL,                        /* tp_getattro */
    (setattrofunc)NULL,                        /* tp_setattro */
    NULL,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags */
    NULL,                                      /* tp_doc */
    (traverseproc)NULL,                        /* tp_traverse */
    (inquiry)NULL,                             /* tp_clear */
    (richcmpfunc)NULL,                         /* tp_richcompare */
    0,                                         /* tp_weaklistoffset */
    (getiterfunc)NULL,                         /* tp_iter */
    (iternextfunc)NULL,                        /* tp_iternext */
    NULL,                                      /* tp_methods */
    NULL,                                      /* tp_members */
    NULL,                                      /* tp_getset */
    (PyTypeObject *)NULL,                      /* tp_base */
};

PyObject *
_pygi_boxed_new (PyTypeObject *type,
                 gpointer      boxed,
                 gboolean      free_on_dealloc)
{
    PyGIBoxed *self;

    if (!boxed) {
        Py_RETURN_NONE;
    }

    if (!PyType_IsSubtype(type, &PyGIBoxed_Type)) {
        PyErr_SetString(PyExc_TypeError, "must be a subtype of gi.Boxed");
        return NULL;
    }

    self = (PyGIBoxed *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    ((PyGBoxed *)self)->gtype = pyg_type_from_object((PyObject *)type);
    ((PyGBoxed *)self)->boxed = boxed;
    ((PyGBoxed *)self)->free_on_dealloc = free_on_dealloc;
    self->size = 0;
    self->slice_allocated = FALSE;

    return (PyObject *)self;
}

void
_pygi_boxed_register_types (PyObject *m)
{
    PyGIBoxed_Type.ob_type = &PyType_Type;
    PyGIBoxed_Type.tp_base = &PyGBoxed_Type;
    PyGIBoxed_Type.tp_new = (newfunc)_boxed_new;
    PyGIBoxed_Type.tp_init = (initproc)_boxed_init;
    if (PyType_Ready(&PyGIBoxed_Type))
        return;
    if (PyModule_AddObject(m, "Boxed", (PyObject *)&PyGIBoxed_Type))
        return;
}

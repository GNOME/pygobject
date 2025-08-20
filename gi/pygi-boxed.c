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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <girepository/girepository.h>

#include "pygi-type.h"

#include "pygboxed.h"
#include "pygi-basictype.h"
#include "pygi-boxed.h"
#include "pygi-info.h"
#include "pygi-util.h"

struct _PyGIBoxed {
    PyGBoxed base;
    gboolean slice_allocated;
    gsize size;
};

static void
boxed_clear (PyGIBoxed *self)
{
    gpointer boxed = pyg_boxed_get_ptr (self);
    GType g_type = ((PyGBoxed *)self)->gtype;

    if (((PyGBoxed *)self)->free_on_dealloc && boxed != NULL) {
        if (self->slice_allocated) {
            if (g_type && g_type_is_a (g_type, G_TYPE_VALUE))
                g_value_unset (boxed);
            g_slice_free1 (self->size, boxed);
            self->slice_allocated = FALSE;
            self->size = 0;
        } else {
            g_boxed_free (g_type, boxed);
        }
    }
    pyg_boxed_set_ptr (self, NULL);
}

static PyObject *
boxed_clear_wrapper (PyGIBoxed *self)
{
    boxed_clear (self);

    Py_RETURN_NONE;
}


static void
boxed_dealloc (PyGIBoxed *self)
{
    boxed_clear (self);

    Py_TYPE (self)->tp_free ((PyObject *)self);
}

void *
pygi_boxed_alloc (GIBaseInfo *info, gsize *size_out)
{
    gpointer boxed = NULL;
    gsize size = 0;

    if (GI_IS_UNION_INFO (info)) {
        size = gi_union_info_get_size ((GIUnionInfo *)info);
    } else if (GI_IS_STRUCT_INFO (info)) {
        size = gi_struct_info_get_size ((GIStructInfo *)info);
    } else {
        PyErr_Format (PyExc_TypeError,
                      "info should be Boxed or Union, not '%d'",
                      g_type_name (G_TYPE_FROM_INSTANCE (info)));
        return NULL;
    }

    if (size == 0) {
        PyErr_Format (PyExc_TypeError,
                      "boxed cannot be created directly; try using a "
                      "constructor, see: help(%s.%s)",
                      gi_base_info_get_namespace (info),
                      gi_base_info_get_name (info));
        return NULL;
    }

    if (size_out != NULL) *size_out = size;

    boxed = g_slice_alloc0 (size);
    if (boxed == NULL) PyErr_NoMemory ();
    return boxed;
}

static PyObject *
boxed_new (PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    GIBaseInfo *info;
    gsize size = 0;
    gpointer boxed;
    PyGIBoxed *self = NULL;

    info = _pygi_object_get_gi_info ((PyObject *)type, &PyGIBaseInfo_Type);
    if (info == NULL) {
        if (PyErr_ExceptionMatches (PyExc_AttributeError)) {
            PyErr_Format (PyExc_TypeError,
                          "missing introspection information");
        }
        return NULL;
    }

    boxed = pygi_boxed_alloc (info, &size);
    if (boxed == NULL) {
        goto out;
    }

    self = (PyGIBoxed *)pygi_boxed_new (type, boxed, TRUE, size);
    if (self == NULL) {
        g_slice_free1 (size, boxed);
        goto out;
    }

    self->size = size;
    self->slice_allocated = TRUE;

out:
    gi_base_info_unref (info);

    return (PyObject *)self;
}

static int
boxed_init (PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "", kwlist)) {
        PyErr_Clear ();
        PyErr_Warn (
            PyExc_DeprecationWarning,
            "Passing arguments to gi.types.Boxed.__init__() is deprecated. "
            "All arguments passed will be ignored.");
    }

    /* Don't call PyGBoxed's init, which raises an exception. */
    return 0;
}

PYGI_DEFINE_TYPE ("gi.Boxed", PyGIBoxed_Type, PyGIBoxed);

PyObject *
pygi_boxed_new (PyTypeObject *type, gpointer boxed, gboolean free_on_dealloc,
                gsize allocated_slice)
{
    PyGIBoxed *self;

    if (!boxed) {
        Py_RETURN_NONE;
    }

    if (!PyType_IsSubtype (type, &PyGIBoxed_Type)) {
        PyErr_SetString (PyExc_TypeError, "must be a subtype of gi.Boxed");
        return NULL;
    }

    self = (PyGIBoxed *)type->tp_alloc (type, 0);
    if (self == NULL) {
        return NULL;
    }

    ((PyGBoxed *)self)->gtype = pyg_type_from_object ((PyObject *)type);
    ((PyGBoxed *)self)->free_on_dealloc = free_on_dealloc;
    pyg_boxed_set_ptr (self, boxed);
    if (allocated_slice > 0) {
        self->size = allocated_slice;
        self->slice_allocated = TRUE;
    } else {
        self->size = 0;
        self->slice_allocated = FALSE;
    }

    return (PyObject *)self;
}

/**
 * pygi_boxed_copy_in_place:
 *
 * Replace the boxed pointer held by this wrapper with a boxed copy
 * freeing the previously held pointer (when free_on_dealloc is TRUE).
 * This can be used in cases where Python is passed a reference which
 * it does not own and the wrapper is held by the Python program
 * longer than the duration of a callback it was passed to.
 */
void
pygi_boxed_copy_in_place (PyGIBoxed *self)
{
    PyGBoxed *pygboxed = (PyGBoxed *)self;
    gpointer ptr = pyg_boxed_get_ptr (self);
    gpointer copy = NULL;

    if (ptr) copy = g_boxed_copy (pygboxed->gtype, ptr);

    boxed_clear (self);
    pyg_boxed_set_ptr (pygboxed, copy);
    pygboxed->free_on_dealloc = TRUE;
}

static PyMethodDef boxed_methods[] = {
    { "_clear_boxed", (PyCFunction)boxed_clear_wrapper, METH_NOARGS },
    { NULL, NULL, 0 },
};

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_boxed_register_types (PyObject *m)
{
    Py_SET_TYPE (&PyGIBoxed_Type, &PyType_Type);
    g_assert (Py_TYPE (&PyGBoxed_Type) != NULL);
    PyGIBoxed_Type.tp_base = &PyGBoxed_Type;
    PyGIBoxed_Type.tp_new = (newfunc)boxed_new;
    PyGIBoxed_Type.tp_init = (initproc)boxed_init;
    PyGIBoxed_Type.tp_dealloc = (destructor)boxed_dealloc;
    PyGIBoxed_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    PyGIBoxed_Type.tp_methods = boxed_methods;

    if (PyType_Ready (&PyGIBoxed_Type) < 0) return -1;
    Py_INCREF ((PyObject *)&PyGIBoxed_Type);
    if (PyModule_AddObject (m, "Boxed", (PyObject *)&PyGIBoxed_Type) < 0) {
        Py_DECREF ((PyObject *)&PyGIBoxed_Type);
        return -1;
    }

    return 0;
}

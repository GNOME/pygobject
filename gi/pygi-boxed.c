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

#include "pygi-private.h"
#include "pygobject-private.h"

#include <girepository.h>
#include <pyglib-python-compat.h>

static void
_boxed_dealloc (PyGIBoxed *self)
{
    Py_TYPE (self)->tp_free ((PyObject *)self);
}

static PyObject *
boxed_del (PyGIBoxed *self)
{
    GType g_type;
    gpointer boxed = pyg_boxed_get_ptr (self);

    if ( ( (PyGBoxed *) self)->free_on_dealloc && boxed != NULL) {
        if (self->slice_allocated) {
            g_slice_free1 (self->size, boxed);
        } else {
            g_type = pyg_type_from_object ( (PyObject *) self);
            g_boxed_free (g_type, boxed);
        }
    }
    pyg_boxed_set_ptr (self, NULL);

    Py_RETURN_NONE;
}

void *
_pygi_boxed_alloc (GIBaseInfo *info, gsize *size_out)
{
    gpointer boxed = NULL;
    gsize size = 0;

    switch (g_base_info_get_type (info)) {
        case GI_INFO_TYPE_UNION:
            size = g_union_info_get_size ( (GIUnionInfo *) info);
            break;
        case GI_INFO_TYPE_BOXED:
        case GI_INFO_TYPE_STRUCT:
            size = g_struct_info_get_size ( (GIStructInfo *) info);
            break;
        default:
            PyErr_Format (PyExc_TypeError,
                          "info should be Boxed or Union, not '%d'",
                          g_base_info_get_type (info));
            return NULL;
    }

    if (size == 0) {
        PyErr_Format (PyExc_TypeError,
            "boxed cannot be created directly; try using a constructor, see: help(%s.%s)",
            g_base_info_get_namespace (info),
            g_base_info_get_name (info));
        return NULL;
    }

    if( size_out != NULL)
        *size_out = size;

    boxed = g_slice_alloc0 (size);
    if (boxed == NULL)
        PyErr_NoMemory();
    return boxed;
}

static PyObject *
_boxed_new (PyTypeObject *type,
            PyObject     *args,
            PyObject     *kwargs)
{
    GIBaseInfo *info;
    gsize size = 0;
    gpointer boxed;
    PyGIBoxed *self = NULL;

    info = _pygi_object_get_gi_info ( (PyObject *) type, &PyGIBaseInfo_Type);
    if (info == NULL) {
        if (PyErr_ExceptionMatches (PyExc_AttributeError)) {
            PyErr_Format (PyExc_TypeError, "missing introspection information");
        }
        return NULL;
    }

    boxed = _pygi_boxed_alloc (info, &size);
    if (boxed == NULL) {
        goto out;
    }

    self = (PyGIBoxed *) _pygi_boxed_new (type, boxed, FALSE, size);
    if (self == NULL) {
        g_slice_free1 (size, boxed);
        goto out;
    }

    self->size = size;
    self->slice_allocated = TRUE;

out:
    g_base_info_unref (info);

    return (PyObject *) self;
}

static int
_boxed_init (PyObject *self,
             PyObject *args,
             PyObject *kwargs)
{
    static char *kwlist[] = { NULL };

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "", kwlist)) {
        PyErr_Clear ();
        PyErr_Warn (PyExc_TypeError,
                "Passing arguments to gi.types.Boxed.__init__() is deprecated. "
                "All arguments passed will be ignored.");
    }

    /* Don't call PyGBoxed's init, which raises an exception. */
    return 0;
}

PYGLIB_DEFINE_TYPE("gi.Boxed", PyGIBoxed_Type, PyGIBoxed);

PyObject *
_pygi_boxed_new (PyTypeObject *pytype,
                 gpointer      boxed,
                 gboolean      copy_boxed,
                 gsize         allocated_slice)
{
    PyGIBoxed *self;
    GType gtype;

    if (!boxed) {
        Py_RETURN_NONE;
    }

    if (!PyType_IsSubtype (pytype, &PyGIBoxed_Type)) {
        PyErr_SetString (PyExc_TypeError, "must be a subtype of gi.Boxed");
        return NULL;
    }

    gtype = pyg_type_from_object ((PyObject *)pytype);

    /* Boxed objects with slice allocation means they come from caller allocated
     * out arguments. In this case copy_boxed does not make sense because we
     * already own the slice allocated memory and we should be receiving full
     * ownership transfer. */
    if (copy_boxed) {
        g_assert (allocated_slice == 0);
        boxed = g_boxed_copy (gtype, boxed);
    }

    self = (PyGIBoxed *) pytype->tp_alloc (pytype, 0);
    if (self == NULL) {
        return NULL;
    }

    /* We always free on dealloc because we always own the memory due to:
     *   1) copy_boxed == TRUE
     *   2) allocated_slice > 0
     *   3) otherwise the mode is assumed "transfer everything".
     */
    ((PyGBoxed *)self)->free_on_dealloc = TRUE;
    ((PyGBoxed *)self)->gtype = gtype;
    pyg_boxed_set_ptr (self, boxed);

    if (allocated_slice > 0) {
        self->size = allocated_slice;
        self->slice_allocated = TRUE;
    } else {
        self->size = 0;
        self->slice_allocated = FALSE;
    }

    return (PyObject *) self;
}

static PyObject *
_pygi_boxed_get_free_on_dealloc(PyGIBoxed *self, void *closure)
{
  return PyBool_FromLong( ((PyGBoxed *)self)->free_on_dealloc );
}

/**
 * _pygi_boxed_copy_in_place:
 *
 * Replace the boxed pointer held by this wrapper with a boxed copy
 * freeing the previously held pointer (when free_on_dealloc is TRUE).
 * This can be used in cases where Python is passed a reference which
 * it does not own and the wrapper is held by the Python program
 * longer than the duration of a callback it was passed to.
 */
void
_pygi_boxed_copy_in_place (PyGIBoxed *self)
{
    PyGBoxed *pygboxed = (PyGBoxed *)self;
    gpointer copy = g_boxed_copy (pygboxed->gtype, pyg_boxed_get_ptr (self));

    boxed_del (self);
    pyg_boxed_set_ptr (pygboxed, copy);
    pygboxed->free_on_dealloc = TRUE;
}

static PyGetSetDef pygi_boxed_getsets[] = {
    { "_free_on_dealloc", (getter)_pygi_boxed_get_free_on_dealloc, (setter)0 },
    { NULL, 0, 0 }
};

static PyMethodDef boxed_methods[] = {
    { "__del__", (PyCFunction)boxed_del, METH_NOARGS },
    { NULL, NULL, 0 }
};

void
_pygi_boxed_register_types (PyObject *m)
{
    Py_TYPE(&PyGIBoxed_Type) = &PyType_Type;
    PyGIBoxed_Type.tp_base = &PyGBoxed_Type;
    PyGIBoxed_Type.tp_new = (newfunc) _boxed_new;
    PyGIBoxed_Type.tp_init = (initproc) _boxed_init;
    PyGIBoxed_Type.tp_dealloc = (destructor) _boxed_dealloc;
    PyGIBoxed_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    PyGIBoxed_Type.tp_getset = pygi_boxed_getsets;
    PyGIBoxed_Type.tp_methods = boxed_methods;

    if (PyType_Ready (&PyGIBoxed_Type))
        return;
    if (PyModule_AddObject (m, "Boxed", (PyObject *) &PyGIBoxed_Type))
        return;
}

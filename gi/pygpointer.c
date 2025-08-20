/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   pygpointer.c: wrapper for GPointer
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

#include <config.h>

#include <glib-object.h>
#include <pythoncapi_compat.h>

#include "pygi-type.h"
#include "pygi-util.h"
#include "pygpointer.h"

GQuark pygpointer_class_key;

PYGI_DEFINE_TYPE ("gobject.GPointer", PyGPointer_Type, PyGPointer);

static void
pyg_pointer_dealloc (PyGPointer *self)
{
    Py_TYPE (self)->tp_free ((PyObject *)self);
}

static PyObject *
pyg_pointer_richcompare (PyObject *self, PyObject *other, int op)
{
    if (Py_TYPE (self) == Py_TYPE (other))
        return pyg_ptr_richcompare (pyg_pointer_get_ptr (self),
                                    pyg_pointer_get_ptr (other), op);
    else {
        Py_RETURN_NOTIMPLEMENTED;
    }
}

static Py_hash_t
pyg_pointer_hash (PyGPointer *self)
{
    return (Py_hash_t)(gintptr)(pyg_pointer_get_ptr (self));
}

static PyObject *
pyg_pointer_repr (PyGPointer *self)
{
    gchar buf[128];

    g_snprintf (buf, sizeof (buf), "<%s at 0x%" G_GUINTPTR_FORMAT ">",
                g_type_name (self->gtype),
                (guintptr)pyg_pointer_get_ptr (self));
    return PyUnicode_FromString (buf);
}

static int
pyg_pointer_init (PyGPointer *self, PyObject *args, PyObject *kwargs)
{
    gchar buf[512];

    if (!PyArg_ParseTuple (args, ":GPointer.__init__")) return -1;

    pyg_pointer_set_ptr (self, NULL);
    self->gtype = 0;

    g_snprintf (buf, sizeof (buf), "%s can not be constructed",
                Py_TYPE (self)->tp_name);
    PyErr_SetString (PyExc_NotImplementedError, buf);
    return -1;
}

static void
pyg_pointer_free (PyObject *op)
{
    PyObject_Free (op);
}

/**
 * pyg_register_pointer:
 * @dict: the module dictionary to store the wrapper class.
 * @class_name: the Python name for the wrapper class.
 * @pointer_type: the GType of the pointer type being wrapped.
 * @type: the wrapper class.
 *
 * Registers a wrapper for a pointer type.  The wrapper class will be
 * a subclass of gobject.GPointer, and a reference to the wrapper
 * class will be stored in the provided module dictionary.
 */
void
pyg_register_pointer (PyObject *dict, const gchar *class_name,
                      GType pointer_type, PyTypeObject *type)
{
    PyObject *o;

    g_return_if_fail (dict != NULL);
    g_return_if_fail (class_name != NULL);
    g_return_if_fail (pointer_type != 0);

    if (!type->tp_dealloc) type->tp_dealloc = (destructor)pyg_pointer_dealloc;

    Py_SET_TYPE (type, &PyType_Type);
    g_assert (Py_TYPE (&PyGPointer_Type) != NULL);
    type->tp_base = &PyGPointer_Type;

    if (PyType_Ready (type) < 0) {
        g_warning ("could not get type `%s' ready", type->tp_name);
        return;
    }

    PyDict_SetItemString (type->tp_dict, "__gtype__",
                          o = pyg_type_wrapper_new (pointer_type));
    Py_DECREF (o);

    g_type_set_qdata (pointer_type, pygpointer_class_key, type);

    PyDict_SetItemString (dict, (char *)class_name, (PyObject *)type);
}

/**
 * pyg_pointer_new:
 * @pointer_type: the GType of the pointer value.
 * @pointer: the pointer value.
 *
 * Creates a wrapper for a pointer value.  Since G_TYPE_POINTER types
 * don't register any information about how to copy/free them, there
 * is no guarantee that the pointer will remain valid, and there is
 * nothing registered to release the pointer when the pointer goes out
 * of scope.  This is why we don't recommend people use these types.
 *
 * Returns: the boxed wrapper.
 */
PyObject *
pyg_pointer_new (GType pointer_type, gpointer pointer)
{
    PyGILState_STATE state;
    PyGPointer *self;
    PyTypeObject *tp;
    g_return_val_if_fail (pointer_type != 0, NULL);

    state = PyGILState_Ensure ();

    if (!pointer) {
        Py_INCREF (Py_None);
        PyGILState_Release (state);
        return Py_None;
    }

    tp = g_type_get_qdata (pointer_type, pygpointer_class_key);

    if (!tp) tp = (PyTypeObject *)pygi_type_import_by_g_type (pointer_type);

    if (!tp) tp = (PyTypeObject *)&PyGPointer_Type; /* fallback */
    self = PyObject_New (PyGPointer, tp);

    PyGILState_Release (state);

    if (self == NULL) return NULL;

    pyg_pointer_set_ptr (self, pointer);
    self->gtype = pointer_type;

    return (PyObject *)self;
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_pointer_register_types (PyObject *d)
{
    PyObject *pygtype;

    pygpointer_class_key = g_quark_from_static_string ("PyGPointer::class");

    PyGPointer_Type.tp_dealloc = (destructor)pyg_pointer_dealloc;
    PyGPointer_Type.tp_richcompare = pyg_pointer_richcompare;
    PyGPointer_Type.tp_repr = (reprfunc)pyg_pointer_repr;
    PyGPointer_Type.tp_hash = (hashfunc)pyg_pointer_hash;
    PyGPointer_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    PyGPointer_Type.tp_init = (initproc)pyg_pointer_init;
    PyGPointer_Type.tp_free = (freefunc)pyg_pointer_free;
    PyGPointer_Type.tp_alloc = PyType_GenericAlloc;
    PyGPointer_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready (&PyGPointer_Type)) return -1;

    pygtype = pyg_type_wrapper_new (G_TYPE_POINTER);
    PyDict_SetItemString (PyGPointer_Type.tp_dict, "__gtype__", pygtype);
    Py_DECREF (pygtype);

    PyDict_SetItemString (d, "GPointer", (PyObject *)&PyGPointer_Type);

    return 0;
}

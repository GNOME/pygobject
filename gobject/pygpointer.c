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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <pyglib.h>
#if HAVE_PYGI_H
#   include <pygi.h>
#endif
#include "pygobject-private.h"
#include "pygpointer.h"

GQuark pygpointer_class_key;

PYGLIB_DEFINE_TYPE("gobject.GPointer", PyGPointer_Type, PyGPointer);

static void
pyg_pointer_dealloc(PyGPointer *self)
{
    if (self->free_on_dealloc) {
        g_free(self->pointer);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
pyg_pointer_compare(PyGPointer *self, PyGPointer *v)
{
    if (self->pointer == v->pointer) return 0;
    if (self->pointer > v->pointer)  return -1;
    return 1;
}

static long
pyg_pointer_hash(PyGPointer *self)
{
    return (long)self->pointer;
}

static PyObject *
pyg_pointer_repr(PyGPointer *self)
{
    gchar buf[128];

    g_snprintf(buf, sizeof(buf), "<%s at 0x%lx>", g_type_name(self->gtype),
	       (long)self->pointer);
    return _PyUnicode_FromString(buf);
}

static void
pyg_pointer_free(PyObject *op)
{
  PyObject_FREE(op);
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
pyg_register_pointer(PyObject *dict, const gchar *class_name,
		     GType pointer_type, PyTypeObject *type)
{
    PyObject *o;

    g_return_if_fail(dict != NULL);
    g_return_if_fail(class_name != NULL);
    g_return_if_fail(pointer_type != 0);

    if (!type->tp_dealloc) type->tp_dealloc = (destructor)pyg_pointer_dealloc;

    Py_TYPE(type) = &PyType_Type;
    type->tp_base = &PyGPointer_Type;

    if (PyType_Ready(type) < 0) {
	g_warning("could not get type `%s' ready", type->tp_name);
	return;
    }

    PyDict_SetItemString(type->tp_dict, "__gtype__",
			 o=pyg_type_wrapper_new(pointer_type));
    Py_DECREF(o);

    g_type_set_qdata(pointer_type, pygpointer_class_key, type);

    PyDict_SetItemString(dict, (char *)class_name, (PyObject *)type);
}

PyObject *
pyg_pointer_new_from_type (PyTypeObject *type,
                           gpointer      pointer,
                           gboolean      free_on_dealloc)
{
    PyGPointer *self;
    GType g_type;

    if (!PyType_IsSubtype(type, &PyGPointer_Type)) {
        PyErr_SetString(PyExc_TypeError, "must be a subtype of gobject.GPointer");
        return NULL;
    }

    self = (PyGPointer *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    g_type = pyg_type_from_object((PyObject *)type);

    self->pointer = pointer;
    self->gtype = g_type;
    self->free_on_dealloc = free_on_dealloc;

    return (PyObject *)self;
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
pyg_pointer_new(GType pointer_type, gpointer pointer)
{
    PyGILState_STATE state;
    PyGPointer *self;
    PyTypeObject *tp;
    g_return_val_if_fail(pointer_type != 0, NULL);

    state = pyglib_gil_state_ensure();

    if (!pointer) {
	Py_INCREF(Py_None);
	pyglib_gil_state_release(state);
	return Py_None;
    }

    tp = g_type_get_qdata(pointer_type, pygpointer_class_key);

#if HAVE_PYGI_H
    if (tp == NULL) {
        GIRepository *repository;
        GIBaseInfo *info;

        repository = g_irepository_get_default();

        info = g_irepository_find_by_gtype(repository, pointer_type);

        if (info != NULL) {
            if (pygi_import() < 0) {
                PyErr_WarnEx(NULL, "unable to import gi", 1);
                PyErr_Clear();
            } else {
                tp = (PyTypeObject *)pygi_type_find_by_gi_info(info);
                g_base_info_unref(info);
                if (tp == NULL) {
                    PyErr_Clear();
                } else {
                    /* Note: The type is registered, so at least a reference remains. */
                    Py_DECREF((PyObject *)tp);
                }
            }
        }
    }
#endif

    if (!tp)
	tp = (PyTypeObject *)&PyGPointer_Type; /* fallback */

    self = (PyGPointer *)pyg_pointer_new_from_type(tp, pointer, FALSE);

    pyglib_gil_state_release(state);

    if (self == NULL) {
        return NULL;
    }

    /* In case the g_type has no wrapper, we don't want self->gtype to be G_TYPE_POINTER. */
    self->gtype = pointer_type;

    return (PyObject *)self;
}


#if HAVE_PYGI_H
static PyObject *
_pyg_pointer_new (PyTypeObject *type,
                  PyObject     *args,
                  PyObject     *kwargs)
{
    static char *kwlist[] = { NULL };

    GIBaseInfo *info;
    gboolean is_simple;
    gsize size;
    gpointer pointer;
    PyObject *self = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist)) {
        return NULL;
    }

    if (pygi_import() < 0) {
        return NULL;
    }

    info = pygi_object_get_gi_info((PyObject *)type, &PyGIStructInfo_Type);
    if (info == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Format(PyExc_TypeError, "cannot create '%s' instances", type->tp_name);
        }
        return NULL;
    }

    is_simple = pygi_g_struct_info_is_simple((GIStructInfo *)info);
    if (!is_simple) {
        PyErr_Format(PyExc_TypeError, "cannot create '%s' instances", type->tp_name);
        goto out;
    }

    size = g_struct_info_get_size((GIStructInfo *)info);
    pointer = g_try_malloc(size);
    if (pointer == NULL) {
        PyErr_NoMemory();
        goto out;
    }

    self = pyg_pointer_new_from_type(type, pointer, TRUE);
    if (self == NULL) {
        g_free(pointer);
    }

out:
    g_base_info_unref(info);

    return (PyObject *)self;
}
#endif

void
pygobject_pointer_register_types(PyObject *d)
{
    pygpointer_class_key     = g_quark_from_static_string("PyGPointer::class");

    PyGPointer_Type.tp_dealloc = (destructor)pyg_pointer_dealloc;
    PyGPointer_Type.tp_compare = (cmpfunc)pyg_pointer_compare;
    PyGPointer_Type.tp_repr = (reprfunc)pyg_pointer_repr;
    PyGPointer_Type.tp_hash = (hashfunc)pyg_pointer_hash;
    PyGPointer_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    PyGPointer_Type.tp_free = (freefunc)pyg_pointer_free;

#if HAVE_PYGI_H
    PyGPointer_Type.tp_new = (newfunc)_pyg_pointer_new;
#endif

    PYGOBJECT_REGISTER_GTYPE(d, PyGPointer_Type, "GPointer", G_TYPE_POINTER); 

#if !HAVE_PYGI_H
    /* We don't want instances to be created in Python, but
     * PYGOBJECT_REGISTER_GTYPE assigned PyObject_GenericNew as instance
     * constructor. It's not too late to revert it to NULL, though. */
    PyGPointer_Type.tp_new = (newfunc)NULL;
#endif
}

/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   pygboxed.c: wrapper for GBoxed
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
#    include <pygi.h>
#endif
#include "pygobject-private.h"
#include "pygboxed.h"

GQuark pygboxed_type_key;
GQuark pygboxed_marshal_key;

PYGLIB_DEFINE_TYPE("gobject.GBoxed", PyGBoxed_Type, PyGBoxed);

static void
pyg_boxed_dealloc(PyGBoxed *self)
{
    if (self->free_on_dealloc && self->boxed) {
	PyGILState_STATE state = pyglib_gil_state_ensure();
        if (!g_type_is_a(self->gtype, G_TYPE_BOXED)) {
            /* Void pointers are also allowed. */
            g_free(self->boxed);
        } else {
            g_boxed_free(self->gtype, self->boxed);
        }
	pyglib_gil_state_release(state);
    }

    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
pyg_boxed_compare(PyGBoxed *self, PyGBoxed *v)
{
    if (self->boxed == v->boxed) return 0;
    if (self->boxed > v->boxed)  return -1;
    return 1;
}

static long
pyg_boxed_hash(PyGBoxed *self)
{
    return (long)self->boxed;
}

static PyObject *
pyg_boxed_repr(PyGBoxed *self)
{
    return PyString_FromFormat("<%s object at 0x%p (%s at 0x%p)>",
            self->ob_type->tp_name, self, g_type_name(self->gtype), self->boxed);
}

static void
pyg_boxed_free(PyObject *op)
{
  PyObject_FREE(op);
}

static PyObject *
pyg_boxed_copy(PyGBoxed *self)
{
    return pyg_boxed_new (self->gtype, self->boxed, TRUE, TRUE);
}


static PyMethodDef pygboxed_methods[] = {
    { "copy", (PyCFunction) pyg_boxed_copy, METH_NOARGS },
    { NULL, NULL, 0 }
};


void
pyg_register_boxed_type (GType         g_type,
                         PyTypeObject *type)
{
    Py_INCREF((PyObject *)type);
    g_type_set_qdata(g_type, pygboxed_type_key, type);
}

/**
 * pyg_register_boxed:
 * @dict: the module dictionary to store the wrapper class.
 * @class_name: the Python name for the wrapper class.
 * @boxed_type: the GType of the boxed type being wrapped.
 * @type: the wrapper class.
 *
 * Registers a wrapper for a boxed type.  The wrapper class will be a
 * subclass of gobject.GBoxed, and a reference to the wrapper class
 * will be stored in the provided module dictionary.
 */
void
pyg_register_boxed(PyObject *dict, const gchar *class_name,
		   GType boxed_type, PyTypeObject *type)
{
    PyObject *o;

    g_return_if_fail(dict != NULL);
    g_return_if_fail(class_name != NULL);
    g_return_if_fail(boxed_type != 0);

    if (!type->tp_dealloc)  type->tp_dealloc  = (destructor)pyg_boxed_dealloc;

    Py_TYPE(type) = &PyType_Type;
    type->tp_base = &PyGBoxed_Type;

    if (PyType_Ready(type) < 0) {
	g_warning("could not get type `%s' ready", type->tp_name);
	return;
    }

    PyDict_SetItemString(type->tp_dict, "__gtype__",
			 o=pyg_type_wrapper_new(boxed_type));
    Py_DECREF(o);

    pyg_register_boxed_type (boxed_type, type);

    PyDict_SetItemString(dict, (char *)class_name, (PyObject *)type);

    Py_DECREF((PyObject *)type);
}

/**
 * pyg_boxed_new:
 * @boxed_type: the GType of the boxed value.
 * @boxed: the boxed value.
 * @copy_boxed: whether the new boxed wrapper should hold a copy of the value.
 * @own_ref: whether the boxed wrapper should own the boxed value.
 *
 * Creates a wrapper for a boxed value.  If @copy_boxed is set to
 * True, the wrapper will hold a copy of the value, instead of the
 * value itself.  If @own_ref is True, then the value held by the
 * wrapper will be freed when the wrapper is deallocated.  If
 * @copy_boxed is True, then @own_ref must also be True.
 *
 * Returns: the boxed wrapper.
 */
PyObject *
pyg_boxed_new(GType boxed_type, gpointer boxed, gboolean copy_boxed,
	      gboolean own_ref)
{
    PyGILState_STATE state;
    PyGBoxed *self;
    PyTypeObject *tp;

    g_return_val_if_fail(boxed_type != 0, NULL);
    g_return_val_if_fail(!copy_boxed || (copy_boxed && own_ref), NULL);

    state = pyglib_gil_state_ensure();

    if (!boxed) {
	Py_INCREF(Py_None);
	pyglib_gil_state_release(state);
	return Py_None;
    }

    tp = g_type_get_qdata(boxed_type, pygboxed_type_key);
    if (!tp)
	tp = (PyTypeObject *)&PyGBoxed_Type; /* fallback */
    self = PyObject_NEW(PyGBoxed, tp);

    if (self == NULL) {
	pyglib_gil_state_release(state);
        return NULL;
    }

    if (copy_boxed)
	boxed = g_boxed_copy(boxed_type, boxed);
    self->boxed = boxed;
    self->gtype = boxed_type;
    self->free_on_dealloc = own_ref;

    pyglib_gil_state_release(state);
    
    return (PyObject *)self;
}

#if HAVE_PYGI_H
static PyObject *
pyg_boxed_new_ (PyTypeObject *type,
                PyObject     *args,
                PyObject     *kwds)
{
    static char *kwlist[] = { NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) {
        return NULL;
    }

    if (pygi_import() < 0) {
        return NULL;
    }

    return pygi_boxed_new_from_type(type, NULL, TRUE);
}
#endif /* HAVE_PYGI_H */

void
pygobject_boxed_register_types(PyObject *d)
{
    pygboxed_type_key        = g_quark_from_static_string("PyGBoxed::class");
    pygboxed_marshal_key     = g_quark_from_static_string("PyGBoxed::marshal");

    PyGBoxed_Type.tp_dealloc = (destructor)pyg_boxed_dealloc;
    PyGBoxed_Type.tp_compare = (cmpfunc)pyg_boxed_compare;
    PyGBoxed_Type.tp_repr = (reprfunc)pyg_boxed_repr;
    PyGBoxed_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    PyGBoxed_Type.tp_methods = pygboxed_methods;
    PyGBoxed_Type.tp_free = (freefunc)pyg_boxed_free;
    PyGBoxed_Type.tp_hash = (hashfunc)pyg_boxed_hash;
#if HAVE_PYGI_H
    PyGBoxed_Type.tp_new = (newfunc)pyg_boxed_new_;
#endif

    PYGOBJECT_REGISTER_GTYPE(d, PyGBoxed_Type, "GBoxed", G_TYPE_BOXED);

#if !HAVE_PYGI_H
    /* We don't want instances to be created in Python, but
     * PYGOBJECT_REGISTER_GTYPE assigned PyObject_GenericNew as instance
     * constructor. It's not too late to revert it to NULL, though. */
    PyGBoxed_Type.tp_new = (newfunc)NULL;
#endif
}

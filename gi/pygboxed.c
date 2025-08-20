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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <glib-object.h>
#include <pythoncapi_compat.h>

#include "pygboxed.h"
#include "pygi-type.h"
#include "pygi-util.h"

GQuark pygboxed_type_key;

PYGI_DEFINE_TYPE ("gobject.GBoxed", PyGBoxed_Type, PyGBoxed);

static void
gboxed_dealloc (PyGBoxed *self)
{
    if (self->free_on_dealloc && pyg_boxed_get_ptr (self)) {
        PyGILState_STATE state = PyGILState_Ensure ();
        g_boxed_free (self->gtype, pyg_boxed_get_ptr (self));
        PyGILState_Release (state);
    }

    Py_TYPE (self)->tp_free ((PyObject *)self);
}

static PyObject *
gboxed_richcompare (PyObject *self, PyObject *other, int op)
{
    if (Py_TYPE (self) == Py_TYPE (other)
        && PyObject_IsInstance (self, (PyObject *)&PyGBoxed_Type))
        return pyg_ptr_richcompare (pyg_boxed_get_ptr (self),
                                    pyg_boxed_get_ptr (other), op);
    else {
        Py_RETURN_NOTIMPLEMENTED;
    }
}

static Py_hash_t
gboxed_hash (PyGBoxed *self)
{
    return (Py_hash_t)(gintptr)(pyg_boxed_get_ptr (self));
}

static PyObject *
gboxed_repr (PyGBoxed *boxed)
{
    PyObject *module, *repr, *self = (PyObject *)boxed;
    gchar *module_str, *namespace;

    module = PyObject_GetAttrString (self, "__module__");
    if (module == NULL) return NULL;

    if (!PyUnicode_Check (module)) {
        Py_DECREF (module);
        return NULL;
    }

    module_str = PyUnicode_AsUTF8 (module);
    namespace = g_strrstr (module_str, ".");
    if (namespace == NULL) {
        namespace = module_str;
    } else {
        namespace += 1;
    }

    repr = PyUnicode_FromFormat (
        "<%s.%s object at %p (%s at %p)>", namespace, Py_TYPE (self)->tp_name,
        self, g_type_name (boxed->gtype), pyg_boxed_get_ptr (boxed));
    Py_DECREF (module);
    return repr;
}

static int
gboxed_init (PyGBoxed *self, PyObject *args, PyObject *kwargs)
{
    gchar buf[512];

    if (!PyArg_ParseTuple (args, ":GBoxed.__init__")) return -1;

    pyg_boxed_set_ptr (self, NULL);
    self->gtype = 0;
    self->free_on_dealloc = FALSE;

    g_snprintf (buf, sizeof (buf), "%s can not be constructed",
                Py_TYPE (self)->tp_name);
    PyErr_SetString (PyExc_NotImplementedError, buf);
    return -1;
}

static void
gboxed_free (PyObject *op)
{
    PyObject_Free (op);
}

static PyObject *
gboxed_copy (PyGBoxed *self)
{
    return pygi_gboxed_new (self->gtype, pyg_boxed_get_ptr (self), TRUE, TRUE);
}

static PyMethodDef pygboxed_methods[] = {
    { "copy", (PyCFunction)gboxed_copy, METH_NOARGS },
    { NULL, NULL, 0 },
};


/**
 * pygi_register_gboxed:
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
pygi_register_gboxed (PyObject *dict, const gchar *class_name,
                      GType boxed_type, PyTypeObject *type)
{
    PyObject *o;

    g_return_if_fail (dict != NULL);
    g_return_if_fail (class_name != NULL);
    g_return_if_fail (boxed_type != 0);

    if (!type->tp_dealloc) type->tp_dealloc = (destructor)gboxed_dealloc;

    Py_SET_TYPE (type, &PyType_Type);
    g_assert (Py_TYPE (&PyGBoxed_Type) != NULL);
    type->tp_base = &PyGBoxed_Type;

    if (PyType_Ready (type) < 0) {
        g_warning ("could not get type `%s' ready", type->tp_name);
        return;
    }

    PyDict_SetItemString (type->tp_dict, "__gtype__",
                          o = pyg_type_wrapper_new (boxed_type));
    Py_DECREF (o);

    g_type_set_qdata (boxed_type, pygboxed_type_key, type);

    PyDict_SetItemString (dict, (char *)class_name, (PyObject *)type);
}

/**
 * pygi_gboxed_new:
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
 * Returns: the boxed wrapper or %NULL and sets an exception.
 */
PyObject *
pygi_gboxed_new (GType boxed_type, gpointer boxed, gboolean copy_boxed,
                 gboolean own_ref)
{
    PyGILState_STATE state;
    PyGBoxed *self;
    PyTypeObject *tp;

    g_return_val_if_fail (boxed_type != 0, NULL);
    g_return_val_if_fail (!copy_boxed || (copy_boxed && own_ref), NULL);

    state = PyGILState_Ensure ();

    if (!boxed) {
        Py_INCREF (Py_None);
        PyGILState_Release (state);
        return Py_None;
    }

    tp = g_type_get_qdata (boxed_type, pygboxed_type_key);

    if (!tp) tp = (PyTypeObject *)pygi_type_import_by_g_type (boxed_type);

    if (!tp) tp = (PyTypeObject *)&PyGBoxed_Type; /* fallback */

    if (!PyType_IsSubtype (tp, &PyGBoxed_Type)) {
        PyErr_Format (PyExc_RuntimeError, "%s isn't a GBoxed", tp->tp_name);
        PyGILState_Release (state);
        return NULL;
    }

    self = (PyGBoxed *)tp->tp_alloc (tp, 0);

    if (self == NULL) {
        PyGILState_Release (state);
        return NULL;
    }

    if (copy_boxed) boxed = g_boxed_copy (boxed_type, boxed);
    pyg_boxed_set_ptr (self, boxed);
    self->gtype = boxed_type;
    self->free_on_dealloc = own_ref;

    PyGILState_Release (state);

    return (PyObject *)self;
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_gboxed_register_types (PyObject *d)
{
    PyObject *pygtype;

    pygboxed_type_key = g_quark_from_static_string ("PyGBoxed::class");

    PyGBoxed_Type.tp_dealloc = (destructor)gboxed_dealloc;
    PyGBoxed_Type.tp_richcompare = gboxed_richcompare;
    PyGBoxed_Type.tp_repr = (reprfunc)gboxed_repr;
    PyGBoxed_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    PyGBoxed_Type.tp_methods = pygboxed_methods;
    PyGBoxed_Type.tp_init = (initproc)gboxed_init;
    PyGBoxed_Type.tp_free = (freefunc)gboxed_free;
    PyGBoxed_Type.tp_hash = (hashfunc)gboxed_hash;
    PyGBoxed_Type.tp_alloc = PyType_GenericAlloc;
    PyGBoxed_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready (&PyGBoxed_Type)) return -1;

    pygtype = pyg_type_wrapper_new (G_TYPE_POINTER);
    PyDict_SetItemString (PyGBoxed_Type.tp_dict, "__gtype__", pygtype);
    Py_DECREF (pygtype);

    PyDict_SetItemString (d, "GBoxed", (PyObject *)&PyGBoxed_Type);

    return 0;
}

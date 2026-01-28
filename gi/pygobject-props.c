/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   pygobject.c: wrapper for the GObject type.
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

#include "pygi-fundamental.h"
#include "pygi-property.h"
#include "pygi-type.h"
#include "pygi-value.h"
#include "pygi-util.h"
#include "pygobject-object.h"
#include "pygobject-props.h"

typedef struct {
    PyObject_HEAD
    GParamSpec **props;
    guint n_props;
    guint index;
} PyGPropsIter;

PYGI_DEFINE_TYPE ("gi._gi.GPropsIter", PyGPropsIter_Type, PyGPropsIter);

static void
pyg_props_iter_dealloc (PyGPropsIter *self)
{
    g_free (self->props);
    PyObject_Free ((PyObject *)self);
}

static PyObject *
pygobject_props_iter_next (PyGPropsIter *iter)
{
    if (iter->index < iter->n_props)
        return pygi_fundamental_new (iter->props[iter->index++]);
    else {
        PyErr_SetNone (PyExc_StopIteration);
        return NULL;
    }
}

typedef struct {
    PyObject_HEAD
    /* a reference to the object containing the properties */
    PyGObject *pygobject;
    GType gtype;
} PyGProps;

PYGI_DEFINE_TYPE ("gi._gi.GProps", PyGProps_Type, PyGProps);

static void
PyGProps_dealloc (PyGProps *self)
{
    PyObject_GC_UnTrack ((PyObject *)self);

    Py_CLEAR (self->pygobject);

    PyObject_GC_Del ((PyObject *)self);
}

/* Copied from glib. gobject uses hyphens in property names, but in Python
 * we can only represent hyphens as underscores. Convert underscores to
 * hyphens for glib compatibility. */
static void
canonicalize_key (gchar *key)
{
    gchar *p;

    for (p = key; *p != 0; p++) {
        gchar c = *p;

        if (c != '-' && (c < '0' || c > '9') && (c < 'A' || c > 'Z')
            && (c < 'a' || c > 'z'))
            *p = '-';
    }
}

static PyObject *
PyGProps_getattro (PyGProps *self, PyObject *attr)
{
    char *attr_name, *property_name;
    GObjectClass *class;
    GParamSpec *pspec;

    attr_name = PyUnicode_AsUTF8 (attr);
    if (!attr_name) {
        PyErr_Clear ();
        return PyObject_GenericGetAttr ((PyObject *)self, attr);
    }

    class = g_type_class_ref (self->gtype);

    /* g_object_class_find_property recurses through the class hierarchy,
     * so the resulting pspec tells us the owner_type that owns the property
     * we're dealing with. */
    property_name = g_strdup (attr_name);
    canonicalize_key (property_name);
    pspec = g_object_class_find_property (class, property_name);
    g_free (property_name);
    g_type_class_unref (class);

    if (!pspec) {
        return PyObject_GenericGetAttr ((PyObject *)self, attr);
    }

    if (!self->pygobject) {
        /* If we're doing it without an instance, return a GParamSpec */
        return pygi_fundamental_new (pspec);
    }

    return pygi_get_property_value (self->pygobject, pspec);
}

static int
PyGProps_setattro (PyGProps *self, PyObject *attr, PyObject *pvalue)
{
    GParamSpec *pspec;
    char *attr_name, *property_name;
    GObject *obj;

    if (pvalue == NULL) {
        PyErr_SetString (PyExc_TypeError,
                         "properties cannot be "
                         "deleted");
        return -1;
    }

    attr_name = PyUnicode_AsUTF8 (attr);
    if (!attr_name) {
        PyErr_Clear ();
        return PyObject_GenericSetAttr ((PyObject *)self, attr, pvalue);
    }

    if (!self->pygobject) {
        PyErr_SetString (PyExc_TypeError,
                         "cannot set GOject properties without an instance");
        return -1;
    }

    obj = self->pygobject->obj;

    property_name = g_strdup (attr_name);
    canonicalize_key (property_name);

    /* g_object_class_find_property recurses through the class hierarchy,
     * so the resulting pspec tells us the owner_type that owns the property
     * we're dealing with. */
    pspec =
        g_object_class_find_property (G_OBJECT_GET_CLASS (obj), property_name);
    g_free (property_name);
    if (!pspec) {
        return PyObject_GenericSetAttr ((PyObject *)self, attr, pvalue);
    }

    pygi_set_property_value (self->pygobject, pspec, pvalue);
    if (PyErr_Occurred ()) return -1;

    return 0;
}

static int
pygobject_props_traverse (PyGProps *self, visitproc visit, void *arg)
{
    if (self->pygobject && visit ((PyObject *)self->pygobject, arg) < 0)
        return -1;
    return 0;
}

static PyObject *
pygobject_props_get_iter (PyGProps *self)
{
    PyGPropsIter *iter;
    GObjectClass *class;

    iter = PyObject_New (PyGPropsIter, &PyGPropsIter_Type);
    class = g_type_class_ref (self->gtype);
    iter->props = g_object_class_list_properties (class, &iter->n_props);
    iter->index = 0;
    g_type_class_unref (class);
    return (PyObject *)iter;
}

static PyObject *
pygobject_props_dir (PyGProps *self)
{
    GObjectClass *class;
    GParamSpec **props;
    guint n_props = 0, i;
    PyObject *prop_str;
    PyObject *props_list;

    class = g_type_class_ref (self->gtype);
    props = g_object_class_list_properties (class, &n_props);
    props_list = PyList_New (n_props);

    for (i = 0; i < n_props; i++) {
        char *name;
        name = g_strdup (g_param_spec_get_name (props[i]));
        /* hyphens cannot belong in identifiers */
        g_strdelimit (name, "-", '_');
        prop_str = PyUnicode_FromString (name);

        PyList_SetItem (props_list, i, prop_str);
        g_free (name);
    }

    g_type_class_unref (class);

    if (props) g_free (props);

    return props_list;
}

static PyMethodDef pygobject_props_methods[] = {
    { "__dir__", (PyCFunction)pygobject_props_dir, METH_NOARGS },
    { NULL, NULL, 0 },
};


static Py_ssize_t
PyGProps_length (PyGProps *self)
{
    GObjectClass *class;
    GParamSpec **props;
    guint n_props;

    class = g_type_class_ref (self->gtype);
    props = g_object_class_list_properties (class, &n_props);
    g_type_class_unref (class);
    g_free (props);

    return (Py_ssize_t)n_props;
}

static PySequenceMethods _PyGProps_as_sequence = {
    (lenfunc)PyGProps_length, 0, 0, 0, 0, 0, 0,
};

PYGI_DEFINE_TYPE ("gi._gi.GPropsDescr", PyGPropsDescr_Type, PyObject);

static PyObject *
pyg_props_descr_descr_get (PyObject *self, PyObject *obj, PyObject *type)
{
    PyGProps *gprops;

    gprops = PyObject_GC_New (PyGProps, &PyGProps_Type);
    if (obj == NULL || Py_IsNone (obj)) {
        gprops->pygobject = NULL;
        gprops->gtype = pyg_type_from_object (type);
    } else {
        if (!PyObject_IsInstance (obj, (PyObject *)&PyGObject_Type)) {
            PyErr_SetString (PyExc_TypeError,
                             "cannot use GObject property"
                             " descriptor on non-GObject instances");
            return NULL;
        }
        gprops->pygobject = (PyGObject *)Py_NewRef (obj);
        gprops->gtype = pyg_type_from_object (obj);
    }
    return (PyObject *)gprops;
}

int
pyg_object_props_register_types (PyObject *d)
{
    /* GProps */
    PyGProps_Type.tp_dealloc = (destructor)PyGProps_dealloc;
    PyGProps_Type.tp_as_sequence = (PySequenceMethods *)&_PyGProps_as_sequence;
    PyGProps_Type.tp_getattro = (getattrofunc)PyGProps_getattro;
    PyGProps_Type.tp_setattro = (setattrofunc)PyGProps_setattro;
    PyGProps_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
    PyGProps_Type.tp_doc =
        "The properties of the GObject accessible as "
        "Python attributes.";
    PyGProps_Type.tp_traverse = (traverseproc)pygobject_props_traverse;
    PyGProps_Type.tp_iter = (getiterfunc)pygobject_props_get_iter;
    PyGProps_Type.tp_methods = pygobject_props_methods;
    if (PyType_Ready (&PyGProps_Type) < 0) return -1;

    /* GPropsDescr */
    PyGPropsDescr_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGPropsDescr_Type.tp_descr_get = pyg_props_descr_descr_get;
    if (PyType_Ready (&PyGPropsDescr_Type) < 0) return -1;


    /* GPropsIter */
    PyGPropsIter_Type.tp_dealloc = (destructor)pyg_props_iter_dealloc;
    PyGPropsIter_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGPropsIter_Type.tp_doc = "GObject properties iterator";
    PyGPropsIter_Type.tp_iter = PyObject_SelfIter;
    PyGPropsIter_Type.tp_iternext = (iternextfunc)pygobject_props_iter_next;
    if (PyType_Ready (&PyGPropsIter_Type) < 0) return -1;

    return 0;
}

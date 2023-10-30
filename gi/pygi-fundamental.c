/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2009 Simon van der Linden <svdlinden@src.gnome.org>
 * Copyright (C) 2010 Tomeu Vizoso <tomeu.vizoso@collabora.co.uk>
 * Copyright (C) 2012 Bastian Winkler <buz@netbuz.org>
 *
 *   pygi-fundamental.c: wrapper to handle instances of fundamental types.
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


#include <girepository.h>

#include "pygobject-internal.h"
#include "pygi-info.h"
#include "pygi-util.h"
#include "pygi-fundamental.h"
#include "pygobject-object.h" // for pygobject_lookup_class


static PyGIFundamental *
_pygi_fundamental_new_internal (PyTypeObject *type,
                                gpointer      pointer);


static void
fundamental_dealloc (PyGIFundamental *self)
{
    pygi_fundamental_unref (self);
    self->instance = NULL;

    PyObject_GC_UnTrack ((PyObject *) self);
    PyObject_ClearWeakRefs ((PyObject *) self);

    Py_TYPE (self)->tp_free ((PyObject *) self);
}

static PyObject *
fundamental_new (PyTypeObject *type,
                 PyObject     *args,
                 PyObject     *kwargs)
{
    static char *kwlist[] = { NULL };

    GIBaseInfo *info;
    gpointer pointer;
    PyGIFundamental *self = NULL;
    GType g_type;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "", kwlist)) {
        return NULL;
    }

    info = _pygi_object_get_gi_info ((PyObject *) type, &PyGIObjectInfo_Type);
    if (info == NULL) {
        if (PyErr_ExceptionMatches (PyExc_AttributeError)) {
            PyErr_Format (PyExc_TypeError, "missing introspection information");
        }
        return NULL;
    }

    g_type = pyg_type_from_object ((PyObject *) type);
    if (G_TYPE_IS_ABSTRACT (g_type)) {
        PyErr_Format (PyExc_TypeError, "cannot instantiate abstract type %s", g_type_name (g_type));
        return NULL;
    }

    pointer = g_type_create_instance (g_type);
    if (pointer == NULL) {
        PyErr_NoMemory();
        goto out;
    }

    self = _pygi_fundamental_new_internal (type, pointer);
    if (self == NULL) {
        g_free (pointer);
        PyErr_Format (PyExc_TypeError, "cannot instantiate Fundamental Python wrapper type %s", g_type_name (g_type));
        goto out;
    }

out:
    g_base_info_unref (info);

    return (PyObject *) self;
}

static PyObject*
fundamental_richcompare (PyObject *self,
                         PyObject *other,
                         int op)
{
    if (Py_TYPE (self) == Py_TYPE (other)) {
        return pyg_ptr_richcompare (((PyGIFundamental*) self)->instance,
                                    ((PyGIFundamental*) other)->instance,
                                    op);
    } else {
        Py_INCREF (Py_NotImplemented);
        return Py_NotImplemented;
    }
}

static Py_hash_t
fundamental_hash (PyGIFundamental *self)
{
    return (Py_hash_t)(gintptr)(self->instance);
}

static PyObject *
fundamental_repr (PyGIFundamental *self)
{
    gchar buf[128];

    g_snprintf (buf, sizeof(buf), "<%s at 0x%" G_GUINTPTR_FORMAT ">",
                g_type_name (self->gtype),
                (guintptr) self->instance);
    return PyUnicode_FromString (buf);
}

PYGI_DEFINE_TYPE("gi.Fundamental", PyGIFundamental_Type, PyGIFundamental);


PyObject *
pygi_fundamental_new (gpointer instance)
{
    GType gtype;
    PyTypeObject *type;
    PyGIFundamental *self;

    if (!instance) {
        Py_RETURN_NONE;
    }

    gtype = G_TYPE_FROM_INSTANCE (instance);
    type = pygobject_lookup_class (gtype);

    self = _pygi_fundamental_new_internal (type, instance);
    pygi_fundamental_ref (self);
    return (PyObject *) self;
}

static PyGIFundamental *
_pygi_fundamental_new_internal (PyTypeObject *type,
                                gpointer      instance)
{
    PyGIFundamental *self;
    GIObjectInfo *info;

    if (!PyType_IsSubtype (type, &PyGIFundamental_Type)) {
        PyErr_SetString (PyExc_TypeError, "must be a subtype of gi.Fundamental");
        return NULL;
    }

    info = _pygi_object_get_gi_info ( (PyObject *) type, &PyGIObjectInfo_Type);
    if (info == NULL) {
        if (PyErr_ExceptionMatches (PyExc_AttributeError)) {
            PyErr_Format (PyExc_TypeError, "missing introspection information");
        }
        return NULL;
    }

    self = (PyGIFundamental *) type->tp_alloc (type, 0);
    if (self == NULL) {
        return NULL;
    }

    self->gtype = pyg_type_from_object ((PyObject *) type);
    self->instance = instance;

    self->ref_func = g_object_info_get_ref_function_pointer (info);
    self->unref_func = g_object_info_get_unref_function_pointer (info);

    g_base_info_unref (info);

    return self;
}

void
pygi_fundamental_ref (PyGIFundamental *self)
{
    if (self->ref_func && self->instance)
        self->ref_func (self->instance);
}

void
pygi_fundamental_unref (PyGIFundamental *self)
{
    if (self->unref_func && self->instance)
        self->unref_func (self->instance);
}

GTypeInstance*
pygi_fundamental_get (PyObject *self)
{
    if (PyObject_TypeCheck (self, &PyGIFundamental_Type)) {
        return ((PyGIFundamental *) self)->instance;
    } else {
        PyErr_SetString (PyExc_TypeError, "Expected GObject Fundamental type");
        return NULL;
    }
}

int
pygi_fundamental_register_types (PyObject *m)
{
    Py_SET_TYPE(&PyGIFundamental_Type, &PyType_Type);
    g_assert (Py_TYPE (&PyGIFundamental_Type) != NULL);

    PyGIFundamental_Type.tp_alloc = PyType_GenericAlloc;
    PyGIFundamental_Type.tp_new = (newfunc) fundamental_new;
    PyGIFundamental_Type.tp_dealloc = (destructor) fundamental_dealloc;
    PyGIFundamental_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    PyGIFundamental_Type.tp_richcompare = fundamental_richcompare;
    PyGIFundamental_Type.tp_repr = (reprfunc) fundamental_repr;
    PyGIFundamental_Type.tp_hash = (hashfunc) fundamental_hash;

    if (PyType_Ready (&PyGIFundamental_Type))
        return -1;
    if (PyModule_AddObject (m, "Fundamental", (PyObject *) &PyGIFundamental_Type))
        return -1;

    return 0;
}


GTypeInstance*
pygi_fundamental_from_value (const GValue *value)
{
    GIRepository *repository = g_irepository_get_default ();
    GIBaseInfo *info = g_irepository_find_by_gtype (repository, G_VALUE_TYPE (value));
    GTypeInstance *instance = NULL;

    if (info == NULL)
        return NULL;

    if (GI_IS_OBJECT_INFO (info)) {
        GIObjectInfoGetValueFunction get_value_func = g_object_info_get_get_value_function_pointer ((GIObjectInfo *) info);
        if (get_value_func) {
            instance = get_value_func (value);
        }
    }

    g_base_info_unref (info);

    return instance;
}

gboolean
pygi_fundamental_set_value (GValue *value, GTypeInstance *instance)
{
    GIRepository *repository;
    GIBaseInfo *info;
    gboolean result = FALSE;

    if (instance == NULL) 
        return result;

    repository = g_irepository_get_default ();
    info = g_irepository_find_by_gtype (repository, G_TYPE_FROM_INSTANCE (instance));

    if (info == NULL)
        return result;

    if (GI_IS_OBJECT_INFO (info)) {
        GIObjectInfoSetValueFunction set_value_func = g_object_info_get_set_value_function_pointer ((GIObjectInfo *) info);
        if (set_value_func) {
            set_value_func (value, instance);
            result = TRUE;
        }
    }

    g_base_info_unref (info);
    return result;
}

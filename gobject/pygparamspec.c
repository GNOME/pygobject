/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 * Copyright (C) 2004       Johan Dahlin
 *
 *   pygenum.c: GEnum and GFlag wrappers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "pygobject-private.h"

static int
pyg_param_spec_compare(PyGParamSpec *self, PyGParamSpec *v)
{
    if (self->pspec == v->pspec) return 0;
    if (self->pspec > v->pspec) return -1;
    return 1;
}

static long
pyg_param_spec_hash(PyGParamSpec *self)
{
    return (long)self->pspec;
}

static PyObject *
pyg_param_spec_repr(PyGParamSpec *self)
{
    char buf[80];

    g_snprintf(buf, sizeof(buf), "<%s '%s'>",
	       G_PARAM_SPEC_TYPE_NAME(self->pspec),
	       g_param_spec_get_name(self->pspec));
    return PyString_FromString(buf);
}

static void
pyg_param_spec_dealloc(PyGParamSpec *self)
{
    g_param_spec_unref(self->pspec);
    PyObject_DEL(self);
}

PyObject *
pyg_param_spec_getattr(PyGParamSpec *self, const gchar *attr)
{
    if (!strcmp(attr, "__members__")) {
      	if (G_IS_PARAM_SPEC_ENUM(self->pspec))
	  return Py_BuildValue("[sssssssss]", "__doc__", "__gtype__", "blurb",
			       "flags", "name", "nick", "owner_type",
			       "value_type", "enum_class");
	else if (G_IS_PARAM_SPEC_FLAGS(self->pspec))
	  return Py_BuildValue("[sssssssss]", "__doc__", "__gtype__", "blurb",
			       "flags", "name", "nick", "owner_type",
			       "value_type", "flags_class");
	else
	  return Py_BuildValue("[ssssssss]", "__doc__", "__gtype__", "blurb",
			       "flags", "name", "nick", "owner_type",
			       "value_type");
    } else if (!strcmp(attr, "__gtype__")) {
	return pyg_type_wrapper_new(G_PARAM_SPEC_TYPE(self->pspec));
    } else if (!strcmp(attr, "name")) {
	const gchar *name = g_param_spec_get_name(self->pspec);

	if (name)
	    return PyString_FromString(name);
	Py_INCREF(Py_None);
	return Py_None;
    } else if (!strcmp(attr, "nick")) {
	const gchar *nick = g_param_spec_get_nick(self->pspec);

	if (nick)
	    return PyString_FromString(nick);
	Py_INCREF(Py_None);
	return Py_None;
    } else if (!strcmp(attr, "blurb") || !strcmp(attr, "__doc__")) {
	const gchar *blurb = g_param_spec_get_blurb(self->pspec);

	if (blurb)
	    return PyString_FromString(blurb);
	Py_INCREF(Py_None);
	return Py_None;
    } else if (!strcmp(attr, "flags")) {
	return PyInt_FromLong(self->pspec->flags);
    } else if (!strcmp(attr, "value_type")) {
	return pyg_type_wrapper_new(self->pspec->value_type);
    } else if (!strcmp(attr, "owner_type")) {
	return pyg_type_wrapper_new(self->pspec->owner_type);
    } else if (!strcmp(attr, "default_value")) {
	GParamSpec *pspec = self->pspec;
	if (G_IS_PARAM_SPEC_CHAR(pspec)) {
	    return PyString_FromFormat("%c", G_PARAM_SPEC_CHAR(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_UCHAR(pspec)) {
	    return PyString_FromFormat("%c", G_PARAM_SPEC_UCHAR(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_BOOLEAN(pspec)) {
	    return PyBool_FromLong(G_PARAM_SPEC_BOOLEAN(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_INT(pspec)) {
	    return PyInt_FromLong(G_PARAM_SPEC_INT(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_UINT(pspec)) {
	    return PyInt_FromLong(G_PARAM_SPEC_UINT(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_LONG(pspec)) {
	    return PyLong_FromLong(G_PARAM_SPEC_LONG(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_ULONG(pspec)) {
	    return PyLong_FromLong(G_PARAM_SPEC_ULONG(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_INT64(pspec)) {
	    return PyInt_FromLong(G_PARAM_SPEC_INT64(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_UINT64(pspec)) {
	    return PyInt_FromLong(G_PARAM_SPEC_UINT64(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_UNICHAR(pspec)) {
	    return PyString_FromFormat("%c", G_PARAM_SPEC_UNICHAR(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_ENUM(pspec)) {
	    return PyInt_FromLong(G_PARAM_SPEC_ENUM(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_FLAGS(pspec)) {
	    return PyInt_FromLong(G_PARAM_SPEC_FLAGS(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_FLOAT(pspec)) {
	    return PyFloat_FromDouble(G_PARAM_SPEC_FLOAT(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_DOUBLE(pspec)) {
	    return PyFloat_FromDouble(G_PARAM_SPEC_DOUBLE(pspec)->default_value);
	} else if (G_IS_PARAM_SPEC_STRING(pspec)) {
	    if (G_PARAM_SPEC_STRING(pspec)->default_value) {
		return PyString_FromString(G_PARAM_SPEC_STRING(pspec)->default_value);
	    }
	}
	
	/* If we don't know how to convert it, just set it to None
	 * for consistency
	 */
	Py_INCREF(Py_None);
	return Py_None;
    } else if (!strcmp(attr, "enum_class")) {
	if (G_IS_PARAM_SPEC_ENUM(self->pspec)) {
	    GQuark quark;
	    PyObject *pyclass;
	    GParamSpecEnum *pspec;
	    GType enum_type;
	    
	    quark = g_quark_from_static_string("PyGEnum::class");
	    pspec = G_PARAM_SPEC_ENUM(self->pspec);
	    enum_type = G_ENUM_CLASS_TYPE(pspec->enum_class);
	    pyclass = (PyObject*)g_type_get_qdata(enum_type, quark);
	    if (pyclass == NULL) {
		pyclass = Py_None;
	    }
	    Py_INCREF(pyclass);
	    return pyclass;
	    
	}
    } else if (!strcmp(attr, "flags_class")) {
	if (G_IS_PARAM_SPEC_FLAGS(self->pspec)) {
	    GQuark quark;
	    PyObject *pyclass;
	    GParamSpecFlags *pspec;
	    GType flag_type;

	    quark = g_quark_from_static_string("PyGFlags::class");
	    pspec = G_PARAM_SPEC_FLAGS(self->pspec);
	    flag_type = G_FLAGS_CLASS_TYPE(pspec->flags_class);
	    pyclass = (PyObject*)g_type_get_qdata(flag_type, quark);
	    if (pyclass == NULL) {
		pyclass = Py_None;
	    }
	    Py_INCREF(pyclass);
	    return pyclass;
	}
    }
    
    PyErr_SetString(PyExc_AttributeError, attr);
    return NULL;
}

PyTypeObject PyGParamSpec_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "gobject.GParamSpec",
    sizeof(PyGParamSpec),
    0,
    (destructor)pyg_param_spec_dealloc,
    (printfunc)0,
    (getattrfunc)pyg_param_spec_getattr,
    (setattrfunc)0,
    (cmpfunc)pyg_param_spec_compare,
    (reprfunc)pyg_param_spec_repr,
    0,
    0,
    0,
    (hashfunc)pyg_param_spec_hash,
    (ternaryfunc)0,
    (reprfunc)0,
    0L,0L,0L,0L,
    NULL
};

/**
 * pyg_param_spec_new:
 * @pspec: a GParamSpec.
 *
 * Creates a wrapper for a GParamSpec.
 *
 * Returns: the GParamSpec wrapper.
 */
PyObject *
pyg_param_spec_new(GParamSpec *pspec)
{
    PyGParamSpec *self;

    self = (PyGParamSpec *)PyObject_NEW(PyGParamSpec,
					&PyGParamSpec_Type);
    if (self == NULL)
	return NULL;

    self->pspec = g_param_spec_ref(pspec);
    return (PyObject *)self;
}

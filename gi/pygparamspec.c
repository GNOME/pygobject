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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <Python.h>
#include <glib-object.h>

#include "pygenum.h"
#include "pygflags.h"
#include "pygi-type.h"
#include "pygparamspec.h"
#include "pygi-util.h"
#include "pygi-basictype.h"

PYGLIB_DEFINE_TYPE("gobject.GParamSpec", PyGParamSpec_Type, PyGParamSpec);

static PyObject*
pyg_param_spec_richcompare(PyObject *self, PyObject *other, int op)
{
    if (Py_TYPE(self) == Py_TYPE(other) && Py_TYPE(self) == &PyGParamSpec_Type)
        return pyg_ptr_richcompare (pyg_param_spec_get (self),
                                    pyg_param_spec_get (other),
                                    op);
    else {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
}

static PYGLIB_Py_hash_t
pyg_param_spec_hash(PyGParamSpec *self)
{
    return PYGLIB_Py_hash_t_FromVoidPtr (pyg_param_spec_get (self));
}

static PyObject *
pyg_param_spec_repr(PyGParamSpec *self)
{
    char buf[80];

    g_snprintf(buf, sizeof(buf), "<%s '%s'>",
	       G_PARAM_SPEC_TYPE_NAME (pyg_param_spec_get (self)),
	       g_param_spec_get_name (pyg_param_spec_get (self)));
    return PYGLIB_PyUnicode_FromString(buf);
}

static void
pyg_param_spec_dealloc(PyGParamSpec *self)
{
    g_param_spec_unref (pyg_param_spec_get (self));
    PyObject_DEL(self);
}


static PyObject *
pygenum_from_pspec(GParamSpec *pspec)
{
    PyObject *pyclass;
    GParamSpecEnum *enum_pspec;
    GType enum_type;
    
    enum_pspec = G_PARAM_SPEC_ENUM(pspec);
    enum_type = G_ENUM_CLASS_TYPE(enum_pspec->enum_class);
    pyclass = (PyObject*)g_type_get_qdata(enum_type, pygenum_class_key);
    if (pyclass == NULL) {
	pyclass = pyg_enum_add(NULL, g_type_name(enum_type), NULL, enum_type);
	if (pyclass == NULL)
	    pyclass = Py_None;
    }
    
    Py_INCREF(pyclass);
    return pyclass;
}

static PyObject *
pygflags_from_pspec(GParamSpec *pspec)
{
    PyObject *pyclass;
    GParamSpecFlags *flag_pspec;
    GType flag_type;

    flag_pspec = G_PARAM_SPEC_FLAGS(pspec);
    flag_type = G_FLAGS_CLASS_TYPE(flag_pspec->flags_class);
    pyclass = (PyObject*)g_type_get_qdata(flag_type, pygflags_class_key);
    if (pyclass == NULL) {
	pyclass = pyg_flags_add(NULL, g_type_name(flag_type), NULL, flag_type);
	if (pyclass == NULL)
	    pyclass = Py_None;
    }
    Py_INCREF(pyclass);
    return pyclass;
}    

static PyObject *
pyg_param_spec_getattr(PyGParamSpec *self, const gchar *attr)
{
    GParamSpec *pspec;

    pspec = pyg_param_spec_get (self);

    /* common attributes */
    if (!strcmp(attr, "__gtype__")) {
	return pyg_type_wrapper_new(G_PARAM_SPEC_TYPE(pspec));
    } else if (!strcmp(attr, "name")) {
	return Py_BuildValue("s", g_param_spec_get_name(pspec));
    } else if (!strcmp(attr, "nick")) {
	return Py_BuildValue("s", g_param_spec_get_nick(pspec));
    } else if (!strcmp(attr, "blurb") || !strcmp(attr, "__doc__")) {
	return Py_BuildValue("s", g_param_spec_get_blurb(pspec));
    } else if (!strcmp(attr, "flags")) {
	return pygi_guint_to_py (pspec->flags);
    } else if (!strcmp(attr, "value_type")) {
	return pyg_type_wrapper_new(pspec->value_type);
    } else if (!strcmp(attr, "owner_type")) {
	return pyg_type_wrapper_new(pspec->owner_type);
    }

    if (G_IS_PARAM_SPEC_CHAR(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return PYGLIB_PyUnicode_FromFormat(
		"%c", G_PARAM_SPEC_CHAR(pspec)->default_value);
	} else if (!strcmp(attr, "minimum")) {
	    return pygi_gint8_to_py (G_PARAM_SPEC_CHAR(pspec)->minimum);
	} else if (!strcmp(attr, "maximum")) {
	    return pygi_gint8_to_py (G_PARAM_SPEC_CHAR(pspec)->maximum);
	}
    } else if (G_IS_PARAM_SPEC_UCHAR(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return PYGLIB_PyUnicode_FromFormat(
		"%c", G_PARAM_SPEC_UCHAR(pspec)->default_value);
	} else if (!strcmp(attr, "minimum")) {
	    return pygi_guint8_to_py (G_PARAM_SPEC_UCHAR(pspec)->minimum);
	} else if (!strcmp(attr, "maximum")) {
	    return pygi_guint8_to_py (G_PARAM_SPEC_UCHAR(pspec)->maximum);
	}
    } else if (G_IS_PARAM_SPEC_BOOLEAN(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return pygi_gboolean_to_py (G_PARAM_SPEC_BOOLEAN(pspec)->default_value);
	}
    } else if (G_IS_PARAM_SPEC_INT(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return pygi_gint_to_py (G_PARAM_SPEC_INT(pspec)->default_value);
	} else if (!strcmp(attr, "minimum")) {
	    return pygi_gint_to_py (G_PARAM_SPEC_INT(pspec)->minimum);
	} else if (!strcmp(attr, "maximum")) {
	    return pygi_gint_to_py (G_PARAM_SPEC_INT(pspec)->maximum);
	}
    } else if (G_IS_PARAM_SPEC_UINT(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return pygi_guint_to_py (G_PARAM_SPEC_UINT(pspec)->default_value);
	} else if (!strcmp(attr, "minimum")) {
	    return pygi_guint_to_py (G_PARAM_SPEC_UINT(pspec)->minimum);
	} else if (!strcmp(attr, "maximum")) {
	    return pygi_guint_to_py (G_PARAM_SPEC_UINT(pspec)->maximum);
	}
    } else if (G_IS_PARAM_SPEC_LONG(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return pygi_glong_to_py (G_PARAM_SPEC_LONG(pspec)->default_value);
	} else if (!strcmp(attr, "minimum")) {
	    return pygi_glong_to_py (G_PARAM_SPEC_LONG(pspec)->minimum);
	} else if (!strcmp(attr, "maximum")) {
	    return pygi_glong_to_py (G_PARAM_SPEC_LONG(pspec)->maximum);
	}
    } else if (G_IS_PARAM_SPEC_ULONG(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return pygi_gulong_to_py (G_PARAM_SPEC_ULONG(pspec)->default_value);
	} else if (!strcmp(attr, "minimum")) {
	    return pygi_gulong_to_py (G_PARAM_SPEC_ULONG(pspec)->minimum);
	} else if (!strcmp(attr, "maximum")) {
	    return pygi_gulong_to_py (G_PARAM_SPEC_ULONG(pspec)->maximum);
	}
    } else if (G_IS_PARAM_SPEC_INT64(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return pygi_gint64_to_py (G_PARAM_SPEC_INT64(pspec)->default_value);
	} else if (!strcmp(attr, "minimum")) {
	    return pygi_gint64_to_py (G_PARAM_SPEC_INT64(pspec)->minimum);
	} else if (!strcmp(attr, "maximum")) {
	    return pygi_gint64_to_py (G_PARAM_SPEC_INT64(pspec)->maximum);
	}
    } else if (G_IS_PARAM_SPEC_UINT64(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return pygi_guint64_to_py (G_PARAM_SPEC_UINT64(pspec)->default_value);
	} else if (!strcmp(attr, "minimum")) {
	    return pygi_guint64_to_py (G_PARAM_SPEC_UINT64(pspec)->minimum);
	} else if (!strcmp(attr, "maximum")) {
	    return pygi_guint64_to_py (G_PARAM_SPEC_UINT64(pspec)->maximum);
	}
    } else if (G_IS_PARAM_SPEC_UNICHAR(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return PYGLIB_PyUnicode_FromFormat(
		"%c", G_PARAM_SPEC_UNICHAR(pspec)->default_value);
	}
    } else if (G_IS_PARAM_SPEC_ENUM(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return pyg_enum_from_gtype(
		pspec->value_type, G_PARAM_SPEC_ENUM(pspec)->default_value);
	} else if (!strcmp(attr, "enum_class")) {
	    return pygenum_from_pspec(pspec);
	}
    } else if (G_IS_PARAM_SPEC_FLAGS(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return pyg_flags_from_gtype(
		pspec->value_type, G_PARAM_SPEC_FLAGS(pspec)->default_value);
	} else if (!strcmp(attr, "flags_class")) {
	    return pygflags_from_pspec(pspec);
	}
    } else if (G_IS_PARAM_SPEC_FLOAT(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return pygi_gfloat_to_py (G_PARAM_SPEC_FLOAT(pspec)->default_value);
	} else if (!strcmp(attr, "minimum")) {
	    return pygi_gfloat_to_py (G_PARAM_SPEC_FLOAT(pspec)->minimum);
	} else if (!strcmp(attr, "maximum")) {
	    return pygi_gfloat_to_py (G_PARAM_SPEC_FLOAT(pspec)->maximum);
	} else if (!strcmp(attr, "epsilon")) {
	    return pygi_gfloat_to_py (G_PARAM_SPEC_FLOAT(pspec)->epsilon);
	}
    } else if (G_IS_PARAM_SPEC_DOUBLE(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return pygi_gdouble_to_py (
		G_PARAM_SPEC_DOUBLE(pspec)->default_value);
	} else if (!strcmp(attr, "minimum")) {
	    return pygi_gdouble_to_py (G_PARAM_SPEC_DOUBLE(pspec)->minimum);
	} else if (!strcmp(attr, "maximum")) {
	    return pygi_gdouble_to_py (G_PARAM_SPEC_DOUBLE(pspec)->maximum);
	} else if (!strcmp(attr, "epsilon")) {
	    return pygi_gdouble_to_py (G_PARAM_SPEC_DOUBLE(pspec)->epsilon);
	}
    } else if (G_IS_PARAM_SPEC_STRING(pspec)) {
	if (!strcmp(attr, "default_value")) {
	    return Py_BuildValue(
		"s", G_PARAM_SPEC_STRING(pspec)->default_value);
	} else if (!strcmp(attr, "cset_first")) {
	    return Py_BuildValue(
		"s", G_PARAM_SPEC_STRING(pspec)->cset_first);
	} else if (!strcmp(attr, "cset_nth")) {
	    return Py_BuildValue(
		"s", G_PARAM_SPEC_STRING(pspec)->cset_nth);
	} else if (!strcmp(attr, "substitutor")) {
	    return Py_BuildValue(
		"c", G_PARAM_SPEC_STRING(pspec)->substitutor);
	} else if (!strcmp(attr, "null_fold_if_empty")) {
	    return pygi_gboolean_to_py (
		G_PARAM_SPEC_STRING(pspec)->null_fold_if_empty);
	} else if (!strcmp(attr, "ensure_non_null")) {
	    return pygi_gboolean_to_py (
		G_PARAM_SPEC_STRING(pspec)->ensure_non_null);
	}
    } else {
	/* This is actually not what's exported by GObjects paramspecs,
	 * But we exported this in earlier versions, so it's better to keep it here
	 * compatibility. But don't return it in __dir__, to "hide" it.
	 */
	if (!strcmp(attr, "default_value")) {
	    /* XXX: Raise deprecation warning */
	    Py_INCREF(Py_None);
	    return Py_None;
	}
    }

    PyErr_SetString(PyExc_AttributeError, attr);
    return NULL;
}


static PyObject *
pyg_param_spec_dir(PyGParamSpec *self, PyObject *dummy)
{
    GParamSpec *pspec = pyg_param_spec_get (self);

    if (G_IS_PARAM_SPEC_CHAR(pspec)) {
        return Py_BuildValue("[sssssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value", "flags",
                             "maximum", "minimum", "name", "nick",
                             "owner_type", "value_type");
    } else if (G_IS_PARAM_SPEC_UCHAR(pspec)) {
        return Py_BuildValue("[sssssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value",
                             "flags", "maximum", "minimum",
                             "name", "nick", "owner_type",
                             "value_type");
    } else if (G_IS_PARAM_SPEC_BOOLEAN(pspec)) {
        return Py_BuildValue("[sssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value",
                             "flags", "name", "nick", "owner_type",
                             "value_type");
    } else if (G_IS_PARAM_SPEC_INT(pspec)) {
        return Py_BuildValue("[sssssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value",
                             "flags", "maximum", "minimum", "name",
                             "nick", "owner_type", "value_type");
    } else if (G_IS_PARAM_SPEC_UINT(pspec)) {
        return Py_BuildValue("[sssssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value",
                             "flags", "maximum", "minimum",
                             "name", "nick", "owner_type",
                             "value_type");
    } else if (G_IS_PARAM_SPEC_LONG(pspec)) {
        return Py_BuildValue("[sssssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value",
                             "flags", "maximum", "minimum", "name",
                             "nick", "owner_type", "value_type");
    } else if (G_IS_PARAM_SPEC_ULONG(pspec)) {
        return Py_BuildValue("[sssssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value",
                             "flags", "maximum", "minimum", "name",
                             "nick", "owner_type", "value_type");
    } else if (G_IS_PARAM_SPEC_INT64(pspec)) {
        return Py_BuildValue("[sssssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value",
                             "flags", "maximum", "minimum", "name",
                             "nick", "owner_type", "value_type");
    } else if (G_IS_PARAM_SPEC_UINT64(pspec)) {
        return Py_BuildValue("[sssssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value",
                             "flags", "maximum", "minimum",
                             "name", "nick", "owner_type",
                             "value_type");
    } else if (G_IS_PARAM_SPEC_UNICHAR(pspec)) {
        return Py_BuildValue("[sssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value",
                             "flags", "name", "nick", "owner_type",
                             "value_type");
    } else if (G_IS_PARAM_SPEC_ENUM(pspec)) {
        return Py_BuildValue("[ssssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value", "enum_class",
                             "flags", "name", "nick", "owner_type",
                             "value_type");
    } else if (G_IS_PARAM_SPEC_FLAGS(pspec)) {
        return Py_BuildValue("[ssssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value",
                             "flags", "flags_class", "name", "nick",
                             "owner_type", "value_type");
    } else if (G_IS_PARAM_SPEC_FLOAT(pspec)) {
        return Py_BuildValue("[ssssssssssss]", "__doc__", "__gtype__",
                             "blurb", "epsilon",
                             "flags", "maximum", "minimum", "name", "nick", "owner_type",
                             "value_type",
                             "default_value");
    } else if (G_IS_PARAM_SPEC_DOUBLE(pspec)) {
        return Py_BuildValue("[ssssssssssss]", "__doc__", "__gtype__",
                             "blurb", "default_value", "epsilon",
                             "flags", "maximum", "minimum", "name", "nick",
                             "owner_type", "value_type");
    } else if (G_IS_PARAM_SPEC_STRING(pspec)) {
        return Py_BuildValue("[ssssssssssssss]", "__doc__", "__gtype__",
                             "blurb", "cset_first", "cset_nth", "default_value",
                             "ensure_non_null", "flags", "name", "nick",
                             "null_fold_if_empty", "owner_type", "substitutor",
                             "value_type");
    } else {
        return Py_BuildValue("[ssssssss]", "__doc__", "__gtype__", "blurb",
                             "flags", "name", "nick",
                             "owner_type", "value_type");
    }
}

static PyMethodDef pyg_param_spec_methods[] = {
    { "__dir__", (PyCFunction)pyg_param_spec_dir, METH_NOARGS},
    { NULL, NULL, 0}
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

    pyg_param_spec_set (self, g_param_spec_ref (pspec));
    return (PyObject *)self;
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_paramspec_register_types(PyObject *d)
{
    Py_TYPE(&PyGParamSpec_Type) = &PyType_Type;
    PyGParamSpec_Type.tp_dealloc = (destructor)pyg_param_spec_dealloc;
    PyGParamSpec_Type.tp_getattr = (getattrfunc)pyg_param_spec_getattr;
    PyGParamSpec_Type.tp_richcompare = pyg_param_spec_richcompare;
    PyGParamSpec_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGParamSpec_Type.tp_repr = (reprfunc)pyg_param_spec_repr;
    PyGParamSpec_Type.tp_hash = (hashfunc)pyg_param_spec_hash;
    PyGParamSpec_Type.tp_methods = pyg_param_spec_methods;

    if (PyType_Ready(&PyGParamSpec_Type))
        return -1;
    PyDict_SetItemString(d, "GParamSpec", (PyObject *)&PyGParamSpec_Type);

    return 0;
}

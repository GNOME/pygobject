/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 * Copyright (C) 2004       Johan Dahlin
 *
 *   pygenum.c: GEnum wrapper
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

#include <pythoncapi_compat.h>

#include "pygi-type.h"

#include "pygenum.h"
#include "pygi-basictype.h"
#include "pygi-util.h"

GQuark pygenum_class_key;

PyTypeObject *PyGEnum_Type;
static PyObject *IntEnum_Type;

PyObject *
pyg_enum_val_new (PyObject *pyclass, int value)
{
    PyObject *retval, *intvalue;
    PyObject *args[2] = { NULL };

    intvalue = PyLong_FromLong (value);
    if (!intvalue) return NULL;

    args[1] = intvalue;
    retval = PyObject_Vectorcall (
	pyclass, &args[1], 1 + PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
    if (!retval && PyErr_ExceptionMatches (PyExc_ValueError)) {
	PyErr_Clear();
	return intvalue;
    }
    Py_DECREF(intvalue);

    return retval;
}

PyObject*
pyg_enum_from_gtype (GType gtype, int value)
{
    PyObject *pyclass;

    g_return_val_if_fail(gtype != G_TYPE_INVALID, NULL);

    /* Get a wrapper class by:
     * 1. check for one attached to the gtype
     * 2. lookup one in a typelib
     * 3. creating a new one
     */
    pyclass = (PyObject*)g_type_get_qdata(gtype, pygenum_class_key);
    if (!pyclass)
        pyclass = pygi_type_import_by_g_type(gtype);
    if (!pyclass)
        pyclass = pyg_enum_add(NULL, g_type_name(gtype), NULL, gtype);
    if (!pyclass)
	return PyLong_FromLong(value);

    return pyg_enum_val_new (pyclass, value);
}

static void
add_value (PyObject *dict, const char *value_nick, int value)
{
    g_autofree char *upper = g_ascii_strup(value_nick, -1);
    char *c;

    for (c = upper; *c != '\0'; c++) {
	if (*c == '-') *c = '_';
    }

    /* skip if the name already exists in the dictionary */
    if (PyDict_GetItemString (dict, upper) != NULL) return;

    PyDict_SetItemString (dict, upper, PyLong_FromLong (value));
}

PyObject *
pyg_enum_add_full (PyObject   *module,
		   const char *typename,
		   GType       gtype,
		   GIEnumInfo *info)
{
    PyObject *stub, *o;
    PyObject *base_class, *enum_name, *values, *module_name, *kwnames;
    PyObject *args[4] = { NULL };

    if (gtype == G_TYPE_NONE && info == NULL) {
	PyErr_SetString (PyExc_ValueError, "cannot create enum without a GType or EnumInfo");
	return NULL;
    }
    if (gtype != G_TYPE_NONE && !g_type_is_a (gtype, G_TYPE_ENUM)) {
        PyErr_Format (PyExc_TypeError, "Trying to register gtype '%s' as enum when in fact it is of type '%s'",
                      g_type_name (gtype), g_type_name (G_TYPE_FUNDAMENTAL (gtype)));
        return NULL;
    }
    if (info != NULL) {
	GType info_gtype = gi_registered_type_info_get_g_type (GI_REGISTERED_TYPE_INFO (info));

	if (info_gtype != gtype) {
	    PyErr_Format (PyExc_ValueError, "gtype '%s' does not match EnumInfo '%s'", g_type_name (gtype), gi_base_info_get_name (GI_BASE_INFO (info)));
	    return NULL;
	}
    }

    values = PyDict_New ();
    /* Collect values from registered GType */
    if (gtype != G_TYPE_NONE) {
	GEnumClass *eclass;
	unsigned int i;

	eclass = G_ENUM_CLASS (g_type_class_ref(gtype));
	for (i = 0; i < eclass->n_values; i++) {
	    add_value (values, eclass->values[i].value_nick, eclass->values[i].value);
	}
	g_type_class_unref (eclass);
    }

    /* Collect values from GIEnumInfo (which may include additional aliases) */
    if (info != NULL) {
	unsigned int i, length;

	length = gi_enum_info_get_n_values (info);
	for (i = 0; i < length; i++) {
	    GIValueInfo *v = gi_enum_info_get_value (info, i);

	    add_value (values, gi_base_info_get_name (GI_BASE_INFO (v)),
		       gi_value_info_get_value (v));
	}
    }

    enum_name = PyUnicode_FromString (typename);
    if (module) {
	module_name = PyModule_GetNameObject (module);
    } else {
	Py_INCREF (Py_None);
	module_name = Py_None;
    }
    kwnames = Py_BuildValue("(s)", "module");

    if (gtype == G_TYPE_NONE)
	base_class = IntEnum_Type;
    else
	base_class = (PyObject *)PyGEnum_Type;
    args[0] = NULL;
    args[1] = enum_name;
    args[2] = values;
    args[3] = module_name;

    stub = PyObject_Vectorcall (base_class,
				&args[1], 2 + PY_VECTORCALL_ARGUMENTS_OFFSET,
				kwnames);
    Py_DECREF (kwnames);
    Py_DECREF (module_name);
    Py_DECREF (values);
    Py_DECREF (enum_name);

    if (!stub) return NULL;

    ((PyTypeObject *)stub)->tp_flags &= ~Py_TPFLAGS_BASETYPE;
    if (gtype != G_TYPE_NONE) {
	g_type_set_qdata(gtype, pygenum_class_key, stub);

	o = pyg_type_wrapper_new(gtype);
	PyDict_SetItemString(((PyTypeObject *)stub)->tp_dict, "__gtype__", o);
	Py_DECREF (o);
    }

    return stub;
}

/*
 * pyg_enum_add
 * Dynamically create a class derived from PyGEnum based on the given GType.
 */
PyObject *
pyg_enum_add (PyObject *   module,
	      const char * typename,
	      const char * strip_prefix,
	      GType        gtype)
{
    PyGILState_STATE state;
    PyObject *stub;

    g_return_val_if_fail(typename != NULL, NULL);
    if (!g_type_is_a (gtype, G_TYPE_ENUM)) {
        PyErr_Format (PyExc_TypeError, "Trying to register gtype '%s' as enum when in fact it is of type '%s'",
                      g_type_name (gtype), g_type_name (G_TYPE_FUNDAMENTAL (gtype)));
        return NULL;
    }

    state = PyGILState_Ensure();

    stub = pyg_enum_add_full (module, typename, gtype, NULL);
    if (!stub) {
	//PyErr_SetString(PyExc_RuntimeError, "can't create const");
	PyGILState_Release(state);
	return NULL;
    }

    if (module) {
	GEnumClass *eclass;
	guint i;

	/* Add it to the module name space */
	PyModule_AddObject(module, (char*)typename, stub);
	Py_INCREF(stub);

	/* Register enum values */
	eclass = G_ENUM_CLASS(g_type_class_ref(gtype));
	for (i = 0; i < eclass->n_values; i++) {
	    PyObject *item, *intval;
	    PyObject *args[2] = { NULL };
	    char *prefix;

	    intval = PyLong_FromLong(eclass->values[i].value);
	    args[1] = intval;
	    item = PyObject_Vectorcall (
		stub, &args[1], 1 + PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
	    Py_DECREF (intval);

	    prefix = g_strdup(pyg_constant_strip_prefix(eclass->values[i].value_name, strip_prefix));
	    PyModule_AddObject(module, prefix, item);
	    g_free(prefix);
	}
	g_type_class_unref(eclass);
    }

    PyGILState_Release(state);
    return stub;
}

static GType
get_enum_gtype (PyTypeObject *type)
{
    PyObject *pytc;
    GType gtype;

    pytc = PyObject_GetAttrString((PyObject *)type, "__gtype__");
    if (!pytc)
	return G_TYPE_INVALID;

    if (!PyObject_TypeCheck(pytc, &PyGTypeWrapper_Type)) {
	Py_DECREF(pytc);
	PyErr_SetString(PyExc_TypeError,
			"__gtype__ attribute not a typecode");
	return G_TYPE_INVALID;
    }

    gtype = pyg_type_from_object(pytc);
    Py_DECREF(pytc);

    if (!G_TYPE_IS_ENUM (gtype)) {
	PyErr_SetString(PyExc_TypeError,
			"__gtype__ attribute not an enum typecode");
	return G_TYPE_INVALID;
    }

    return gtype;
}

static PyObject *
pyg_enum_get_value_name(PyObject *self, void *closure)
{
  GType gtype;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  PyObject *retval;
  gint intvalue;

  if (!pygi_gint_from_py ((PyObject*) self, &intvalue))
    return NULL;

  gtype = get_enum_gtype (Py_TYPE (self));
  if (gtype == G_TYPE_INVALID)
    return NULL;
  enum_class = g_type_class_ref(gtype);
  g_assert(G_IS_ENUM_CLASS(enum_class));

  enum_value = g_enum_get_value(enum_class, intvalue);

  retval = pygi_utf8_to_py (enum_value->value_name);
  g_type_class_unref(enum_class);

  return retval;
}

static PyObject *
pyg_enum_get_value_nick(PyObject *self, void *closure)
{
  GType gtype;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  PyObject *retval;
  gint intvalue;

  if (!pygi_gint_from_py ((PyObject*) self, &intvalue))
    return NULL;

  gtype = get_enum_gtype (Py_TYPE (self));
  if (gtype == G_TYPE_INVALID)
    return NULL;
  enum_class = g_type_class_ref(gtype);
  g_assert(G_IS_ENUM_CLASS(enum_class));

  enum_value = g_enum_get_value(enum_class, intvalue);

  retval = pygi_utf8_to_py (enum_value->value_nick);

  g_type_class_unref(enum_class);

  return retval;
}

int
pyg_enum_check_type (PyObject *obj, GType expected_type)
{
    GType gtype;

    if (expected_type == G_TYPE_NONE) return 0;

    /* Only match the GType if this is a PyGEnum_Type subclass */
    if (!PyObject_TypeCheck (obj, PyGEnum_Type)) return 0;

    gtype = get_enum_gtype (Py_TYPE (obj));
    if (gtype == G_TYPE_INVALID) return -1;

    if (gtype != expected_type) {
	PyErr_Format (PyExc_TypeError,
		      "expected enumeration type %s, but got %s instead",
		      g_type_name (expected_type), g_type_name (gtype));
	return -1;
    }

    return 0;
}

static PyGetSetDef pyg_enum_getsets[] = {
    { "value_name", (getter)pyg_enum_get_value_name, (setter)0 },
    { "value_nick", (getter)pyg_enum_get_value_nick, (setter)0 },
    { NULL, 0, 0 },
};

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_enum_register_types(PyObject *mod)
{
    PyObject *enum_module, *item;
    int i;

    pygenum_class_key        = g_quark_from_static_string("PyGEnum::class");

    enum_module = PyImport_ImportModule("enum");
    if (!enum_module)
	return -1;

    IntEnum_Type = PyObject_GetAttrString(enum_module, "IntEnum");
    Py_DECREF (enum_module);
    if (!IntEnum_Type)
	return -1;

    PyGEnum_Type = (PyTypeObject *)PyObject_CallFunction(IntEnum_Type, "s()", "GEnum");
    if (!PyGEnum_Type)
	return -1;

    item = PyModule_GetNameObject (mod);
    PyObject_SetAttrString ((PyObject *)PyGEnum_Type, "__module__", item);

    item = pyg_type_wrapper_new (G_TYPE_ENUM);
    PyObject_SetAttrString ((PyObject *)PyGEnum_Type, "__gtype__", item);
    Py_DECREF (item);

    for (i = 0; pyg_enum_getsets[i].name != NULL; i++) {
	item = PyDescr_NewGetSet(PyGEnum_Type, &pyg_enum_getsets[i]);
	PyObject_SetAttrString ((PyObject *)PyGEnum_Type, pyg_enum_getsets[i].name, item);
	Py_DECREF (item);
    }

    Py_INCREF (PyGEnum_Type);
    PyModule_AddObject (mod, "GEnum", (PyObject *)PyGEnum_Type);

    return 0;
}

/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   pygtype.c: glue code to wrap the GType code.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <pyglib.h>

#include "pygobject-private.h"
#include "pygparamspec.h"
#include "pygtype.h"

#include "pygi-type.h"
#include "pygi-value.h"

/* -------------- __gtype__ objects ---------------------------- */

typedef struct {
    PyObject_HEAD
    GType type;
} PyGTypeWrapper;

PYGLIB_DEFINE_TYPE("gobject.GType", PyGTypeWrapper_Type, PyGTypeWrapper);

static PyObject*
pyg_type_wrapper_richcompare(PyObject *self, PyObject *other, int op)
{
    if (Py_TYPE(self) == Py_TYPE(other) && Py_TYPE(self) == &PyGTypeWrapper_Type)
        return _pyglib_generic_long_richcompare(((PyGTypeWrapper*)self)->type,
                                                ((PyGTypeWrapper*)other)->type,
                                                op);
    else {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
}

static long
pyg_type_wrapper_hash(PyGTypeWrapper *self)
{
    return (long)self->type;
}

static PyObject *
pyg_type_wrapper_repr(PyGTypeWrapper *self)
{
    char buf[80];
    const gchar *name = g_type_name(self->type);

    g_snprintf(buf, sizeof(buf), "<GType %s (%lu)>",
	       name?name:"invalid", (unsigned long int) self->type);
    return PYGLIB_PyUnicode_FromString(buf);
}

static void
pyg_type_wrapper_dealloc(PyGTypeWrapper *self)
{
    PyObject_DEL(self);
}

static GQuark
_pyg_type_key(GType type) {
    GQuark key;

    if (g_type_is_a(type, G_TYPE_INTERFACE)) {
        key = pyginterface_type_key;
    } else if (g_type_is_a(type, G_TYPE_ENUM)) {
        key = pygenum_class_key;
    } else if (g_type_is_a(type, G_TYPE_FLAGS)) {
        key = pygflags_class_key;
    } else if (g_type_is_a(type, G_TYPE_POINTER)) {
        key = pygpointer_class_key;
    } else if (g_type_is_a(type, G_TYPE_BOXED)) {
        key = pygboxed_type_key;
    } else {
        key = pygobject_class_key;
    }

    return key;
}

static PyObject *
_wrap_g_type_wrapper__get_pytype(PyGTypeWrapper *self, void *closure)
{
    GQuark key;
    PyObject *py_type;

    key = _pyg_type_key(self->type);

    py_type = g_type_get_qdata(self->type, key);
    if (!py_type)
      py_type = Py_None;

    Py_INCREF(py_type);
    return py_type;
}

static int
_wrap_g_type_wrapper__set_pytype(PyGTypeWrapper *self, PyObject* value, void *closure)
{
    GQuark key;
    PyObject *py_type;

    key = _pyg_type_key(self->type);

    py_type = g_type_get_qdata(self->type, key);
    Py_CLEAR(py_type);
    if (value == Py_None)
	g_type_set_qdata(self->type, key, NULL);
    else if (PyType_Check(value)) {
	Py_INCREF(value);
	g_type_set_qdata(self->type, key, value);
    } else {
	PyErr_SetString(PyExc_TypeError, "Value must be None or a type object");
	return -1;
    }

    return 0;
}

static PyObject *
_wrap_g_type_wrapper__get_name(PyGTypeWrapper *self, void *closure)
{
   const char *name = g_type_name(self->type);
   return PYGLIB_PyUnicode_FromString(name ? name : "invalid");
}

static PyObject *
_wrap_g_type_wrapper__get_parent(PyGTypeWrapper *self, void *closure)
{
   return pyg_type_wrapper_new(g_type_parent(self->type));
}

static PyObject *
_wrap_g_type_wrapper__get_fundamental(PyGTypeWrapper *self, void *closure)
{
   return pyg_type_wrapper_new(g_type_fundamental(self->type));
}

static PyObject *
_wrap_g_type_wrapper__get_children(PyGTypeWrapper *self, void *closure)
{
  guint n_children, i;
  GType *children;
  PyObject *retval;

  children = g_type_children(self->type, &n_children);

  retval = PyList_New(n_children);
  for (i = 0; i < n_children; i++)
      PyList_SetItem(retval, i, pyg_type_wrapper_new(children[i]));
  g_free(children);

  return retval;
}

static PyObject *
_wrap_g_type_wrapper__get_interfaces(PyGTypeWrapper *self, void *closure)
{
  guint n_interfaces, i;
  GType *interfaces;
  PyObject *retval;

  interfaces = g_type_interfaces(self->type, &n_interfaces);

  retval = PyList_New(n_interfaces);
  for (i = 0; i < n_interfaces; i++)
      PyList_SetItem(retval, i, pyg_type_wrapper_new(interfaces[i]));
  g_free(interfaces);

  return retval;
}

static PyObject *
_wrap_g_type_wrapper__get_depth(PyGTypeWrapper *self, void *closure)
{
  return PYGLIB_PyLong_FromLong(g_type_depth(self->type));
}

static PyGetSetDef _PyGTypeWrapper_getsets[] = {
    { "pytype", (getter)_wrap_g_type_wrapper__get_pytype, (setter)_wrap_g_type_wrapper__set_pytype },
    { "name",  (getter)_wrap_g_type_wrapper__get_name, (setter)0 },
    { "fundamental",  (getter)_wrap_g_type_wrapper__get_fundamental, (setter)0 },
    { "parent",  (getter)_wrap_g_type_wrapper__get_parent, (setter)0 },
    { "children",  (getter)_wrap_g_type_wrapper__get_children, (setter)0 },
    { "interfaces",  (getter)_wrap_g_type_wrapper__get_interfaces, (setter)0 },
    { "depth",  (getter)_wrap_g_type_wrapper__get_depth, (setter)0 },
    { NULL, (getter)0, (setter)0 }
};

static PyObject*
_wrap_g_type_is_interface(PyGTypeWrapper *self)
{
    return PyBool_FromLong(G_TYPE_IS_INTERFACE(self->type));
}

static PyObject*
_wrap_g_type_is_classed(PyGTypeWrapper *self)
{
    return PyBool_FromLong(G_TYPE_IS_CLASSED(self->type));
}

static PyObject*
_wrap_g_type_is_instantiatable(PyGTypeWrapper *self)
{
    return PyBool_FromLong(G_TYPE_IS_INSTANTIATABLE(self->type));
}

static PyObject*
_wrap_g_type_is_derivable(PyGTypeWrapper *self)
{
    return PyBool_FromLong(G_TYPE_IS_DERIVABLE(self->type));
}

static PyObject*
_wrap_g_type_is_deep_derivable(PyGTypeWrapper *self)
{
    return PyBool_FromLong(G_TYPE_IS_DEEP_DERIVABLE(self->type));
}

static PyObject*
_wrap_g_type_is_abstract(PyGTypeWrapper *self)
{
    return PyBool_FromLong(G_TYPE_IS_ABSTRACT(self->type));
}

static PyObject*
_wrap_g_type_is_value_abstract(PyGTypeWrapper *self)
{
    return PyBool_FromLong(G_TYPE_IS_VALUE_ABSTRACT(self->type));
}

static PyObject*
_wrap_g_type_is_value_type(PyGTypeWrapper *self)
{
    return PyBool_FromLong(G_TYPE_IS_VALUE_TYPE(self->type));
}

static PyObject*
_wrap_g_type_has_value_table(PyGTypeWrapper *self)
{
    return PyBool_FromLong(G_TYPE_HAS_VALUE_TABLE(self->type));
}

static PyObject*
_wrap_g_type_from_name(PyGTypeWrapper *_, PyObject *args)
{
    char *type_name;
    GType type;

    if (!PyArg_ParseTuple(args, "s:GType.from_name", &type_name))
	return NULL;

    type = g_type_from_name(type_name);
    if (type == 0) {
	PyErr_SetString(PyExc_RuntimeError, "unknown type name");
	return NULL;
    }

    return pyg_type_wrapper_new(type);
}

static PyObject*
_wrap_g_type_is_a(PyGTypeWrapper *self, PyObject *args)
{
    PyObject *gparent;
    GType parent;

    if (!PyArg_ParseTuple(args, "O:GType.is_a", &gparent))
	return NULL;
    else if ((parent = pyg_type_from_object(gparent)) == 0)
	return NULL;

    return PyBool_FromLong(g_type_is_a(self->type, parent));
}

static PyMethodDef _PyGTypeWrapper_methods[] = {
    { "is_interface", (PyCFunction)_wrap_g_type_is_interface, METH_NOARGS },
    { "is_classed", (PyCFunction)_wrap_g_type_is_classed, METH_NOARGS },
    { "is_instantiatable", (PyCFunction)_wrap_g_type_is_instantiatable, METH_NOARGS },
    { "is_derivable", (PyCFunction)_wrap_g_type_is_derivable, METH_NOARGS },
    { "is_deep_derivable", (PyCFunction)_wrap_g_type_is_deep_derivable, METH_NOARGS },
    { "is_abstract", (PyCFunction)_wrap_g_type_is_abstract, METH_NOARGS },
    { "is_value_abstract", (PyCFunction)_wrap_g_type_is_value_abstract, METH_NOARGS },
    { "is_value_type", (PyCFunction)_wrap_g_type_is_value_type, METH_NOARGS },
    { "has_value_table", (PyCFunction)_wrap_g_type_has_value_table, METH_NOARGS },
    { "from_name", (PyCFunction)_wrap_g_type_from_name, METH_VARARGS | METH_STATIC },
    { "is_a", (PyCFunction)_wrap_g_type_is_a, METH_VARARGS },
    { NULL,  0, 0 }
};

static int
pyg_type_wrapper_init(PyGTypeWrapper *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "object", NULL };
    PyObject *py_object;
    GType type;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "O:GType.__init__",
				     kwlist, &py_object))
        return -1;

    if (!(type = pyg_type_from_object(py_object)))
	return -1;

    self->type = type;

    return 0;
}

/**
 * pyg_type_wrapper_new:
 * type: a GType
 *
 * Creates a Python wrapper for a GType.
 *
 * Returns: the Python wrapper.
 */
PyObject *
pyg_type_wrapper_new(GType type)
{
    PyGTypeWrapper *self;

    self = (PyGTypeWrapper *)PyObject_NEW(PyGTypeWrapper,
					  &PyGTypeWrapper_Type);
    if (self == NULL)
	return NULL;

    self->type = type;
    return (PyObject *)self;
}

/**
 * pyg_type_from_object_strict:
 * obj: a Python object
 * strict: if set to TRUE, raises an exception if it can't perform the
 *         conversion
 *
 * converts a python object to a GType.  If strict is set, raises an 
 * exception if it can't perform the conversion, otherwise returns
 * PY_TYPE_OBJECT.
 *
 * Returns: the corresponding GType, or 0 on error.
 */

GType
pyg_type_from_object_strict(PyObject *obj, gboolean strict)
{
    PyObject *gtype;
    GType type;

    /* NULL check */
    if (!obj) {
	PyErr_SetString(PyExc_TypeError, "can't get type from NULL object");
	return 0;
    }

    /* map some standard types to primitive GTypes ... */
    if (obj == Py_None)
	return G_TYPE_NONE;
    if (PyType_Check(obj)) {
	PyTypeObject *tp = (PyTypeObject *)obj;

	if (tp == &PYGLIB_PyLong_Type)
	    return G_TYPE_INT;
	else if (tp == &PyBool_Type)
	    return G_TYPE_BOOLEAN;
	else if (tp == &PyLong_Type)
	    return G_TYPE_LONG;
	else if (tp == &PyFloat_Type)
	    return G_TYPE_DOUBLE;
	else if (tp == &PYGLIB_PyUnicode_Type)
	    return G_TYPE_STRING;
	else if (tp == &PyBaseObject_Type)
	    return PY_TYPE_OBJECT;
    }

    if (Py_TYPE(obj) == &PyGTypeWrapper_Type) {
	return ((PyGTypeWrapper *)obj)->type;
    }

    /* handle strings */
    if (PYGLIB_PyUnicode_Check(obj)) {
	gchar *name = PYGLIB_PyUnicode_AsString(obj);

	type = g_type_from_name(name);
	if (type != 0) {
	    return type;
	}
    }

    /* finally, look for a __gtype__ attribute on the object */
    gtype = PyObject_GetAttrString(obj, "__gtype__");

    if (gtype) {
	if (Py_TYPE(gtype) == &PyGTypeWrapper_Type) {
	    type = ((PyGTypeWrapper *)gtype)->type;
	    Py_DECREF(gtype);
	    return type;
	}
	Py_DECREF(gtype);
    }

    PyErr_Clear();

    /* Some API like those that take GValues can hold a python object as
     * a pointer.  This is potentially dangerous becuase everything is 
     * passed in as a PyObject so we can't actually type check it.  Only
     * fallback to PY_TYPE_OBJECT if strict checking is disabled
     */
    if (!strict)
        return PY_TYPE_OBJECT;

    PyErr_SetString(PyExc_TypeError, "could not get typecode from object");
    return 0;
}

/**
 * pyg_type_from_object:
 * obj: a Python object
 *
 * converts a python object to a GType.  Raises an exception if it
 * can't perform the conversion.
 *
 * Returns: the corresponding GType, or 0 on error.
 */
GType
pyg_type_from_object(PyObject *obj)
{
    /* Legacy call always defaults to strict type checking */
    return pyg_type_from_object_strict(obj, TRUE);
}

/**
 * pyg_enum_get_value:
 * @enum_type: the GType of the flag.
 * @obj: a Python object representing the flag value
 * @val: a pointer to the location to store the integer representation of the flag.
 *
 * Converts a Python object to the integer equivalent.  The conversion
 * will depend on the type of the Python object.  If the object is an
 * integer, it is passed through directly.  If it is a string, it will
 * be treated as a full or short enum name as defined in the GType.
 *
 * Returns: 0 on success or -1 on failure
 */
gint
pyg_enum_get_value(GType enum_type, PyObject *obj, gint *val)
{
    GEnumClass *eclass = NULL;
    gint res = -1;

    g_return_val_if_fail(val != NULL, -1);
    if (!obj) {
	*val = 0;
	res = 0;
    } else if (PYGLIB_PyLong_Check(obj)) {
	*val = PYGLIB_PyLong_AsLong(obj);
	res = 0;

	if (PyObject_TypeCheck(obj, &PyGEnum_Type) && ((PyGEnum *) obj)->gtype != enum_type) {
	    g_warning("expected enumeration type %s, but got %s instead",
		      g_type_name(enum_type),
		      g_type_name(((PyGEnum *) obj)->gtype));
	}
    /* Dumb code duplication, but probably not worth it to have yet another macro. */
    } else if (PyLong_Check(obj)) {
	*val = PyLong_AsLong(obj);
	res = 0;

	if (PyObject_TypeCheck(obj, &PyGEnum_Type) && ((PyGEnum *) obj)->gtype != enum_type) {
	    g_warning("expected enumeration type %s, but got %s instead",
		      g_type_name(enum_type),
		      g_type_name(((PyGEnum *) obj)->gtype));
	}
    } else if (PYGLIB_PyUnicode_Check(obj)) {
	GEnumValue *info;
	char *str = PYGLIB_PyUnicode_AsString(obj);

	if (enum_type != G_TYPE_NONE)
	    eclass = G_ENUM_CLASS(g_type_class_ref(enum_type));
	else {
	    PyErr_SetString(PyExc_TypeError, "could not convert string to enum because there is no GType associated to look up the value");
	    res = -1;
	}
	info = g_enum_get_value_by_name(eclass, str);
	g_type_class_unref(eclass);

	if (!info)
	    info = g_enum_get_value_by_nick(eclass, str);
	if (info) {
	    *val = info->value;
	    res = 0;
	} else {
	    PyErr_SetString(PyExc_TypeError, "could not convert string");
	    res = -1;
	}
    } else {
	PyErr_SetString(PyExc_TypeError,"enum values must be strings or ints");
	res = -1;
    }
    return res;
}

/**
 * pyg_flags_get_value:
 * @flag_type: the GType of the flag.
 * @obj: a Python object representing the flag value
 * @val: a pointer to the location to store the integer representation of the flag.
 *
 * Converts a Python object to the integer equivalent.  The conversion
 * will depend on the type of the Python object.  If the object is an
 * integer, it is passed through directly.  If it is a string, it will
 * be treated as a full or short flag name as defined in the GType.
 * If it is a tuple, then the items are treated as strings and ORed
 * together.
 *
 * Returns: 0 on success or -1 on failure
 */
gint
pyg_flags_get_value(GType flag_type, PyObject *obj, guint *val)
{
    GFlagsClass *fclass = NULL;
    gint res = -1;

    g_return_val_if_fail(val != NULL, -1);
    if (!obj) {
	*val = 0;
	res = 0;
    } else if (PYGLIB_PyLong_Check(obj)) {
	*val = PYGLIB_PyLong_AsUnsignedLong(obj);
	res = 0;
    } else if (PyLong_Check(obj)) {
        *val = PyLong_AsLongLong(obj);
        res = 0;
    } else if (PYGLIB_PyUnicode_Check(obj)) {
	GFlagsValue *info;
	char *str = PYGLIB_PyUnicode_AsString(obj);

	if (flag_type != G_TYPE_NONE)
	    fclass = G_FLAGS_CLASS(g_type_class_ref(flag_type));
	else {
	    PyErr_SetString(PyExc_TypeError, "could not convert string to flag because there is no GType associated to look up the value");
	    res = -1;
	}
	info = g_flags_get_value_by_name(fclass, str);
	g_type_class_unref(fclass);

	if (!info)
	    info = g_flags_get_value_by_nick(fclass, str);
	if (info) {
	    *val = info->value;
	    res = 0;
	} else {
	    PyErr_SetString(PyExc_TypeError, "could not convert string");
	    res = -1;
	}
    } else if (PyTuple_Check(obj)) {
	int i, len;

	len = PyTuple_Size(obj);
	*val = 0;
	res = 0;

	if (flag_type != G_TYPE_NONE)
	    fclass = G_FLAGS_CLASS(g_type_class_ref(flag_type));
	else {
	    PyErr_SetString(PyExc_TypeError, "could not convert string to flag because there is no GType associated to look up the value");
	    res = -1;
	}

	for (i = 0; i < len; i++) {
	    PyObject *item = PyTuple_GetItem(obj, i);
	    char *str = PYGLIB_PyUnicode_AsString(item);
	    GFlagsValue *info = g_flags_get_value_by_name(fclass, str);

	    if (!info)
		info = g_flags_get_value_by_nick(fclass, str);
	    if (info) {
		*val |= info->value;
	    } else {
		PyErr_SetString(PyExc_TypeError, "could not convert string");
		res = -1;
		break;
	    }
	}
	g_type_class_unref(fclass);
    } else {
	PyErr_SetString(PyExc_TypeError,
			"flag values must be strings, ints, longs, or tuples");
	res = -1;
    }
    return res;
}

static GQuark pyg_type_marshal_key = 0;

PyGTypeMarshal *
pyg_type_lookup(GType type)
{
    GType	ptype = type;
    PyGTypeMarshal	*tm = NULL;

    /* recursively lookup types */
    while (ptype) {
        pygi_type_import_by_g_type (ptype);
	if ((tm = g_type_get_qdata(ptype, pyg_type_marshal_key)) != NULL)
	    break;
	ptype = g_type_parent(ptype);
    }
    return tm;
}

/**
 * pyg_register_gtype_custom:
 * @gtype: the GType for the new type
 * @from_func: a function to convert GValues to Python objects
 * @to_func: a function to convert Python objects to GValues
 *
 * In order to handle specific conversion of gboxed types or new
 * fundamental types, you may use this function to register conversion
 * handlers.
 */

void
pyg_register_gtype_custom(GType gtype,
			  fromvaluefunc from_func,
                          tovaluefunc to_func)
{
    PyGTypeMarshal *tm;

    if (!pyg_type_marshal_key)
        pyg_type_marshal_key = g_quark_from_static_string("PyGType::marshal");

    tm = g_new(PyGTypeMarshal, 1);
    tm->fromvalue = from_func;
    tm->tovalue = to_func;
    g_type_set_qdata(gtype, pyg_type_marshal_key, tm);
}

/* -------------- PyGClosure ----------------- */

static void
pyg_closure_invalidate(gpointer data, GClosure *closure)
{
    PyGClosure *pc = (PyGClosure *)closure;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();
    Py_XDECREF(pc->callback);
    Py_XDECREF(pc->extra_args);
    Py_XDECREF(pc->swap_data);
    pyglib_gil_state_release(state);

    pc->callback = NULL;
    pc->extra_args = NULL;
    pc->swap_data = NULL;
}

static void
pyg_closure_marshal(GClosure *closure,
		    GValue *return_value,
		    guint n_param_values,
		    const GValue *param_values,
		    gpointer invocation_hint,
		    gpointer marshal_data)
{
    PyGILState_STATE state;
    PyGClosure *pc = (PyGClosure *)closure;
    PyObject *params, *ret;
    guint i;

    state = pyglib_gil_state_ensure();

    /* construct Python tuple for the parameter values */
    params = PyTuple_New(n_param_values);
    for (i = 0; i < n_param_values; i++) {
	/* swap in a different initial data for connect_object() */
	if (i == 0 && G_CCLOSURE_SWAP_DATA(closure)) {
	    g_return_if_fail(pc->swap_data != NULL);
	    Py_INCREF(pc->swap_data);
	    PyTuple_SetItem(params, 0, pc->swap_data);
	} else {
	    PyObject *item = pyg_value_as_pyobject(&param_values[i], FALSE);

	    /* error condition */
	    if (!item) {
		goto out;
	    }
	    PyTuple_SetItem(params, i, item);
	}
    }
    /* params passed to function may have extra arguments */
    if (pc->extra_args) {
	PyObject *tuple = params;
	params = PySequence_Concat(tuple, pc->extra_args);
	Py_DECREF(tuple);
    }
    ret = PyObject_CallObject(pc->callback, params);
    if (ret == NULL) {
	if (pc->exception_handler)
	    pc->exception_handler(return_value, n_param_values, param_values);
	else
	    PyErr_Print();
	goto out;
    }

    if (G_IS_VALUE(return_value) && pyg_value_from_pyobject(return_value, ret) != 0) {
	/* If we already have an exception set, use that, otherwise set a
	 * generic one */
	if (!PyErr_Occurred())
	    PyErr_SetString(PyExc_TypeError,
                            "can't convert return value to desired type");

	if (pc->exception_handler)
	    pc->exception_handler(return_value, n_param_values, param_values);
	else
	    PyErr_Print();
    }
    Py_DECREF(ret);

 out:
    Py_DECREF(params);
    pyglib_gil_state_release(state);
}

/**
 * pyg_closure_new:
 * callback: a Python callable object
 * extra_args: a tuple of extra arguments, or None/NULL.
 * swap_data: an alternative python object to pass first.
 *
 * Creates a GClosure wrapping a Python callable and optionally a set
 * of additional function arguments.  This is needed to attach python
 * handlers to signals, for instance.
 *
 * Returns: the new closure.
 */
GClosure *
pyg_closure_new(PyObject *callback, PyObject *extra_args, PyObject *swap_data)
{
    GClosure *closure;

    g_return_val_if_fail(callback != NULL, NULL);
    closure = g_closure_new_simple(sizeof(PyGClosure), NULL);
    g_closure_add_invalidate_notifier(closure, NULL, pyg_closure_invalidate);
    g_closure_set_marshal(closure, pyg_closure_marshal);
    Py_INCREF(callback);
    ((PyGClosure *)closure)->callback = callback;
    if (extra_args && extra_args != Py_None) {
	Py_INCREF(extra_args);
	if (!PyTuple_Check(extra_args)) {
	    PyObject *tmp = PyTuple_New(1);
	    PyTuple_SetItem(tmp, 0, extra_args);
	    extra_args = tmp;
	}
	((PyGClosure *)closure)->extra_args = extra_args;
    }
    if (swap_data) {
	Py_INCREF(swap_data);
	((PyGClosure *)closure)->swap_data = swap_data;
	closure->derivative_flag = TRUE;
    }
    return closure;
}

/**
 * pyg_closure_set_exception_handler:
 * @closure: a closure created with pyg_closure_new()
 * @handler: the handler to call when an exception occurs or NULL for none
 *
 * Sets the handler to call when an exception occurs during closure invocation.
 * The handler is responsible for providing a proper return value to the
 * closure invocation. If @handler is %NULL, the default handler will be used.
 * The default handler prints the exception to stderr and doesn't touch the
 * closure's return value.
 */
void
pyg_closure_set_exception_handler(GClosure *closure,
				  PyClosureExceptionHandler handler)
{
    PyGClosure *pygclosure;

    g_return_if_fail(closure != NULL);

    pygclosure = (PyGClosure *)closure;
    pygclosure->exception_handler = handler;
}
/* -------------- PySignalClassClosure ----------------- */
/* a closure used for the `class closure' of a signal.  As this gets
 * all the info from the first argument to the closure and the
 * invocation hint, we can have a single closure that handles all
 * class closure cases.  We call a method by the name of the signal
 * with "do_" prepended.
 *
 *  We also remove the first argument from the * param list, as it is
 *  the instance object, which is passed * implicitly to the method
 *  object. */

static void
pyg_signal_class_closure_marshal(GClosure *closure,
				 GValue *return_value,
				 guint n_param_values,
				 const GValue *param_values,
				 gpointer invocation_hint,
				 gpointer marshal_data)
{
    PyGILState_STATE state;
    GObject *object;
    PyObject *object_wrapper;
    GSignalInvocationHint *hint = (GSignalInvocationHint *)invocation_hint;
    gchar *method_name, *tmp;
    PyObject *method;
    PyObject *params, *ret;
    guint i, len;

    state = pyglib_gil_state_ensure();

    g_return_if_fail(invocation_hint != NULL);
    /* get the object passed as the first argument to the closure */
    object = g_value_get_object(&param_values[0]);
    g_return_if_fail(object != NULL && G_IS_OBJECT(object));

    /* get the wrapper for this object */
    object_wrapper = pygobject_new(object);
    g_return_if_fail(object_wrapper != NULL);

    /* construct method name for this class closure */
    method_name = g_strconcat("do_", g_signal_name(hint->signal_id), NULL);

    /* convert dashes to underscores.  For some reason, g_signal_name
     * seems to convert all the underscores in the signal name to
       dashes??? */
    for (tmp = method_name; *tmp != '\0'; tmp++)
	if (*tmp == '-') *tmp = '_';

    method = PyObject_GetAttrString(object_wrapper, method_name);
    g_free(method_name);

    if (!method) {
	PyErr_Clear();
	Py_DECREF(object_wrapper);
	pyglib_gil_state_release(state);
	return;
    }
    Py_DECREF(object_wrapper);

    /* construct Python tuple for the parameter values; don't copy boxed values
       initially because we'll check after the call to see if a copy is needed. */
    params = PyTuple_New(n_param_values - 1);
    for (i = 1; i < n_param_values; i++) {
	PyObject *item = pyg_value_as_pyobject(&param_values[i], FALSE);

	/* error condition */
	if (!item) {
	    Py_DECREF(params);
	    pyglib_gil_state_release(state);
	    return;
	}
	PyTuple_SetItem(params, i - 1, item);
    }

    ret = PyObject_CallObject(method, params);

    /* Copy boxed values if others ref them, this needs to be done regardless of
       exception status. */
    len = PyTuple_Size(params);
    for (i = 0; i < len; i++) {
	PyObject *item = PyTuple_GetItem(params, i);
	if (item != NULL && PyObject_TypeCheck(item, &PyGBoxed_Type)
	    && item->ob_refcnt != 1) {
	    PyGBoxed* boxed_item = (PyGBoxed*)item;
	    if (!boxed_item->free_on_dealloc) {
		gpointer boxed_ptr = pyg_boxed_get_ptr (boxed_item);
		pyg_boxed_set_ptr (boxed_item, g_boxed_copy (boxed_item->gtype, boxed_ptr));
		boxed_item->free_on_dealloc = TRUE;
	    }
	}
    }

    if (ret == NULL) {
	PyErr_Print();
	Py_DECREF(method);
	Py_DECREF(params);
	pyglib_gil_state_release(state);
	return;
    }
    Py_DECREF(method);
    Py_DECREF(params);
    if (G_IS_VALUE(return_value))
	pyg_value_from_pyobject(return_value, ret);
    Py_DECREF(ret);
    pyglib_gil_state_release(state);
}

/**
 * pyg_signal_class_closure_get:
 *
 * Returns the GClosure used for the class closure of signals.  When
 * called, it will invoke the method do_signalname (for the signal
 * "signalname").
 *
 * Returns: the closure.
 */
GClosure *
pyg_signal_class_closure_get(void)
{
    static GClosure *closure;

    if (closure == NULL) {
	closure = g_closure_new_simple(sizeof(GClosure), NULL);
	g_closure_set_marshal(closure, pyg_signal_class_closure_marshal);

	g_closure_ref(closure);
	g_closure_sink(closure);
    }
    return closure;
}

GClosure *
gclosure_from_pyfunc(PyGObject *object, PyObject *func)
{
    GSList *l;
    PyGObjectData *inst_data;
    inst_data = pyg_object_peek_inst_data(object->obj);
    if (inst_data) {
        for (l = inst_data->closures; l; l = l->next) {
            PyGClosure *pyclosure = l->data;
            int res = PyObject_RichCompareBool(pyclosure->callback, func, Py_EQ);
            if (res == -1) {
                PyErr_Clear(); /* Is there anything else to do? */
            } else if (res) {
                return (GClosure*)pyclosure;
            }
        }
    }
    return NULL;
}

/* ----- __doc__ descriptor for GObject and GInterface ----- */

static void
object_doc_dealloc(PyObject *self)
{
    PyObject_FREE(self);
}

/* append information about signals of a particular gtype */
static void
add_signal_docs(GType gtype, GString *string)
{
    GTypeClass *class = NULL;
    guint *signal_ids, n_ids = 0, i;

    if (G_TYPE_IS_CLASSED(gtype))
	class = g_type_class_ref(gtype);
    signal_ids = g_signal_list_ids(gtype, &n_ids);

    if (n_ids > 0) {
	g_string_append_printf(string, "Signals from %s:\n",
			       g_type_name(gtype));

	for (i = 0; i < n_ids; i++) {
	    GSignalQuery query;
	    guint j;

	    g_signal_query(signal_ids[i], &query);

	    g_string_append(string, "  ");
	    g_string_append(string, query.signal_name);
	    g_string_append(string, " (");
	    for (j = 0; j < query.n_params; j++) {
		g_string_append(string, g_type_name(query.param_types[j]));
		if (j != query.n_params - 1)
		    g_string_append(string, ", ");
	    }
	    g_string_append(string, ")");
	    if (query.return_type && query.return_type != G_TYPE_NONE) {
		g_string_append(string, " -> ");
		g_string_append(string, g_type_name(query.return_type));
	    }
	    g_string_append(string, "\n");
	}
	g_free(signal_ids);
	g_string_append(string, "\n");
    }
    if (class)
	g_type_class_unref(class);
}

static void
add_property_docs(GType gtype, GString *string)
{
    GObjectClass *class;
    GParamSpec **props;
    guint n_props = 0, i;
    gboolean has_prop = FALSE;
    G_CONST_RETURN gchar *blurb=NULL;

    class = g_type_class_ref(gtype);
    props = g_object_class_list_properties(class, &n_props);

    for (i = 0; i < n_props; i++) {
	if (props[i]->owner_type != gtype)
	    continue; /* these are from a parent type */

	/* print out the heading first */
	if (!has_prop) {
	    g_string_append_printf(string, "Properties from %s:\n",
				   g_type_name(gtype));
	    has_prop = TRUE;
	}
	g_string_append_printf(string, "  %s -> %s: %s\n",
			       g_param_spec_get_name(props[i]),
			       g_type_name(props[i]->value_type),
			       g_param_spec_get_nick(props[i]));

	/* g_string_append_printf crashes on win32 if the third
	   argument is NULL. */
	blurb=g_param_spec_get_blurb(props[i]);
	if (blurb)
	    g_string_append_printf(string, "    %s\n",blurb);
    }
    g_free(props);
    if (has_prop)
	g_string_append(string, "\n");
    g_type_class_unref(class);
}

static PyObject *
object_doc_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    GType gtype = 0;
    GString *string;
    PyObject *pystring;

    if (obj && pygobject_check(obj, &PyGObject_Type)) {
	gtype = G_OBJECT_TYPE(pygobject_get(obj));
	if (!gtype)
	    PyErr_SetString(PyExc_RuntimeError, "could not get object type");
    } else {
	gtype = pyg_type_from_object(type);
    }
    if (!gtype)
	return NULL;

    string = g_string_new_len(NULL, 512);

    if (g_type_is_a(gtype, G_TYPE_INTERFACE))
	g_string_append_printf(string, "Interface %s\n\n", g_type_name(gtype));
    else if (g_type_is_a(gtype, G_TYPE_OBJECT))
	g_string_append_printf(string, "Object %s\n\n", g_type_name(gtype));
    else
	g_string_append_printf(string, "%s\n\n", g_type_name(gtype));

    if (((PyTypeObject *) type)->tp_doc)
        g_string_append_printf(string, "%s\n\n", ((PyTypeObject *) type)->tp_doc);

    if (g_type_is_a(gtype, G_TYPE_OBJECT)) {
	GType parent = G_TYPE_OBJECT;
        GArray *parents = g_array_new(FALSE, FALSE, sizeof(GType));
        int iparent;

        while (parent) {
            g_array_append_val(parents, parent);
            parent = g_type_next_base(gtype, parent);
        }

        for (iparent = parents->len - 1; iparent >= 0; --iparent) {
	    GType *interfaces;
	    guint n_interfaces, i;

            parent = g_array_index(parents, GType, iparent);
	    add_signal_docs(parent, string);
	    add_property_docs(parent, string);

	    /* add docs for implemented interfaces */
	    interfaces = g_type_interfaces(parent, &n_interfaces);
	    for (i = 0; i < n_interfaces; i++)
		add_signal_docs(interfaces[i], string);
	    g_free(interfaces);
	}
        g_array_free(parents, TRUE);
    }

    pystring = PYGLIB_PyUnicode_FromStringAndSize(string->str, string->len);
    g_string_free(string, TRUE);
    return pystring;
}

PYGLIB_DEFINE_TYPE("gobject.GObject.__doc__", PyGObjectDoc_Type, PyObject);

/**
 * pyg_object_descr_doc_get:
 *
 * Returns an object intended to be the __doc__ attribute of GObject
 * wrappers.  When read in the context of the object it will return
 * some documentation about the signals and properties of the object.
 *
 * Returns: the descriptor.
 */
PyObject *
pyg_object_descr_doc_get(void)
{
    static PyObject *doc_descr = NULL;

    if (!doc_descr) {
	Py_TYPE(&PyGObjectDoc_Type) = &PyType_Type;
	if (PyType_Ready(&PyGObjectDoc_Type))
	    return NULL;

	doc_descr = PyObject_NEW(PyObject, &PyGObjectDoc_Type);
	if (doc_descr == NULL)
	    return NULL;
    }
    return doc_descr;
}


/**
 * pyg_pyobj_to_unichar_conv:
 *
 * Converts PyObject value to a unichar and write result to memory
 * pointed to by ptr.  Follows the calling convention of a ParseArgs
 * converter (O& format specifier) so it may be used to convert function
 * arguments.
 *
 * Returns: 1 if the conversion succeeds and 0 otherwise.  If the conversion
 *          did not succeesd, a Python exception is raised
 */
int pyg_pyobj_to_unichar_conv(PyObject* py_obj, void* ptr)
{
    gunichar* u = ptr;
    const Py_UNICODE* uni_buffer;
    PyObject* tmp_uni = NULL;

    if (PyUnicode_Check(py_obj)) {
	tmp_uni = py_obj;
	Py_INCREF(tmp_uni);
    }
    else {
	tmp_uni = PyUnicode_FromObject(py_obj);
	if (tmp_uni == NULL)
	    goto failure;
    }

    if ( PyUnicode_GetSize(tmp_uni) != 1) {
	PyErr_SetString(PyExc_ValueError, "unicode character value must be 1 character uniode string");
	goto failure;
    }
    uni_buffer = PyUnicode_AsUnicode(tmp_uni);
    if ( uni_buffer == NULL)
	goto failure;
    *u = uni_buffer[0];

    Py_DECREF(tmp_uni);
    return 1;

  failure:
    Py_XDECREF(tmp_uni);
    return 0;
}

gboolean
pyg_gtype_is_custom(GType gtype)
{
    return g_type_get_qdata (gtype, pygobject_custom_key) != NULL;
}

void
pygobject_type_register_types(PyObject *d)
{
    PyGTypeWrapper_Type.tp_dealloc = (destructor)pyg_type_wrapper_dealloc;
    PyGTypeWrapper_Type.tp_richcompare = pyg_type_wrapper_richcompare;
    PyGTypeWrapper_Type.tp_repr = (reprfunc)pyg_type_wrapper_repr;
    PyGTypeWrapper_Type.tp_hash = (hashfunc)pyg_type_wrapper_hash;
    PyGTypeWrapper_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGTypeWrapper_Type.tp_methods = _PyGTypeWrapper_methods;
    PyGTypeWrapper_Type.tp_getset = _PyGTypeWrapper_getsets;
    PyGTypeWrapper_Type.tp_init = (initproc)pyg_type_wrapper_init;
    PYGLIB_REGISTER_TYPE(d, PyGTypeWrapper_Type, "GType");

    /* This type lazily registered in pyg_object_descr_doc_get */
    PyGObjectDoc_Type.tp_dealloc = (destructor)object_doc_dealloc;
    PyGObjectDoc_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGObjectDoc_Type.tp_descr_get = (descrgetfunc)object_doc_descr_get;

    pyg_register_gtype_custom(G_TYPE_STRV,
			      pyg_strv_from_gvalue,
			      pyg_strv_to_gvalue);
}

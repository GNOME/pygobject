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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "pygobject-private.h"

static const gchar *pygenum_class_id     = "PyGEnum::class";
static GQuark       pygenum_class_key    = 0;

#define GET_INT(x) (((PyIntObject*)x)->ob_ival)
static int
pyg_enum_compare(PyGEnum *self, PyObject *other)
{
    if (!PyInt_CheckExact(other) && ((PyGEnum*)other)->gtype != self->gtype) {
	PyErr_Warn(PyExc_Warning, "comparing different enum types");
	return -1;
    }
    
    if (GET_INT(self) == GET_INT(other))
      return 0;
    else if (GET_INT(self) < GET_INT(other))
      return -1;
    else
      return 1;
}
#undef GET_INT
    
static PyObject *
pyg_enum_repr(PyGEnum *self)
{
  GEnumClass *enum_class;
  char *value;
  guint index;
  static char tmp[256];
  
  enum_class = g_type_class_ref(self->gtype);
  g_assert(G_IS_ENUM_CLASS(enum_class));

  for (index = 0; index < enum_class->n_values; index++)
      if (self->parent.ob_ival == enum_class->values[index].value)
          break;
  value = enum_class->values[index].value_name;
  if (value)
      sprintf(tmp, "<enum %s of type %s>", value, g_type_name(self->gtype));
  else
      sprintf(tmp, "<enum %ld of type %s>", self->parent.ob_ival, g_type_name(self->gtype));

  g_type_class_unref(enum_class);

  return PyString_FromString(tmp);
}

static PyObject *
pyg_enum_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "value", NULL };
    long value;
    PyObject *pytc, *values, *ret;
    GType gtype;
    GEnumClass *eclass;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "l", kwlist, &value))
	return NULL;
    
    pytc = PyObject_GetAttrString((PyObject *)type, "__gtype__");
    if (!pytc)
	return NULL;
    
    if (!PyObject_TypeCheck(pytc, &PyGTypeWrapper_Type)) {
	Py_DECREF(pytc);
	PyErr_SetString(PyExc_TypeError,
			"__gtype__ attribute not a typecode");
	return NULL;
    }
    
    gtype = pyg_type_from_object(pytc);
    Py_DECREF(pytc);

    eclass = G_ENUM_CLASS(g_type_class_ref(gtype));

    if (value < 0 || value > eclass->n_values) {
	PyErr_SetString(PyExc_ValueError, "value out of range");
	g_type_class_unref(eclass);
	return NULL;
    }

    values = PyObject_GetAttrString((PyObject *)type, "__enum_values__");
    if (!values) {
	g_type_class_unref(eclass);
	return NULL;
    }
    
    if (!PyDict_Check(values) || PyDict_Size(values) != eclass->n_values) {
	PyErr_SetString(PyExc_TypeError, "__enum_values__ badly formed");
	Py_DECREF(values);
	g_type_class_unref(eclass);
	return NULL;
    }

    g_type_class_unref(eclass);
    
    ret = PyDict_GetItem(values, PyInt_FromLong(value));
    Py_INCREF(ret);
    Py_DECREF(values);
    return ret;
}

PyObject*
pyg_enum_from_gtype (GType gtype, int value)
{
    PyObject *pyclass, *values, *retval;

    g_return_val_if_fail(gtype != G_TYPE_INVALID, NULL);
    
    pyclass = (PyObject*)g_type_get_qdata(gtype, pygenum_class_key);
    if (pyclass == NULL) {
	pyclass = pyg_enum_add(NULL, g_type_name(gtype), NULL, gtype);
	if (!pyclass)
	    return PyInt_FromLong(value);
    }
    
    values = PyDict_GetItemString(((PyTypeObject *)pyclass)->tp_dict,
				  "__enum_values__");
    retval = PyDict_GetItem(values, PyInt_FromLong(value));
    if (!retval) {
	PyErr_Clear();
	retval = ((PyTypeObject *)pyclass)->tp_alloc((PyTypeObject *)pyclass, 0);
	g_assert(retval != NULL);
	
	((PyIntObject*)retval)->ob_ival = value;
	((PyGFlags*)retval)->gtype = gtype;
	//return PyInt_FromLong(value);
    }
    
    Py_INCREF(retval);
    return retval;
}

PyObject *
pyg_enum_add (PyObject *   module,
	      const char * typename,
	      const char * strip_prefix,
	      GType        gtype)
{
    PyGILState_STATE state;
    PyObject *instance_dict, *stub, *values, *o;
    GEnumClass *eclass;
    int i;
    
    g_return_val_if_fail(typename != NULL, NULL);
    g_return_val_if_fail(g_type_is_a(gtype, G_TYPE_ENUM), NULL);
    
    state = pyg_gil_state_ensure();

    instance_dict = PyDict_New();
    stub = PyObject_CallFunction((PyObject *)&PyType_Type, "s(O)O",
                                 typename, (PyObject *)&PyGEnum_Type,
                                 instance_dict);
    Py_DECREF(instance_dict);
    if (!stub) {
	PyErr_SetString(PyExc_RuntimeError, "can't create const");
	pyg_gil_state_release(state);
	return NULL;
    }

    ((PyTypeObject *)stub)->tp_flags &= ~Py_TPFLAGS_BASETYPE;
    ((PyTypeObject *)stub)->tp_new = pyg_enum_new;

    if (module)
	PyDict_SetItemString(((PyTypeObject *)stub)->tp_dict,
			     "__module__",
			     PyString_FromString(PyModule_GetName(module)));
    
    if (!pygenum_class_key)
        pygenum_class_key = g_quark_from_static_string(pygenum_class_id);

    g_type_set_qdata(gtype, pygenum_class_key, stub);

    o = pyg_type_wrapper_new(gtype);
    PyDict_SetItemString(((PyTypeObject *)stub)->tp_dict, "__gtype__", o);
    Py_DECREF(o);

    if (module) {
	/* Add it to the module name space */
	PyModule_AddObject(module, (char*)typename, stub);
	Py_INCREF(stub);
    }

    /* Register enum values */
    eclass = G_ENUM_CLASS(g_type_class_ref(gtype));

    values = PyDict_New();
    for (i = 0; i < eclass->n_values; i++) {
	PyObject *item;
      
	item = ((PyTypeObject *)stub)->tp_alloc((PyTypeObject *)stub, 0);
	((PyIntObject*)item)->ob_ival = eclass->values[i].value;
	((PyGEnum*)item)->gtype = gtype;
	
	PyDict_SetItem(values, PyInt_FromLong(eclass->values[i].value), item);
	
	if (module) {
	    PyModule_AddObject(module,
			       pyg_constant_strip_prefix(eclass->values[i].value_name,
							 strip_prefix),
			       item);
	    Py_INCREF(item);
	}
    }
    
    PyDict_SetItemString(((PyTypeObject *)stub)->tp_dict,
			 "__enum_values__", values);
    Py_DECREF(values);

    g_type_class_unref(eclass);
    
    pyg_gil_state_release(state);
    return stub;
}

static PyObject *
pyg_enum_get_value_name(PyGEnum *self, void *closure)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  PyObject *retval;
  
  enum_class = g_type_class_ref(self->gtype);
  g_assert(G_IS_ENUM_CLASS(enum_class));
  
  enum_value = g_enum_get_value(enum_class, self->parent.ob_ival);

  retval = PyString_FromString(enum_value->value_name);
  g_type_class_unref(enum_class);

  return retval;
}

static PyObject *
pyg_enum_get_value_nick(PyGEnum *self, void *closure)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  PyObject *retval;
  
  enum_class = g_type_class_ref(self->gtype);
  g_assert(G_IS_ENUM_CLASS(enum_class));
  
  enum_value = g_enum_get_value(enum_class, self->parent.ob_ival);

  retval = PyString_FromString(enum_value->value_nick);
  g_type_class_unref(enum_class);

  return retval;
}


static PyGetSetDef pyg_enum_getsets[] = {
    { "value_name", (getter)pyg_enum_get_value_name, (setter)0 },
    { "value_nick", (getter)pyg_enum_get_value_nick, (setter)0 },
    { NULL, 0, 0 }
};

PyTypeObject PyGEnum_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"gobject.GEnum",
	sizeof(PyGEnum),
	0,
	0,					  /* tp_dealloc */
	0,                			  /* tp_print */
	0,					  /* tp_getattr */
	0,					  /* tp_setattr */
	(cmpfunc)pyg_enum_compare,		  /* tp_compare */
	(reprfunc)pyg_enum_repr,		  /* tp_repr */
	0,                   			  /* tp_as_number */
	0,					  /* tp_as_sequence */
	0,					  /* tp_as_mapping */
	0,					  /* tp_hash */
        0,					  /* tp_call */
	(reprfunc)pyg_enum_repr,		  /* tp_str */
	0,					  /* tp_getattro */
	0,					  /* tp_setattro */
	0,				  	  /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	0,      				  /* tp_doc */
	0,					  /* tp_traverse */
	0,					  /* tp_clear */
	0,					  /* tp_richcompare */
	0,					  /* tp_weaklistoffset */
	0,					  /* tp_iter */
	0,					  /* tp_iternext */
	0,					  /* tp_methods */
	0,					  /* tp_members */
	pyg_enum_getsets,			  /* tp_getset */
	0,	  				  /* tp_base */
	0,					  /* tp_dict */
	0,					  /* tp_descr_get */
	0,					  /* tp_descr_set */
	0,					  /* tp_dictoffset */
	0,				  	  /* tp_init */
	0,					  /* tp_alloc */
	pyg_enum_new,				  /* tp_new */
};


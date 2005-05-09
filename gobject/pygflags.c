/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 * Copyright (C) 2004       Johan Dahlin
 *
 *   pygenum.c: GFlags wrapper
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

static const gchar *pygflags_class_id     = "PyGFlags::class";
static GQuark       pygflags_class_key    = 0;

#define GET_INT_VALUE(x) (((PyIntObject*)x)->ob_ival)
static int
pyg_flags_compare(PyGFlags *self, PyObject *other)
{
    if (!PyInt_CheckExact(other) && ((PyGFlags*)other)->gtype != self->gtype) {
	PyErr_Warn(PyExc_Warning, "comparing different flags types");
	return -1;
    }

    if (GET_INT_VALUE(self) == GET_INT_VALUE(other))
      return 0;
    else if (GET_INT_VALUE(self) < GET_INT_VALUE(other))
      return -1;
    else
      return 1;
}

static char *
generate_repr(GType gtype, int value)
{
    GFlagsClass *flags_class;
    char *retval = NULL, *tmp;
    int i;

    flags_class = g_type_class_ref(gtype);
    g_assert(G_IS_FLAGS_CLASS(flags_class));
  
    for (i = 0; i < flags_class->n_values; i++) {
	/* Some types (eg GstElementState in GStreamer 0.8) has flags with 0 values,
         * we're just ignore them for now otherwise they'll always show up
         */
        if (flags_class->values[i].value == 0)
            continue;

        if ((value & flags_class->values[i].value) == flags_class->values[i].value) {
	    if (retval) {
		tmp = g_strdup_printf("%s | %s", retval, flags_class->values[i].value_name);
		g_free(retval);
		retval = tmp;
	    } else {
		retval = g_strdup_printf("%s", flags_class->values[i].value_name);
	    }
	}
    }
    
    g_type_class_unref(flags_class);

    return retval;
}
	    
static PyObject *
pyg_flags_repr(PyGFlags *self)
{
    char *tmp, *retval;
    PyObject *pyretval;
  
    tmp = generate_repr(self->gtype, self->parent.ob_ival);

    if (tmp)
        retval = g_strdup_printf("<flags %s of type %s>", tmp,
                                 g_type_name(self->gtype));
    else
        retval = g_strdup_printf("<flags %ld of type %s>", self->parent.ob_ival,
                                 g_type_name(self->gtype));
    g_free(tmp);
    
    pyretval = PyString_FromString(retval);
    g_free(retval);
  
    return pyretval;
}

static PyObject *
pyg_flags_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "value", NULL };
    long value;
    PyObject *pytc, *values, *ret;
    GType gtype;
    GFlagsClass *eclass;
    
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

    eclass = G_FLAGS_CLASS(g_type_class_ref(gtype));

    values = PyObject_GetAttrString((PyObject *)type, "__flags_values__");
    if (!values) {
	g_type_class_unref(eclass);
	return NULL;
    }
    
    if (!PyDict_Check(values) || PyDict_Size(values) != eclass->n_values) {
	PyErr_SetString(PyExc_TypeError, "__flags_values__ badly formed");
	Py_DECREF(values);
	g_type_class_unref(eclass);
	return NULL;
    }

    g_type_class_unref(eclass);
    
    ret = PyDict_GetItem(values, PyInt_FromLong(value));
    Py_DECREF(values);
    if (ret)
        Py_INCREF(ret);
    else
        PyErr_Format(PyExc_ValueError, "invalid flag value: %ld", value);
    return ret;
}

PyObject*
pyg_flags_from_gtype (GType gtype, int value)
{
    PyObject *pyclass, *values, *retval;

    g_return_val_if_fail(gtype != G_TYPE_INVALID, NULL);
    
    pyclass = (PyObject*)g_type_get_qdata(gtype, pygflags_class_key);
    if (pyclass == NULL) {
	pyclass = pyg_flags_add(NULL, g_type_name(gtype), NULL, gtype);
	if (!pyclass)
	    return PyInt_FromLong(value);
    }

    
    values = PyDict_GetItemString(((PyTypeObject *)pyclass)->tp_dict,
				  "__flags_values__");
    retval = PyDict_GetItem(values, PyInt_FromLong(value));
    if (!retval) {
	PyErr_Clear();

	retval = ((PyTypeObject *)pyclass)->tp_alloc((PyTypeObject *)pyclass, 0);
	g_assert(retval != NULL);
	
	((PyIntObject*)retval)->ob_ival = value;
	((PyGFlags*)retval)->gtype = gtype;
    } 

    Py_INCREF(retval);
    return retval;
}

PyObject *
pyg_flags_add (PyObject *   module,
	       const char * typename,
	       const char * strip_prefix,
	       GType        gtype)
{
    PyGILState_STATE state;
    PyObject *instance_dict, *stub, *values, *o;
    GFlagsClass *eclass;
    int i;

    g_return_val_if_fail(module != NULL, NULL);
    g_return_val_if_fail(typename != NULL, NULL);
    g_return_val_if_fail(g_type_is_a(gtype, G_TYPE_FLAGS), NULL);
    
    state = pyg_gil_state_ensure();

    instance_dict = PyDict_New();
    stub = PyObject_CallFunction((PyObject *)&PyType_Type, "s(O)O",
                                 typename, (PyObject *)&PyGFlags_Type,
                                 instance_dict);
    Py_DECREF(instance_dict);
    if (!stub) {
	PyErr_SetString(PyExc_RuntimeError, "can't create const");
	pyg_gil_state_release(state);
        return NULL;
    }
    
    ((PyTypeObject *)stub)->tp_flags &= ~Py_TPFLAGS_BASETYPE;
    ((PyTypeObject *)stub)->tp_new = pyg_flags_new;

    PyDict_SetItemString(((PyTypeObject *)stub)->tp_dict,
			 "__module__",
			 PyString_FromString(PyModule_GetName(module)));
    
    /* Add it to the module name space */
    PyModule_AddObject(module, (char*)typename, stub);
    Py_INCREF(stub);

    if (!pygflags_class_key)
        pygflags_class_key = g_quark_from_static_string(pygflags_class_id);

    g_type_set_qdata(gtype, pygflags_class_key, stub);

    o = pyg_type_wrapper_new(gtype);
    PyDict_SetItemString(((PyTypeObject *)stub)->tp_dict, "__gtype__", o);
    Py_DECREF(o);
    
    /* Register flag values */
    eclass = G_FLAGS_CLASS(g_type_class_ref(gtype));

    values = PyDict_New();
    for (i = 0; i < eclass->n_values; i++) {
      PyObject *item;
      
      item = ((PyTypeObject *)stub)->tp_alloc((PyTypeObject *)stub, 0);
      ((PyIntObject*)item)->ob_ival = eclass->values[i].value;
      ((PyGFlags*)item)->gtype = gtype;
            
      PyDict_SetItem(values, PyInt_FromLong(eclass->values[i].value), item);

      PyModule_AddObject(module,
			 pyg_constant_strip_prefix(eclass->values[i].value_name,
						   strip_prefix),
			 item);
      Py_INCREF(item);
    }
    
    PyDict_SetItemString(((PyTypeObject *)stub)->tp_dict,
			 "__flags_values__", values);
    Py_DECREF(values);

    g_type_class_unref(eclass);

    pyg_gil_state_release(state);

    return stub;
}

static PyObject *
pyg_flags_and(PyGFlags *a, PyGFlags *b)
{
	if (!PyGFlags_Check(a) || !PyGFlags_Check(b))
		return PyInt_Type.tp_as_number->nb_and((PyObject*)a,
						       (PyObject*)b);
	
	return pyg_flags_from_gtype(a->gtype,
				    GET_INT_VALUE(a) & GET_INT_VALUE(b));
}

static PyObject *
pyg_flags_or(PyGFlags *a, PyGFlags *b)
{
	if (!PyGFlags_Check(a) || !PyGFlags_Check(b))
		return PyInt_Type.tp_as_number->nb_or((PyObject*)a,
						      (PyObject*)b);

	return pyg_flags_from_gtype(a->gtype, GET_INT_VALUE(a) | GET_INT_VALUE(b));
}

static PyObject *
pyg_flags_xor(PyGFlags *a, PyGFlags *b)
{
	if (!PyGFlags_Check(a) || !PyGFlags_Check(b))
		return PyInt_Type.tp_as_number->nb_xor((PyObject*)a,
						       (PyObject*)b);

	return pyg_flags_from_gtype(a->gtype,
				    GET_INT_VALUE(a) ^ GET_INT_VALUE(b));

}

static PyObject *
pyg_flags_warn (PyObject *self, PyObject *args)
{
    if (PyErr_Warn(PyExc_Warning, "unsupported arithmetic operation for flags type"))
	return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pyg_flags_get_first_value_name(PyGFlags *self, void *closure)
{
  GFlagsClass *flags_class;
  GFlagsValue *flags_value;
  PyObject *retval;
  
  flags_class = g_type_class_ref(self->gtype);
  g_assert(G_IS_FLAGS_CLASS(flags_class));
  flags_value = g_flags_get_first_value(flags_class, self->parent.ob_ival);
  retval = PyString_FromString(flags_value->value_name);

  g_type_class_unref(flags_class);

  return retval;
}

static PyObject *
pyg_flags_get_first_value_nick(PyGFlags *self, void *closure)
{
  GFlagsClass *flags_class;
  GFlagsValue *flags_value;
  PyObject *retval;
  
  flags_class = g_type_class_ref(self->gtype);
  g_assert(G_IS_FLAGS_CLASS(flags_class));

  flags_value = g_flags_get_first_value(flags_class, self->parent.ob_ival);
  retval = PyString_FromString(flags_value->value_nick);
  g_type_class_unref(flags_class);

  return retval;
}

static PyObject *
pyg_flags_get_value_names(PyGFlags *self, void *closure)
{
  GFlagsClass *flags_class;
  PyObject *retval;
  int i;
  
  flags_class = g_type_class_ref(self->gtype);
  g_assert(G_IS_FLAGS_CLASS(flags_class));
  
  retval = PyList_New(0);
  for (i = 0; i < flags_class->n_values; i++)
      if ((self->parent.ob_ival & flags_class->values[i].value) == flags_class->values[i].value)
	  PyList_Append(retval, PyString_FromString(flags_class->values[i].value_name));

  g_type_class_unref(flags_class);

  return retval;
}

static PyObject *
pyg_flags_get_value_nicks(PyGFlags *self, void *closure)
{
  GFlagsClass *flags_class;
  PyObject *retval;
  int i;
  
  flags_class = g_type_class_ref(self->gtype);
  g_assert(G_IS_FLAGS_CLASS(flags_class));
  
  retval = PyList_New(0);
  for (i = 0; i < flags_class->n_values; i++)
      if ((self->parent.ob_ival & flags_class->values[i].value) == flags_class->values[i].value)
	  PyList_Append(retval, PyString_FromString(flags_class->values[i].value_nick));

  g_type_class_unref(flags_class);

  return retval;
}

static PyGetSetDef pyg_flags_getsets[] = {
    { "first_value_name", (getter)pyg_flags_get_first_value_name, (setter)0 },
    { "first_value_nick", (getter)pyg_flags_get_first_value_nick, (setter)0 },
    { "value_names", (getter)pyg_flags_get_value_names, (setter)0 },
    { "value_nicks", (getter)pyg_flags_get_value_nicks, (setter)0 },
    { NULL, 0, 0 }
};

static PyNumberMethods pyg_flags_as_number = {
	(binaryfunc)pyg_flags_warn,		/* nb_add */
	(binaryfunc)pyg_flags_warn,		/* nb_subtract */
	(binaryfunc)pyg_flags_warn,		/* nb_multiply */
	(binaryfunc)pyg_flags_warn,		/* nb_divide */
	(binaryfunc)pyg_flags_warn,		/* nb_remainder */
	(binaryfunc)pyg_flags_warn,		/* nb_divmod */
	(ternaryfunc)pyg_flags_warn,		/* nb_power */
	0,					/* nb_negative */
	0,					/* nb_positive */
	0,					/* nb_absolute */
	0,					/* nb_nonzero */
	0,					/* nb_invert */
	0,					/* nb_lshift */
	0,					/* nb_rshift */
	(binaryfunc)pyg_flags_and,		/* nb_and */
	(binaryfunc)pyg_flags_xor,		/* nb_xor */
	(binaryfunc)pyg_flags_or,		/* nb_or */
	0,					/* nb_coerce */
	0,					/* nb_int */
	0,					/* nb_long */
	0,					/* nb_float */
	0,					/* nb_oct */
	0,		 			/* nb_hex */
	0,					/* nb_inplace_add */
	0,					/* nb_inplace_subtract */
	0,					/* nb_inplace_multiply */
	0,					/* nb_inplace_divide */
	0,					/* nb_inplace_remainder */
	0,					/* nb_inplace_power */
	0,					/* nb_inplace_lshift */
	0,					/* nb_inplace_rshift */
	0,					/* nb_inplace_and */
	0,					/* nb_inplace_xor */
	0,					/* nb_inplace_or */
	0,					/* nb_floor_divide */
	0,					/* nb_true_divide */
	0,					/* nb_inplace_floor_divide */
	0,					/* nb_inplace_true_divide */
};

PyTypeObject PyGFlags_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"gobject.GFlags",
	sizeof(PyGFlags),
	0,
	0,					  /* tp_dealloc */
	0,                			  /* tp_print */
	0,					  /* tp_getattr */
	0,					  /* tp_setattr */
	(cmpfunc)pyg_flags_compare,		  /* tp_compare */
	(reprfunc)pyg_flags_repr,		  /* tp_repr */
	&pyg_flags_as_number,			  /* tp_as_number */
	0,					  /* tp_as_sequence */
	0,					  /* tp_as_mapping */
	0,					  /* tp_hash */
        0,					  /* tp_call */
	(reprfunc)pyg_flags_repr,		  /* tp_str */
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
	pyg_flags_getsets,			  /* tp_getset */
	0,	  				  /* tp_base */
	0,					  /* tp_dict */
	0,					  /* tp_descr_get */
	0,					  /* tp_descr_set */
	0,					  /* tp_dictoffset */
	0,					  /* tp_init */
	0,					  /* tp_alloc */
	pyg_flags_new,				  /* tp_new */
};

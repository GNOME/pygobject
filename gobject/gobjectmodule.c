/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   gobjectmodule.c: wrapper for the gobject library.
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
#include "pythread.h"
#include <gobject/gvaluecollector.h>

static PyObject *gerror_exc = NULL;
static gboolean use_gil_state_api = FALSE;
static PyObject *_pyg_signal_accumulator_true_handled_func;

GQuark pyginterface_type_key;
GQuark pygobject_class_init_key;
GQuark pyginterface_info_key;
GQuark pygboxed_type_key;
GQuark pygobject_class_key;
GQuark pygobject_wrapper_key;
GQuark pygboxed_marshal_key;
GQuark pygenum_class_key;
GQuark pygflags_class_key;
GQuark pygpointer_class_key;
GQuark pygobject_has_updated_constructor_key;



static void pyg_flags_add_constants(PyObject *module, GType flags_type,
				    const gchar *strip_prefix);

    
/* -------------- GDK threading hooks ---------------------------- */

/**
 * pyg_set_thread_block_funcs:
 * @block_threads_func: a function to block Python threads.
 * @unblock_threads_func: a function to unblock Python threads.
 *
 * an interface to allow pygtk to add hooks to handle threading
 * similar to the old PyGTK 0.6.x releases.  May not work quite right
 * anymore.
 */
static void
pyg_set_thread_block_funcs (PyGThreadBlockFunc block_threads_func,
			    PyGThreadBlockFunc unblock_threads_func)
{
    g_return_if_fail(pygobject_api_functions.block_threads == NULL && pygobject_api_functions.unblock_threads == NULL);

    pygobject_api_functions.block_threads   = block_threads_func;
    pygobject_api_functions.unblock_threads = unblock_threads_func;
}

static void
object_free(PyObject *op)
{
    PyObject_FREE(op);
}

/**
 * pyg_destroy_notify:
 * @user_data: a PyObject pointer.
 *
 * A function that can be used as a GDestroyNotify callback that will
 * call Py_DECREF on the data.
 */
void
pyg_destroy_notify(gpointer user_data)
{
    PyObject *obj = (PyObject *)user_data;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();
    Py_DECREF(obj);
    pyg_gil_state_release(state);
}


/* ---------------- GBoxed functions -------------------- */

GType PY_TYPE_OBJECT = 0;

static gpointer
pyobject_copy(gpointer boxed)
{
    PyObject *object = boxed;

    Py_INCREF(object);
    return object;
}

static void
pyobject_free(gpointer boxed)
{
    PyObject *object = boxed;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();
    Py_DECREF(object);
    pyg_gil_state_release(state);
}


/* ---------------- GInterface functions -------------------- */

static int
pyg_interface_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    gchar buf[512];

    if (!PyArg_ParseTuple(args, ":GInterface.__init__"))
	return -1;

    g_snprintf(buf, sizeof(buf), "%s can not be constructed", self->ob_type->tp_name);
    PyErr_SetString(PyExc_NotImplementedError, buf);
    return -1;
}

PyTypeObject PyGInterface_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gobject.GInterface",               /* tp_name */
    sizeof(PyObject),                   /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)0,                      /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)0,                        /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
    (reprfunc)0,                        /* tp_str */
    (getattrofunc)0,                    /* tp_getattro */
    (setattrofunc)0,                    /* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,			/* tp_traverse */
    (inquiry)0,				/* tp_clear */
    (richcmpfunc)0,			/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    (getiterfunc)0,			/* tp_iter */
    (iternextfunc)0,			/* tp_iternext */
    0,					/* tp_methods */
    0,					/* tp_members */
    0,					/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)pyg_interface_init,	/* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
    (freefunc)object_free,		/* tp_free */
    (inquiry)0,				/* tp_is_gc */
    (PyObject *)0,			/* tp_bases */
};

/**
 * pyg_register_interface:
 * @dict: a module dictionary.
 * @class_name: the class name for the wrapper class.
 * @gtype: the GType of the interface.
 * @type: the wrapper class for the interface.
 *
 * Registers a Python class as the wrapper for a GInterface.  As a
 * convenience it will also place a reference to the wrapper class in
 * the provided module dictionary.
 */
static void
pyg_register_interface(PyObject *dict, const gchar *class_name,
                       GType gtype, PyTypeObject *type)
{
    PyObject *o;

    type->ob_type = &PyType_Type;
    type->tp_base = &PyGInterface_Type;

    if (PyType_Ready(type) < 0) {
        g_warning("could not ready `%s'", type->tp_name);
        return;
    }

    if (gtype) {
        o = pyg_type_wrapper_new(gtype);
        PyDict_SetItemString(type->tp_dict, "__gtype__", o);
        Py_DECREF(o);
    }

    g_type_set_qdata(gtype, pyginterface_type_key, type);
    
    PyDict_SetItemString(dict, (char *)class_name, (PyObject *)type);
    
}

static void
pyg_register_interface_info(GType gtype, const GInterfaceInfo *info)
{
    g_type_set_qdata(gtype, pyginterface_info_key, (gpointer) info);
}

static const GInterfaceInfo *
pyg_lookup_interface_info(GType gtype)
{
    return g_type_get_qdata(gtype, pyginterface_info_key);
}

/* -------------- GMainContext objects ---------------------------- */


/* ---------------- gobject module functions -------------------- */

static PyObject *
pyg_type_name (PyObject *self, PyObject *args)
{
    PyObject *gtype;
    GType type;
    const gchar *name;

#if 0
    if (PyErr_Warn(PyExc_DeprecationWarning,
		   "gobject.type_name is deprecated; "
		   "use GType.name instead"))
        return NULL;
#endif
    
    if (!PyArg_ParseTuple(args, "O:gobject.type_name", &gtype))
	return NULL;
    if ((type = pyg_type_from_object(gtype)) == 0)
	return NULL;
    name = g_type_name(type);
    if (name)
	return PyString_FromString(name);
    PyErr_SetString(PyExc_RuntimeError, "unknown typecode");
    return NULL;
}

static PyObject *
pyg_type_from_name (PyObject *self, PyObject *args)
{
    const gchar *name;
    GType type;
#if 0
    if (PyErr_Warn(PyExc_DeprecationWarning,
		   "gobject.type_from_name is deprecated; "
		   "use GType.from_name instead"))
        return NULL;
#endif    
    if (!PyArg_ParseTuple(args, "s:gobject.type_from_name", &name))
	return NULL;
    type = g_type_from_name(name);
    if (type != 0)
	return pyg_type_wrapper_new(type);
    PyErr_Format(PyExc_RuntimeError, "%s: unknown type name: %s",
		 PyString_AsString(PyObject_Repr((PyObject*)self)),
		 name);
    return NULL;
}

static PyObject *
pyg_type_parent (PyObject *self, PyObject *args)
{
    PyObject *gtype;
    GType type, parent;
#if 0
    if (PyErr_Warn(PyExc_DeprecationWarning,
		   "gobject.type_parent is deprecated; "
		   "use GType.parent instead"))
        return NULL;
#endif    
    if (!PyArg_ParseTuple(args, "O:gobject.type_parent", &gtype))
	return NULL;
    if ((type = pyg_type_from_object(gtype)) == 0)
	return NULL;
    parent = g_type_parent(type);
    if (parent != 0)
	return pyg_type_wrapper_new(parent);
    PyErr_SetString(PyExc_RuntimeError, "no parent for type");
    return NULL;
}

static PyObject *
pyg_type_is_a (PyObject *self, PyObject *args)
{
    PyObject *gtype, *gparent;
    GType type, parent;
#if 0
    if (PyErr_Warn(PyExc_DeprecationWarning,
		   "gobject.type_is_a is deprecated; "
		   "use GType.is_a instead"))
        return NULL;
#endif    
    if (!PyArg_ParseTuple(args, "OO:gobject.type_is_a", &gtype, &gparent))
	return NULL;
    if ((type = pyg_type_from_object(gtype)) == 0)
	return NULL;
    if ((parent = pyg_type_from_object(gparent)) == 0)
	return NULL;
    return PyBool_FromLong(g_type_is_a(type, parent));
}

static PyObject *
pyg_type_children (PyObject *self, PyObject *args)
{
    PyObject *gtype, *list;
    GType type, *children;
    guint n_children, i;
#if 0
    if (PyErr_Warn(PyExc_DeprecationWarning,
		   "gobject.type_children is deprecated; "
		   "use GType.children instead"))
        return NULL;
#endif    
    if (!PyArg_ParseTuple(args, "O:gobject.type_children", &gtype))
	return NULL;
    if ((type = pyg_type_from_object(gtype)) == 0)
	return NULL;
    children = g_type_children(type, &n_children);
    if (children) {
        list = PyList_New(0);
	for (i = 0; i < n_children; i++) {
	    PyObject *o;
	    PyList_Append(list, o=pyg_type_wrapper_new(children[i]));
	    Py_DECREF(o);
	}
	g_free(children);
	return list;
    }
    PyErr_SetString(PyExc_RuntimeError, "invalid type, or no children");
    return NULL;
}

static PyObject *
pyg_type_interfaces (PyObject *self, PyObject *args)
{
    PyObject *gtype, *list;
    GType type, *interfaces;
    guint n_interfaces, i;
#if 0
    if (PyErr_Warn(PyExc_DeprecationWarning,
		   "gobject.type_interfaces is deprecated; "
		   "use GType.interfaces instead"))
        return NULL;
#endif    
    if (!PyArg_ParseTuple(args, "O:gobject.type_interfaces", &gtype))
	return NULL;
    if ((type = pyg_type_from_object(gtype)) == 0)
	return NULL;
    interfaces = g_type_interfaces(type, &n_interfaces);
    if (interfaces) {
        list = PyList_New(0);
	for (i = 0; i < n_interfaces; i++) {
	    PyObject *o;
	    PyList_Append(list, o=pyg_type_wrapper_new(interfaces[i]));
	    Py_DECREF(o);
	}
	g_free(interfaces);
	return list;
    }
    PyErr_SetString(PyExc_RuntimeError, "invalid type, or no interfaces");
    return NULL;
}

static void
pyg_object_set_property (GObject *object, guint property_id,
			 const GValue *value, GParamSpec *pspec)
{
    PyObject *object_wrapper, *retval;
    PyObject *py_pspec, *py_value;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();

    object_wrapper = pygobject_new(object);

    if (object_wrapper == NULL) {
	pyg_gil_state_release(state);
	return;
    }

    py_pspec = pyg_param_spec_new(pspec);
    py_value = pyg_value_as_pyobject (value, TRUE);

    retval = PyObject_CallMethod(object_wrapper, "do_set_property",
				 "OO", py_pspec, py_value);
    if (retval) {
	Py_DECREF(retval);
    } else {
	PyErr_Print();
    }

    Py_DECREF(object_wrapper);
    Py_DECREF(py_pspec);
    Py_DECREF(py_value);

    pyg_gil_state_release(state);
}

static void
pyg_object_get_property (GObject *object, guint property_id,
			 GValue *value, GParamSpec *pspec)
{
    PyObject *object_wrapper, *retval;
    PyObject *py_pspec;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();

    object_wrapper = pygobject_new(object);

    if (object_wrapper == NULL) {
	pyg_gil_state_release(state);
	return;
    }

    py_pspec = pyg_param_spec_new(pspec);
    retval = PyObject_CallMethod(object_wrapper, "do_get_property",
				 "O", py_pspec);
    if (retval == NULL || pyg_value_from_pyobject(value, retval) < 0) {
	PyErr_Print();
    }
    Py_DECREF(object_wrapper);
    Py_DECREF(py_pspec);
    Py_XDECREF(retval);
    
    pyg_gil_state_release(state);
}

static void
pyg_object_class_init(GObjectClass *class, PyObject *py_class)
{
    class->set_property = pyg_object_set_property;
    class->get_property = pyg_object_get_property;
}

typedef struct _PyGSignalAccumulatorData {
    PyObject *callable;
    PyObject *user_data;
} PyGSignalAccumulatorData;

static gboolean
_pyg_signal_accumulator(GSignalInvocationHint *ihint,
                        GValue *return_accu,
                        const GValue *handler_return,
                        gpointer _data)
{
    PyObject *py_ihint, *py_return_accu, *py_handler_return, *py_detail;
    PyObject *py_retval;
    gboolean retval = FALSE;
    PyGSignalAccumulatorData *data = _data;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();
    if (ihint->detail)
        py_detail = PyString_FromString(g_quark_to_string(ihint->detail));
    else {
        Py_INCREF(Py_None);
        py_detail = Py_None;
    }

    py_ihint = Py_BuildValue("lNi", (long int) ihint->signal_id,
                             py_detail, ihint->run_type);
    py_handler_return = pyg_value_as_pyobject(handler_return, TRUE);
    py_return_accu = pyg_value_as_pyobject(return_accu, FALSE);
    if (data->user_data)
        py_retval = PyObject_CallFunction(data->callable, "NNNO", py_ihint,
                                          py_return_accu, py_handler_return,
                                          data->user_data);
    else
        py_retval = PyObject_CallFunction(data->callable, "NNN", py_ihint,
                                          py_return_accu, py_handler_return);
    if (!py_retval)
	PyErr_Print();
    else {
        if (!PyTuple_Check(py_retval) || PyTuple_Size(py_retval) != 2) {
            PyErr_SetString(PyExc_TypeError, "accumulator function must return"
                            " a (bool, object) tuple");
            PyErr_Print();
        } else {
            retval = PyObject_IsTrue(PyTuple_GET_ITEM(py_retval, 0));
            if (pyg_value_from_pyobject(return_accu, PyTuple_GET_ITEM(py_retval, 1))) {
                PyErr_Print();
            }
        }
        Py_DECREF(py_retval);
    }
    pyg_gil_state_release(state);
    return retval;
}

static gboolean
create_signal (GType instance_type, const gchar *signal_name, PyObject *tuple)
{
    GSignalFlags signal_flags;
    PyObject *py_return_type, *py_param_types;
    GType return_type;
    guint n_params, i;
    GType *param_types;
    guint signal_id;
    GSignalAccumulator accumulator = NULL;
    PyGSignalAccumulatorData *accum_data = NULL;
    PyObject *py_accum = NULL, *py_accum_data = NULL;

    if (!PyArg_ParseTuple(tuple, "iOO|OO", &signal_flags, &py_return_type,
			  &py_param_types, &py_accum, &py_accum_data))
    {
	gchar buf[128];

	PyErr_Clear();
	g_snprintf(buf, sizeof(buf), "value for __gsignals__['%s'] not in correct format", signal_name);
	PyErr_SetString(PyExc_TypeError, buf);
	return FALSE;
    }

    if (py_accum && py_accum != Py_None && !PyCallable_Check(py_accum))
    {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf), "accumulator for __gsignals__['%s'] must be callable", signal_name);
	PyErr_SetString(PyExc_TypeError, buf);
	return FALSE;
    }

    return_type = pyg_type_from_object(py_return_type);
    if (!return_type)
	return FALSE;
    if (!PySequence_Check(py_param_types)) {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf), "third element of __gsignals__['%s'] tuple must be a sequence", signal_name);
	PyErr_SetString(PyExc_TypeError, buf);
	return FALSE;
    }
    n_params = PySequence_Length(py_param_types);
    param_types = g_new(GType, n_params);
    for (i = 0; i < n_params; i++) {
	PyObject *item = PySequence_GetItem(py_param_types, i);

	param_types[i] = pyg_type_from_object(item);
	if (param_types[i] == 0) {
	    Py_DECREF(item);
	    g_free(param_types);
	    return FALSE;
	}
	Py_DECREF(item);
    }

    if (py_accum == _pyg_signal_accumulator_true_handled_func)
        accumulator = g_signal_accumulator_true_handled;
    else {
        if (py_accum != NULL && py_accum != Py_None) {
            accum_data = g_new(PyGSignalAccumulatorData, 1);
            accum_data->callable = py_accum;
            Py_INCREF(py_accum);
            accum_data->user_data = py_accum_data;
            Py_XINCREF(py_accum_data);
            accumulator = _pyg_signal_accumulator;
        }
    }

    signal_id = g_signal_newv(signal_name, instance_type, signal_flags,
			      pyg_signal_class_closure_get(),
			      accumulator, accum_data,
			      (GSignalCMarshaller)0,
			      return_type, n_params, param_types);
    g_free(param_types);

    if (signal_id == 0) {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf), "could not create signal for %s",
		   signal_name);
	PyErr_SetString(PyExc_RuntimeError, buf);
	return FALSE;
    }
    return TRUE;
}

static gboolean
override_signal(GType instance_type, const gchar *signal_name)
{
    guint signal_id;

    signal_id = g_signal_lookup(signal_name, instance_type);
    if (!signal_id) {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf), "could not look up %s", signal_name);
	PyErr_SetString(PyExc_TypeError, buf);
	return FALSE;
    }
    g_signal_override_class_closure(signal_id, instance_type,
				    pyg_signal_class_closure_get());
    return TRUE;
}

static PyObject *
add_signals (GType instance_type, PyObject *signals)
{
    gboolean ret = TRUE;
    GObjectClass *oclass;
    int pos = 0;
    PyObject *key, *value, *overridden_signals = NULL;

    overridden_signals = PyDict_New();
    oclass = g_type_class_ref(instance_type);
    while (PyDict_Next(signals, &pos, &key, &value)) {
	const gchar *signal_name;
        gchar *signal_name_canon, *c;

	if (!PyString_Check(key)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gsignals__ keys must be strings");
	    ret = FALSE;
	    break;
	}
	signal_name = PyString_AsString (key);

	if (value == Py_None ||
	    (PyString_Check(value) &&
	     !strcmp(PyString_AsString(value), "override")))
        {
              /* canonicalize signal name, replacing '-' with '_' */
            signal_name_canon = g_strdup(signal_name);
            for (c = signal_name_canon; *c; ++c)
                if (*c == '-')
                    *c = '_';
            if (PyDict_SetItemString(overridden_signals, signal_name_canon, key)) {
                g_free(signal_name_canon);
                ret = FALSE;
                break;
            }
            g_free(signal_name_canon);

	    ret = override_signal(instance_type, signal_name);
	} else {
	    ret = create_signal(instance_type, signal_name, value);
	}

	if (!ret)
	    break;
    }
    g_type_class_unref(oclass);
    if (ret)
        return overridden_signals;
    else {
        Py_XDECREF(overridden_signals);
        return NULL;
    }
}

static GParamSpec *
create_property (const gchar  *prop_name,
		 GType         prop_type,
		 const gchar  *nick,
		 const gchar  *blurb,
		 PyObject     *args,
		 GParamFlags   flags)
{
    GParamSpec *pspec = NULL;

    switch (G_TYPE_FUNDAMENTAL(prop_type)) {
    case G_TYPE_CHAR:
	{
	    gchar minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "ccc", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_char (prop_name, nick, blurb, minimum,
				       maximum, default_value, flags);
	}
	break;
    case G_TYPE_UCHAR:
	{
	    gchar minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "ccc", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_uchar (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_BOOLEAN:
	{
	    gboolean default_value;

	    if (!PyArg_ParseTuple(args, "i", &default_value))
		return NULL;
	    pspec = g_param_spec_boolean (prop_name, nick, blurb,
					  default_value, flags);
	}
	break;
    case G_TYPE_INT:
	{
	    gint minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "iii", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_int (prop_name, nick, blurb, minimum,
				      maximum, default_value, flags);
	}
	break;
    case G_TYPE_UINT:
	{
	    guint minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "iii", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_uint (prop_name, nick, blurb, minimum,
				       maximum, default_value, flags);
	}
	break;
    case G_TYPE_LONG:
	{
	    glong minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "lll", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_long (prop_name, nick, blurb, minimum,
				       maximum, default_value, flags);
	}
	break;
    case G_TYPE_ULONG:
	{
	    gulong minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "lll", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_ulong (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_INT64:
	{
	    gint64 minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "LLL", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_int64 (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_UINT64:
	{
	    guint64 minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "LLL", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_uint64 (prop_name, nick, blurb, minimum,
					 maximum, default_value, flags);
	}
	break;
    case G_TYPE_ENUM:
	{
	    gint default_value;
	    PyObject *pydefault;
	    
	    if (!PyArg_ParseTuple(args, "O", &pydefault))
		return NULL;
	    
	    if (pyg_enum_get_value(prop_type, pydefault,
				   (gint *)&default_value))
		return NULL;
	    
	    pspec = g_param_spec_enum (prop_name, nick, blurb,
				       prop_type, default_value, flags);
	}
	break;
    case G_TYPE_FLAGS:
	{
	    guint default_value;
	    PyObject *pydefault;

	    if (!PyArg_ParseTuple(args, "O", &pydefault))
		return NULL;
	    
	    if (pyg_flags_get_value(prop_type, pydefault,
				    (gint *)&default_value))
		return NULL;
	    
	    pspec = g_param_spec_flags (prop_name, nick, blurb,
					prop_type, default_value, flags);
	}
	break;
    case G_TYPE_FLOAT:
	{
	    gfloat minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "fff", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_float (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_DOUBLE:
	{
	    gdouble minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "ddd", &minimum, &maximum,
				  &default_value))
		return NULL;
	    pspec = g_param_spec_double (prop_name, nick, blurb, minimum,
					 maximum, default_value, flags);
	}
	break;
    case G_TYPE_STRING:
	{
	    const gchar *default_value;

	    if (!PyArg_ParseTuple(args, "z", &default_value))
		return NULL;
	    pspec = g_param_spec_string (prop_name, nick, blurb,
					 default_value, flags);
	}
	break;
    case G_TYPE_PARAM:
	if (!PyArg_ParseTuple(args, ""))
	    return NULL;
	pspec = g_param_spec_param (prop_name, nick, blurb, prop_type, flags);
	break;
    case G_TYPE_BOXED:
	if (!PyArg_ParseTuple(args, ""))
	    return NULL;
	pspec = g_param_spec_boxed (prop_name, nick, blurb, prop_type, flags);
	break;
    case G_TYPE_POINTER:
	if (!PyArg_ParseTuple(args, ""))
	    return NULL;
	pspec = g_param_spec_pointer (prop_name, nick, blurb, flags);
	break;
    case G_TYPE_OBJECT:
	if (!PyArg_ParseTuple(args, ""))
	    return NULL;
	pspec = g_param_spec_object (prop_name, nick, blurb, prop_type, flags);
	break;
    default:
	/* unhandled pspec type ... */
	break;
    }

    if (!pspec) {
	char buf[128];

	g_snprintf(buf, sizeof(buf), "could not create param spec for type %s",
		   g_type_name(prop_type));
	PyErr_SetString(PyExc_TypeError, buf);
	return NULL;
    }
    
    return pspec;
}

GParamSpec *
pyg_param_spec_from_object (PyObject *tuple)
{
    gint val_length;
    const gchar *prop_name;
    GType prop_type;
    const gchar *nick, *blurb;
    PyObject *slice, *item, *py_prop_type;
    GParamSpec *pspec;

    val_length = PyTuple_Size(tuple);
    if (val_length < 4) {
	PyErr_SetString(PyExc_TypeError,
			"paramspec tuples must be at least 4 elements long");
	return NULL;
    }	    

    slice = PySequence_GetSlice(tuple, 0, 4);
    if (!slice) {
	return NULL;
    }
    
    if (!PyArg_ParseTuple(slice, "sOzz", &prop_name, &py_prop_type, &nick, &blurb)) {
	Py_DECREF(slice);
	return NULL;
    }
    
    Py_DECREF(slice);
    
    prop_type = pyg_type_from_object(py_prop_type);
    if (!prop_type) {
	return NULL;
    }
    
    item = PyTuple_GetItem(tuple, val_length-1);
    if (!PyInt_Check(item)) {
	PyErr_SetString(PyExc_TypeError,
			"last element in tuple must be an int");
	return NULL;
    }

    /* slice is the extra items in the tuple */
    slice = PySequence_GetSlice(tuple, 4, val_length-1);
    pspec = create_property(prop_name, prop_type,
			    nick, blurb, slice,
			    PyInt_AsLong(item));

    return pspec;
}

static gboolean
add_properties (GType instance_type, PyObject *properties)
{
    gboolean ret = TRUE;
    GObjectClass *oclass;
    int pos = 0;
    PyObject *key, *value;

    oclass = g_type_class_ref(instance_type);
    while (PyDict_Next(properties, &pos, &key, &value)) {
	const gchar *prop_name;
	GType prop_type;
	const gchar *nick, *blurb;
	GParamFlags flags;
	gint val_length;
	PyObject *slice, *item, *py_prop_type;
	GParamSpec *pspec;
	
	/* values are of format (type,nick,blurb, type_specific_args, flags) */
	
	if (!PyString_Check(key)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gproperties__ keys must be strings");
	    ret = FALSE;
	    break;
	}
	prop_name = PyString_AsString (key);

	if (!PyTuple_Check(value)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gproperties__ values must be tuples");
	    ret = FALSE;
	    break;
	}
	val_length = PyTuple_Size(value);
	if (val_length < 4) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gproperties__ values must be at least 4 elements long");
	    ret = FALSE;
	    break;
	}	    

	slice = PySequence_GetSlice(value, 0, 3);
	if (!slice) {
	    ret = FALSE;
	    break;
	}
	if (!PyArg_ParseTuple(slice, "Ozz", &py_prop_type, &nick, &blurb)) {
	    Py_DECREF(slice);
	    ret = FALSE;
	    break;
	}
	Py_DECREF(slice);
	prop_type = pyg_type_from_object(py_prop_type);
	if (!prop_type) {
	    ret = FALSE;
	    break;
	}
	item = PyTuple_GetItem(value, val_length-1);
	if (!PyInt_Check(item)) {
	    PyErr_SetString(PyExc_TypeError,
		"last element in __gproperties__ value tuple must be an int");
	    ret = FALSE;
	    break;
	}
	flags = PyInt_AsLong(item);

	/* slice is the extra items in the tuple */
	slice = PySequence_GetSlice(value, 3, val_length-1);
	pspec = create_property(prop_name, prop_type, nick, blurb,
				slice, flags);
	Py_DECREF(slice);
	
	if (pspec) {
	    g_object_class_install_property(oclass, 1, pspec);
	} else {
            PyObject *type, *value, *traceback;
	    ret = FALSE;
            PyErr_Fetch(&type, &value, &traceback);
            if (PyString_Check(value)) {
                char msg[256];
                g_snprintf(msg, 256, "%s (while registering property '%s' for GType '%s')",
                           PyString_AsString(value), prop_name, g_type_name(instance_type));
                Py_DECREF(value);
                value = PyString_FromString(msg);
            }
            PyErr_Restore(type, value, traceback);
	    break;
	}
    }
    
    g_type_class_unref(oclass);
    return ret;
}

static void
pyg_register_class_init(GType gtype, PyGClassInitFunc class_init)
{
    GSList *list;

    list = g_type_get_qdata(gtype, pygobject_class_init_key);
    list = g_slist_prepend(list, class_init);
    g_type_set_qdata(gtype, pygobject_class_init_key, list);
}

static int
pyg_run_class_init(GType gtype, gpointer gclass, PyTypeObject *pyclass)
{
    GSList *list;
    PyGClassInitFunc class_init;
    GType parent_type;
    int rv;

    parent_type = g_type_parent(gtype);
    if (parent_type) {
        rv = pyg_run_class_init(parent_type, gclass, pyclass);
        if (rv)
	    return rv;
    }
    
    list = g_type_get_qdata(gtype, pygobject_class_init_key);
    for (; list; list = list->next) {
	class_init = list->data;
        rv = class_init(gclass, pyclass);
        if (rv)
	    return rv;
    }
    
    return 0;
}

static PyObject *
_wrap_pyg_type_register(PyObject *self, PyObject *args)
{
    PyTypeObject *class;

    if (!PyArg_ParseTuple(args, "O!:gobject.type_register", &PyType_Type, &class))
	return NULL;
    if (!PyType_IsSubtype(class, &PyGObject_Type)) {
	PyErr_SetString(PyExc_TypeError,"argument must be a GObject subclass");
	return NULL;
    }

      /* Check if type already registered */
    if (pyg_type_from_object((PyObject *) class) ==
        pyg_type_from_object((PyObject *) class->tp_base))
    {
        if (pyg_type_register(class, NULL))
            return NULL;
    }
 
    Py_INCREF(class);
    return (PyObject *) class;
}

static char *
get_type_name_for_class(PyTypeObject *class)
{
    gint i, name_serial;
    char name_serial_str[16];
    PyObject *module;
    char *type_name;
    
    /* make name for new GType */
    name_serial = 1;
    /* give up after 1000 tries, just in case.. */
    while (name_serial < 1000) 
    {
	snprintf(name_serial_str, 16, "-v%i", name_serial);
	module = PyObject_GetAttrString((PyObject *)class, "__module__");
	if (module && PyString_Check(module)) {
	    type_name = g_strconcat(PyString_AsString(module), ".",
				    class->tp_name,
				    name_serial > 1 ? name_serial_str : NULL,
				    NULL);
	    Py_DECREF(module);
	} else {
	    if (module)
		Py_DECREF(module);
	    else
		PyErr_Clear();
	    type_name = g_strconcat(class->tp_name,
				    name_serial > 1 ? name_serial_str : NULL,
				    NULL);
	}
	/* convert '.' in type name to '+', which isn't banned (grumble) */
	for (i = 0; type_name[i] != '\0'; i++)
	    if (type_name[i] == '.')
		type_name[i] = '+';
	if (g_type_from_name(type_name) == 0)
	    break;              /* we now have a unique name */
	++name_serial;
    }

    return type_name;
}


static GStaticPrivate pygobject_contruction_wrapper = G_STATIC_PRIVATE_INIT;

static inline void
pygobject_init_wrapper_set(PyObject *wrapper)
{
    g_static_private_set(&pygobject_contruction_wrapper, wrapper, NULL);
}

static inline PyObject *
pygobject_init_wrapper_get(void)
{
    return (PyObject *) g_static_private_get(&pygobject_contruction_wrapper);
}

static void
pygobject__g_instance_init(GTypeInstance   *instance,
                           gpointer         g_class)
{
    GObject *object = (GObject *) instance;
    PyObject *wrapper, *args, *kwargs;

    if (!g_type_get_qdata(G_OBJECT_TYPE(object), pygobject_has_updated_constructor_key))
        return;

    wrapper = g_object_get_qdata(object, pygobject_wrapper_key); 
    if (wrapper == NULL) {
        wrapper = pygobject_init_wrapper_get();
        if (wrapper) {
            g_object_set_qdata(object, pygobject_wrapper_key, wrapper);
        }
    }
    pygobject_init_wrapper_set(NULL);
    if (wrapper == NULL) {
          /* this looks like a python object created through
           * g_object_new -> we have no python wrapper, so create it
           * now */
        PyGILState_STATE state;
        state = pyg_gil_state_ensure();
        wrapper = pygobject_new_full(object, FALSE);
        args = PyTuple_New(0);
        kwargs = PyDict_New();
        if (wrapper->ob_type->tp_init(wrapper, args, kwargs))
            PyErr_Print();
        Py_DECREF(args);
        Py_DECREF(kwargs);
        pyg_gil_state_release(state);
    }
}

int
pyg_type_register(PyTypeObject *class, const char *type_name)
{
    PyObject *gtype, *gsignals, *gproperties, *overridden_signals;
    GType parent_type, instance_type;
    gint i;
    GTypeQuery query;
    gpointer gclass;
    gpointer has_new_constructor_api;
    GTypeInfo type_info = {
	0,    /* class_size */

	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,

	(GClassInitFunc) pyg_object_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class_data */

	0,    /* instance_size */
	0,    /* n_preallocs */
	(GInstanceInitFunc) pygobject__g_instance_init
    };
    gchar *new_type_name;

    /* find the GType of the parent */
    parent_type = pyg_type_from_object((PyObject *)class);
    if (!parent_type) {
	return -1;
    }

    if (type_name)
        new_type_name = (gchar *) type_name; /* care is taken below not to free this */
    else
	new_type_name = get_type_name_for_class(class);
    
    /* set class_data that will be passed to the class_init function. */
    type_info.class_data = class;

    /* fill in missing values of GTypeInfo struct */
    g_type_query(parent_type, &query);
    type_info.class_size = query.class_size;
    type_info.instance_size = query.instance_size;

    /* create new typecode */
    instance_type = g_type_register_static(parent_type, new_type_name,
					   &type_info, 0);
    if (type_name == NULL)
        g_free(new_type_name);
    if (instance_type == 0) {
	PyErr_SetString(PyExc_RuntimeError, "could not create new GType");
	return -1;
    }

    /* store pointer to the class with the GType */
    Py_INCREF(class);
    g_type_set_qdata(instance_type, g_quark_from_string("PyGObject::class"),
		     class);

    /* set new value of __gtype__ on class */
    gtype = pyg_type_wrapper_new(instance_type);
    PyDict_SetItemString(class->tp_dict, "__gtype__", gtype);
    Py_DECREF(gtype);

      /* propagate new constructor API compatility flag from parent to child type */
    has_new_constructor_api = g_type_get_qdata(parent_type,
                                               pygobject_has_updated_constructor_key);
    if (has_new_constructor_api != NULL)
        g_type_set_qdata(instance_type, pygobject_has_updated_constructor_key,
                         has_new_constructor_api);

    /* if no __doc__, set it to the auto doc descriptor */
    if (PyDict_GetItemString(class->tp_dict, "__doc__") == NULL) {
	PyDict_SetItemString(class->tp_dict, "__doc__",
			     pyg_object_descr_doc_get());
    }

    /* we look this up in the instance dictionary, so we don't
     * accidentally get a parent type's __gsignals__ attribute. */
    gsignals = PyDict_GetItemString(class->tp_dict, "__gsignals__");
    if (gsignals) {
	if (!PyDict_Check(gsignals)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gsignals__ attribute not a dict!");
	    return -1;
	}
	if (!(overridden_signals = add_signals(instance_type, gsignals))) {
	    return -1;
	}
        if (PyDict_SetItemString(class->tp_dict, "__gsignals__", overridden_signals))
            return -1;
        Py_DECREF(overridden_signals);
    } else {
	PyErr_Clear();
    }

    /* we look this up in the instance dictionary, so we don't
     * accidentally get a parent type's __gsignals__ attribute. */
    gproperties = PyDict_GetItemString(class->tp_dict, "__gproperties__");
    if (gproperties) {
	if (!PyDict_Check(gproperties)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gproperties__ attribute not a dict!");
	    return -1;
	}
	if (!add_properties(instance_type, gproperties)) {
	    return -1;
	}
	PyDict_DelItemString(class->tp_dict, "__gproperties__");
	/* Borrowed reference. Py_DECREF(gproperties); */
    } else {
	PyErr_Clear();
    }

    gclass = g_type_class_ref(instance_type);
    if (pyg_run_class_init(instance_type, gclass, class)) {
        g_type_class_unref(gclass);
        return -1;
    }
    g_type_class_unref(gclass);

    if (gsignals)
        PyDict_DelItemString(class->tp_dict, "__gsignals__");

      /* Register interface implementations  */
    if (class->tp_bases) {
        for (i = 0; i < PyTuple_GET_SIZE(class->tp_bases); ++i)
        {
            PyTypeObject *base = (PyTypeObject *) PyTuple_GET_ITEM(class->tp_bases, i);
            GType itype;
            const GInterfaceInfo *iinfo;
            
            if (((PyTypeObject *) base)->tp_base != &PyGInterface_Type)
                continue;

            itype = pyg_type_from_object((PyObject *) base);
            iinfo = pyg_lookup_interface_info(itype);
            if (!iinfo) {
                char *msg;
                msg = g_strdup_printf("Interface type %s "
                                      "has no python implementation support",
                                      base->tp_name);
                PyErr_Warn(PyExc_RuntimeWarning, msg);
                g_free(msg);
                continue;
            }
            g_type_add_interface_static(instance_type, itype, iinfo);
        }
    } else
        g_warning("type has no tp_bases");

    return 0;
}

static PyObject *
pyg_signal_new(PyObject *self, PyObject *args)
{
    gchar *signal_name;
    PyObject *py_type;
    GSignalFlags signal_flags;
    GType return_type;
    PyObject *py_return_type, *py_param_types;

    GType instance_type = 0;
    guint n_params, i;
    GType *param_types;

    guint signal_id;

    if (!PyArg_ParseTuple(args, "sOiOO:gobject.signal_new", &signal_name,
			  &py_type, &signal_flags, &py_return_type,
			  &py_param_types))
	return NULL;
    instance_type = pyg_type_from_object(py_type);
    if (!instance_type)
	return NULL;
    return_type = pyg_type_from_object(py_return_type);
    if (!return_type)
	return NULL;
    if (!PySequence_Check(py_param_types)) {
	PyErr_SetString(PyExc_TypeError,
			"argument 5 must be a sequence of GType codes");
	return NULL;
    }
    n_params = PySequence_Length(py_param_types);
    param_types = g_new(GType, n_params);
    for (i = 0; i < n_params; i++) {
	PyObject *item = PySequence_GetItem(py_param_types, i);

	param_types[i] = pyg_type_from_object(item);
	if (param_types[i] == 0) {
	    PyErr_Clear();
	    Py_DECREF(item);
	    PyErr_SetString(PyExc_TypeError,
			    "argument 5 must be a sequence of GType codes");
	    return NULL;
	}
	Py_DECREF(item);
    }

    signal_id = g_signal_newv(signal_name, instance_type, signal_flags,
			      pyg_signal_class_closure_get(),
			      (GSignalAccumulator)0, NULL,
			      (GSignalCMarshaller)0,
			      return_type, n_params, param_types);
    g_free(param_types);
    if (signal_id != 0)
	return PyInt_FromLong(signal_id);
    PyErr_SetString(PyExc_RuntimeError, "could not create signal");
    return NULL;
}

static PyObject *
pyg_signal_list_names (PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "type", NULL };
    PyObject *py_itype, *list;
    GObjectClass *class = NULL;
    GType itype;
    guint n;
    guint *ids;
    guint i;
    gpointer iface = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O:gobject.signal_list_names",
                                     kwlist, &py_itype))
	return NULL;
    if ((itype = pyg_type_from_object(py_itype)) == 0)
	return NULL;

    if (G_TYPE_IS_INSTANTIATABLE(itype)) {
	class = g_type_class_ref(itype);
	if (!class) {
	    PyErr_SetString(PyExc_RuntimeError,
			    "could not get a reference to type class");
	    return NULL;
	}
    } else if (!G_TYPE_IS_INTERFACE(itype)) {
	PyErr_SetString(PyExc_TypeError,
			"type must be instantiable or an interface");
	return NULL;
    } else {
        iface = g_type_default_interface_ref(itype);
    }
    
    ids = g_signal_list_ids(itype, &n);

    list = PyTuple_New((gint)n);
    if (list != NULL) {
	for (i = 0; i < n; i++)
	    PyTuple_SetItem(list, i,
			    PyString_FromString(g_signal_name(ids[i])));
    }
    
    g_free(ids);
    if (class)
	   g_type_class_unref(class);
    else
       g_type_default_interface_unref(iface);

    return list;
}

static PyObject *
pyg_signal_list_ids (PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "type", NULL };
    PyObject *py_itype, *list;
    GObjectClass *class;
    GType itype;
    guint n;
    guint *ids;
    guint i;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O:gobject.signal_list_ids",
                                     kwlist, &py_itype))
	return NULL;
    if ((itype = pyg_type_from_object(py_itype)) == 0)
	return NULL;

    if (!G_TYPE_IS_INSTANTIATABLE(itype) && !G_TYPE_IS_INTERFACE(itype)) {
	PyErr_SetString(PyExc_TypeError,
			"type must be instantiable or an interface");
	return NULL;
    }

    class = g_type_class_ref(itype);
    if (!class) {
	PyErr_SetString(PyExc_RuntimeError,
			"could not get a reference to type class");
	return NULL;
    }
    ids = g_signal_list_ids(itype, &n);

    list = PyTuple_New((gint)n);
    if (list == NULL) {
	g_free(ids);
	g_type_class_unref(class);
	return NULL;
    }

    for (i = 0; i < n; i++)
	PyTuple_SetItem(list, i, PyInt_FromLong(ids[i]));
    g_free(ids);
    g_type_class_unref(class);
    return list;
}

static PyObject *
pyg_signal_lookup (PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "name", "type", NULL };
    PyObject *py_itype;
    GObjectClass *class;
    GType itype;
    gchar *signal_name;
    guint id;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO:gobject.signal_lookup",
                                     kwlist, &signal_name, &py_itype))
	return NULL;
    if ((itype = pyg_type_from_object(py_itype)) == 0)
	return NULL;

    if (!G_TYPE_IS_INSTANTIATABLE(itype) && !G_TYPE_IS_INTERFACE(itype)) {
	PyErr_SetString(PyExc_TypeError,
			"type must be instantiable or an interface");
	return NULL;
    }

    class = g_type_class_ref(itype);
    if (!class) {
	PyErr_SetString(PyExc_RuntimeError,
			"could not get a reference to type class");
	return NULL;
    }
    id = g_signal_lookup(signal_name, itype);

    g_type_class_unref(class);
    return PyInt_FromLong(id);
}

static PyObject *
pyg_signal_name (PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "signal_id", NULL };
    const gchar *signal_name;
    guint id;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i:gobject.signal_name",
                                     kwlist, &id))
	return NULL;
    signal_name = g_signal_name(id);
    if (signal_name)
        return PyString_FromString(signal_name);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pyg_signal_query (PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist1[] = { "name", "type", NULL };
    static char *kwlist2[] = { "signal_id", NULL };
    PyObject *py_query, *params_list, *py_itype;
    GObjectClass *class = NULL;
    GType itype;
    gchar *signal_name;
    guint i;
    GSignalQuery query;
    guint id;

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "sO:gobject.signal_query",
                                     kwlist1, &signal_name, &py_itype)) {
        if ((itype = pyg_type_from_object(py_itype)) == 0)
            return NULL;

        if (!G_TYPE_IS_INSTANTIATABLE(itype) && !G_TYPE_IS_INTERFACE(itype)) {
            PyErr_SetString(PyExc_TypeError,
                            "type must be instantiable or an interface");
            return NULL;
        }

        class = g_type_class_ref(itype);
        if (!class) {
            PyErr_SetString(PyExc_RuntimeError,
                            "could not get a reference to type class");
            return NULL;
        }
        id = g_signal_lookup(signal_name, itype);
    } else {
	PyErr_Clear();
        if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                         "i:gobject.signal_query",
                                         kwlist2, &id)) {
            PyErr_Clear();
            PyErr_SetString(PyExc_TypeError,
                            "Usage: one of:\n"
                            "  gobject.signal_query(name, type)\n"
                            "  gobject.signal_query(signal_id)");
             
	return NULL;
        }
    }

    g_signal_query(id, &query);

    if (query.signal_id == 0) {
        Py_INCREF(Py_None);
        py_query = Py_None;
        goto done;
    }
    py_query = PyTuple_New(6);
    if (py_query == NULL) {
        goto done;
    }
    params_list = PyTuple_New(query.n_params);
    if (params_list == NULL) {
        Py_DECREF(py_query);
        py_query = NULL;
        goto done;
    }

    PyTuple_SET_ITEM(py_query, 0, PyInt_FromLong(query.signal_id));
    PyTuple_SET_ITEM(py_query, 1, PyString_FromString(query.signal_name));
    PyTuple_SET_ITEM(py_query, 2, pyg_type_wrapper_new(query.itype));
    PyTuple_SET_ITEM(py_query, 3, PyInt_FromLong(query.signal_flags));
    PyTuple_SET_ITEM(py_query, 4, pyg_type_wrapper_new(query.return_type));
    for (i = 0; i < query.n_params; i++) {
        PyTuple_SET_ITEM(params_list, i,
                         pyg_type_wrapper_new(query.param_types[i]));
    }
    PyTuple_SET_ITEM(py_query, 5, params_list);

 done:
    if (class)
        g_type_class_unref(class);

    return py_query;
}

static PyObject *
pyg_object_class_list_properties (PyObject *self, PyObject *args)
{
    GParamSpec **specs;
    PyObject *py_itype, *list;
    GType itype;
    GObjectClass *class;
    guint nprops;
    guint i;

    if (!PyArg_ParseTuple(args, "O:gobject.list_properties",
			  &py_itype))
	return NULL;
    if ((itype = pyg_type_from_object(py_itype)) == 0)
	return NULL;

    if (!g_type_is_a(itype, G_TYPE_OBJECT)) {
	PyErr_SetString(PyExc_TypeError, "type must be derived from GObject");
	return NULL;
    }

    class = g_type_class_ref(itype);
    if (!class) {
	PyErr_SetString(PyExc_RuntimeError,
			"could not get a reference to type class");
	return NULL;
    }

    specs = g_object_class_list_properties(class, &nprops);
    list = PyTuple_New(nprops);
    if (list == NULL) {
	g_free(specs);
	g_type_class_unref(class);
	return NULL;
    }
    for (i = 0; i < nprops; i++) {
	PyTuple_SetItem(list, i, pyg_param_spec_new(specs[i]));
    }
    g_free(specs);
    g_type_class_unref(class);

    return list;
}

static PyObject *
pyg_object_new (PyGObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *pytype;
    GType type;
    GObject *obj = NULL;
    GObjectClass *class;
    int n_params = 0, i;
    GParameter *params = NULL;

    if (!PyArg_ParseTuple (args, "O:gobject.new", &pytype)) {
	return NULL;
    }

    if ((type = pyg_type_from_object (pytype)) == 0)
	return NULL;

    if (G_TYPE_IS_ABSTRACT(type)) {
	PyErr_Format(PyExc_TypeError, "cannot create instance of abstract "
		     "(non-instantiable) type `%s'", g_type_name(type));
	return NULL;
    }

    if ((class = g_type_class_ref (type)) == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"could not get a reference to type class");
	return NULL;
    }

    if (kwargs) {
	int pos = 0;
	PyObject *key;
	PyObject *value;

	params = g_new0(GParameter, PyDict_Size(kwargs));
	while (PyDict_Next (kwargs, &pos, &key, &value)) {
	    GParamSpec *pspec;
	    const gchar *key_str = PyString_AsString (key);

	    pspec = g_object_class_find_property (class, key_str);
	    if (!pspec) {
		PyErr_Format(PyExc_TypeError,
			     "gobject `%s' doesn't support property `%s'",
			     g_type_name(type), key_str);
		goto cleanup;
	    }
	    g_value_init(&params[n_params].value,
			 G_PARAM_SPEC_VALUE_TYPE(pspec));
	    if (pyg_param_gvalue_from_pyobject(&params[n_params].value,
					       value, pspec) < 0) {
		PyErr_Format(PyExc_TypeError,
			     "could not convert value for property `%s' from %s to %s",
			     key_str, value->ob_type->tp_name, g_type_name(G_PARAM_SPEC_VALUE_TYPE(pspec)));
		goto cleanup;
	    }
	    params[n_params].name = g_strdup(key_str);
	    n_params++;
	}
    }

    obj = g_object_newv(type, n_params, params);
    if (!obj)
	PyErr_SetString (PyExc_RuntimeError, "could not create object");
	   
 cleanup:
    for (i = 0; i < n_params; i++) {
	g_free((gchar *) params[i].name);
	g_value_unset(&params[i].value);
    }
    g_free(params);
    g_type_class_unref(class);
    
    if (obj) {
	self = (PyGObject *) pygobject_new_full((GObject *)obj, FALSE);
        g_object_unref(obj);
        pygobject_sink(obj);
    } else
        self = NULL;

    return (PyObject *) self;
}

static gint
get_handler_priority(gint *priority, PyObject *kwargs)
{
    gint len, pos;
    PyObject *key, *val;

    /* no keyword args? leave as default */
    if (kwargs == NULL)	return 0;

    len = PyDict_Size(kwargs);
    if (len == 0) return 0;

    if (len != 1) {
	PyErr_SetString(PyExc_TypeError,
			"expecting at most one keyword argument");
	return -1;
    }
    pos = 0;
    PyDict_Next(kwargs, &pos, &key, &val);
    if (!PyString_Check(key)) {
	PyErr_SetString(PyExc_TypeError,
			"keyword argument name is not a string");
	return -1;
    }

    if (strcmp(PyString_AsString(key), "priority") != 0) {
	PyErr_SetString(PyExc_TypeError,
			"only 'priority' keyword argument accepted");
	return -1;
    }

    *priority = PyInt_AsLong(val);
    if (PyErr_Occurred()) {
	PyErr_Clear();
	PyErr_SetString(PyExc_ValueError, "could not get priority value");
	return -1;
    }
    return 0;
}

gboolean
pyg_handler_marshal(gpointer user_data)
{
    PyObject *tuple, *ret;
    gboolean res;
    PyGILState_STATE state;

    g_return_val_if_fail(user_data != NULL, FALSE);

    state = pyg_gil_state_ensure();

    tuple = (PyObject *)user_data;
    ret = PyObject_CallObject(PyTuple_GetItem(tuple, 0),
			      PyTuple_GetItem(tuple, 1));
    if (!ret) {
	PyErr_Print();
	res = FALSE;
    } else {
	res = PyObject_IsTrue(ret);
	Py_DECREF(ret);
    }
    
    pyg_gil_state_release(state);

    return res;
}

static PyObject *
pyg_idle_add(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *first, *callback, *cbargs = NULL, *data;
    gint len, priority = G_PRIORITY_DEFAULT_IDLE;
    guint handler_id;

    len = PyTuple_Size(args);
    if (len < 1) {
	PyErr_SetString(PyExc_TypeError,
			"idle_add requires at least 1 argument");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 1);
    if (!PyArg_ParseTuple(first, "O:idle_add", &callback)) {
	Py_DECREF(first);
        return NULL;
    }
    Py_DECREF(first);
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "first argument not callable");
        return NULL;
    }
    if (get_handler_priority(&priority, kwargs) < 0)
	return NULL;

    cbargs = PySequence_GetSlice(args, 1, len);
    if (cbargs == NULL)
	return NULL;

    data = Py_BuildValue("(ON)", callback, cbargs);
    if (data == NULL)
	return NULL;
    handler_id = g_idle_add_full(priority, pyg_handler_marshal, data,
				 pyg_destroy_notify);
    return PyInt_FromLong(handler_id);
}

static PyObject *
pyg_timeout_add(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *first, *callback, *cbargs = NULL, *data;
    gint len, priority = G_PRIORITY_DEFAULT;
    guint interval, handler_id;

    len = PyTuple_Size(args);
    if (len < 2) {
	PyErr_SetString(PyExc_TypeError,
			"timeout_add requires at least 2 args");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 2);
    if (!PyArg_ParseTuple(first, "IO:timeout_add", &interval, &callback)) {
	Py_DECREF(first);
        return NULL;
    }
    Py_DECREF(first);
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "second argument not callable");
        return NULL;
    }
    if (get_handler_priority(&priority, kwargs) < 0)
	return NULL;

    cbargs = PySequence_GetSlice(args, 2, len);
    if (cbargs == NULL)
	return NULL;

    data = Py_BuildValue("(ON)", callback, cbargs);
    if (data == NULL)
	return NULL;
    handler_id = g_timeout_add_full(priority, interval,
				    pyg_handler_marshal, data,
				    pyg_destroy_notify);
    return PyInt_FromLong(handler_id);
}

static gboolean
iowatch_marshal(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
    PyGILState_STATE state;
    PyObject *tuple, *func, *firstargs, *args, *ret;
    gboolean res;

    g_return_val_if_fail(user_data != NULL, FALSE);

    state = pyg_gil_state_ensure();

    tuple = (PyObject *)user_data;
    func = PyTuple_GetItem(tuple, 0);

    /* arg vector is (fd, condtion, *args) */
    firstargs = Py_BuildValue("(Oi)", PyTuple_GetItem(tuple, 1), condition);
    args = PySequence_Concat(firstargs, PyTuple_GetItem(tuple, 2));
    Py_DECREF(firstargs);

    ret = PyObject_CallObject(func, args);
    Py_DECREF(args);
    if (!ret) {
	PyErr_Print();
	res = FALSE;
    } else {
        if (ret == Py_None) {
            if (PyErr_Warn(PyExc_Warning, "gobject.io_add_watch callback returned None;"
                           " should return True/False")) {
                PyErr_Print();
            }
        }
	res = PyObject_IsTrue(ret);
	Py_DECREF(ret);
    }

    pyg_gil_state_release(state);

    return res;
}

static PyObject *
pyg_io_add_watch(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *first, *pyfd, *callback, *cbargs = NULL, *data;
    gint fd, priority = G_PRIORITY_DEFAULT, condition, len;
    GIOChannel *iochannel;
    guint handler_id;

    len = PyTuple_Size(args);
    if (len < 3) {
	PyErr_SetString(PyExc_TypeError,
			"io_add_watch requires at least 3 args");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 3);
    if (!PyArg_ParseTuple(first, "OiO:io_add_watch", &pyfd, &condition,
			  &callback)) {
	Py_DECREF(first);
        return NULL;
    }
    Py_DECREF(first);
    fd = PyObject_AsFileDescriptor(pyfd);
    if (fd < 0) {
	return NULL;
    }
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "second argument not callable");
        return NULL;
    }
    if (get_handler_priority(&priority, kwargs) < 0)
	return NULL;

    cbargs = PySequence_GetSlice(args, 3, len);
    if (cbargs == NULL)
      return NULL;
    data = Py_BuildValue("(OON)", callback, pyfd, cbargs);
    if (data == NULL)
      return NULL;
    iochannel = g_io_channel_unix_new(fd);
    handler_id = g_io_add_watch_full(iochannel, priority, condition,
				     iowatch_marshal, data,
				     (GDestroyNotify)pyg_destroy_notify);
    g_io_channel_unref(iochannel);
    
    return PyInt_FromLong(handler_id);
}

static PyObject *
pyg_source_remove(PyObject *self, PyObject *args)
{
    guint tag;

    if (!PyArg_ParseTuple(args, "i:source_remove", &tag))
	return NULL;

    return PyBool_FromLong(g_source_remove(tag));
}

static PyObject *
pyg_main_context_default(PyObject *unused)
{
    return pyg_main_context_new(g_main_context_default());
}

static int pyg_thread_state_tls_key = -1;

/* Enable threading; note that the GIL must be held by the current
   thread when this function is called */
static int
pyg_enable_threads (void)
{
    if (getenv ("PYGTK_USE_GIL_STATE_API"))
	use_gil_state_api = TRUE;

#ifndef DISABLE_THREADING
    if (!pygobject_api_functions.threads_enabled) {
	PyEval_InitThreads();
	if (!g_threads_got_initialized)
	    g_thread_init(NULL);
	pygobject_api_functions.threads_enabled = TRUE;
	pyg_thread_state_tls_key = PyThread_create_key();
    }

    return 0;
#else
    PyErr_SetString(PyExc_RuntimeError,
                    "pygtk threading disabled at compile time");
    return -1;
#endif
}

static int
pyg_gil_state_ensure_py23 (void)
{
    return PyGILState_Ensure();
}

static void
pyg_gil_state_release_py23 (int flag)
{
    PyGILState_Release(flag);
}

static PyObject *
pyg_threads_init (PyObject *unused, PyObject *args, PyObject *kwargs)
{
    if (pyg_enable_threads())
        return NULL;
    
    Py_INCREF(Py_None);
    return Py_None;
}

struct _PyGChildData {
    PyObject *func;
    PyObject *data;
};

static void
child_watch_func(GPid pid, gint status, gpointer data)
{
    struct _PyGChildData *child_data = (struct _PyGChildData *) data;
    PyObject *retval;
    PyGILState_STATE gil;

    gil = pyg_gil_state_ensure();
    if (child_data->data)
        retval = PyObject_CallFunction(child_data->func, "iiO", pid, status,
                                       child_data->data);
    else
        retval = PyObject_CallFunction(child_data->func, "ii", pid, status);

    if (retval)
	Py_DECREF(retval);
    else
	PyErr_Print();

    pyg_gil_state_release(gil);
}

static void
child_watch_dnotify(gpointer data)
{
    struct _PyGChildData *child_data = (struct _PyGChildData *) data;
    Py_DECREF(child_data->func);
    Py_XDECREF(child_data->data);
    g_free(child_data);
}


static PyObject *
pyg_child_watch_add(PyObject *unused, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "pid", "function", "data", "priority", NULL };
    guint id;
    gint priority = G_PRIORITY_DEFAULT;
    int pid;
    PyObject *func, *user_data = NULL;
    struct _PyGChildData *child_data;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO|Oi:gobject.child_watch_add", kwlist,
                                     &pid, &func, &user_data, &priority))
        return NULL;
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                        "gobject.child_watch_add: second argument must be callable");
        return NULL;
    }

    child_data = g_new(struct _PyGChildData, 1);
    child_data->func = func;
    child_data->data = user_data;
    Py_INCREF(child_data->func);
    if (child_data->data)
        Py_INCREF(child_data->data);
    id = g_child_watch_add_full(priority, pid, child_watch_func,
                                child_data, child_watch_dnotify);
    return PyInt_FromLong(id);
}

struct _PyGChildSetupData {
    PyObject *func;
    PyObject *data;
};

static void
_pyg_spawn_async_callback(gpointer user_data)
{
    struct _PyGChildSetupData *data;
    PyObject *retval;
    PyGILState_STATE gil;

    data = (struct _PyGChildSetupData *) user_data;
    gil = pyg_gil_state_ensure();
    if (data->data)
        retval = PyObject_CallFunction(data->func, "O", data->data);
    else
        retval = PyObject_CallFunction(data->func, NULL);
    if (retval)
	Py_DECREF(retval);
    else
	PyErr_Print();
    Py_DECREF(data->func);
    Py_XDECREF(data->data);
    g_free(data);
    pyg_gil_state_release(gil);
}

static PyObject *
pyg_spawn_async(PyObject *unused, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "argv", "envp", "working_directory", "flags",
                              "child_setup", "user_data", "standard_input",
                              "standard_output", "standard_error", NULL };
    PyObject *pyargv, *pyenvp = NULL;
    char **argv, **envp = NULL;
    PyObject *func = NULL, *user_data = NULL;
    char *working_directory = NULL;
    int flags = 0, _stdin = -1, _stdout = -1, _stderr = -1;
    PyObject *pystdin = NULL, *pystdout = NULL, *pystderr = NULL;
    gint *standard_input, *standard_output, *standard_error;
    struct _PyGChildSetupData *callback_data = NULL;
    GError *error = NULL;
    GPid child_pid = -1;
    int len, i;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OsiOOOOO:gobject.spawn_async",
                                     kwlist,
                                     &pyargv, &pyenvp, &working_directory, &flags,
                                     &func, &user_data,
                                     &pystdin, &pystdout, &pystderr))
        return NULL;

    if (pystdin && PyObject_IsTrue(pystdin))
        standard_input = &_stdin;
    else
        standard_input = NULL;

    if (pystdout && PyObject_IsTrue(pystdout))
        standard_output = &_stdout;
    else
        standard_output = NULL;

    if (pystderr && PyObject_IsTrue(pystderr))
        standard_error = &_stderr;
    else
        standard_error = NULL;

      /* parse argv */
    if (!PySequence_Check(pyargv)) {
        PyErr_SetString(PyExc_TypeError,
                        "gobject.spawn_async: first argument must be a sequence of strings");
        return NULL;
    }
    len = PySequence_Length(pyargv);
    argv = g_new0(char *, len + 1);
    for (i = 0; i < len; ++i) {
        PyObject *tmp = PySequence_ITEM(pyargv, i);
        if (!PyString_Check(tmp)) {
            PyErr_SetString(PyExc_TypeError,
                            "gobject.spawn_async: first argument must be a sequence of strings");
            g_free(argv);
            Py_XDECREF(tmp);
            return NULL;
        }
        argv[i] = PyString_AsString(tmp);
        Py_DECREF(tmp);
    }

      /* parse envp */
    if (pyenvp) {
        if (!PySequence_Check(pyenvp)) {
            PyErr_SetString(PyExc_TypeError,
                            "gobject.spawn_async: second argument must be a sequence of strings");
            g_free(argv);
            return NULL;
        }
        len = PySequence_Length(pyenvp);
        envp = g_new0(char *, len + 1);
        for (i = 0; i < len; ++i) {
            PyObject *tmp = PySequence_ITEM(pyenvp, i);
            if (!PyString_Check(tmp)) {
                PyErr_SetString(PyExc_TypeError,
                                "gobject.spawn_async: second argument must be a sequence of strings");
                g_free(envp);
                Py_XDECREF(tmp);
                return NULL;
            }
            envp[i] = PyString_AsString(tmp);
            Py_DECREF(tmp);
        }
    }

    if (func) {
        callback_data = g_new(struct _PyGChildSetupData, 1);
        callback_data->func = func;
        callback_data->data = user_data;
        Py_INCREF(callback_data->func);
        if (callback_data->data)
            Py_INCREF(callback_data->data);
    }

    if (!g_spawn_async_with_pipes(working_directory, argv, envp, flags,
                                  func? _pyg_spawn_async_callback : NULL,
                                  callback_data, &child_pid,
                                  standard_input,
                                  standard_output,
                                  standard_error,
                                  &error))
    {
        g_free(argv);
        if (envp) g_free(envp);
        if (callback_data) {
            Py_DECREF(callback_data->func);
            Py_XDECREF(callback_data->data);
            g_free(callback_data);
        }
        pyg_error_check(&error);
        return NULL;
    }
    g_free(argv);
    if (envp) g_free(envp);

    if (standard_input)
        pystdin = PyInt_FromLong(*standard_input);
    else {
        Py_INCREF(Py_None);
        pystdin = Py_None;
    }

    if (standard_output)
        pystdout = PyInt_FromLong(*standard_output);
    else {
        Py_INCREF(Py_None);
        pystdout = Py_None;
    }

    if (standard_error)
        pystderr = PyInt_FromLong(*standard_error);
    else {
        Py_INCREF(Py_None);
        pystderr = Py_None;
    }

    return Py_BuildValue("iNNN", child_pid, pystdin, pystdout, pystderr);
}


static PyObject *
pyg_markup_escape_text(PyObject *unused, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "text", NULL };
    char *text_in, *text_out;
    int text_size;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#:gobject.markup_escape_text", kwlist,
                                     &text_in, &text_size))
        return NULL;

    text_out = g_markup_escape_text(text_in, text_size);
    retval = PyString_FromString(text_out);
    g_free(text_out);
    return retval;
}

static PyObject *
pyg_get_current_time(PyObject *unused)
{
    GTimeVal timeval;
    double ret;

    g_get_current_time(&timeval);
    ret = (double)timeval.tv_sec + (double)timeval.tv_usec * 0.000001;
    return PyFloat_FromDouble(ret);
}

static PyObject *
pyg_main_depth(PyObject *unused)
{
    return PyInt_FromLong(g_main_depth());
}

static PyObject *
pyg_signal_accumulator_true_handled(PyObject *unused, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError, "signal_accumulator_true_handled can only"
                    " be used as accumulator argument when registering signals");
    return NULL;
}

static gboolean
marshal_emission_hook(GSignalInvocationHint *ihint,
		      guint n_param_values,
		      const GValue *param_values,
		      gpointer user_data)
{
    PyGILState_STATE state;
    gboolean retval = FALSE;
    PyObject *func, *args;
    PyObject *retobj;
    PyObject *params;
    guint i;

    state = pyg_gil_state_ensure();

    /* construct Python tuple for the parameter values */
    params = PyTuple_New(n_param_values);

    for (i = 0; i < n_param_values; i++) {
	PyObject *item = pyg_value_as_pyobject(&param_values[i], FALSE);
	
	/* error condition */
	if (!item) {
	    goto out;
	}
	PyTuple_SetItem(params, i, item);
    }

    args = (PyObject *)user_data;
    func = PyTuple_GetItem(args, 0);
    args = PySequence_Concat(params, PyTuple_GetItem(args, 1));
    Py_DECREF(params);

    /* params passed to function may have extra arguments */

    retobj = PyObject_CallObject(func, args);
    Py_DECREF(args);
    if (retobj == NULL) {
        PyErr_Print();
    }
    
    retval = (retobj == Py_True ? TRUE : FALSE);
    Py_XDECREF(retobj);
out:
    pyg_gil_state_release(state);
    return retval;
}

static PyObject *
pyg_add_emission_hook(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *data;
    gchar *name;
    gulong hook_id;
    guint sigid, len;
    GQuark detail = 0;
    GType gtype;
    PyObject *pygtype;

    len = PyTuple_Size(args);
    if (len < 3) {
	PyErr_SetString(PyExc_TypeError,
			"kiwi.add_emission_hook requires at least 3 arguments");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 3);
    if (!PyArg_ParseTuple(first, "OsO:add_emission_hook",
			  &pygtype, &name, &callback)) {
	Py_DECREF(first);
	return NULL;
    }
    Py_DECREF(first);
    
    if ((gtype = pyg_type_from_object(pygtype)) == 0) {
	return NULL;
    }
    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "third argument must be callable");
	return NULL;
    }

    if (!g_signal_parse_name(name, gtype, &sigid, &detail, TRUE)) {
	PyErr_Format(PyExc_TypeError, "%s: unknown signal name: %s",
		     PyString_AsString(PyObject_Repr((PyObject*)self)),
		     name);
	return NULL;
    }
    extra_args = PySequence_GetSlice(args, 3, len);
    if (extra_args == NULL)
	return NULL;

    data = Py_BuildValue("(ON)", callback, extra_args);
    if (data == NULL)
      return NULL;
    
    hook_id = g_signal_add_emission_hook(sigid, detail,
					 marshal_emission_hook,
					 data,
					 (GDestroyNotify)pyg_destroy_notify);
        
    Py_DECREF(extra_args);
    return PyInt_FromLong(hook_id);
}

static PyObject *
pyg_remove_emission_hook(PyGObject *self, PyObject *args)
{
    PyObject *pygtype;
    char *name;
    guint signal_id;
    gulong hook_id;
    GType gtype;
    
    if (!PyArg_ParseTuple(args, "Osi:gobject.remove_emission_hook",
			  &pygtype, &name, &hook_id))
	return NULL;
    
    if ((gtype = pyg_type_from_object(pygtype)) == 0) {
	return NULL;
    }
    
    if (!g_signal_parse_name(name, gtype, &signal_id, NULL, TRUE)) {
	PyErr_Format(PyExc_TypeError, "%s: unknown signal name: %s",
		     PyString_AsString(PyObject_Repr((PyObject*)self)),
		     name);
	return NULL;
    }

    g_signal_remove_emission_hook(signal_id, hook_id);
    
    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef pygobject_functions[] = {
    { "type_name", pyg_type_name, METH_VARARGS },
    { "type_from_name", pyg_type_from_name, METH_VARARGS },
    { "type_parent", pyg_type_parent, METH_VARARGS },
    { "type_is_a", pyg_type_is_a, METH_VARARGS },
    { "type_children", pyg_type_children, METH_VARARGS },
    { "type_interfaces", pyg_type_interfaces, METH_VARARGS },
    { "type_register", _wrap_pyg_type_register, METH_VARARGS },
    { "signal_new", pyg_signal_new, METH_VARARGS },
    { "signal_list_names", (PyCFunction)pyg_signal_list_names, METH_VARARGS|METH_KEYWORDS },
    { "signal_list_ids", (PyCFunction)pyg_signal_list_ids, METH_VARARGS|METH_KEYWORDS },
    { "signal_lookup", (PyCFunction)pyg_signal_lookup, METH_VARARGS|METH_KEYWORDS },
    { "signal_name", (PyCFunction)pyg_signal_name, METH_VARARGS|METH_KEYWORDS },
    { "signal_query", (PyCFunction)pyg_signal_query, METH_VARARGS|METH_KEYWORDS },
    { "list_properties", pyg_object_class_list_properties, METH_VARARGS },
    { "new", (PyCFunction)pyg_object_new, METH_VARARGS|METH_KEYWORDS },
    { "idle_add", (PyCFunction)pyg_idle_add, METH_VARARGS|METH_KEYWORDS },
    { "timeout_add", (PyCFunction)pyg_timeout_add, METH_VARARGS|METH_KEYWORDS },
    { "io_add_watch", (PyCFunction)pyg_io_add_watch, METH_VARARGS|METH_KEYWORDS },
    { "source_remove", pyg_source_remove, METH_VARARGS },
    { "main_context_default", (PyCFunction)pyg_main_context_default, METH_NOARGS },
    { "threads_init", (PyCFunction)pyg_threads_init, METH_VARARGS|METH_KEYWORDS },
    { "child_watch_add", (PyCFunction)pyg_child_watch_add, METH_VARARGS|METH_KEYWORDS },
    { "spawn_async", (PyCFunction)pyg_spawn_async, METH_VARARGS|METH_KEYWORDS },
    { "markup_escape_text", (PyCFunction)pyg_markup_escape_text, METH_VARARGS|METH_KEYWORDS },
    { "get_current_time", (PyCFunction)pyg_get_current_time, METH_NOARGS },
    { "main_depth", (PyCFunction)pyg_main_depth, METH_NOARGS },
    { "signal_accumulator_true_handled", (PyCFunction)pyg_signal_accumulator_true_handled, METH_VARARGS },
    { "add_emission_hook", (PyCFunction)pyg_add_emission_hook, METH_VARARGS },
    { "remove_emission_hook", (PyCFunction)pyg_remove_emission_hook, METH_VARARGS },

    { NULL, NULL, 0 }
};


/* ----------------- Constant extraction ------------------------ */

/**
 * pyg_constant_strip_prefix:
 * @name: the constant name.
 * @strip_prefix: the prefix to strip.
 *
 * Advances the pointer @name by strlen(@strip_prefix) characters.  If
 * the resulting name does not start with a letter or underscore, the
 * @name pointer will be rewound.  This is to ensure that the
 * resulting name is a valid identifier.  Hence the returned string is
 * a pointer into the string @name.
 *
 * Returns: the stripped constant name.
 */
char *
pyg_constant_strip_prefix(gchar *name, const gchar *strip_prefix)
{
    gint prefix_len;
    guint j;
    
    prefix_len = strlen(strip_prefix);
    
    /* strip off prefix from value name, while keeping it a valid
     * identifier */
    for (j = prefix_len; j >= 0; j--) {
	if (g_ascii_isalpha(name[j]) || name[j] == '_') {
	    return &name[j];
	}
    }
    return name;
}

/**
 * pyg_enum_add_constants:
 * @module: a Python module
 * @enum_type: the GType of the enumeration.
 * @strip_prefix: the prefix to strip from the constant names.
 *
 * Adds constants to the given Python module for each value name of
 * the enumeration.  A prefix will be stripped from each enum name.
 */
static void
pyg_enum_add_constants(PyObject *module, GType enum_type,
		       const gchar *strip_prefix)
{
    GEnumClass *eclass;
    guint i;

    if (!G_TYPE_IS_ENUM(enum_type)) {
	if (G_TYPE_IS_FLAGS(enum_type))	/* See bug #136204 */
	    pyg_flags_add_constants(module, enum_type, strip_prefix);
	else
	    g_warning("`%s' is not an enum type", g_type_name(enum_type));
	return;
    }
    g_return_if_fail (strip_prefix != NULL);

    eclass = G_ENUM_CLASS(g_type_class_ref(enum_type));

    for (i = 0; i < eclass->n_values; i++) {
	gchar *name = eclass->values[i].value_name;
	gint value = eclass->values[i].value;

	PyModule_AddIntConstant(module,
				pyg_constant_strip_prefix(name, strip_prefix),
				(long) value);
    }

    g_type_class_unref(eclass);
}

/**
 * pyg_flags_add_constants:
 * @module: a Python module
 * @flags_type: the GType of the flags type.
 * @strip_prefix: the prefix to strip from the constant names.
 *
 * Adds constants to the given Python module for each value name of
 * the flags set.  A prefix will be stripped from each flag name.
 */
static void
pyg_flags_add_constants(PyObject *module, GType flags_type,
			const gchar *strip_prefix)
{
    GFlagsClass *fclass;
    guint i;

    if (!G_TYPE_IS_FLAGS(flags_type)) {
	if (G_TYPE_IS_ENUM(flags_type))	/* See bug #136204 */
	    pyg_enum_add_constants(module, flags_type, strip_prefix);
	else
	    g_warning("`%s' is not an flags type", g_type_name(flags_type));
	return;
    }
    g_return_if_fail (strip_prefix != NULL);

    fclass = G_FLAGS_CLASS(g_type_class_ref(flags_type));

    for (i = 0; i < fclass->n_values; i++) {
	gchar *name = fclass->values[i].value_name;
	guint value = fclass->values[i].value;

	PyModule_AddIntConstant(module,
				pyg_constant_strip_prefix(name, strip_prefix),
				(long) value);
    }

    g_type_class_unref(fclass);
}

/**
 * pyg_error_check:
 * @error: a pointer to the GError.
 *
 * Checks to see if the GError has been set.  If the error has been
 * set, then the gobject.GError Python exception will be raised, and
 * the GError cleared.
 *
 * Returns: True if an error was set.
 */
gboolean
pyg_error_check(GError **error)
{
    PyGILState_STATE state;

    g_return_val_if_fail(error != NULL, FALSE);

    if (*error != NULL) {
	PyObject *exc_instance;
	PyObject *d;
	
	state = pyg_gil_state_ensure();
	
	exc_instance = PyObject_CallFunction(gerror_exc, "z",
					     (*error)->message);
	PyObject_SetAttrString(exc_instance, "domain",
			       d=PyString_FromString(g_quark_to_string((*error)->domain)));
	Py_DECREF(d);
	PyObject_SetAttrString(exc_instance, "code",
			       d=PyInt_FromLong((*error)->code));
	Py_DECREF(d);
	if ((*error)->message) {
	    PyObject_SetAttrString(exc_instance, "message",
				   d=PyString_FromString((*error)->message));
	    Py_DECREF(d);
	} else {
	    PyObject_SetAttrString(exc_instance, "message", Py_None);
	}

	PyErr_SetObject(gerror_exc, exc_instance);
	Py_DECREF(exc_instance);
	g_clear_error(error);
	
	pyg_gil_state_release(state);
	
	return TRUE;
    }
    return FALSE;
}


static PyObject *
_pyg_strv_from_gvalue(const GValue *value)
{
    gchar    **argv = (gchar **) g_value_get_boxed(value);
    int        argc = 0, i;
    PyObject  *py_argv;

    if (argv) {
        while (argv[argc])
            argc++;
    }
    py_argv = PyList_New(argc);
    for (i = 0; i < argc; ++i)
	PyList_SET_ITEM(py_argv, i, PyString_FromString(argv[i]));
    return py_argv;
}

static int
_pyg_strv_to_gvalue(GValue *value, PyObject *obj)
{
    int     argc, i;
    gchar **argv;

    if (!(PyTuple_Check(obj) || PyList_Check(obj)))
        return -1;

    argc = PySequence_Length(obj);
    for (i = 0; i < argc; ++i)
	if (!PyString_Check(PySequence_Fast_GET_ITEM(obj, i)))
	    return -1;
    argv = g_new(gchar *, argc + 1);
    for (i = 0; i < argc; ++i)
	argv[i] = g_strdup(PyString_AsString(PySequence_Fast_GET_ITEM(obj, i)));
    argv[i] = NULL;
    g_value_take_boxed(value, argv);
    return 0;
}

/**
 * pyg_parse_constructor_args: helper function for PyGObject constructors
 * @obj_type: GType of the GObject, for parameter introspection
 * @arg_names: %NULL-terminated array of constructor argument names
 * @prop_names: %NULL-terminated array of property names, with direct
 * correspondence to @arg_names
 * @params: GParameter array where parameters will be placed; length
 * of this array must be at least equal to the number of
 * arguments/properties
 * @nparams: output parameter to contain actual number of arguments found
 * @py_args: array of PyObject* containing the actual constructor arguments
 * 
 * Parses an array of PyObject's and creates a GParameter array
 * 
 * Return value: %TRUE if all is successful, otherwise %FALSE and
 * python exception set.
 **/
static gboolean
pyg_parse_constructor_args(GType        obj_type,
                           char       **arg_names,
                           char       **prop_names,
                           GParameter  *params,
                           guint       *nparams,
                           PyObject   **py_args)
{
    guint arg_i, param_i;
    GObjectClass *oclass;

    oclass = g_type_class_ref(obj_type);
    g_return_val_if_fail(oclass, FALSE);

    for (param_i = arg_i = 0; arg_names[arg_i]; ++arg_i) {
        GParamSpec *spec;
        if (!py_args[arg_i])
            continue;
        spec = g_object_class_find_property(oclass, prop_names[arg_i]);
        params[param_i].name = prop_names[arg_i];
        g_value_init(&params[param_i].value, spec->value_type);
        if (pyg_value_from_pyobject(&params[param_i].value, py_args[arg_i]) == -1) {
            int i;
            PyErr_Format(PyExc_TypeError, "could not convert parameter '%s' of type '%s'",
                         arg_names[arg_i], g_type_name(spec->value_type));
            g_type_class_unref(oclass);
            for (i = 0; i < param_i; ++i)
                g_value_unset(&params[i].value);
            return FALSE;
        }
        ++param_i;
    }
    g_type_class_unref(oclass);
    *nparams = param_i;
    return TRUE;
}

int
pygobject_constructv(PyGObject  *self,
                     guint       n_parameters,
                     GParameter *parameters)
{
    if (self->obj == NULL) {
        pygobject_init_wrapper_set((PyObject *) self);
        self->obj = g_object_newv(pyg_type_from_object((PyObject *) self),
                                  n_parameters, parameters);
        pygobject_init_wrapper_set(NULL);
        pygobject_register_wrapper((PyObject *) self);
    } else {
        int i;
        for (i = 0; i < n_parameters; ++i)
            g_object_set_property(self->obj,
				  parameters[i].name,
				  &parameters[i].value);
    }
    return 0;
}

/* This function is mostly juste copy-paste from g_object_new, but
 * calls pygobject_constructv instead of g_object_newv */
int
pygobject_construct(PyGObject *self, const char *first_property_name, ...)
{
    va_list var_args;
    GObjectClass *class;
    GParameter *params;
    const gchar *name;
    guint n_params = 0, n_alloced_params = 16;
    GType object_type = pyg_type_from_object((PyObject *) self);
    int retval;

    if (!first_property_name)
        return pygobject_constructv(self, 0, NULL);

    va_start(var_args, first_property_name);
    class = g_type_class_ref(object_type);

    params = g_new(GParameter, n_alloced_params);
    name = first_property_name;
    while (name)
    {
        gchar *error = NULL;
        GParamSpec *pspec = g_object_class_find_property(class, name);

        if (!pspec)
	{
            g_warning("%s: object class `%s' has no property named `%s'", 
                      G_STRFUNC, 
                      g_type_name(object_type), 
                      name);
            break;
	}
        if (n_params >= n_alloced_params)
	{
            n_alloced_params += 16;
            params = g_renew(GParameter, params, n_alloced_params);
	}
        params[n_params].name = name;
        params[n_params].value.g_type = 0;
        g_value_init(&params[n_params].value, G_PARAM_SPEC_VALUE_TYPE(pspec));
        G_VALUE_COLLECT(&params[n_params].value, var_args, 0, &error);
        if (error)
	{
            g_warning("%s: %s", G_STRFUNC, error);
            g_free(error);
            g_value_unset(&params[n_params].value);
            break;
	}
        n_params++;
        name = va_arg(var_args, gchar*);
    }

    retval = pygobject_constructv(self, n_params, params);

    while (n_params--)
        g_value_unset(&params[n_params].value);
    g_free(params);
    va_end(var_args);
    g_type_class_unref(class);
    return retval;
}

void
pyg_set_object_has_new_constructor(GType type)
{
    g_type_set_qdata(type, pygobject_has_updated_constructor_key, GINT_TO_POINTER(1));
}

#define GET_INT(x) (((PyIntObject*)x)->ob_ival)
PyObject *
pyg_integer_richcompare(PyObject *v, PyObject *w, int op)
{
    PyObject *result;
    gboolean t;

    switch (op) {
    case Py_EQ: t = GET_INT(v) == GET_INT(w); break;
    case Py_NE: t = GET_INT(v) != GET_INT(w); break;
    case Py_LE: t = GET_INT(v) <= GET_INT(w); break;
    case Py_GE: t = GET_INT(v) >= GET_INT(w); break;
    case Py_LT: t = GET_INT(v) <  GET_INT(w); break;
    case Py_GT: t = GET_INT(v) >  GET_INT(w); break;
    default: g_assert_not_reached();
    }

    result = t ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}
#undef GET_INT

static void
_log_func(const gchar *log_domain,
          GLogLevelFlags log_level,
          const gchar *message,
          gpointer user_data)
{
    PyGILState_STATE state;
    PyObject* warning = user_data;

    state = pyg_gil_state_ensure();
    PyErr_Warn(warning, (char *) message);
    pyg_gil_state_release(state);
}

/* ----------------- gobject module initialisation -------------- */

struct _PyGObject_Functions pygobject_api_functions = {
  pygobject_register_class,
  pygobject_register_wrapper,
  pygobject_register_sinkfunc,
  pygobject_lookup_class,
  pygobject_new,

  pyg_closure_new,
  pygobject_watch_closure,
  pyg_destroy_notify,

  pyg_type_from_object,
  pyg_type_wrapper_new,
  pyg_enum_get_value,
  pyg_flags_get_value,
  pyg_register_gtype_custom,
  pyg_value_from_pyobject,
  pyg_value_as_pyobject,

  pyg_register_interface,

  &PyGBoxed_Type,
  pyg_register_boxed,
  pyg_boxed_new,

  &PyGPointer_Type,
  pyg_register_pointer,
  pyg_pointer_new,

  pyg_enum_add_constants,
  pyg_flags_add_constants,
  
  pyg_constant_strip_prefix,

  pyg_error_check,

  pyg_set_thread_block_funcs,
  (PyGThreadBlockFunc)0, /* block_threads */
  (PyGThreadBlockFunc)0, /* unblock_threads */

  &PyGParamSpec_Type,
  pyg_param_spec_new,
  pyg_param_spec_from_object,

  pyg_pyobj_to_unichar_conv,
  pyg_parse_constructor_args,
  pyg_param_gvalue_as_pyobject,
  pyg_param_gvalue_from_pyobject,

  &PyGEnum_Type,
  pyg_enum_add,
  pyg_enum_from_gtype,
  
  &PyGFlags_Type,
  pyg_flags_add,
  pyg_flags_from_gtype,

  FALSE, /* threads_enabled */
  pyg_enable_threads,
  pyg_gil_state_ensure_py23,
  pyg_gil_state_release_py23,
  pyg_register_class_init,
  pyg_register_interface_info,

  pyg_closure_set_exception_handler,
  pygobject_constructv,
  pygobject_construct,
  pyg_set_object_has_new_constructor
};

#define REGISTER_TYPE(d, type, name) \
    type.ob_type = &PyType_Type; \
    type.tp_alloc = PyType_GenericAlloc; \
    type.tp_new = PyType_GenericNew; \
    if (PyType_Ready(&type)) \
	return; \
    PyDict_SetItemString(d, name, (PyObject *)&type);

#define REGISTER_GTYPE(d, type, name, gtype) \
    REGISTER_TYPE(d, type, name); \
    PyDict_SetItemString(type.tp_dict, "__gtype__", \
			 o=pyg_type_wrapper_new(gtype)); \
    Py_DECREF(o);

DL_EXPORT(void)
initgobject(void)
{
    PyObject *m, *d, *o, *tuple;
    PyObject *descr;
    PyObject *warning;
    
    PyGParamSpec_Type.ob_type = &PyType_Type;
    
    m = Py_InitModule("gobject", pygobject_functions);
    d = PyModule_GetDict(m);

    g_type_init();

    pygboxed_type_key        = g_quark_from_static_string("PyGBoxed::class");
    pygboxed_marshal_key     = g_quark_from_static_string("PyGBoxed::marshal");
    pygenum_class_key        = g_quark_from_static_string("PyGEnum::class");
    pygflags_class_key       = g_quark_from_static_string("PyGFlags::class");
    pygobject_class_key      = g_quark_from_static_string("PyGObject::class");
    pygobject_class_init_key = g_quark_from_static_string("PyGObject::class-init");
    pygobject_wrapper_key    = g_quark_from_static_string("PyGObject::wrapper");
    pyginterface_type_key    = g_quark_from_static_string("PyGInterface::type");
    pyginterface_info_key    = g_quark_from_static_string("PyGInterface::info");
    pygpointer_class_key     = g_quark_from_static_string("PyGPointer::class");
    pygobject_has_updated_constructor_key =\
        g_quark_from_static_string("PyGObject::has-updated-constructor");

    REGISTER_TYPE(d, PyGTypeWrapper_Type, "GType");

    if (!PY_TYPE_OBJECT)
	PY_TYPE_OBJECT = g_boxed_type_register_static("PyObject",
						      pyobject_copy,
						      pyobject_free);

    gerror_exc = PyErr_NewException("gobject.GError", PyExc_RuntimeError,NULL);
    PyDict_SetItemString(d, "GError", gerror_exc);

    PyGObject_MetaType.tp_base = &PyType_Type;
    PyGObject_MetaType.tp_traverse = PyType_Type.tp_traverse;
    PyGObject_MetaType.tp_clear = PyType_Type.tp_clear;
    PyGObject_MetaType.tp_is_gc = PyType_Type.tp_is_gc;
    PyType_Ready(&PyGObject_MetaType);
    PyDict_SetItemString(d, "GObjectMeta", (PyObject *) &PyGObject_MetaType);

    PyGObject_Type.tp_alloc = PyType_GenericAlloc;
    PyGObject_Type.tp_new = PyType_GenericNew;
    pygobject_register_class(d, "GObject", G_TYPE_OBJECT,
			     &PyGObject_Type, NULL);
    PyDict_SetItemString(PyGObject_Type.tp_dict, "__gdoc__",
			 pyg_object_descr_doc_get());
    pyg_set_object_has_new_constructor(G_TYPE_OBJECT);

      /* GObject properties descriptor */
    if (PyType_Ready(&PyGProps_Type) < 0)
        return;
    if (PyType_Ready(&PyGPropsDescr_Type) < 0)
        return;
    if (PyType_Ready(&PyGPropsIter_Type) < 0)
        return;
    descr = PyObject_New(PyObject, &PyGPropsDescr_Type);
    PyDict_SetItemString(PyGObject_Type.tp_dict, "props", descr);

    REGISTER_GTYPE(d, PyGInterface_Type, "GInterface", G_TYPE_INTERFACE);
    PyDict_SetItemString(PyGInterface_Type.tp_dict, "__doc__",
			 pyg_object_descr_doc_get());
    PyDict_SetItemString(PyGInterface_Type.tp_dict, "__gdoc__",
			 pyg_object_descr_doc_get());

    REGISTER_GTYPE(d, PyGBoxed_Type, "GBoxed", G_TYPE_BOXED);
    REGISTER_GTYPE(d, PyGPointer_Type, "GPointer", G_TYPE_POINTER); 
    PyGEnum_Type.tp_base = &PyInt_Type;
    REGISTER_GTYPE(d, PyGEnum_Type, "GEnum", G_TYPE_ENUM);
    PyGFlags_Type.tp_base = &PyInt_Type;
    REGISTER_GTYPE(d, PyGFlags_Type, "GFlags", G_TYPE_FLAGS);

    REGISTER_TYPE(d, PyGMainLoop_Type, "MainLoop"); 
    REGISTER_TYPE(d, PyGMainContext_Type, "MainContext"); 

    REGISTER_TYPE(d, PyGIOChannel_Type, "IOChannel");

    REGISTER_TYPE(d, PyGSource_Type, "Source");
    REGISTER_TYPE(d, PyGIdle_Type, "Idle");
    REGISTER_TYPE(d, PyGTimeout_Type, "Timeout");
    REGISTER_TYPE(d, PyGPollFD_Type, "PollFD");

    /* glib version */
    tuple = Py_BuildValue ("(iii)", glib_major_version, glib_minor_version,
			   glib_micro_version);
    PyDict_SetItemString(d, "glib_version", tuple);    
    Py_DECREF(tuple);

    /* pygobject version */
    tuple = Py_BuildValue ("(iii)",
			   PYGOBJECT_MAJOR_VERSION,
			   PYGOBJECT_MINOR_VERSION,
			   PYGOBJECT_MICRO_VERSION);
    PyDict_SetItemString(d, "pygobject_version", tuple);
    /* backwards compatibility */
    PyDict_SetItemString(d, "pygtk_version", tuple);
    Py_DECREF(tuple);

    /* for addon libraries ... */
    PyDict_SetItemString(d, "_PyGObject_API",
			 o=PyCObject_FromVoidPtr(&pygobject_api_functions,NULL));
    Py_DECREF(o);
	
    /* some constants */
    PyModule_AddIntConstant(m, "SIGNAL_RUN_FIRST", G_SIGNAL_RUN_FIRST);
    PyModule_AddIntConstant(m, "SIGNAL_RUN_LAST", G_SIGNAL_RUN_LAST);
    PyModule_AddIntConstant(m, "SIGNAL_RUN_CLEANUP", G_SIGNAL_RUN_CLEANUP);
    PyModule_AddIntConstant(m, "SIGNAL_NO_RECURSE", G_SIGNAL_NO_RECURSE);
    PyModule_AddIntConstant(m, "SIGNAL_DETAILED", G_SIGNAL_DETAILED);
    PyModule_AddIntConstant(m, "SIGNAL_ACTION", G_SIGNAL_ACTION);
    PyModule_AddIntConstant(m, "SIGNAL_NO_HOOKS", G_SIGNAL_NO_HOOKS);

    PyModule_AddIntConstant(m, "PARAM_READABLE", G_PARAM_READABLE);
    PyModule_AddIntConstant(m, "PARAM_WRITABLE", G_PARAM_WRITABLE);
    PyModule_AddIntConstant(m, "PARAM_CONSTRUCT", G_PARAM_CONSTRUCT);
    PyModule_AddIntConstant(m, "PARAM_CONSTRUCT_ONLY", G_PARAM_CONSTRUCT_ONLY);
    PyModule_AddIntConstant(m, "PARAM_LAX_VALIDATION", G_PARAM_LAX_VALIDATION);
    PyModule_AddIntConstant(m, "PARAM_READWRITE", G_PARAM_READWRITE);

    PyModule_AddIntConstant(m, "PRIORITY_HIGH", G_PRIORITY_HIGH);
    PyModule_AddIntConstant(m, "PRIORITY_DEFAULT", G_PRIORITY_DEFAULT);
    PyModule_AddIntConstant(m, "PRIORITY_HIGH_IDLE", G_PRIORITY_HIGH_IDLE);
    PyModule_AddIntConstant(m, "PRIORITY_DEFAULT_IDLE",G_PRIORITY_DEFAULT_IDLE);
    PyModule_AddIntConstant(m, "PRIORITY_LOW", G_PRIORITY_LOW);

    PyModule_AddIntConstant(m, "IO_IN",   G_IO_IN);
    PyModule_AddIntConstant(m, "IO_OUT",  G_IO_OUT);
    PyModule_AddIntConstant(m, "IO_PRI",  G_IO_PRI);
    PyModule_AddIntConstant(m, "IO_ERR",  G_IO_ERR);
    PyModule_AddIntConstant(m, "IO_HUP",  G_IO_HUP);
    PyModule_AddIntConstant(m, "IO_NVAL", G_IO_NVAL);

#define addint(x) PyModule_AddIntConstant(m, #x, G_##x)

    addint(IO_STATUS_ERROR);
    addint(IO_STATUS_NORMAL);
    addint(IO_STATUS_EOF);
    addint(IO_STATUS_AGAIN);

    addint(IO_FLAG_APPEND);
    addint(IO_FLAG_NONBLOCK);
    addint(IO_FLAG_IS_READABLE);
    addint(IO_FLAG_IS_WRITEABLE);
    addint(IO_FLAG_IS_SEEKABLE);
    addint(IO_FLAG_MASK);
    addint(IO_FLAG_GET_MASK);
    addint(IO_FLAG_SET_MASK);

#undef addint

    PyModule_AddIntConstant(m, "SPAWN_LEAVE_DESCRIPTORS_OPEN", G_SPAWN_LEAVE_DESCRIPTORS_OPEN);
    PyModule_AddIntConstant(m, "SPAWN_DO_NOT_REAP_CHILD", G_SPAWN_DO_NOT_REAP_CHILD);
    PyModule_AddIntConstant(m, "SPAWN_SEARCH_PATH", G_SPAWN_SEARCH_PATH);
    PyModule_AddIntConstant(m, "SPAWN_STDOUT_TO_DEV_NULL", G_SPAWN_STDOUT_TO_DEV_NULL);
    PyModule_AddIntConstant(m, "SPAWN_STDERR_TO_DEV_NULL", G_SPAWN_STDERR_TO_DEV_NULL);
    PyModule_AddIntConstant(m, "SPAWN_CHILD_INHERITS_STDIN", G_SPAWN_CHILD_INHERITS_STDIN);
    PyModule_AddIntConstant(m, "SPAWN_FILE_AND_ARGV_ZERO", G_SPAWN_FILE_AND_ARGV_ZERO);


    PyModule_AddObject(m, "TYPE_INVALID", pyg_type_wrapper_new(G_TYPE_INVALID));
    PyModule_AddObject(m, "TYPE_NONE", pyg_type_wrapper_new(G_TYPE_NONE));
    PyModule_AddObject(m, "TYPE_INTERFACE", pyg_type_wrapper_new(G_TYPE_INTERFACE));
    PyModule_AddObject(m, "TYPE_CHAR", pyg_type_wrapper_new(G_TYPE_CHAR));
    PyModule_AddObject(m, "TYPE_UCHAR", pyg_type_wrapper_new(G_TYPE_UCHAR));
    PyModule_AddObject(m, "TYPE_UNICHAR", pyg_type_wrapper_new(G_TYPE_UINT));
    PyModule_AddObject(m, "TYPE_BOOLEAN", pyg_type_wrapper_new(G_TYPE_BOOLEAN));
    PyModule_AddObject(m, "TYPE_INT", pyg_type_wrapper_new(G_TYPE_INT));
    PyModule_AddObject(m, "TYPE_UINT", pyg_type_wrapper_new(G_TYPE_UINT));
    PyModule_AddObject(m, "TYPE_LONG", pyg_type_wrapper_new(G_TYPE_LONG));
    PyModule_AddObject(m, "TYPE_ULONG", pyg_type_wrapper_new(G_TYPE_ULONG));
    PyModule_AddObject(m, "TYPE_INT64", pyg_type_wrapper_new(G_TYPE_INT64));
    PyModule_AddObject(m, "TYPE_UINT64", pyg_type_wrapper_new(G_TYPE_UINT64));
    PyModule_AddObject(m, "TYPE_ENUM", pyg_type_wrapper_new(G_TYPE_ENUM));
    PyModule_AddObject(m, "TYPE_FLAGS", pyg_type_wrapper_new(G_TYPE_FLAGS));
    PyModule_AddObject(m, "TYPE_FLOAT", pyg_type_wrapper_new(G_TYPE_FLOAT));
    PyModule_AddObject(m, "TYPE_DOUBLE", pyg_type_wrapper_new(G_TYPE_DOUBLE));
    PyModule_AddObject(m, "TYPE_STRING", pyg_type_wrapper_new(G_TYPE_STRING));
    PyModule_AddObject(m, "TYPE_POINTER", pyg_type_wrapper_new(G_TYPE_POINTER));
    PyModule_AddObject(m, "TYPE_BOXED", pyg_type_wrapper_new(G_TYPE_BOXED));
    PyModule_AddObject(m, "TYPE_PARAM", pyg_type_wrapper_new(G_TYPE_PARAM));
    PyModule_AddObject(m, "TYPE_OBJECT", pyg_type_wrapper_new(G_TYPE_OBJECT));
    PyModule_AddObject(m, "TYPE_PYOBJECT", pyg_type_wrapper_new(PY_TYPE_OBJECT));

    pyg_register_gtype_custom(G_TYPE_STRV,
			      _pyg_strv_from_gvalue,
			      _pyg_strv_to_gvalue);

    warning = PyErr_NewException("gobject.Warning", PyExc_Warning, NULL);
    PyDict_SetItemString(d, "Warning", warning);
    g_log_set_handler("GLib", G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_WARNING,
                      _log_func, warning);
    g_log_set_handler("GLib-GObject", G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_WARNING,
                      _log_func, warning);
    g_log_set_handler("GThread", G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_WARNING,
                      _log_func, warning);

      /* signal registration recognizes this special accumulator 'constant' */
    _pyg_signal_accumulator_true_handled_func = \
        PyDict_GetItemString(d, "signal_accumulator_true_handled");
}

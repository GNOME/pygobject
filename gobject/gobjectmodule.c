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

static PyObject *gerror_exc = NULL;
static const gchar *pyginterface_type_id   = "PyGInterface::type";
GQuark pyginterface_type_key  = 0;

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

    pyg_block_threads();
    Py_DECREF(obj);
    pyg_unblock_threads();
}

/* -------------- GParamSpec objects ---------------------------- */

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

static PyObject *
pyg_param_spec_getattr(PyGParamSpec *self, const gchar *attr)
{
    if (!strcmp(attr, "__members__")) {
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

    pyg_block_threads();
    Py_DECREF(object);
    pyg_unblock_threads();
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
    object_free,			/* tp_free */
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

/* -------------- GMainContext objects ---------------------------- */

static int
pyg_main_context_new(PyGMainContext *self)
{
    self->context = g_main_context_new();
    return 0;
}

static int
pyg_main_context_compare(PyGMainContext *self, PyGMainContext *v)
{
    if (self->context == v->context) return 0;
    if (self->context > v->context) return -1;
    return 1;
}

static PyObject *
pyg_main_context_default (PyObject *unused)
{
    PyGMainContext *self;

    self = (PyGMainContext *)PyObject_NEW(PyGMainContext,
					  &PyGMainContext_Type);
    if (self == NULL)
	return NULL;

    self->context = g_main_context_default();
    return (PyObject *)self;

}

static PyObject *
_wrap_g_main_context_iteration (PyGMainContext *self, PyObject *args)
{
    PyObject *py_ret;
    gboolean may_block = TRUE;
    
    if (!PyArg_ParseTuple(args, "|b:GMainContext.iteration",
			  &may_block))
	return NULL;
	
    py_ret = g_main_context_iteration(self->context, may_block)
	? Py_True : Py_False;
    
    Py_INCREF(py_ret);
    return py_ret;
}

static PyObject *
_wrap_g_main_context_pending (PyGMainContext *self)
{
    PyObject *py_ret;

    py_ret = g_main_context_pending(self->context) ? Py_True : Py_False;
    
    Py_INCREF(py_ret);
    return py_ret;
}

static PyMethodDef _PyGMainContext_methods[] = {
    { "iteration", (PyCFunction)_wrap_g_main_context_iteration, METH_VARARGS },
    { "pending", (PyCFunction)_wrap_g_main_context_pending, METH_NOARGS },
    { NULL, NULL, 0 }
};

PyTypeObject PyGMainContext_Type = {
    PyObject_HEAD_INIT(NULL)
    0,			
    "gobject.MainContext",	
    sizeof(PyGMainContext),	
    0,			
    /* methods */
    (destructor)0,	
    (printfunc)0,	
    (getattrfunc)0,	
    (setattrfunc)0,	
    (cmpfunc)pyg_main_context_compare,		
    (reprfunc)0,	
    0,			
    0,		
    0,		
    (hashfunc)0,		
    (ternaryfunc)0,		
    (reprfunc)0,		
    (getattrofunc)0,		
    (setattrofunc)0,		
    0,				
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, 
    NULL,
    (traverseproc)0,		
    (inquiry)0,
    (richcmpfunc)0,	
    0,             
    (getiterfunc)0,
    (iternextfunc)0,
    _PyGMainContext_methods,
    0,				
    0,		       	
    NULL,		
    NULL,		
    (descrgetfunc)0,	
    (descrsetfunc)0,	
    0,                 
    (initproc)pyg_main_context_new,
};

/* -------------- GMainLoop objects ---------------------------- */

static int
pyg_main_loop_new(PyGMainLoop *self, PyObject *args, PyObject *kwargs)
{

    static char *kwlist[] = { "context", "is_running", NULL };
    PyObject *py_context = Py_None;
    int is_running;
    GMainContext *context;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "|Ob:GMainLoop.__init__",
				     kwlist, &py_context, &is_running))
        return -1;

    if (!PyObject_TypeCheck(py_context, &PyGMainContext_Type) &&
	py_context != Py_None) {
	PyErr_SetString(PyExc_TypeError,
			"context must be a gobject.GMainContext or None");
	return -1;
    }

    if (py_context != Py_None) {
	context = ((PyGMainContext*)py_context)->context;
    } else {
	context = NULL;
    }

    self->loop = g_main_loop_new(context, is_running);
    return 0;
}

static int
pyg_main_loop_compare(PyGMainLoop *self, PyGMainLoop *v)
{
    if (self->loop == v->loop) return 0;
    if (self->loop > v->loop) return -1;
    return 1;
}

static PyObject *
_wrap_g_main_loop_get_context (PyGMainLoop *loop)
{
    PyGMainContext *self;

    self = (PyGMainContext *)PyObject_NEW(PyGMainContext,
					  &PyGMainContext_Type);
    
    self->context = g_main_loop_get_context(loop->loop);
    
    if (self->context == NULL)
	return NULL;

    return (PyObject *)self;
}

static PyObject *
_wrap_g_main_loop_is_running (PyGMainLoop *self)
{
    PyObject *py_ret;
    
    py_ret = g_main_loop_is_running(self->loop) ? Py_True : Py_False;
    Py_INCREF(py_ret);
    return py_ret;
    
}

static PyObject *
_wrap_g_main_loop_quit (PyGMainLoop *self)
{
    g_main_loop_quit(self->loop);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_g_main_loop_run (PyGMainLoop *self)
{
    g_main_loop_run(self->loop);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef _PyGMainLoop_methods[] = {
    { "get_context", (PyCFunction)_wrap_g_main_loop_get_context, METH_NOARGS },
    { "is_running", (PyCFunction)_wrap_g_main_loop_is_running, METH_NOARGS },
    { "quit", (PyCFunction)_wrap_g_main_loop_quit, METH_NOARGS },
    { "run", (PyCFunction)_wrap_g_main_loop_run, METH_NOARGS },
    { NULL, NULL, 0 }
};

PyTypeObject PyGMainLoop_Type = {
    PyObject_HEAD_INIT(NULL)
    0,			
    "gobject.MainLoop",	
    sizeof(PyGMainLoop),	
    0,			
    /* methods */
    (destructor)0,	
    (printfunc)0,	
    (getattrfunc)0,	
    (setattrfunc)0,	
    (cmpfunc)pyg_main_loop_compare,
    (reprfunc)0,	
    0,			
    0,		
    0,		
    (hashfunc)0,		
    (ternaryfunc)0,		
    (reprfunc)0,		
    (getattrofunc)0,		
    (setattrofunc)0,		
    0,				
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, 
    NULL,
    (traverseproc)0,		
    (inquiry)0,
    (richcmpfunc)0,	
    0,             
    (getiterfunc)0,
    (iternextfunc)0,
    _PyGMainLoop_methods,
    0,				
    0,		       	
    NULL,		
    NULL,		
    (descrgetfunc)0,	
    (descrsetfunc)0,	
    0,                 
    (initproc)pyg_main_loop_new,
};

/* ---------------- gobject module functions -------------------- */

static PyObject *
pyg_type_name (PyObject *self, PyObject *args)
{
    PyObject *gtype;
    GType type;
    const gchar *name;

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

    if (!PyArg_ParseTuple(args, "s:gobject.type_from_name", &name))
	return NULL;
    type = g_type_from_name(name);
    if (type != 0)
	return pyg_type_wrapper_new(type);
    PyErr_SetString(PyExc_RuntimeError, "unknown type name");
    return NULL;
}

static PyObject *
pyg_type_parent (PyObject *self, PyObject *args)
{
    PyObject *gtype;
    GType type, parent;

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

    if (!PyArg_ParseTuple(args, "OO:gobject.type_is_a", &gtype, &gparent))
	return NULL;
    if ((type = pyg_type_from_object(gtype)) == 0)
	return NULL;
    if ((parent = pyg_type_from_object(gparent)) == 0)
	return NULL;
    return PyInt_FromLong(g_type_is_a(type, parent));
}

static PyObject *
pyg_type_children (PyObject *self, PyObject *args)
{
    PyObject *gtype, *list;
    GType type, *children;
    guint n_children, i;

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

    pyg_block_threads();

    object_wrapper = pygobject_new(object);

    if (object_wrapper == NULL) {
	pyg_unblock_threads();
	g_return_if_fail(object_wrapper != NULL);
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

    pyg_unblock_threads();
}

static void
pyg_object_get_property (GObject *object, guint property_id,
			 GValue *value, GParamSpec *pspec)
{
    PyObject *object_wrapper, *retval;
    PyObject *py_pspec;

    pyg_block_threads();

    object_wrapper = pygobject_new(object);

    if (object_wrapper == NULL) {
	pyg_unblock_threads();
	g_return_if_fail(object_wrapper != NULL);
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
    
    pyg_unblock_threads();        
}

static void
pyg_object_class_init(GObjectClass *class, PyObject *py_class)
{
    class->set_property = pyg_object_set_property;
    class->get_property = pyg_object_get_property;
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

    if (!PyArg_ParseTuple(tuple, "iOO", &signal_flags, &py_return_type,
			  &py_param_types)) {
	gchar buf[128];

	PyErr_Clear();
	g_snprintf(buf, sizeof(buf), "value for __gsignals__['%s'] not in correct format", signal_name);
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

    signal_id = g_signal_newv(signal_name, instance_type, signal_flags,
			      pyg_signal_class_closure_get(),
			      (GSignalAccumulator)0, NULL,
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

static gboolean
add_signals (GType instance_type, PyObject *signals)
{
    gboolean ret = TRUE;
    GObjectClass *oclass;
    int pos = 0;
    PyObject *key, *value;

    oclass = g_type_class_ref(instance_type);
    while (PyDict_Next(signals, &pos, &key, &value)) {
	const gchar *signal_name;

	if (!PyString_Check(key)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gsignals__ keys must be strings");
	    ret = FALSE;
	    break;
	}
	signal_name = PyString_AsString (key);

	if (value == Py_None ||
	    (PyString_Check(value) &&
	     !strcmp(PyString_AsString(value), "override"))) {
	    ret = override_signal(instance_type, signal_name);
	} else {
	    ret = create_signal(instance_type, signal_name, value);
	}

	if (!ret)
	    break;
    }
    g_type_class_unref(oclass);
    return ret;
}

static gboolean
create_property (GObjectClass *oclass, const gchar *prop_name,
		 GType prop_type, const gchar *nick, const gchar *blurb,
		 PyObject *args, GParamFlags flags)
{
    GParamSpec *pspec = NULL;

    switch (G_TYPE_FUNDAMENTAL(prop_type)) {
    case G_TYPE_CHAR:
	{
	    gchar minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "ccc", &minimum, &maximum,
				  &default_value))
		return FALSE;
	    pspec = g_param_spec_char (prop_name, nick, blurb, minimum,
				       maximum, default_value, flags);
	}
	break;
    case G_TYPE_UCHAR:
	{
	    gchar minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "ccc", &minimum, &maximum,
				  &default_value))
		return FALSE;
	    pspec = g_param_spec_uchar (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_BOOLEAN:
	{
	    gboolean default_value;

	    if (!PyArg_ParseTuple(args, "i", &default_value))
		return FALSE;
	    pspec = g_param_spec_boolean (prop_name, nick, blurb,
					  default_value, flags);
	}
	break;
    case G_TYPE_INT:
	{
	    gint minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "iii", &minimum, &maximum,
				  &default_value))
		return FALSE;
	    pspec = g_param_spec_int (prop_name, nick, blurb, minimum,
				      maximum, default_value, flags);
	}
	break;
    case G_TYPE_UINT:
	{
	    guint minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "iii", &minimum, &maximum,
				  &default_value))
		return FALSE;
	    pspec = g_param_spec_uint (prop_name, nick, blurb, minimum,
				       maximum, default_value, flags);
	}
	break;
    case G_TYPE_LONG:
	{
	    glong minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "lll", &minimum, &maximum,
				  &default_value))
		return FALSE;
	    pspec = g_param_spec_long (prop_name, nick, blurb, minimum,
				       maximum, default_value, flags);
	}
	break;
    case G_TYPE_ULONG:
	{
	    gulong minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "lll", &minimum, &maximum,
				  &default_value))
		return FALSE;
	    pspec = g_param_spec_ulong (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_INT64:
	{
	    gint64 minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "LLL", &minimum, &maximum,
				  &default_value))
		return FALSE;
	    pspec = g_param_spec_int64 (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_UINT64:
	{
	    guint64 minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "LLL", &minimum, &maximum,
				  &default_value))
		return FALSE;
	    pspec = g_param_spec_uint64 (prop_name, nick, blurb, minimum,
					 maximum, default_value, flags);
	}
	break;
    case G_TYPE_ENUM:
	{
	    gint default_value;

	    if (!PyArg_ParseTuple(args, "i", &default_value))
		return FALSE;
	    pspec = g_param_spec_enum (prop_name, nick, blurb,
				       prop_type, default_value, flags);
	}
	break;
    case G_TYPE_FLAGS:
	{
	    guint default_value;

	    if (!PyArg_ParseTuple(args, "i", &default_value))
		return FALSE;
	    pspec = g_param_spec_flags (prop_name, nick, blurb,
					prop_type, default_value, flags);
	}
	break;
    case G_TYPE_FLOAT:
	{
	    gfloat minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "fff", &minimum, &maximum,
				  &default_value))
		return FALSE;
	    pspec = g_param_spec_float (prop_name, nick, blurb, minimum,
					maximum, default_value, flags);
	}
	break;
    case G_TYPE_DOUBLE:
	{
	    gdouble minimum, maximum, default_value;

	    if (!PyArg_ParseTuple(args, "ddd", &minimum, &maximum,
				  &default_value))
		return FALSE;
	    pspec = g_param_spec_double (prop_name, nick, blurb, minimum,
					 maximum, default_value, flags);
	}
	break;
    case G_TYPE_STRING:
	{
	    const gchar *default_value;

	    if (!PyArg_ParseTuple(args, "z", &default_value))
		return FALSE;
	    pspec = g_param_spec_string (prop_name, nick, blurb,
					 default_value, flags);
	}
	break;
    case G_TYPE_PARAM:
	if (!PyArg_ParseTuple(args, ""))
	    return FALSE;
	pspec = g_param_spec_param (prop_name, nick, blurb, prop_type, flags);
	break;
    case G_TYPE_BOXED:
	if (!PyArg_ParseTuple(args, ""))
	    return FALSE;
	pspec = g_param_spec_boxed (prop_name, nick, blurb, prop_type, flags);
	break;
    case G_TYPE_POINTER:
	if (!PyArg_ParseTuple(args, ""))
	    return FALSE;
	pspec = g_param_spec_pointer (prop_name, nick, blurb, flags);
	break;
    case G_TYPE_OBJECT:
	if (!PyArg_ParseTuple(args, ""))
	    return FALSE;
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
	return FALSE;
    }
    g_object_class_install_property(oclass, 1, pspec);
    return TRUE;
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
	ret = create_property(oclass, prop_name, prop_type, nick, blurb,
			      slice, flags);
	Py_DECREF(slice);

	if (!ret)
	    break;
    }
    g_type_class_unref(oclass);
    return ret;
}

static PyObject *
pyg_type_register(PyObject *self, PyObject *args)
{
    PyObject *gtype, *module, *gsignals, *gproperties;
    PyTypeObject *class;
    GType parent_type, instance_type;
    gchar *type_name = NULL;
    gint i;
    GTypeQuery query;
    GTypeInfo type_info = {
	0,    /* class_size */

	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,

	(GClassInitFunc) pyg_object_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class_data */

	0,    /* instance_size */
	0,    /* n_preallocs */
	(GInstanceInitFunc) NULL
    };

    if (!PyArg_ParseTuple(args, "O:gobject.type_register", &class))
	return NULL;
    if (!PyType_IsSubtype(class, &PyGObject_Type)) {
	PyErr_SetString(PyExc_TypeError,"argument must be a GObject subclass");
	return NULL;
    }

    /* find the GType of the parent */
    parent_type = pyg_type_from_object((PyObject *)class);
    if (!parent_type) {
	return NULL;
    }

    /* make name for new widget */
    module = PyObject_GetAttrString((PyObject *)class, "__module__");
    if (module && PyString_Check(module)) {
	type_name = g_strconcat(PyString_AsString(module), ".",
				class->tp_name, NULL);
    } else {
	if (module)
	    Py_DECREF(module);
	else
	    PyErr_Clear();
	type_name = g_strdup(class->tp_name);
    }
    /* convert '.' in type name to '+', which isn't banned (grumble) */
    for (i = 0; type_name[i] != '\0'; i++)
	if (type_name[i] == '.')
	    type_name[i] = '+';

    /* set class_data that will be passed to the class_init function. */
    type_info.class_data = class;

    /* fill in missing values of GTypeInfo struct */
    g_type_query(parent_type, &query);
    type_info.class_size = query.class_size;
    type_info.instance_size = query.instance_size;

    /* create new typecode */
    instance_type = g_type_register_static(parent_type, type_name,
					   &type_info, 0);
    g_free(type_name);
    if (instance_type == 0) {
	PyErr_SetString(PyExc_RuntimeError, "could not create new GType");
	return NULL;
    }

    /* store pointer to the class with the GType */
    Py_INCREF(class);
    g_type_set_qdata(instance_type, g_quark_from_string("PyGObject::class"),
		     class);

    /* set new value of __gtype__ on class */
    gtype = pyg_type_wrapper_new(instance_type);
    PyDict_SetItemString(class->tp_dict, "__gtype__", gtype);
    Py_DECREF(gtype);

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
	    Py_DECREF(gsignals);
	    return NULL;
	}
	if (!add_signals(instance_type, gsignals)) {
	    Py_DECREF(gsignals);
	    return NULL;
	}
	PyDict_DelItemString(class->tp_dict, "__gsignals__");
	/* Borrowed reference. Py_DECREF(gsignals); */
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
	    Py_DECREF(gproperties);
	    return NULL;
	}
	if (!add_properties(instance_type, gproperties)) {
	    Py_DECREF(gproperties);
	    return NULL;
	}
	PyDict_DelItemString(class->tp_dict, "__gproperties__");
	/* Borrowed reference. Py_DECREF(gproperties); */
    } else {
	PyErr_Clear();
    }

    Py_INCREF(Py_None);
    return Py_None;
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
pyg_signal_list_names (PyObject *self, PyObject *args)
{
    PyObject *py_itype, *list;
    GObjectClass *class;
    GType itype;
    guint n;
    guint *ids;
    guint i;

    if (!PyArg_ParseTuple(args, "O:gobject.signal_list_names", &py_itype))
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
	PyTuple_SetItem(list, i, PyString_FromString(g_signal_name(ids[i])));
    g_free(ids);
    g_type_class_unref(class);
    return list;
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
	    if (pyg_value_from_pyobject(&params[n_params].value, value)) {
		PyErr_Format(PyExc_TypeError,
			     "could not convert value for property `%s'",
			     key_str);
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
    
    if (obj)
	return pygobject_new ((GObject *)obj);
    return NULL;
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

static gboolean
handler_marshal(gpointer user_data)
{
    PyObject *tuple, *ret;
    gboolean res;

    g_return_val_if_fail(user_data != NULL, FALSE);

    pyg_block_threads();

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
    pyg_unblock_threads();

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
    handler_id = g_idle_add_full(priority, handler_marshal, data,
				 (GDestroyNotify)pyg_destroy_notify);
    return PyInt_FromLong(handler_id);
}

static PyObject *
pyg_timeout_add(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *first, *callback, *cbargs = NULL, *data;
    gint len, priority = G_PRIORITY_DEFAULT, interval;
    guint handler_id;

    len = PyTuple_Size(args);
    if (len < 2) {
	PyErr_SetString(PyExc_TypeError,
			"timeout_add requires at least 2 args");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 2);
    if (!PyArg_ParseTuple(first, "iO:timeout_add", &interval, &callback)) {
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
    handler_id = g_timeout_add_full(priority, interval, handler_marshal, data,
				    (GDestroyNotify)pyg_destroy_notify);
    return PyInt_FromLong(handler_id);
}

static gboolean
iowatch_marshal(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
    PyObject *tuple, *func, *firstargs, *args, *ret;
    gboolean res;

    g_return_val_if_fail(user_data != NULL, FALSE);

    pyg_block_threads();

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
	res = PyObject_IsTrue(ret);
	Py_DECREF(ret);
    }

    pyg_unblock_threads();

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
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "i:source_remove", &tag))
	return NULL;

    ret = g_source_remove(tag) ? Py_True : Py_False;
    Py_INCREF(ret);
    return ret;
}

static PyMethodDef pygobject_functions[] = {
    { "type_name", pyg_type_name, METH_VARARGS },
    { "type_from_name", pyg_type_from_name, METH_VARARGS },
    { "type_parent", pyg_type_parent, METH_VARARGS },
    { "type_is_a", pyg_type_is_a, METH_VARARGS },
    { "type_children", pyg_type_children, METH_VARARGS },
    { "type_interfaces", pyg_type_interfaces, METH_VARARGS },
    { "type_register", pyg_type_register, METH_VARARGS },
    { "signal_new", pyg_signal_new, METH_VARARGS },
    { "signal_list_names", pyg_signal_list_names, METH_VARARGS },
    { "list_properties", pyg_object_class_list_properties, METH_VARARGS },
    { "new", (PyCFunction)pyg_object_new, METH_VARARGS|METH_KEYWORDS },
    { "idle_add", (PyCFunction)pyg_idle_add, METH_VARARGS|METH_KEYWORDS },
    { "timeout_add", (PyCFunction)pyg_timeout_add, METH_VARARGS|METH_KEYWORDS },
    { "io_add_watch", (PyCFunction)pyg_io_add_watch, METH_VARARGS|METH_KEYWORDS },
    { "source_remove", pyg_source_remove, METH_VARARGS },
    { "main_context_default", (PyCFunction)pyg_main_context_default, METH_NOARGS },
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
static char *
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
static gboolean
pyg_error_check(GError **error)
{
    g_return_val_if_fail(error != NULL, FALSE);

    if (*error != NULL) {
	PyObject *exc_instance;
	PyObject *d;
	
	pyg_block_threads();
	
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
	
	pyg_unblock_threads();
	
	return TRUE;
    }
    return FALSE;
}


static PyObject *
_pyg_strv_from_gvalue(const GValue *value)
{
    gchar    **argv = (gchar **) g_value_get_boxed(value);
    int        argc, i;
    PyObject  *py_argv;

    for (argc = -1; argv[++argc];);
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

    if (!PySequence_Check(obj)) return -1;
    argc = PySequence_Length(obj);
    for (i = 0; i < argc; ++i)
	if (!PyString_Check(PySequence_Fast_GET_ITEM(obj, i)))
	    return -1;
    argv = g_new(gchar *, argc + 1);
    for (i = 0; i < argc; ++i)
	argv[i] = g_strdup(PyString_AsString(PySequence_Fast_GET_ITEM(obj, i)));
    argv[i] = NULL;
    g_value_init(value, G_TYPE_STRV);
    g_value_take_boxed(value, argv);
    return 0;
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
  pyg_register_boxed_custom,
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
};

DL_EXPORT(void)
initgobject(void)
{
    PyObject *m, *d, *o, *tuple;

    PyGTypeWrapper_Type.ob_type = &PyType_Type;
    PyGParamSpec_Type.ob_type = &PyType_Type;

    m = Py_InitModule("gobject", pygobject_functions);
    d = PyModule_GetDict(m);

#ifdef ENABLE_PYGTK_THREADING
    if (!g_threads_got_initialized)
	g_thread_init(NULL);
#endif

    g_type_init();

    PY_TYPE_OBJECT = g_boxed_type_register_static("PyObject",
						  pyobject_copy,
						  pyobject_free);

    gerror_exc = PyErr_NewException("gobject.GError", PyExc_RuntimeError,NULL);
    PyDict_SetItemString(d, "GError", gerror_exc);

    PyGObject_Type.tp_alloc = PyType_GenericAlloc;
    PyGObject_Type.tp_new = PyType_GenericNew;
    pygobject_register_class(d, "GObject", G_TYPE_OBJECT,
			     &PyGObject_Type, NULL);
    PyDict_SetItemString(PyGObject_Type.tp_dict, "__gdoc__",
			 pyg_object_descr_doc_get());

    PyGInterface_Type.ob_type = &PyType_Type;
    PyGInterface_Type.tp_alloc = PyType_GenericAlloc;
    PyGInterface_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGInterface_Type))
	return;
    PyDict_SetItemString(d, "GInterface", (PyObject *)&PyGInterface_Type);
    PyDict_SetItemString(PyGInterface_Type.tp_dict, "__gtype__",
			 o=pyg_type_wrapper_new(G_TYPE_INTERFACE));
    Py_DECREF(o);
    PyDict_SetItemString(PyGInterface_Type.tp_dict, "__doc__",
			 pyg_object_descr_doc_get());
    PyDict_SetItemString(PyGInterface_Type.tp_dict, "__gdoc__",
			 pyg_object_descr_doc_get());
    pyginterface_type_key = g_quark_from_static_string(pyginterface_type_id);

    PyGBoxed_Type.ob_type = &PyType_Type;
    PyGBoxed_Type.tp_alloc = PyType_GenericAlloc;
    PyGBoxed_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGBoxed_Type))
	return;
    PyDict_SetItemString(d, "GBoxed", (PyObject *)&PyGBoxed_Type);
    PyDict_SetItemString(PyGBoxed_Type.tp_dict, "__gtype__",
			 o=pyg_type_wrapper_new(G_TYPE_BOXED));
    Py_DECREF(o);

    PyGMainLoop_Type.ob_type = &PyType_Type;
    PyGMainLoop_Type.tp_alloc = PyType_GenericAlloc;
    PyGMainLoop_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGMainLoop_Type))
	return;
    PyDict_SetItemString(d, "MainLoop", (PyObject *)&PyGMainLoop_Type);
    
    PyGMainContext_Type.ob_type = &PyType_Type;
    PyGMainContext_Type.tp_alloc = PyType_GenericAlloc;
    PyGMainContext_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGMainContext_Type))
	return;
    PyDict_SetItemString(d, "MainContext", (PyObject *)&PyGMainContext_Type);

    PyGPointer_Type.ob_type = &PyType_Type;
    PyGPointer_Type.tp_alloc = PyType_GenericAlloc;
    PyGPointer_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&PyGPointer_Type))
	return;
    PyDict_SetItemString(d, "GPointer", (PyObject *)&PyGPointer_Type);
    PyDict_SetItemString(PyGPointer_Type.tp_dict, "__gtype__",
			 o=pyg_type_wrapper_new(G_TYPE_POINTER));
    Py_DECREF(o);

    tuple = Py_BuildValue ("(iii)", glib_major_version, glib_minor_version,
			   glib_micro_version);
    PyDict_SetItemString(d, "glib_version", tuple);    
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
    PyModule_AddIntConstant(m,"PRIORITY_DEFAULT_IDLE",G_PRIORITY_DEFAULT_IDLE);
    PyModule_AddIntConstant(m, "PRIORITY_LOW", G_PRIORITY_LOW);

    PyModule_AddIntConstant(m, "IO_IN",   G_IO_IN);
    PyModule_AddIntConstant(m, "IO_OUT",  G_IO_OUT);
    PyModule_AddIntConstant(m, "IO_PRI",  G_IO_PRI);
    PyModule_AddIntConstant(m, "IO_ERR",  G_IO_ERR);
    PyModule_AddIntConstant(m, "IO_HUP",  G_IO_HUP);
    PyModule_AddIntConstant(m, "IO_NVAL", G_IO_NVAL);

    PyModule_AddObject(m, "TYPE_INVALID", pyg_type_wrapper_new(G_TYPE_INVALID));
    PyModule_AddObject(m, "TYPE_NONE", pyg_type_wrapper_new(G_TYPE_NONE));
    PyModule_AddObject(m, "TYPE_INTERFACE", pyg_type_wrapper_new(G_TYPE_INTERFACE));
    PyModule_AddObject(m, "TYPE_CHAR", pyg_type_wrapper_new(G_TYPE_CHAR));
    PyModule_AddObject(m, "TYPE_UCHAR", pyg_type_wrapper_new(G_TYPE_UCHAR));
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

    pyg_register_boxed_custom(G_TYPE_STRV,
			      _pyg_strv_from_gvalue,
			      _pyg_strv_to_gvalue);
}

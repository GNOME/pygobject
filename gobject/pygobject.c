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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */
#include "pygobject-private.h"

static const gchar *pygobject_class_id     = "PyGObject::class";
static GQuark       pygobject_class_key    = 0;
static const gchar *pygobject_wrapper_id   = "PyGObject::wrapper";
static GQuark       pygobject_wrapper_key  = 0;

static void pygobject_dealloc(PyGObject *self);
static int  pygobject_traverse(PyGObject *self, visitproc visit, void *arg);
static int  pygobject_clear(PyGObject *self);

/* -------------- class <-> wrapper manipulation --------------- */

typedef struct {
    GType type;
    void (* sinkfunc)(GObject *object);
} SinkFunc;
static GArray *sink_funcs = NULL;

static inline void
sink_object(GObject *obj)
{
    if (sink_funcs) {
	gint i;

	for (i = 0; i < sink_funcs->len; i++) {
	    if (g_type_is_a(G_OBJECT_TYPE(obj),
			    g_array_index(sink_funcs, SinkFunc, i).type)) {
		g_array_index(sink_funcs, SinkFunc, i).sinkfunc(obj);
		break;
	    }
	}
    }
}

/**
 * pygobject_register_sinkfunc:
 * type: the GType the sink function applies to.
 * sinkfunc: a function to remove the floating reference on an object.
 *
 * As Python handles reference counting for us, the "floating
 * reference" code in GTK is not all that useful.  In fact, it can
 * cause leaks.  For this reason, PyGTK removes the floating
 * references on objects on construction.
 *
 * The sinkfunc should be able to remove the floating reference on
 * instances of the given type, or any subclasses.
 */
void
pygobject_register_sinkfunc(GType type, void (* sinkfunc)(GObject *object))
{
    SinkFunc sf;

#if 0
    g_return_if_fail(G_TYPE_IS_OBJECT(type));
#endif
    g_return_if_fail(sinkfunc != NULL);
    
    if (!sink_funcs)
	sink_funcs = g_array_new(FALSE, FALSE, sizeof(SinkFunc));

    sf.type = type;
    sf.sinkfunc = sinkfunc;
    g_array_append_val(sink_funcs, sf);
}

/**
 * pygobject_register_class:
 * @dict: the module dictionary.  A reference to the type will be stored here.
 * @type_name: not used ?
 * @gtype: the GType of the GObject subclass.
 * @type: the Python type object for this wrapper.
 * @bases: a tuple of Python type objects that are the bases of this type.
 *
 * This function is used to register a Python type as the wrapper for
 * a particular GObject subclass.  It will also insert a reference to
 * the wrapper class into the module dictionary passed as a reference,
 * which simplifies initialisation.
 */
void
pygobject_register_class(PyObject *dict, const gchar *type_name,
			 GType gtype, PyTypeObject *type,
			 PyObject *bases)
{
    PyObject *o;
    const char *class_name, *s;

    if (!pygobject_class_key)
	pygobject_class_key = g_quark_from_static_string(pygobject_class_id);

    class_name = type->tp_name;
    s = strrchr(class_name, '.');
    if (s != NULL)
	class_name = s + 1;
	
    type->ob_type = &PyType_Type;
    if (bases) {
	type->tp_bases = bases;
	type->tp_base = (PyTypeObject *)PyTuple_GetItem(bases, 0);
    }

    if (PyType_Ready(type) < 0) {
	g_warning ("couldn't make the type `%s' ready", type->tp_name);
	return;
    }

    if (gtype) {
	o = pyg_type_wrapper_new(gtype);
	PyDict_SetItemString(type->tp_dict, "__gtype__", o);
	Py_DECREF(o);

	/* stash a pointer to the python class with the GType */
	Py_INCREF(type);
	g_type_set_qdata(gtype, pygobject_class_key, type);
    }

    /* set up __doc__ descriptor on type */
    PyDict_SetItemString(type->tp_dict, "__doc__",
			 pyg_object_descr_doc_get());

    PyDict_SetItemString(dict, (char *)class_name, (PyObject *)type);
}

/**
 * pygobject_register_wrapper:
 * @self: the wrapper instance
 *
 * In the constructor of PyGTK wrappers, this function should be
 * called after setting the obj member.  It will tie the wrapper
 * instance to the GObject so that the same wrapper instance will
 * always be used for this GObject instance.  It will also sink any
 * floating references on the GObject.
 */
void
pygobject_register_wrapper(PyObject *self)
{
    GObject *obj = ((PyGObject *)self)->obj;

    if (!pygobject_wrapper_key)
	pygobject_wrapper_key=g_quark_from_static_string(pygobject_wrapper_id);

    sink_object(obj);
    Py_INCREF(self);
    g_object_set_qdata_full(obj, pygobject_wrapper_key, self,
			    pyg_destroy_notify);
}

/**
 * pygobject_new_with_interfaces
 * @gtype: the GType of the GObject subclass.
 *
 * Creates a new PyTypeObject from the given GType with interfaces attached in
 * bases. It will currently not filter out interfaces already implemented by
 * it parents.
 *
 * Returns: a PyTypeObject for the new type of NULL if it couldn't be created
 */
PyTypeObject *
pygobject_new_with_interfaces(GType gtype)
{
    PyGILState_STATE state;
    PyObject *o;
    PyTypeObject *type;
    PyObject *dict;
    PyTypeObject *py_parent_type, *py_interface_type;
    GType *interfaces;
    guint n_interfaces;
    int i;
    PyObject *bases;
    GType parent_type, interface_type;
    PyObject *modules, *module;
    gchar *type_name, *mod_name, *gtype_name;

    interfaces = g_type_interfaces (gtype, &n_interfaces);
    bases = PyTuple_New(n_interfaces+1);
    
    /* Lookup the parent type */
    parent_type = g_type_parent(gtype);
    py_parent_type = pygobject_lookup_class(parent_type);

    /* We will always put the parent at the first position in bases */
    PyTuple_SetItem(bases, 0, (PyObject*)py_parent_type);

    /* And traverse interfaces */
    if (n_interfaces) {
	for (i = 0; i < n_interfaces; i++) {
	    interface_type = interfaces[i];
	    py_interface_type = pygobject_lookup_class(interface_type);
	    PyTuple_SetItem(bases, i+1, (PyObject*)py_interface_type);
	}
	
	g_free(interfaces);
    }
	
    dict = PyDict_New();
    
    o = pyg_type_wrapper_new(gtype);
    PyDict_SetItemString(dict, "__gtype__", o);
    Py_DECREF(o);

    /* set up __doc__ descriptor on type */
    PyDict_SetItemString(dict, "__doc__", pyg_object_descr_doc_get());

    /* generate the pygtk module name and extract the base type name */
    gtype_name = (gchar *)g_type_name(gtype);
    if (g_str_has_prefix(gtype_name, "Gtk")) {
	mod_name = "gtk";
	gtype_name += 3;
	type_name = g_strconcat(mod_name, ".", gtype_name, NULL);
    } else if (g_str_has_prefix(gtype_name, "Gdk")) {
	mod_name = "gtk.gdk";
	gtype_name += 3;
	type_name = g_strconcat(mod_name, ".", gtype_name, NULL);
    } else if (g_str_has_prefix(gtype_name, "Atk")) {
	mod_name = "atk";
	gtype_name += 3;
	type_name = g_strconcat(mod_name, ".", gtype_name, NULL);
    } else if (g_str_has_prefix(gtype_name, "Pango")) {
	mod_name = "pango";
	gtype_name += 5;
	type_name = g_strconcat(mod_name, ".", gtype_name, NULL);
    } else {
	mod_name = "__main__";
	type_name = g_strconcat(mod_name, ".", gtype_name, NULL);
    }

    state = pyg_gil_state_ensure();

    type = (PyTypeObject*)PyObject_CallFunction((PyObject*)&PyType_Type, "sOO",
						type_name, bases, dict);
    g_free(type_name);

    pyg_gil_state_release(state);
    
    if (type == NULL) {
	PyErr_Print();
	return NULL;
    }

      /* Workaround python tp_(get|set)attr slot inheritance bug.
       * Fixes bug #144135. */
    if (!type->tp_getattr && py_parent_type->tp_getattr) {
        type->tp_getattro = NULL;
        type->tp_getattr = py_parent_type->tp_getattr;
    }
    if (!type->tp_setattr && py_parent_type->tp_setattr) {
        type->tp_setattro = NULL;
        type->tp_setattr = py_parent_type->tp_setattr;
    }

    if (PyType_Ready(type) < 0) {
	g_warning ("couldn't make the type `%s' ready", type->tp_name);
	return NULL;
    }
    
    /* insert type name in module dict */
    modules = PyImport_GetModuleDict();
    if ((module = PyDict_GetItemString(modules, mod_name)) != NULL) {
	PyObject *mod_dict = PyModule_GetDict(module);
	if (mod_dict != NULL)
	    PyDict_SetItemString(mod_dict, gtype_name, (PyObject *)type);
    }

    if (!pygobject_class_key)
	pygobject_class_key = g_quark_from_static_string(pygobject_class_id);

    /* stash a pointer to the python class with the GType */
    Py_INCREF(type);
    g_type_set_qdata(gtype, pygobject_class_key, type);

    return type;
}

/**
 * pygobject_lookup_class:
 * @gtype: the GType of the GObject subclass.
 *
 * This function looks up the wrapper class used to represent
 * instances of a GObject represented by @gtype.  If no wrapper class
 * or interface has been registered for the given GType, then a new
 * type will be created.
 *
 * Returns: The wrapper class for the GObject or NULL if the
 *          GType has no registered type and a new type couldn't be created
 */
PyTypeObject *
pygobject_lookup_class(GType gtype)
{
    PyTypeObject *py_type;

    if (gtype == G_TYPE_INTERFACE)
	return &PyGInterface_Type;

    py_type = g_type_get_qdata(gtype, pygobject_class_key);
    if (py_type == NULL) {
	py_type = g_type_get_qdata(gtype, pyginterface_type_key);
	if (py_type == NULL) {
	    py_type = pygobject_new_with_interfaces(gtype);
	    g_type_set_qdata(gtype, pyginterface_type_key, py_type);
	}
    }
    
    return py_type;
}

/**
 * pygobject_new:
 * @obj: a GObject instance.
 *
 * This function gets a reference to a wrapper for the given GObject
 * instance.  If a wrapper has already been created, a new reference
 * to that wrapper will be returned.  Otherwise, a wrapper instance
 * will be created.
 *
 * Returns: a reference to the wrapper for the GObject.
 */
PyObject *
pygobject_new(GObject *obj)
{
    PyGObject *self;

    if (!pygobject_wrapper_key)
	pygobject_wrapper_key = g_quark_from_static_string(pygobject_wrapper_id);

    if (obj == NULL) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    /* we already have a wrapper for this object -- return it. */
    self = (PyGObject *)g_object_get_qdata(obj, pygobject_wrapper_key);
    if (self != NULL) {
	Py_INCREF(self);
    } else {
	/* create wrapper */
	PyTypeObject *tp = pygobject_lookup_class(G_OBJECT_TYPE(obj));
        /* need to bump type refcount if created with
           pygobject_new_with_interfaces(). fixes bug #141042 */
        if (tp->tp_flags & Py_TPFLAGS_HEAPTYPE)
            Py_INCREF(tp);
	self = PyObject_GC_New(PyGObject, tp);
	if (self == NULL)
	    return NULL;
        pyg_begin_allow_threads;
	self->obj = g_object_ref(obj);
	pyg_end_allow_threads;
	sink_object(self->obj);

	self->inst_dict = NULL;
	self->weakreflist = NULL;
	self->closures = NULL;
	/* save wrapper pointer so we can access it later */
	Py_INCREF(self);
	g_object_set_qdata_full(obj, pygobject_wrapper_key, self,
				pyg_destroy_notify);

	PyObject_GC_Track((PyObject *)self);
    }

    return (PyObject *)self;
}

static void
pygobject_unwatch_closure(gpointer data, GClosure *closure)
{
    PyGObject *self = (PyGObject *)data;

    self->closures = g_slist_remove (self->closures, closure);
}

/**
 * pygobject_watch_closure:
 * @self: a GObject wrapper instance
 * @closure: a GClosure to watch
 *
 * Adds a closure to the list of watched closures for the wrapper.
 * The closure must be one returned by pyg_closure_new().  When the
 * cycle GC traverses the wrapper instance, it will enumerate the
 * references to Python objects stored in watched closures.  If the
 * cycle GC tells the wrapper to clear itself, the watched closures
 * will be invalidated.
 */
void
pygobject_watch_closure(PyObject *self, GClosure *closure)
{
    PyGObject *gself;

    g_return_if_fail(self != NULL);
    g_return_if_fail(PyObject_TypeCheck(self, &PyGObject_Type));
    g_return_if_fail(closure != NULL);
    g_return_if_fail(g_slist_find(((PyGObject *)self)->closures, closure) == NULL);

    gself = (PyGObject *)self;
    gself->closures = g_slist_prepend(gself->closures, closure);
    g_closure_add_invalidate_notifier(closure,self, pygobject_unwatch_closure);
}

/* -------------- PyGObject behaviour ----------------- */

static void
pygobject_dealloc(PyGObject *self)
{
    GSList *tmp;

    PyObject_ClearWeakRefs((PyObject *)self);

    PyObject_GC_UnTrack((PyObject *)self);

    if (self->obj) {
	pyg_begin_allow_threads;
	g_object_unref(self->obj);
	pyg_end_allow_threads;
    }
    self->obj = NULL;

    if (self->inst_dict) {
	Py_DECREF(self->inst_dict);
    }
    self->inst_dict = NULL;

    pyg_begin_allow_threads;
    tmp = self->closures;
    while (tmp) {
	GClosure *closure = tmp->data;

	/* we get next item first, because the current link gets
	 * invalidated by pygobject_unwatch_closure */
	tmp = tmp->next;
	g_closure_invalidate(closure);
    }
    self->closures = NULL;
    pyg_end_allow_threads;

    /* the following causes problems with subclassed types */
    /* self->ob_type->tp_free((PyObject *)self); */
    PyObject_GC_Del(self);
}

static int
pygobject_compare(PyGObject *self, PyGObject *v)
{
    if (self->obj == v->obj) return 0;
    if (self->obj > v->obj)  return -1;
    return 1;
}

static long
pygobject_hash(PyGObject *self)
{
    return (long)self->obj;
}

static PyObject *
pygobject_repr(PyGObject *self)
{
    gchar buf[256];

    g_snprintf(buf, sizeof(buf),
	       "<%s object (%s) at 0x%lx>",
	       self->ob_type->tp_name,
	       self->obj ? G_OBJECT_TYPE_NAME(self->obj) : "uninitialized",
	       (long)self);
    return PyString_FromString(buf);
}

static int
pygobject_traverse(PyGObject *self, visitproc visit, void *arg)
{
    int ret = 0;
    GSList *tmp;

    if (self->inst_dict) ret = visit(self->inst_dict, arg);
    if (ret != 0) return ret;

    for (tmp = self->closures; tmp != NULL; tmp = tmp->next) {
	PyGClosure *closure = tmp->data;

	if (closure->callback) ret = visit(closure->callback, arg);
	if (ret != 0) return ret;

	if (closure->extra_args) ret = visit(closure->extra_args, arg);
	if (ret != 0) return ret;

	if (closure->swap_data) ret = visit(closure->swap_data, arg);
	if (ret != 0) return ret;
    }

    if (self->obj && self->obj->ref_count == 1)
	ret = visit((PyObject *)self, arg);
    if (ret != 0) return ret;

    return 0;
}

static int
pygobject_clear(PyGObject *self)
{
    GSList *tmp;

    if (self->inst_dict) {
	Py_DECREF(self->inst_dict);
    }
    self->inst_dict = NULL;

    pyg_begin_allow_threads;
    tmp = self->closures;
    while (tmp) {
	GClosure *closure = tmp->data;

	/* we get next item first, because the current link gets
	 * invalidated by pygobject_unwatch_closure */
	tmp = tmp->next;
	g_closure_invalidate(closure);
    }
    pyg_end_allow_threads;

    if (self->closures != NULL)
	g_message("invalidated all closures, but self->closures != NULL !");

    if (self->obj) {
	pyg_begin_allow_threads;
	g_object_unref(self->obj);
	pyg_end_allow_threads;
    }
    self->obj = NULL;

    return 0;
}

static void
pygobject_free(PyObject *op)
{
    PyObject_GC_Del(op);
}


/* ---------------- PyGObject methods ----------------- */

static int
pygobject_init(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    GType object_type;
    guint n_params = 0, i;
    GParameter *params = NULL;
    GObjectClass *class;

    if (!PyArg_ParseTuple(args, ":GObject.__init__", &object_type))
	return -1;

    object_type = pyg_type_from_object((PyObject *)self);
    if (!object_type)
	return -1;

    if (G_TYPE_IS_ABSTRACT(object_type)) {
	PyErr_Format(PyExc_TypeError, "cannot create instance of abstract "
		     "(non-instantiable) type `%s'", g_type_name(object_type));
	return -1;
    }

    if ((class = g_type_class_ref (object_type)) == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"could not get a reference to type class");
	return -1;
    }

    if (kwargs) {
	int pos = 0;
	PyObject *key;
	PyObject *value;

	params = g_new0(GParameter, PyDict_Size(kwargs));
	while (PyDict_Next (kwargs, &pos, &key, &value)) {
	    GParamSpec *pspec;
	    gchar *key_str = PyString_AsString(key);

	    pspec = g_object_class_find_property (class, key_str);
	    if (!pspec) {
		PyErr_Format(PyExc_TypeError,
			     "gobject `%s' doesn't support property `%s'",
			     g_type_name(object_type), key_str);
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

    self->obj = g_object_newv(object_type, n_params, params);
    if (self->obj)
	pygobject_register_wrapper((PyObject *)self);
    else
	PyErr_SetString (PyExc_RuntimeError, "could not create object");
	   
 cleanup:
    for (i = 0; i < n_params; i++) {
	g_free((gchar *) params[i].name);
	g_value_unset(&params[i].value);
    }
    g_free(params);
    g_type_class_unref(class);
    
    return (self->obj) ? 0 : -1;
}

static PyObject *
pygobject__gobject_init__(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    if (pygobject_init(self, args, kwargs) < 0)
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_get_property(PyGObject *self, PyObject *args)
{
    gchar *param_name;
    GParamSpec *pspec;
    GValue value = { 0, };
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "s:GObject.get_property", &param_name))
	return NULL;
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(self->obj),
					 param_name);
    if (!pspec) {
	PyErr_SetString(PyExc_TypeError,
			"the object does not support the given parameter");
	return NULL;
    }
    if (!(pspec->flags & G_PARAM_READABLE)) {
	PyErr_Format(PyExc_TypeError, "property %s is not readable",
		     param_name);
	return NULL;
    }
    g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
    g_object_get_property(self->obj, param_name, &value);
    ret = pyg_param_gvalue_as_pyobject(&value, TRUE, pspec);
    g_value_unset(&value);
    return ret;
}

static PyObject *
pygobject_set_property(PyGObject *self, PyObject *args)
{
    gchar *param_name;
    GParamSpec *pspec;
    GValue value = { 0, };
    PyObject *pvalue;

    if (!PyArg_ParseTuple(args, "sO:GObject.set_property", &param_name,
			  &pvalue))
	return NULL;
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(self->obj),
					 param_name);
    if (!pspec) {
	PyErr_SetString(PyExc_TypeError,
			"the object does not support the given parameter");
	return NULL;
    }
    if (!(pspec->flags & G_PARAM_WRITABLE)) {
	PyErr_Format(PyExc_TypeError, "property %s is not writable",
		     param_name);
	return NULL;
    }
    g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
    if (pyg_param_gvalue_from_pyobject(&value, pvalue, pspec) < 0) {
	PyErr_SetString(PyExc_TypeError,
			"could not convert argument to correct param type");
	return NULL;
    }
    g_object_set_property(self->obj, param_name, &value);
    g_value_unset(&value);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_freeze_notify(PyGObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":GObject.freeze_notify"))
	return NULL;
    g_object_freeze_notify(self->obj);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_notify(PyGObject *self, PyObject *args)
{
    char *property_name;

    if (!PyArg_ParseTuple(args, "s:GObject.notify", &property_name))
	return NULL;
    g_object_notify(self->obj, property_name);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_thaw_notify(PyGObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":GObject.thaw_notify"))
	return NULL;
    g_object_thaw_notify(self->obj);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_get_data(PyGObject *self, PyObject *args)
{
    char *key;
    GQuark quark;
    PyObject *data;

    if (!PyArg_ParseTuple(args, "s:GObject.get_data", &key))
	return NULL;
    quark = g_quark_from_string(key);
    data = g_object_get_qdata(self->obj, quark);
    if (!data) data = Py_None;
    Py_INCREF(data);
    return data;
}

static PyObject *
pygobject_set_data(PyGObject *self, PyObject *args)
{
    char *key;
    GQuark quark;
    PyObject *data;

    if (!PyArg_ParseTuple(args, "sO:GObject.get_data", &key, &data))
	return NULL;
    quark = g_quark_from_string(key);
    Py_INCREF(data);
    g_object_set_qdata_full(self->obj, quark, data, pyg_destroy_notify);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_connect(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args;
    gchar *name;
    guint handlerid, sigid, len;
    GQuark detail = 0;
    GClosure *closure;

    len = PyTuple_Size(args);
    if (len < 2) {
	PyErr_SetString(PyExc_TypeError,
			"GObject.connect requires at least 2 arguments");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 2);
    if (!PyArg_ParseTuple(first, "sO:GObject.connect", &name, &callback)) {
	Py_DECREF(first);
	return NULL;
    }
    Py_DECREF(first);
    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "second argument must be callable");
	return NULL;
    }
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(self->obj),
			     &sigid, &detail, TRUE)) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    extra_args = PySequence_GetSlice(args, 2, len);
    if (extra_args == NULL)
	return NULL;
    closure = pyg_closure_new(callback, extra_args, NULL);
    pygobject_watch_closure((PyObject *)self, closure);
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, detail,
					       closure, FALSE);
    Py_DECREF(extra_args);
    return PyInt_FromLong(handlerid);
}

static PyObject *
pygobject_connect_after(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args;
    gchar *name;
    guint handlerid, sigid, len;
    GQuark detail;
    GClosure *closure;

    len = PyTuple_Size(args);
    if (len < 2) {
	PyErr_SetString(PyExc_TypeError,
			"GObject.connect_after requires at least 2 arguments");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 2);
    if (!PyArg_ParseTuple(first, "sO:GObject.connect_after",
			  &name, &callback)) {
	Py_DECREF(first);
	return NULL;
    }
    Py_DECREF(first);
    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "second argument must be callable");
	return NULL;
    }
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(self->obj),
			     &sigid, &detail, TRUE)) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    extra_args = PySequence_GetSlice(args, 2, len);
    if (extra_args == NULL)
	return NULL;
    closure = pyg_closure_new(callback, extra_args, NULL);
    pygobject_watch_closure((PyObject *)self, closure);
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, detail,
					       closure, TRUE);
    Py_DECREF(extra_args);
    return PyInt_FromLong(handlerid);
}

static PyObject *
pygobject_connect_object(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *object;
    gchar *name;
    guint handlerid, sigid, len;
    GQuark detail;
    GClosure *closure;

    len = PyTuple_Size(args);
    if (len < 3) {
	PyErr_SetString(PyExc_TypeError,
		"GObject.connect_object requires at least 3 arguments");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 3);
    if (!PyArg_ParseTuple(first, "sOO:GObject.connect_object",
			  &name, &callback, &object)) {
	Py_DECREF(first);
	return NULL;
    }
    Py_DECREF(first);
    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "second argument must be callable");
	return NULL;
    }
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(self->obj),
			     &sigid, &detail, TRUE)) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    extra_args = PySequence_GetSlice(args, 3, len);
    if (extra_args == NULL)
	return NULL;
    closure = pyg_closure_new(callback, extra_args, object);
    pygobject_watch_closure((PyObject *)self, closure);
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, detail,
					       closure, FALSE);
    Py_DECREF(extra_args);
    return PyInt_FromLong(handlerid);
}

static PyObject *
pygobject_connect_object_after(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *object;
    gchar *name;
    guint handlerid, sigid, len;
    GQuark detail;
    GClosure *closure;

    len = PyTuple_Size(args);
    if (len < 3) {
	PyErr_SetString(PyExc_TypeError,
		"GObject.connect_object_after requires at least 3 arguments");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 3);
    if (!PyArg_ParseTuple(first, "sOO:GObject.connect_object_after",
			  &name, &callback, &object)) {
	Py_DECREF(first);
	return NULL;
    }
    Py_DECREF(first);
    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "second argument must be callable");
	return NULL;
    }
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(self->obj),
			     &sigid, &detail, TRUE)) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    extra_args = PySequence_GetSlice(args, 3, len);
    if (extra_args == NULL)
	return NULL;
    closure = pyg_closure_new(callback, extra_args, object);
    pygobject_watch_closure((PyObject *)self, closure);
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, detail,
					       closure, TRUE);
    Py_DECREF(extra_args);
    return PyInt_FromLong(handlerid);
}

static PyObject *
pygobject_disconnect(PyGObject *self, PyObject *args)
{
    guint handler_id;

    if (!PyArg_ParseTuple(args, "i:GObject.disconnect", &handler_id))
	return NULL;
    g_signal_handler_disconnect(self->obj, handler_id);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_handler_is_connected(PyGObject *self, PyObject *args)
{
    guint handler_id;

    if (!PyArg_ParseTuple(args, "i:GObject.handler_is_connected", &handler_id))
	return NULL;

    return PyBool_FromLong(g_signal_handler_is_connected(self->obj, handler_id));
}

static PyObject *
pygobject_handler_block(PyGObject *self, PyObject *args)
{
    guint handler_id;

    if (!PyArg_ParseTuple(args, "i:GObject.handler_block", &handler_id))
	return NULL;
    g_signal_handler_block(self->obj, handler_id);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_handler_unblock(PyGObject *self, PyObject *args)
{
    guint handler_id;

    if (!PyArg_ParseTuple(args, "i:GObject.handler_unblock", &handler_id))
	return NULL;
    g_signal_handler_unblock(self->obj, handler_id);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_emit(PyGObject *self, PyObject *args)
{
    guint signal_id, i, len;
    GQuark detail;
    PyObject *first, *py_ret;
    gchar *name;
    GSignalQuery query;
    GValue *params, ret = { 0, };
    
    len = PyTuple_Size(args);
    if (len < 1) {
	PyErr_SetString(PyExc_TypeError,"GObject.emit needs at least one arg");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 1);
    if (!PyArg_ParseTuple(first, "s:GObject.emit", &name)) {
	Py_DECREF(first);
	return NULL;
    }
    Py_DECREF(first);
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(self->obj),
			     &signal_id, &detail, TRUE)) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    g_signal_query(signal_id, &query);
    if (len != query.n_params + 1) {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf),
		   "%d parameters needed for signal %s; %d given",
		   query.n_params, name, len - 1);
	PyErr_SetString(PyExc_TypeError, buf);
	return NULL;
    }

    params = g_new0(GValue, query.n_params + 1);
    g_value_init(&params[0], G_OBJECT_TYPE(self->obj));
    g_value_set_object(&params[0], G_OBJECT(self->obj));

    for (i = 0; i < query.n_params; i++)
	g_value_init(&params[i + 1],
		     query.param_types[i] & ~G_SIGNAL_TYPE_STATIC_SCOPE);
    for (i = 0; i < query.n_params; i++) {
	PyObject *item = PyTuple_GetItem(args, i+1);

	if (pyg_value_from_pyobject(&params[i+1], item) < 0) {
	    gchar buf[128];
	    g_snprintf(buf, sizeof(buf),
		"could not convert type %s to %s required for parameter %d",
		item->ob_type->tp_name,
		g_type_name(G_VALUE_TYPE(&params[i+1])), i);
	    PyErr_SetString(PyExc_TypeError, buf);

	    for (i = 0; i < query.n_params + 1; i++)
		g_value_unset(&params[i]);

	    g_free(params);
	    return NULL;
	}
    }
    if (query.return_type != G_TYPE_NONE)
	g_value_init(&ret, query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE);
    
    g_signal_emitv(params, signal_id, detail, &ret);

    for (i = 0; i < query.n_params + 1; i++)
	g_value_unset(&params[i]);
    
    g_free(params);
    if ((query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE) != G_TYPE_NONE) {
	py_ret = pyg_value_as_pyobject(&ret, TRUE);
	g_value_unset(&ret);
    } else {
	Py_INCREF(Py_None);
	py_ret = Py_None;
    }

    return py_ret;
}

static PyObject *
pygobject_stop_emission(PyGObject *self, PyObject *args)
{
    gchar *signal;
    guint signal_id;
    GQuark detail;

    if (!PyArg_ParseTuple(args, "s:GObject.stop_emission", &signal))
	return NULL;
    if (!g_signal_parse_name(signal, G_OBJECT_TYPE(self->obj),
			     &signal_id, &detail, TRUE)) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    g_signal_stop_emission(self->obj, signal_id, detail);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_chain_from_overridden(PyGObject *self, PyObject *args)
{
    GSignalInvocationHint *ihint;
    guint signal_id, i, len;
    PyObject *py_ret;
    const gchar *name;
    GSignalQuery query;
    GValue *params, ret = { 0, };

    ihint = g_signal_get_invocation_hint(self->obj);
    if (!ihint) {
	PyErr_SetString(PyExc_TypeError, "could not find signal invocation "
			"information for this object.");
	return NULL;
    }

    signal_id = ihint->signal_id;
    name = g_signal_name(signal_id);

    len = PyTuple_Size(args);
    if (signal_id == 0) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    g_signal_query(signal_id, &query);
    if (len != query.n_params) {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf),
		   "%d parameters needed for signal %s; %d given",
		   query.n_params, name, len);
	PyErr_SetString(PyExc_TypeError, buf);
	return NULL;
    }
    params = g_new0(GValue, query.n_params + 1);
    g_value_init(&params[0], G_OBJECT_TYPE(self->obj));
    g_value_set_object(&params[0], G_OBJECT(self->obj));

    for (i = 0; i < query.n_params; i++)
	g_value_init(&params[i + 1],
		     query.param_types[i] & ~G_SIGNAL_TYPE_STATIC_SCOPE);
    for (i = 0; i < query.n_params; i++) {
	PyObject *item = PyTuple_GetItem(args, i);

	if (pyg_boxed_check(item, (query.param_types[i] & ~G_SIGNAL_TYPE_STATIC_SCOPE))) {
	    g_value_set_static_boxed(&params[i+1], pyg_boxed_get(item, void));
	}
	else if (pyg_value_from_pyobject(&params[i+1], item) < 0) {
	    gchar buf[128];

	    g_snprintf(buf, sizeof(buf),
		"could not convert type %s to %s required for parameter %d",
		item->ob_type->tp_name,
		g_type_name(G_VALUE_TYPE(&params[i+1])), i);
	    PyErr_SetString(PyExc_TypeError, buf);
	    for (i = 0; i < query.n_params + 1; i++)
		g_value_unset(&params[i]);
	    g_free(params);
	    return NULL;
	}
    }
    if (query.return_type != G_TYPE_NONE)
	g_value_init(&ret, query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE);
    g_signal_chain_from_overridden(params, &ret);
    for (i = 0; i < query.n_params + 1; i++)
	g_value_unset(&params[i]);
    g_free(params);
    if (query.return_type != G_TYPE_NONE) {
	py_ret = pyg_value_as_pyobject(&ret, TRUE);
	g_value_unset(&ret);
    } else {
	Py_INCREF(Py_None);
	py_ret = Py_None;
    }
    return py_ret;
}

static PyMethodDef pygobject_methods[] = {
    { "__gobject_init__", (PyCFunction)pygobject__gobject_init__,
      METH_VARARGS|METH_KEYWORDS },
    { "get_property", (PyCFunction)pygobject_get_property, METH_VARARGS },
    { "set_property", (PyCFunction)pygobject_set_property, METH_VARARGS },
    { "freeze_notify", (PyCFunction)pygobject_freeze_notify, METH_VARARGS },
    { "notify", (PyCFunction)pygobject_notify, METH_VARARGS },
    { "thaw_notify", (PyCFunction)pygobject_thaw_notify, METH_VARARGS },
    { "get_data", (PyCFunction)pygobject_get_data, METH_VARARGS },
    { "set_data", (PyCFunction)pygobject_set_data, METH_VARARGS },
    { "connect", (PyCFunction)pygobject_connect, METH_VARARGS },
    { "connect_after", (PyCFunction)pygobject_connect_after, METH_VARARGS },
    { "connect_object", (PyCFunction)pygobject_connect_object, METH_VARARGS },
    { "connect_object_after", (PyCFunction)pygobject_connect_object_after, METH_VARARGS },
    { "disconnect", (PyCFunction)pygobject_disconnect, METH_VARARGS },
    { "handler_disconnect", (PyCFunction)pygobject_disconnect, METH_VARARGS },
    { "handler_is_connected", (PyCFunction)pygobject_handler_is_connected, METH_VARARGS },
    { "handler_block", (PyCFunction)pygobject_handler_block, METH_VARARGS },
    { "handler_unblock", (PyCFunction)pygobject_handler_unblock,METH_VARARGS },
    { "emit", (PyCFunction)pygobject_emit, METH_VARARGS },
    { "stop_emission", (PyCFunction)pygobject_stop_emission, METH_VARARGS },
    { "emit_stop_by_name", (PyCFunction)pygobject_stop_emission,METH_VARARGS },
    { "chain", (PyCFunction)pygobject_chain_from_overridden,METH_VARARGS },
    { NULL, NULL, 0 }
};

static PyObject *
pygobject_get_dict(PyGObject *self, void *closure)
{
    if (self->inst_dict == NULL) {
	self->inst_dict = PyDict_New();
	if (self->inst_dict == NULL)
	    return NULL;
    }
    Py_INCREF(self->inst_dict);
    return self->inst_dict;
}

static PyObject *
pygobject_get_refcount(PyGObject *self, void *closure)
{
    return PyInt_FromLong(self->obj->ref_count);
}

static PyGetSetDef pygobject_getsets[] = {
    { "__dict__", (getter)pygobject_get_dict, (setter)0 },
    { "__grefcount__", (getter)pygobject_get_refcount, (setter)0, },
    { NULL, 0, 0 }
};

PyTypeObject PyGObject_Type = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "gobject.GObject",			/* tp_name */
    sizeof(PyGObject),			/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)pygobject_dealloc,	/* tp_dealloc */
    (printfunc)0,			/* tp_print */
    (getattrfunc)0,			/* tp_getattr */
    (setattrfunc)0,			/* tp_setattr */
    (cmpfunc)pygobject_compare,		/* tp_compare */
    (reprfunc)pygobject_repr,		/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    (hashfunc)pygobject_hash,		/* tp_hash */
    (ternaryfunc)0,			/* tp_call */
    (reprfunc)0,			/* tp_str */
    (getattrofunc)0,			/* tp_getattro */
    (setattrofunc)0,			/* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
    	Py_TPFLAGS_HAVE_GC,		/* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)pygobject_traverse,	/* tp_traverse */
    (inquiry)pygobject_clear,		/* tp_clear */
    (richcmpfunc)0,			/* tp_richcompare */
    offsetof(PyGObject, weakreflist),	/* tp_weaklistoffset */
    (getiterfunc)0,			/* tp_iter */
    (iternextfunc)0,			/* tp_iternext */
    pygobject_methods,			/* tp_methods */
    0,					/* tp_members */
    pygobject_getsets,			/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    offsetof(PyGObject, inst_dict),	/* tp_dictoffset */
    (initproc)pygobject_init,		/* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
    (freefunc)pygobject_free,		/* tp_free */
    (inquiry)0,				/* tp_is_gc */
    (PyObject *)0,			/* tp_bases */
};


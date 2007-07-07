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

static void pygobject_dealloc(PyGObject *self);
static int  pygobject_traverse(PyGObject *self, visitproc visit, void *arg);
static int  pygobject_clear(PyGObject *self);
static PyObject * pyg_type_get_bases(GType gtype);
static inline int pygobject_clear(PyGObject *self);
static PyObject * pygobject_weak_ref_new(GObject *obj, PyObject *callback, PyObject *user_data);
static inline PyGObjectData * pyg_object_peek_inst_data(GObject *obj);
static PyObject * pygobject_weak_ref_new(GObject *obj, PyObject *callback, PyObject *user_data);

/* -------------- class <-> wrapper manipulation --------------- */

void
pygobject_data_free(PyGObjectData *data)
{
    PyGILState_STATE state = pyg_gil_state_ensure();
    GSList *closures, *tmp;
    Py_DECREF(data->type);
    tmp = closures = data->closures;
#ifndef NDEBUG
    data->closures = NULL;
    data->type = NULL;
#endif
    pyg_begin_allow_threads;
    while (tmp) {
 	GClosure *closure = tmp->data;
 
          /* we get next item first, because the current link gets
           * invalidated by pygobject_unwatch_closure */
 	tmp = tmp->next;
 	g_closure_invalidate(closure);
    }
    pyg_end_allow_threads;
 
    if (data->closures != NULL)
 	g_warning("invalidated all closures, but data->closures != NULL !");

    g_free(data);
    pyg_gil_state_release(state);
}

static inline PyGObjectData *
pygobject_data_new(void)
{
    PyGObjectData *data;
    data = g_new0(PyGObjectData, 1);
    return data;
}

static inline PyGObjectData *
pygobject_get_inst_data(PyGObject *self)
{
    PyGObjectData *inst_data;

    if (G_UNLIKELY(!self->obj))
        return NULL;
    inst_data = g_object_get_qdata(self->obj, pygobject_instance_data_key);
    if (inst_data == NULL)
    {
        inst_data = pygobject_data_new();

        inst_data->type = ((PyObject *)self)->ob_type;
        Py_INCREF((PyObject *) inst_data->type);

        g_object_set_qdata_full(self->obj, pygobject_instance_data_key,
                                inst_data, (GDestroyNotify) pygobject_data_free);
    }
    return inst_data;
}


typedef struct {
    GType type;
    void (* sinkfunc)(GObject *object);
} SinkFunc;
static GArray *sink_funcs = NULL;

GHashTable *custom_type_registration = NULL;

PyTypeObject *PyGObject_MetaType = NULL;

/**
 * pygobject_sink:
 * @obj: a GObject
 * 
 * As Python handles reference counting for us, the "floating
 * reference" code in GTK is not all that useful.  In fact, it can
 * cause leaks.  This function should be called to remove the floating
 * references on objects on construction.
 **/
void
pygobject_sink(GObject *obj)
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

typedef struct {
    PyObject_HEAD
    GParamSpec **props;
    guint n_props;
    guint index;
} PyGPropsIter;

static void
pyg_props_iter_dealloc(PyGPropsIter *self)
{
    g_free(self->props);
    PyObject_Del((PyObject*) self);
}

static PyObject*
pygobject_props_iter_next(PyGPropsIter *iter)
{
    if (iter->index < iter->n_props)
        return pyg_param_spec_new(iter->props[iter->index++]);
    else {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
}


PyTypeObject PyGPropsIter_Type = {
	PyObject_HEAD_INIT(NULL)
	0,					/* ob_size */
	"gobject.GPropsIter",			/* tp_name */
	sizeof(PyGPropsIter),			/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)pyg_props_iter_dealloc,	/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,		       			/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
 	"GObject properties iterator",		/* tp_doc */
	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	(iternextfunc)pygobject_props_iter_next, /* tp_iternext */
};

typedef struct {
    PyObject_HEAD
    /* a reference to the object containing the properties */
    PyGObject *pygobject;
    GType      gtype;
} PyGProps;

static void
PyGProps_dealloc(PyGProps* self)
{
    PyGObject *tmp;

    PyObject_GC_UnTrack((PyObject*)self);

    tmp = self->pygobject;
    self->pygobject = NULL;
    Py_XDECREF(tmp);

    PyObject_GC_Del((PyObject*)self);
}

static PyObject*
build_parameter_list(GObjectClass *class)
{
    GParamSpec **props;
    guint n_props = 0, i;
    PyObject *prop_str;
    char *name;
    PyObject *props_list;

    props = g_object_class_list_properties(class, &n_props);
    props_list = PyList_New(n_props);
    for (i = 0; i < n_props; i++) {
	name = g_strdup(g_param_spec_get_name(props[i]));
	/* hyphens cannot belong in identifiers */
	g_strdelimit(name, "-", '_');
	prop_str = PyString_FromString(name);
	
	PyList_SetItem(props_list, i, prop_str);
    }

    if (props)
        g_free(props);
    
    return props_list;
}

static PyObject*
PyGProps_getattro(PyGProps *self, PyObject *attr)
{
    char *attr_name;
    GObjectClass *class;
    GParamSpec *pspec;
    GValue value = { 0, };
    PyObject *ret;

    attr_name = PyString_AsString(attr);
    if (!attr_name) {
        PyErr_Clear();
        return PyObject_GenericGetAttr((PyObject *)self, attr);
    }

    class = g_type_class_ref(self->gtype);
    
    if (!strcmp(attr_name, "__members__")) {
	return build_parameter_list(class);
    }

    pspec = g_object_class_find_property(class, attr_name);
    g_type_class_unref(class);

    if (!pspec) {
	return PyObject_GenericGetAttr((PyObject *)self, attr);
    }

    if (!(pspec->flags & G_PARAM_READABLE)) {
	PyErr_Format(PyExc_TypeError,
		     "property '%s' is not readable", attr_name);
	return NULL;
    }

    /* If we're doing it without an instance, return a GParamSpec */
    if (!self->pygobject) {
        return pyg_param_spec_new(pspec);
    }
    
    g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
    g_object_get_property(self->pygobject->obj, attr_name, &value);
    ret = pyg_param_gvalue_as_pyobject(&value, TRUE, pspec);
    g_value_unset(&value);
    
    return ret;
}

static gboolean
set_property_from_pspec(GObject *obj,
			char *attr_name,
			GParamSpec *pspec,
			PyObject *pvalue)
{
    GValue value = { 0, };

    if (pspec->flags & G_PARAM_CONSTRUCT_ONLY) {
	PyErr_Format(PyExc_TypeError,
		     "property '%s' can only be set in constructor",
		     attr_name);
	return FALSE;
    }	

    if (!(pspec->flags & G_PARAM_WRITABLE)) {
	PyErr_Format(PyExc_TypeError,
		     "property '%s' is not writable", attr_name);
	return FALSE;
    }	

    g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
    if (pyg_param_gvalue_from_pyobject(&value, pvalue, pspec) < 0) {
	PyErr_SetString(PyExc_TypeError,
			"could not convert argument to correct param type");
	return FALSE;
    }

    pyg_begin_allow_threads;
    g_object_set_property(obj, attr_name, &value);
    pyg_end_allow_threads;

    g_value_unset(&value);
    
    return TRUE;
}

static int
PyGProps_setattro(PyGProps *self, PyObject *attr, PyObject *pvalue)
{
    GParamSpec *pspec;
    char *attr_name;
    GObject *obj;
    
    if (pvalue == NULL) {
	PyErr_SetString(PyExc_TypeError, "properties cannot be "
			"deleted");
	return -1;
    }

    attr_name = PyString_AsString(attr);
    if (!attr_name) {
        PyErr_Clear();
        return PyObject_GenericSetAttr((PyObject *)self, attr, pvalue);
    }

    if (!self->pygobject) {
        PyErr_SetString(PyExc_TypeError,
			"cannot set GOject properties without an instance");
        return -1;
    }

    obj = self->pygobject->obj;
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(obj), attr_name);
    if (!pspec) {
	return PyObject_GenericSetAttr((PyObject *)self, attr, pvalue);
    }

    if (!set_property_from_pspec(obj, attr_name, pspec, pvalue))
	return -1;
				  
    return 0;
}

static int
pygobject_props_traverse(PyGProps *self, visitproc visit, void *arg)
{
    if (self->pygobject && visit((PyObject *) self->pygobject, arg) < 0)
        return -1;
    return 0;
}

static PyObject*
pygobject_props_get_iter(PyGProps *self)
{
    PyGPropsIter *iter;
    GObjectClass *class;

    iter = PyObject_NEW(PyGPropsIter, &PyGPropsIter_Type);
    class = g_type_class_ref(self->gtype);
    iter->props = g_object_class_list_properties(class, &iter->n_props);
    iter->index = 0;
    g_type_class_unref(class);
    return (PyObject *) iter;
}

static Py_ssize_t
PyGProps_length(PyGProps *self)
{
    GObjectClass *class;
    guint n_props;
    
    class = g_type_class_ref(self->gtype);
    g_object_class_list_properties(class, &n_props);
    g_type_class_unref(class);
    
    return (Py_ssize_t)n_props;
}

static PySequenceMethods _PyGProps_as_sequence = {
    (lenfunc) PyGProps_length,
    0,
    0,
    0,
    0,
    0,
    0
};

PyTypeObject PyGProps_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                          /* ob_size */
    "gobject.GProps",                           /* tp_name */
    sizeof(PyGProps),                           /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)PyGProps_dealloc,               /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_compare */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    (PySequenceMethods*)&_PyGProps_as_sequence, /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    (getattrofunc)PyGProps_getattro,            /* tp_getattro */
    (setattrofunc)PyGProps_setattro,            /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,      /* tp_flags */
    "The properties of the GObject accessible as "
    "Python attributes.",                       /* tp_doc */
    (traverseproc)pygobject_props_traverse,     /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    (getiterfunc)pygobject_props_get_iter,      /* tp_iter */
    0,                                          /* tp_iternext */
};


static PyObject *
pyg_props_descr_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    PyGProps *gprops;

    gprops = PyObject_GC_New(PyGProps, &PyGProps_Type);
    if (obj == NULL || obj == Py_None) {
        gprops->pygobject = NULL;
        gprops->gtype = pyg_type_from_object(type);
    } else {
        if (!PyObject_IsInstance(obj, (PyObject *) &PyGObject_Type)) {
            PyErr_SetString(PyExc_TypeError, "cannot use GObject property"
                            " descriptor on non-GObject instances");
            return NULL;
        }
        Py_INCREF(obj);
        gprops->pygobject = (PyGObject *) obj;
        gprops->gtype = pyg_type_from_object(obj);
    }
    return (PyObject *) gprops;
}

PyTypeObject PyGPropsDescr_Type = {
	PyObject_HEAD_INIT(NULL)
	0,					/* ob_size */
	"gobject.GPropsDescr",			/* tp_name */
	0,					/* tp_basicsize */
	0,					/* tp_itemsize */
	0,	 				/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,		       			/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
 	0,					/* tp_doc */
	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	pyg_props_descr_descr_get,		/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	0,					/* tp_new */
	0,               			/* tp_free */
};


/**
 * pygobject_register_class:
 * @dict: the module dictionary.  A reference to the type will be stored here.
 * @type_name: not used ?
 * @gtype: the GType of the GObject subclass.
 * @type: the Python type object for this wrapper.
 * @static_bases: a tuple of Python type objects that are the bases of
 * this type
 *
 * This function is used to register a Python type as the wrapper for
 * a particular GObject subclass.  It will also insert a reference to
 * the wrapper class into the module dictionary passed as a reference,
 * which simplifies initialisation.
 */
void
pygobject_register_class(PyObject *dict, const gchar *type_name,
			 GType gtype, PyTypeObject *type,
			 PyObject *static_bases)
{
    PyObject *o;
    const char *class_name, *s;
    PyObject *runtime_bases;
    PyObject *bases_list, *bases, *mod_name;
    int i;
    
    class_name = type->tp_name;
    s = strrchr(class_name, '.');
    if (s != NULL)
	class_name = s + 1;

    runtime_bases = pyg_type_get_bases(gtype);
    if (static_bases) {
        PyTypeObject *py_parent_type = (PyTypeObject *) PyTuple_GET_ITEM(static_bases, 0);
        bases_list = PySequence_List(static_bases);
          /* we start at index 1 because we want to skip the primary
           * base, otherwise we might get MRO conflict */
        for (i = 1; i < PyTuple_GET_SIZE(runtime_bases); ++i)
        {
            PyObject *base = PyTuple_GET_ITEM(runtime_bases, i);
            int contains = PySequence_Contains(bases_list, base);
            if (contains < 0)
                PyErr_Print();
            else if (!contains) {
                if (!PySequence_Contains(py_parent_type->tp_mro, base)) {
#if 0
                    g_message("Adding missing base %s to type %s",
                              ((PyTypeObject *)base)->tp_name, type->tp_name);
#endif
                    PyList_Append(bases_list, base);
                }
            }
        }
        bases = PySequence_Tuple(bases_list);
        Py_DECREF(bases_list);
        Py_DECREF(runtime_bases);
    } else
        bases = runtime_bases;

    type->ob_type = PyGObject_MetaType;
    type->tp_bases = bases;
    if (G_LIKELY(bases)) {
        type->tp_base = (PyTypeObject *)PyTuple_GetItem(bases, 0);
        Py_INCREF(type->tp_base);
    }

    if (PyType_Ready(type) < 0) {
	g_warning ("couldn't make the type `%s' ready", type->tp_name);
	return;
    }

    /* Set type.__module__ to the name of the module,
     * otherwise it'll default to 'gobject', see #376099
     */
    s = strrchr(type->tp_name, '.');
    if (s != NULL) {
	mod_name = PyString_FromStringAndSize(type->tp_name, (int)(s - type->tp_name));
	PyDict_SetItemString(type->tp_dict, "__module__", mod_name);
	Py_DECREF(mod_name);
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

static void
pyg_toggle_notify (gpointer data, GObject *object, gboolean is_last_ref)
{
    PyGObject *self = (PyGObject*) data;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();

    if (is_last_ref)
	Py_DECREF(self);
    else
        Py_INCREF(self);

    pyg_gil_state_release(state);
}

  /* Called when the inst_dict is first created; switches the 
     reference counting strategy to start using toggle ref to keep the
     wrapper alive while the GObject lives.  In contrast, while
     inst_dict was NULL the python wrapper is allowed to die at
     will and is recreated on demand. */
static inline void
pygobject_switch_to_toggle_ref(PyGObject *self)
{
    g_assert(self->obj->ref_count >= 1);

    if (self->private_flags.flags & PYGOBJECT_USING_TOGGLE_REF)
        return; /* already using toggle ref */
    self->private_flags.flags |= PYGOBJECT_USING_TOGGLE_REF;
      /* Note that add_toggle_ref will never immediately call back into 
         pyg_toggle_notify */
    Py_INCREF((PyObject *) self);
    g_object_add_toggle_ref(self->obj, pyg_toggle_notify, self);
    g_object_unref(self->obj);
}

/**
 * pygobject_register_wrapper:
 * @self: the wrapper instance
 *
 * In the constructor of PyGTK wrappers, this function should be
 * called after setting the obj member.  It will tie the wrapper
 * instance to the GObject so that the same wrapper instance will
 * always be used for this GObject instance.  It may also sink any
 * floating references on the GObject.
 */
static inline void
pygobject_register_wrapper_full(PyGObject *self, gboolean sink)
{
    GObject *obj = self->obj;

    if (sink)
        pygobject_sink(obj);
    g_assert(obj->ref_count >= 1);
      /* save wrapper pointer so we can access it later */
    g_object_set_qdata_full(obj, pygobject_wrapper_key, self, NULL);
    if (self->inst_dict)
        pygobject_switch_to_toggle_ref(self);
}

void
pygobject_register_wrapper(PyObject *self)
{
    pygobject_register_wrapper_full((PyGObject *)self, TRUE);
}

static PyObject *
pyg_type_get_bases(GType gtype)
{
    GType *interfaces, parent_type, interface_type;
    guint n_interfaces;
    PyTypeObject *py_parent_type, *py_interface_type;
    PyObject *bases;
    int i;
    
    if (G_UNLIKELY(gtype == G_TYPE_OBJECT))
        return NULL;

    /* Lookup the parent type */
    parent_type = g_type_parent(gtype);
    py_parent_type = pygobject_lookup_class(parent_type);
    interfaces = g_type_interfaces(gtype, &n_interfaces);
    bases = PyTuple_New(n_interfaces + 1);
    /* We will always put the parent at the first position in bases */
    Py_INCREF(py_parent_type); /* PyTuple_SetItem steals a reference */
    PyTuple_SetItem(bases, 0, (PyObject *) py_parent_type);

    /* And traverse interfaces */
    if (n_interfaces) {
	for (i = 0; i < n_interfaces; i++) {
	    interface_type = interfaces[i];
	    py_interface_type = pygobject_lookup_class(interface_type);
            Py_INCREF(py_interface_type); /* PyTuple_SetItem steals a reference */
	    PyTuple_SetItem(bases, i + 1, (PyObject *) py_interface_type);
	}
    }
    g_free(interfaces);
    return bases;
}

/**
 * pygobject_new_with_interfaces
 * @gtype: the GType of the GObject subclass.
 *
 * Creates a new PyTypeObject from the given GType with interfaces attached in
 * bases.
 *
 * Returns: a PyTypeObject for the new type or NULL if it couldn't be created
 */
PyTypeObject *
pygobject_new_with_interfaces(GType gtype)
{
    PyGILState_STATE state;
    PyObject *o;
    PyTypeObject *type;
    PyObject *dict;
    PyTypeObject *py_parent_type;
    PyObject *bases;
    PyObject *modules, *module;
    gchar *type_name, *mod_name, *gtype_name;

    state = pyg_gil_state_ensure();

    bases = pyg_type_get_bases(gtype);
    py_parent_type = (PyTypeObject *) PyTuple_GetItem(bases, 0);

    dict = PyDict_New();
    
    o = pyg_type_wrapper_new(gtype);
    PyDict_SetItemString(dict, "__gtype__", o);
    Py_DECREF(o);

    /* set up __doc__ descriptor on type */
    PyDict_SetItemString(dict, "__doc__", pyg_object_descr_doc_get());

    /* generate the pygtk module name and extract the base type name */
    gtype_name = (gchar*)g_type_name(gtype);
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

    type = (PyTypeObject*)PyObject_CallFunction((PyObject *) py_parent_type->ob_type,
                                                "sNN", type_name, bases, dict);
    g_free(type_name);

    if (type == NULL) {
	PyErr_Print();
        pyg_gil_state_release(state);
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
      /* override more python stupid hacks behind our back */
    type->tp_dealloc = py_parent_type->tp_dealloc;
    type->tp_alloc = py_parent_type->tp_alloc;
    type->tp_free = py_parent_type->tp_free;
    type->tp_traverse = py_parent_type->tp_traverse;
    type->tp_clear = py_parent_type->tp_clear;

    if (PyType_Ready(type) < 0) {
	g_warning ("couldn't make the type `%s' ready", type->tp_name);
        pyg_gil_state_release(state);
	return NULL;
    }
    /* insert type name in module dict */
    modules = PyImport_GetModuleDict();
    if ((module = PyDict_GetItemString(modules, mod_name)) != NULL) {
        if (PyObject_SetAttrString(module, gtype_name, (PyObject *)type) < 0)
            PyErr_Clear();
    }

    /* stash a pointer to the python class with the GType */
    Py_INCREF(type);
    g_type_set_qdata(gtype, pygobject_class_key, type);

    pyg_gil_state_release(state);

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
    
    py_type = pyg_type_get_custom(g_type_name(gtype));
    if (py_type)
	return py_type;

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
 * pygobject_new_full:
 * @obj: a GObject instance.
 * @sink: whether to sink any floating reference found on the GObject.
 * @g_class: the GObjectClass
 *
 * This function gets a reference to a wrapper for the given GObject
 * instance.  If a wrapper has already been created, a new reference
 * to that wrapper will be returned.  Otherwise, a wrapper instance
 * will be created.
 *
 * Returns: a reference to the wrapper for the GObject.
 */
PyObject *
pygobject_new_full(GObject *obj, gboolean sink, gpointer g_class)
{
    PyGObject *self;

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
        PyGObjectData *inst_data = pyg_object_peek_inst_data(obj);
 	PyTypeObject *tp;
        if (inst_data)
            tp = inst_data->type;
        else {
            if (g_class)
                tp = pygobject_lookup_class(G_OBJECT_CLASS_TYPE(g_class));
            else
                tp = pygobject_lookup_class(G_OBJECT_TYPE(obj));
        }
        g_assert(tp != NULL);
        
        /* need to bump type refcount if created with
           pygobject_new_with_interfaces(). fixes bug #141042 */
        if (tp->tp_flags & Py_TPFLAGS_HEAPTYPE)
            Py_INCREF(tp);
	self = PyObject_GC_New(PyGObject, tp);
	if (self == NULL)
	    return NULL;
        self->inst_dict = NULL;
	self->weakreflist = NULL;
	self->obj = obj;
	g_object_ref(obj);
	pygobject_register_wrapper_full(self, sink);
	PyObject_GC_Track((PyObject *)self);
    }

    return (PyObject *)self;
}


PyObject *
pygobject_new(GObject *obj)
{
    return pygobject_new_full(obj, TRUE, NULL);
}

static void
pygobject_unwatch_closure(gpointer data, GClosure *closure)
{
    PyGObjectData *inst_data = data;

    inst_data->closures = g_slist_remove (inst_data->closures, closure);
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
    PyGObjectData *data;

    g_return_if_fail(self != NULL);
    g_return_if_fail(PyObject_TypeCheck(self, &PyGObject_Type));
    g_return_if_fail(closure != NULL);

    gself = (PyGObject *)self;
    data = pygobject_get_inst_data(gself);
    g_return_if_fail(g_slist_find(data->closures, closure) == NULL);
    data->closures = g_slist_prepend(data->closures, closure);
    g_closure_add_invalidate_notifier(closure, data, pygobject_unwatch_closure);
}

/* -------------- PyGObject behaviour ----------------- */

static void
pygobject_dealloc(PyGObject *self)
{
    PyObject_ClearWeakRefs((PyObject *)self);
    PyObject_GC_UnTrack((PyObject *)self);
      /* this forces inst_data->type to be updated, which could prove
       * important if a new wrapper has to be created and it is of a
       * unregistered type */
    pygobject_get_inst_data(self);
    pygobject_clear(self);
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
	       "<%s object at 0x%lx (%s at 0x%lx)>",
	       self->ob_type->tp_name,
	       (long)self,
	       self->obj ? G_OBJECT_TYPE_NAME(self->obj) : "uninitialized",
               (long)self->obj);
    return PyString_FromString(buf);
}


static int
pygobject_traverse(PyGObject *self, visitproc visit, void *arg)
{
    int ret = 0;
    GSList *tmp;
    PyGObjectData *data = pygobject_get_inst_data(self);

    if (self->inst_dict) ret = visit(self->inst_dict, arg);
    if (ret != 0) return ret;

    if (data) {

        for (tmp = data->closures; tmp != NULL; tmp = tmp->next) {
            PyGClosure *closure = tmp->data;

            if (closure->callback) ret = visit(closure->callback, arg);
            if (ret != 0) return ret;

            if (closure->extra_args) ret = visit(closure->extra_args, arg);
            if (ret != 0) return ret;

            if (closure->swap_data) ret = visit(closure->swap_data, arg);
            if (ret != 0) return ret;
        }
    }
    return ret;
}

static inline int
pygobject_clear(PyGObject *self)
{
    if (self->obj) {
        g_object_set_qdata_full(self->obj, pygobject_wrapper_key, NULL, NULL);
        if (self->inst_dict)
            g_object_remove_toggle_ref(self->obj, pyg_toggle_notify, self);
        else
            g_object_unref(self->obj);
        self->obj = NULL;
    }
    Py_CLEAR(self->inst_dict);
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
	Py_ssize_t pos = 0;
	PyObject *key;
	PyObject *value;

	params = g_new0(GParameter, PyDict_Size(kwargs));
	while (PyDict_Next (kwargs, &pos, &key, &value)) {
	    GParamSpec *pspec;
	    gchar *key_str = PyString_AsString(key);

	    pspec = g_object_class_find_property (class, key_str);
	    if (!pspec) {
		PyErr_Format(PyExc_TypeError,
			     "object of type `%s' doesn't support property `%s'",
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
    if (pygobject_constructv(self, n_params, params))
	PyErr_SetString(PyExc_RuntimeError, "could not create object");
	   
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

#define CHECK_GOBJECT(self) \
    if (!G_IS_OBJECT(self->obj)) {                                           \
	PyErr_Format(PyExc_TypeError,                                        \
                     "object at %p of type %s is not initialized",	     \
                     self, self->ob_type->tp_name);                          \
	return NULL;                                                         \
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

    CHECK_GOBJECT(self);
    
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(self->obj),
					 param_name);
    if (!pspec) {
	PyErr_Format(PyExc_TypeError,
		     "object of type `%s' does not have property `%s'",
		     g_type_name(G_OBJECT_TYPE(self->obj)), param_name);
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
pygobject_get_properties(PyGObject *self, PyObject *args)
{
    GObjectClass *class;
    int len, i;
    PyObject *tuple;

    if ((len = PyTuple_Size(args)) < 1) {
        PyErr_SetString(PyExc_TypeError, "requires at least one argument");
        return NULL;
    }

    tuple = PyTuple_New(len);
    class = G_OBJECT_GET_CLASS(self->obj);
    for (i = 0; i < len; i++) {
        PyObject *py_property = PyTuple_GetItem(args, i);
        gchar *property_name;
        GParamSpec *pspec;
        GValue value = { 0 };
        PyObject *item;

        if (!PyString_Check(py_property)) {
            PyErr_SetString(PyExc_TypeError,
                            "Expected string argument for property.");
            return NULL;
        }

        property_name = PyString_AsString(py_property);

        pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(self->obj),
					 property_name);
        if (!pspec) {
	    PyErr_Format(PyExc_TypeError,
		         "object of type `%s' does not have property `%s'",
		         g_type_name(G_OBJECT_TYPE(self->obj)), property_name);
    	return NULL;
        }
        if (!(pspec->flags & G_PARAM_READABLE)) {
	    PyErr_Format(PyExc_TypeError, "property %s is not readable",
		        property_name);
	    return NULL;
        }
        g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));

        g_object_get_property(self->obj, property_name, &value);

        item = pyg_value_as_pyobject(&value, TRUE);
        PyTuple_SetItem(tuple, i, item);

        g_value_unset(&value);
    }

    return tuple;
}

static PyObject *
pygobject_set_property(PyGObject *self, PyObject *args)
{
    gchar *param_name;
    GParamSpec *pspec;
    PyObject *pvalue;

    if (!PyArg_ParseTuple(args, "sO:GObject.set_property", &param_name,
			  &pvalue))
	return NULL;
    
    CHECK_GOBJECT(self);
    
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(self->obj),
					 param_name);
    if (!pspec) {
	PyErr_Format(PyExc_TypeError,
		     "object of type `%s' does not have property `%s'",
		     g_type_name(G_OBJECT_TYPE(self->obj)), param_name);
	return NULL;
    }
    
    if (!set_property_from_pspec(self->obj, param_name, pspec, pvalue))
	return NULL;
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_set_properties(PyGObject *self, PyObject *args, PyObject *kwargs)
{    
    GObjectClass    *class;
    Py_ssize_t      pos;
    PyObject        *value;
    PyObject        *key;

    CHECK_GOBJECT(self);

    class = G_OBJECT_GET_CLASS(self->obj);
    
    g_object_freeze_notify (G_OBJECT(self->obj));
    pos = 0;

    while (kwargs && PyDict_Next (kwargs, &pos, &key, &value)) {
    gchar *key_str = PyString_AsString (key);
    GParamSpec *pspec;
    GValue gvalue ={ 0, };

    pspec = g_object_class_find_property (class, key_str);
    if (!pspec) {
	    gchar buf[512];

	    g_snprintf(buf, sizeof(buf),
		       "object `%s' doesn't support property `%s'",
		       g_type_name(G_OBJECT_TYPE(self->obj)), key_str);
	    PyErr_SetString(PyExc_TypeError, buf);
	    return NULL;
	}

	g_value_init(&gvalue, G_PARAM_SPEC_VALUE_TYPE(pspec));
	if (pyg_value_from_pyobject(&gvalue, value)) {
	    gchar buf[512];

	    g_snprintf(buf, sizeof(buf),
		       "could not convert value for property `%s'", key_str);
	    PyErr_SetString(PyExc_TypeError, buf);
	    return NULL;
	}
	g_object_set_property(G_OBJECT(self->obj), key_str, &gvalue);
	g_value_unset(&gvalue);
    }

    g_object_thaw_notify (G_OBJECT(self->obj));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_freeze_notify(PyGObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":GObject.freeze_notify"))
	return NULL;
    
    CHECK_GOBJECT(self);
    
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
    
    CHECK_GOBJECT(self);
    
    g_object_notify(self->obj, property_name);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_thaw_notify(PyGObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":GObject.thaw_notify"))
	return NULL;
    
    CHECK_GOBJECT(self);
    
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
    
    CHECK_GOBJECT(self);
    
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

    if (!PyArg_ParseTuple(args, "sO:GObject.set_data", &key, &data))
	return NULL;
    
    CHECK_GOBJECT(self);
    
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
    guint sigid, len;
    gulong handlerid;
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
    
    CHECK_GOBJECT(self);
    
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(self->obj),
			     &sigid, &detail, TRUE)) {
	PyErr_Format(PyExc_TypeError, "%s: unknown signal name: %s",
		     PyString_AsString(PyObject_Repr((PyObject*)self)),
		     name);
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
    return PyLong_FromUnsignedLong(handlerid);
}

static PyObject *
pygobject_connect_after(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args;
    gchar *name;
    guint sigid;
    gulong handlerid;
    Py_ssize_t len;
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
    
    CHECK_GOBJECT(self);
    
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(self->obj),
			     &sigid, &detail, TRUE)) {
	PyErr_Format(PyExc_TypeError, "%s: unknown signal name: %s",
		     PyString_AsString(PyObject_Repr((PyObject*)self)),
		     name);
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
    return PyLong_FromUnsignedLong(handlerid);
}

static PyObject *
pygobject_connect_object(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *object;
    gchar *name;
    guint sigid;
    gulong handlerid;
    Py_ssize_t len;
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
    
    CHECK_GOBJECT(self);
    
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(self->obj),
			     &sigid, &detail, TRUE)) {
	PyErr_Format(PyExc_TypeError, "%s: unknown signal name: %s",
		     PyString_AsString(PyObject_Repr((PyObject*)self)),
		     name);
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
    return PyLong_FromUnsignedLong(handlerid);
}

static PyObject *
pygobject_connect_object_after(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *object;
    gchar *name;
    guint sigid;
    gulong handlerid;
    Py_ssize_t len;
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
    
    CHECK_GOBJECT(self);
    
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(self->obj),
			     &sigid, &detail, TRUE)) {
	PyErr_Format(PyExc_TypeError, "%s: unknown signal name: %s",
		     PyString_AsString(PyObject_Repr((PyObject*)self)),
		     name);
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
    return PyLong_FromUnsignedLong(handlerid);
}

static PyObject *
pygobject_disconnect(PyGObject *self, PyObject *args)
{
    gulong handler_id;

    if (!PyArg_ParseTuple(args, "k:GObject.disconnect", &handler_id))
	return NULL;
    
    CHECK_GOBJECT(self);
    
    g_signal_handler_disconnect(self->obj, handler_id);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_handler_is_connected(PyGObject *self, PyObject *args)
{
    gulong handler_id;

    if (!PyArg_ParseTuple(args, "k:GObject.handler_is_connected", &handler_id))
	return NULL;

    
    CHECK_GOBJECT(self);
    
    return PyBool_FromLong(g_signal_handler_is_connected(self->obj, handler_id));
}

static PyObject *
pygobject_handler_block(PyGObject *self, PyObject *args)
{
    gulong handler_id;

    if (!PyArg_ParseTuple(args, "k:GObject.handler_block", &handler_id))
	return NULL;
    
    CHECK_GOBJECT(self);
    
    g_signal_handler_block(self->obj, handler_id);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_handler_unblock(PyGObject *self, PyObject *args)
{
    gulong handler_id;

    if (!PyArg_ParseTuple(args, "k:GObject.handler_unblock", &handler_id))
	return NULL;
    g_signal_handler_unblock(self->obj, handler_id);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_emit(PyGObject *self, PyObject *args)
{
    guint signal_id, i;
    Py_ssize_t len;
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
    
    CHECK_GOBJECT(self);
    
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(self->obj),
			     &signal_id, &detail, TRUE)) {
	PyErr_Format(PyExc_TypeError, "%s: unknown signal name: %s",
		     PyString_AsString(PyObject_Repr((PyObject*)self)),
		     name);
	return NULL;
    }
    g_signal_query(signal_id, &query);
    if (len != query.n_params + 1) {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf),
		   "%d parameters needed for signal %s; %ld given",
		   query.n_params, name, (long int) (len - 1));
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
    
    CHECK_GOBJECT(self);
    
    if (!g_signal_parse_name(signal, G_OBJECT_TYPE(self->obj),
			     &signal_id, &detail, TRUE)) {
	PyErr_Format(PyExc_TypeError, "%s: unknown signal name: %s",
		     PyString_AsString(PyObject_Repr((PyObject*)self)),
		     signal);
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
    guint signal_id, i;
    Py_ssize_t len;
    PyObject *py_ret;
    const gchar *name;
    GSignalQuery query;
    GValue *params, ret = { 0, };
    
    CHECK_GOBJECT(self);
    
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
		   "%d parameters needed for signal %s; %ld given",
		   query.n_params, name, (long int) len);
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


static PyObject *
pygobject_weak_ref(PyGObject *self, PyObject *args)
{
    int len;
    PyObject *callback = NULL, *user_data = NULL;
    PyObject *retval;

    CHECK_GOBJECT(self);

    if ((len = PySequence_Length(args)) >= 1) {
        callback = PySequence_ITEM(args, 0);
        user_data = PySequence_GetSlice(args, 1, len);
    }
    retval = pygobject_weak_ref_new(self->obj, callback, user_data);
    Py_XDECREF(callback);
    Py_XDECREF(user_data);
    return retval;
}

static PyObject *
pygobject_disconnect_by_func(PyGObject *self, PyObject *args)
{
    PyObject *pyfunc = NULL;
    GClosure *closure = NULL;
    guint retval;
    
    CHECK_GOBJECT(self);

    if (!PyArg_ParseTuple(args, "O:GObject.disconnect_by_func", &pyfunc))
	return NULL;

    if (!PyCallable_Check(pyfunc)) {
	PyErr_SetString(PyExc_TypeError, "first argument must be callable");
	return NULL;
    }

    closure = gclosure_from_pyfunc(self, pyfunc);
    if (!closure) {
	PyErr_Format(PyExc_TypeError, "nothing connected to %s",
		     PyString_AsString(PyObject_Repr((PyObject*)pyfunc)));
	return NULL;
    }
    
    retval = g_signal_handlers_disconnect_matched(self->obj,
						  G_SIGNAL_MATCH_CLOSURE,
						  0, 0,
						  closure,
						  NULL, NULL);
    return PyInt_FromLong(retval);
}

static PyObject *
pygobject_handler_block_by_func(PyGObject *self, PyObject *args)
{
    PyObject *pyfunc = NULL;
    GClosure *closure = NULL;
    guint retval;
    
    CHECK_GOBJECT(self);

    if (!PyArg_ParseTuple(args, "O:GObject.handler_block_by_func", &pyfunc))
	return NULL;

    if (!PyCallable_Check(pyfunc)) {
	PyErr_SetString(PyExc_TypeError, "first argument must be callable");
	return NULL;
    }

    closure = gclosure_from_pyfunc(self, pyfunc);
    if (!closure) {
	PyErr_Format(PyExc_TypeError, "nothing connected to %s",
		     PyString_AsString(PyObject_Repr((PyObject*)pyfunc)));
	return NULL;
    }
    
    retval = g_signal_handlers_block_matched(self->obj,
					     G_SIGNAL_MATCH_CLOSURE,
					     0, 0,
					     closure,
					     NULL, NULL);
    return PyInt_FromLong(retval);
}

static PyObject *
pygobject_handler_unblock_by_func(PyGObject *self, PyObject *args)
{
    PyObject *pyfunc = NULL;
    GClosure *closure = NULL;
    guint retval;
    
    CHECK_GOBJECT(self);

    if (!PyArg_ParseTuple(args, "O:GObject.handler_unblock_by_func", &pyfunc))
	return NULL;

    if (!PyCallable_Check(pyfunc)) {
	PyErr_SetString(PyExc_TypeError, "first argument must be callable");
	return NULL;
    }

    closure = gclosure_from_pyfunc(self, pyfunc);
    if (!closure) {
	PyErr_Format(PyExc_TypeError, "nothing connected to %s",
		     PyString_AsString(PyObject_Repr((PyObject*)pyfunc)));
	return NULL;
    }
    
    retval = g_signal_handlers_unblock_matched(self->obj,
					       G_SIGNAL_MATCH_CLOSURE,
					       0, 0,
					       closure,
					       NULL, NULL);
    return PyInt_FromLong(retval);
}


static PyObject *
pygobject_run_dispose(PyGObject *self, PyObject *args)
{
    CHECK_GOBJECT(self);
    g_object_run_dispose(self->obj);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pygobject_methods[] = {
    { "__gobject_init__", (PyCFunction)pygobject__gobject_init__,
      METH_VARARGS|METH_KEYWORDS },
    { "get_property", (PyCFunction)pygobject_get_property, METH_VARARGS },
    { "get_properties", (PyCFunction)pygobject_get_properties, METH_VARARGS },
    { "set_property", (PyCFunction)pygobject_set_property, METH_VARARGS },
    { "set_properties", (PyCFunction)pygobject_set_properties, METH_KEYWORDS },
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
    { "disconnect_by_func", (PyCFunction)pygobject_disconnect_by_func, METH_VARARGS },
    { "handler_disconnect", (PyCFunction)pygobject_disconnect, METH_VARARGS },
    { "handler_is_connected", (PyCFunction)pygobject_handler_is_connected, METH_VARARGS },
    { "handler_block", (PyCFunction)pygobject_handler_block, METH_VARARGS },
    { "handler_unblock", (PyCFunction)pygobject_handler_unblock,METH_VARARGS },
    { "handler_block_by_func", (PyCFunction)pygobject_handler_block_by_func, METH_VARARGS },
    { "handler_unblock_by_func", (PyCFunction)pygobject_handler_unblock_by_func, METH_VARARGS },
    { "emit", (PyCFunction)pygobject_emit, METH_VARARGS },
    { "stop_emission", (PyCFunction)pygobject_stop_emission, METH_VARARGS },
    { "emit_stop_by_name", (PyCFunction)pygobject_stop_emission,METH_VARARGS },
    { "chain", (PyCFunction)pygobject_chain_from_overridden,METH_VARARGS },
    { "weak_ref", (PyCFunction)pygobject_weak_ref, METH_VARARGS },
    { "run_dispose", (PyCFunction)pygobject_run_dispose, METH_NOARGS },
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

static int
pygobject_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    int res;
    PyGObject *gself = (PyGObject *) self;
    PyObject *inst_dict_before = gself->inst_dict;
      /* call parent type's setattro */
    res = PyGObject_Type.tp_base->tp_setattro(self, name, value);
    if (inst_dict_before == NULL && gself->inst_dict != NULL) {
        if (G_LIKELY(gself->obj))
            pygobject_switch_to_toggle_ref(gself);
    }
    return res;
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
    (setattrofunc)pygobject_setattro,   /* tp_setattro */
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


  /* ------------------------------------ */
  /* ****** GObject weak reference ****** */
  /* ------------------------------------ */

typedef struct {
    PyObject_HEAD
    GObject *obj;
    PyObject *callback;
    PyObject *user_data;
    gboolean have_floating_ref;
} PyGObjectWeakRef;

static int
pygobject_weak_ref_traverse(PyGObjectWeakRef *self, visitproc visit, void *arg)
{
    if (self->callback && visit(self->callback, arg) < 0)
        return -1;
    if (self->user_data && visit(self->user_data, arg) < 0)
        return -1;
    return 0;
}

static void
pygobject_weak_ref_notify(PyGObjectWeakRef *self, GObject *dummy)
{
    self->obj = NULL;
    if (self->callback) {
        PyObject *retval;
        PyGILState_STATE state = pyg_gil_state_ensure();
        retval = PyObject_Call(self->callback, self->user_data, NULL);
        if (retval) {
            if (retval != Py_None)
                PyErr_Format(PyExc_TypeError,
                             "GObject weak notify callback returned a value"
                             " of type %s, should return None",
                             retval->ob_type->tp_name);
            Py_DECREF(retval);
            PyErr_Print();
        } else
            PyErr_Print();
        Py_CLEAR(self->callback);
        Py_CLEAR(self->user_data);
        if (self->have_floating_ref) {
            self->have_floating_ref = FALSE;
            Py_DECREF((PyObject *) self);
        }
        pyg_gil_state_release(state);
    }
}

static inline int
pygobject_weak_ref_clear(PyGObjectWeakRef *self)
{
    Py_CLEAR(self->callback);
    Py_CLEAR(self->user_data);
    if (self->obj) {
        g_object_weak_unref(self->obj, (GWeakNotify) pygobject_weak_ref_notify, self);
        self->obj = NULL;
    }
    return 0;
}

static void
pygobject_weak_ref_dealloc(PyGObjectWeakRef *self)
{
    PyObject_GC_UnTrack((PyObject *)self);
    pygobject_weak_ref_clear(self);
    PyObject_GC_Del(self);
}

static PyObject *
pygobject_weak_ref_new(GObject *obj, PyObject *callback, PyObject *user_data)
{
    PyGObjectWeakRef *self;

    self = PyObject_GC_New(PyGObjectWeakRef, &PyGObjectWeakRef_Type);
    self->callback = callback;
    self->user_data = user_data;
    Py_XINCREF(self->callback);
    Py_XINCREF(self->user_data);
    self->obj = obj;
    g_object_weak_ref(self->obj, (GWeakNotify) pygobject_weak_ref_notify, self);
    if (callback != NULL) {
          /* when we have a callback, we should INCREF the weakref
           * object to make it stay alive even if it goes out of scope */
        self->have_floating_ref = TRUE;
        Py_INCREF((PyObject *) self);
    }
    return (PyObject *) self;
}

static PyObject *
pygobject_weak_ref_unref(PyGObjectWeakRef *self, PyObject *args)
{
    if (!self->obj) {
        PyErr_SetString(PyExc_ValueError, "weak ref already unreffed");
        return NULL;
    }
    g_object_weak_unref(self->obj, (GWeakNotify) pygobject_weak_ref_notify, self);
    self->obj = NULL;
    if (self->have_floating_ref) {
        self->have_floating_ref = FALSE;
        Py_DECREF(self);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pygobject_weak_ref_methods[] = {
    { "unref", (PyCFunction)pygobject_weak_ref_unref, METH_NOARGS},
    { NULL, NULL, 0}
};

static PyObject *
pygobject_weak_ref_call(PyGObjectWeakRef *self, PyObject *args, PyObject *kw)
{
    static char *argnames[] = {NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, ":__call__", argnames))
        return NULL;

    if (self->obj)
        return pygobject_new_full(self->obj, FALSE, NULL);
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyTypeObject PyGObjectWeakRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                          /* ob_size */
    "gobject.GObjectWeakRef",                   /* tp_name */
    sizeof(PyGObjectWeakRef),                   /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)pygobject_weak_ref_dealloc,     /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_compare */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0, 						/* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    (ternaryfunc)pygobject_weak_ref_call,  	/*tp_call*/
    0,                                          /* tp_str */
    (getattrofunc)0,            		/* tp_getattro */
    (setattrofunc)0,            		/* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC,	/* tp_flags */
    "A GObject weak reference",                 /* tp_doc */
    (traverseproc)pygobject_weak_ref_traverse,  /* tp_traverse */
    (inquiry)pygobject_weak_ref_clear,          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,      					/* tp_iter */
    0,                                          /* tp_iternext */
    pygobject_weak_ref_methods,			/* tp_methods */
    0,						/* tp_members */
    0,						/* tp_getset */
    NULL,					/* tp_base */
    0,						/* tp_dict */
    0,						/* tp_descr_get */
    0,						/* tp_descr_set */
    0,						/* tp_dictoffset */
    0,						/* tp_init */
    0,						/* tp_alloc */
    0,						/* tp_new */
    0,						/* tp_free */
    0,						/* tp_is_gc */
    NULL,					/* tp_bases */
};

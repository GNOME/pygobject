/* -*- Mode: C; c-basic-offset: 4 -*- */
#define _INSIDE_PYGOBJECT_
#include "pygobject.h"

static GHashTable *class_hash;

static GQuark pygobject_wrapper_key = 0;
static GQuark pygobject_ownedref_key = 0;

staticforward PyExtensionClass PyGObject_Type;
static void      pygobject_dealloc(PyGObject *self);
static PyObject *pygobject_getattro(PyGObject *self, PyObject *attro);
static int       pygobject_setattr(PyGObject *self, char *attr, PyObject *val);
static int       pygobject_compare(PyGObject *self, PyGObject *v);
static long      pygobject_hash(PyGObject *self);
static PyObject *pygobject_repr(PyGObject *self);

/* -------------- class <-> wrapper manipulation --------------- */

void
pygobject_destroy_notify(gpointer user_data)
{
    PyObject *obj = (PyObject *)user_data;

    /* PyGTK_BLOCK_THREADS */
    Py_DECREF(obj);
    /* PyGTK_UNBLOCK_THREADS */
}

static void
pygobject_register_class(PyObject *dict, const gchar *class_name,
			 PyExtensionClass *ec, PyExtensionClass *parent)
{
    if (!class_hash)
	class_hash = g_hash_table_new(g_str_hash, g_str_equal);

    /* set standard pyobject class functions if they aren't already set */
    if (!ec->tp_dealloc)  ec->tp_dealloc  = (destructor)pygobject_dealloc;
    if (!ec->tp_getattro) ec->tp_getattro = (getattrofunc)pygobject_getattro;
    if (!ec->tp_setattr)  ec->tp_setattr  = (setattrfunc)pygobject_setattr;
    if (!ec->tp_compare)  ec->tp_compare  = (cmpfunc)pygobject_compare;
    if (!ec->tp_repr)     ec->tp_repr     = (reprfunc)pygobject_repr;
    if (!ec->tp_hash)     ec->tp_hash     = (hashfunc)pygobject_hash;

    if (parent) {
        PyExtensionClass_ExportSubclassSingle(dict, (char *)class_name,
                                              *ec, *parent);
    } else {
        PyExtensionClass_Export(dict, (char *)class_name, *ec);
    }

    g_hash_table_insert(class_hash, g_strdup(class_name), ec);
}

void
pygobject_register_wrapper(PyObject *self)
{
    GObject *obj = ((PyGObject *)self)->obj;

    /* g_object_ref(obj); -- not needed because no floating reference */
    g_object_set_qdata(obj, pygobject_wrapper_key, self);
}

static PyExtensionClass *
pygobject_lookup_class(GType type)
{
    PyExtensionClass *ec;

    /* find the python type for this object.  If not found, use parent. */
    while ((ec = g_hash_table_lookup(class_hash, g_type_name(type))) == NULL
           && type != 0)
        type = g_type_parent(type);
    g_assert(ec != NULL);
    return ec;
}

static PyObject *
pygobject_new(GObject *obj)
{
    PyGObject *self;
    PyTypeObject *tp;

    if (obj == NULL) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    /* we already have a wrapper for this object -- return it. */
    if ((self = (PyGObject *)g_object_get_qdata(obj, pygobject_wrapper_key))) {
	/* if the GObject currently owns the wrapper reference ... */
	if (self->hasref) {
	    self->hasref = FALSE;
	    g_object_steal_qdata(obj, pygobject_ownedref_key);
	    g_object_ref(obj);
	}
	Py_INCREF(self);
	return (PyObject *)self;
    }

    tp = (PyTypeObject *)pygobject_lookup_class(G_TYPE_FROM_INSTANCE(obj));
    self = PyObject_NEW(PyGObject, tp);

    if (self == NULL)
	return NULL;
    self->obj = obj;
    g_object_ref(obj);
    /* save wrapper pointer so we can access it later */
    g_object_set_qdata(obj, pygobject_wrapper_key, self);
    self->inst_dict = PyDict_New();

    return (PyObject *)self;
}

/* -------------- GValue marshalling ------------------ */

static gint
pyg_enum_get_value(GType enum_type, PyObject *obj, gint *val)
{
    GEnumClass *eclass = G_ENUM_CLASS(g_type_class_ref(enum_type));
    gint res = -1;

    g_return_val_if_fail(val != NULL, -1);
    if (!obj) {
	*val = 0;
	res = 0;
    } else if (PyInt_Check(obj)) {
	*val = PyInt_AsLong(obj);
	res = 0;
    } else if (PyString_Check(obj)) {
	char *str = PyString_AsString(obj);
	GEnumValue *info = g_enum_get_value_by_name(eclass, str);

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
    g_type_class_unref(eclass);
    return res;
}

static gint
pyg_flags_get_value(GType flag_type, PyObject *obj, gint *val)
{
    GFlagsClass *fclass = G_FLAGS_CLASS(g_type_class_ref(flag_type));
    gint res = -1;

    g_return_val_if_fail(val != NULL, -1);
    if (!obj) {
	*val = 0;
	res = 0;
    } else if (PyInt_Check(obj)) {
	*val = PyInt_AsLong(obj);
	res = 0;
    } else if (PyString_Check(obj)) {
	char *str = PyString_AsString(obj);
	GFlagsValue *info = g_flags_get_value_by_name(fclass, str);

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
	for (i = 0; i < len; i++) {
	    PyObject *item = PyTuple_GetItem(obj, i);
	    char *str = PyString_AsString(item);
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
    } else {
	PyErr_SetString(PyExc_TypeError,
			"flag values must be strings, ints or tuples");
	res = -1;
    }
    g_type_class_unref(fclass);
    return res;
}

typedef PyObject *(* fromvaluefunc)(const GValue *value);
typedef int (*tovaluefunc)(GValue *value, PyObject *obj);
typedef struct {
    fromvaluefunc fromvalue;
    tovaluefunc tovalue;
} PyGBoxedMarshal;
static GHashTable *boxed_marshalers;
#define pyg_boxed_lookup(boxed_type) \
  ((PyGBoxedMarshal *)g_hash_table_lookup(boxed_marshalers, \
                                          GUINT_TO_POINTER(boxed_type)))

static void
pyg_boxed_register(GType boxed_type,
		   PyObject *(* from_func)(const GValue *value),
		   int (* to_func)(GValue *value, PyObject *obj))
{
    PyGBoxedMarshal *bm = g_new(PyGBoxedMarshal, 1);

    bm->fromvalue = from_func;
    bm->tovalue = to_func;
    g_hash_table_insert(boxed_marshalers, GUINT_TO_POINTER(boxed_type), bm);
}

static int
pyg_value_from_pyobject(GValue *value, PyObject *obj)
{
    PyObject *tmp;

    if (G_IS_VALUE_CHAR(value)) {
	if ((tmp = PyObject_Str(obj)))
	    g_value_set_char(value, PyString_AsString(tmp)[0]);
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
    } else if (G_IS_VALUE_UCHAR(value)) {
	if ((tmp = PyObject_Str(obj)))
	    g_value_set_char(value, PyString_AsString(tmp)[0]);
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
    } else if (G_IS_VALUE_BOOLEAN(value)) {
	g_value_set_boolean(value, PyObject_IsTrue(obj));
    } else if (G_IS_VALUE_INT(value)) {
	if ((tmp = PyNumber_Int(obj)))
	    g_value_set_int(value, PyInt_AsLong(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
    } else if (G_IS_VALUE_UINT(value)) {
	if ((tmp = PyNumber_Int(obj)))
	    g_value_set_uint(value, PyInt_AsLong(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
    } else if (G_IS_VALUE_LONG(value)) {
	if ((tmp = PyNumber_Int(obj)))
	    g_value_set_long(value, PyInt_AsLong(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
    } else if (G_IS_VALUE_ULONG(value)) {
	if ((tmp = PyNumber_Int(obj)))
	    g_value_set_ulong(value, PyInt_AsLong(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
    } else if (G_IS_VALUE_FLOAT(value)) {
	if ((tmp = PyNumber_Float(obj)))
	    g_value_set_float(value, PyFloat_AsDouble(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
    } else if (G_IS_VALUE_DOUBLE(value)) {
	if ((tmp = PyNumber_Float(obj)))
	    g_value_set_double(value, PyFloat_AsDouble(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
    } else if (G_IS_VALUE_STRING(value)) {
	if ((tmp = PyObject_Str(obj)))
	    g_value_set_string(value, PyString_AsString(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
    } else if (G_IS_VALUE_OBJECT(value)) {
	PyExtensionClass *ec =pygobject_lookup_class(G_VALUE_TYPE(value));
	if (!ExtensionClassSubclassInstance_Check(obj, ec)) {
	    return -1;
	}
	g_value_set_object(value, pygobject_get(obj));
    } else if (G_IS_VALUE_ENUM(value)) {
	gint val = 0;
	if (pyg_enum_get_value(G_VALUE_TYPE(value), obj, &val) < 0)
	    return -1;
	g_value_set_enum(value, val);
    } else if (G_IS_VALUE_FLAGS(value)) {
	gint val = 0;
	if (pyg_flags_get_value(G_VALUE_TYPE(value), obj, &val) < 0)
	    return -1;
	g_value_set_flags(value, val);
    } else if (G_IS_VALUE_BOXED(value)) {
	PyGBoxedMarshal *bm = pyg_boxed_lookup(G_VALUE_TYPE(value));

	if (bm)
	    return bm->tovalue(value, obj);
	if (PyCObject_Check(obj))
	    g_value_set_boxed(value, PyCObject_AsVoidPtr(obj));
	else
	    return -1;
    } else if (G_IS_VALUE_POINTER(value)) {
	if (PyCObject_Check(obj))
	    g_value_set_pointer(value, PyCObject_AsVoidPtr(obj));
	else
	    return -1;
    }
    return 0;
}

static PyObject *
pyg_value_as_pyobject(const GValue *value)
{
    if (G_IS_VALUE_CHAR(value)) {
	gint8 val = g_value_get_char(value);
	return PyString_FromStringAndSize((char *)&val, 1);
    } else if (G_IS_VALUE_UCHAR(value)) {
	guint8 val = g_value_get_uchar(value);
	return PyString_FromStringAndSize((char *)&val, 1);
    } else if (G_IS_VALUE_INT(value)) {
	return PyInt_FromLong(g_value_get_int(value));
    } else if (G_IS_VALUE_UINT(value)) {
	return PyInt_FromLong(g_value_get_uint(value));
    } else if (G_IS_VALUE_LONG(value)) {
	return PyInt_FromLong(g_value_get_long(value));
    } else if (G_IS_VALUE_ULONG(value)) {
	return PyInt_FromLong(g_value_get_ulong(value));
    } else if (G_IS_VALUE_FLOAT(value)) {
	return PyFloat_FromDouble(g_value_get_float(value));
    } else if (G_IS_VALUE_DOUBLE(value)) {
	return PyFloat_FromDouble(g_value_get_double(value));
    } else if (G_IS_VALUE_STRING(value)) {
	return PyString_FromString(g_value_get_string(value));
    } else if (G_IS_VALUE_OBJECT(value)) {
	return pygobject_new(g_value_get_object(value));
    } else if (G_IS_VALUE_ENUM(value)) {
	return PyInt_FromLong(g_value_get_enum(value));
    } else if (G_IS_VALUE_FLAGS(value)) {
	return PyInt_FromLong(g_value_get_flags(value));
    } else if (G_IS_VALUE_BOXED(value)) {
	PyGBoxedMarshal *bm = pyg_boxed_lookup(G_VALUE_TYPE(value));

	if (bm)
	    return bm->fromvalue(value);
	else
	    return PyCObject_FromVoidPtr(g_value_get_boxed(value), NULL);
    } else if (G_IS_VALUE_POINTER(value)) {
	return PyCObject_FromVoidPtr(g_value_get_pointer(value), NULL);
    }
    PyErr_SetString(PyExc_TypeError, "unknown type");
    return NULL;
}

/* -------------- PyGClosure ----------------- */

typedef struct _PyGClosure PyGClosure;
struct _PyGClosure {
    GClosure closure;
    PyObject *callback;
    PyObject *extra_args; /* tuple of extra args to pass to callback */
    PyObject *swap_data; /* other object for gtk_signal_connect_object */
};

/* XXXX - must handle multithreadedness here */
static void
pyg_closure_destroy(gpointer data, GClosure *closure)
{
    PyGClosure *pc = (PyGClosure *)closure;

    Py_DECREF(pc->callback);
    Py_XDECREF(pc->extra_args);
    Py_XDECREF(pc->swap_data);
}

/* XXXX - need to handle python thread context stuff */
static void
pyg_closure_marshal(GClosure *closure,
		    GValue *return_value,
		    guint n_param_values,
		    const GValue *param_values,
		    gpointer invocation_hint,
		    gpointer marshal_data)
{
    PyGClosure *pc = (PyGClosure *)closure;
    PyObject *params, *ret;
    guint i;

    /* construct Python tuple for the parameter values */
    params = PyTuple_New(n_param_values);
    for (i = 0; i < n_param_values; i++) {
	/* swap in a different initial data for connect_object() */
	if (i == 0 && G_CCLOSURE_SWAP_DATA(closure)) {
	    g_return_if_fail(pc->swap_data != NULL);
	    Py_INCREF(pc->swap_data);
	    PyTuple_SetItem(params, 0, pc->swap_data);
	} else {
	    PyObject *item = pyg_value_as_pyobject(&param_values[i]);

	    /* error condition */
	    if (!item) {
		Py_DECREF(params);
		/* XXXX - clean up if threading was used */
		return;
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
	/* XXXX - do fatal exceptions thing here */
	PyErr_Print();
	PyErr_Clear();
	/* XXXX - clean up if threading was used */
	return;
    }
    pyg_value_from_pyobject(return_value, ret);
    Py_DECREF(ret);
    /* XXXX - clean up if threading was used */
}

static GClosure *
pyg_closure_new(PyObject *callback, PyObject *extra_args, PyObject *swap_data)
{
    GClosure *closure;

    g_return_val_if_fail(callback != NULL, NULL);
    closure = g_closure_new_simple(sizeof(PyGClosure), NULL);
    g_closure_add_fnotify(closure, NULL, pyg_closure_destroy);
    g_closure_set_marshal(closure, pyg_closure_marshal);
    Py_INCREF(callback);
    ((PyGClosure *)closure)->callback = callback;
    if (extra_args) {
	Py_INCREF(extra_args);
	((PyGClosure *)closure)->extra_args = extra_args;
    }
    if (swap_data) {
	Py_INCREF(swap_data);
	((PyGClosure *)closure)->swap_data;
	closure->derivative_flag = TRUE;
    }
    return closure;
}

/* -------------- PyGObject behaviour ----------------- */
static void
pygobject_dealloc(PyGObject *self)
{
    GObject *obj = self->obj;

    if (obj && !(((PyExtensionClass *)self->ob_type)->class_flags &
		 EXTENSIONCLASS_PYSUBCLASS_FLAG)) {
	/* save reference to python wrapper if there are still
         * references to the GObject in such a way that it will be
         * freed when the GObject is destroyed, so is the python
         * wrapper, but if a python wrapper can be */
        if (obj->ref_count > 1) {
            Py_INCREF(self); /* grab a reference on the wrapper */
            self->hasref = TRUE;
            g_object_set_qdata_full(obj, pygobject_ownedref_key,
				    self, pygobject_destroy_notify);
            g_object_unref(obj);
            return;
        }
        if (!self->hasref) /* don't unref the GObject if it owns us */
            g_object_unref(obj);
    }
    /* subclass_dealloc (ExtensionClass.c) does this for us for python
     * subclasses */
    if (self->inst_dict &&
        !(((PyExtensionClass *)self->ob_type)->class_flags &
          EXTENSIONCLASS_PYSUBCLASS_FLAG)) {
        Py_DECREF(self->inst_dict);
    }
    PyMem_DEL(self);
}

/* standard getattr method */
static PyObject *
check_bases(PyGObject *self, PyExtensionClass *class, char *attr)
{
    PyObject *ret;

    if (class->tp_getattr) {
	ret = (* class->tp_getattr)((PyObject *)self, attr);
	if (ret)
	    return ret;
	else
	    PyErr_Clear();
    }
    if (class->bases) {
	guint i, len = PyTuple_Size(class->bases);

	for (i = 0; i < len; i++) {
	    PyExtensionClass *base = (PyExtensionClass *)PyTuple_GetItem(
							class->bases, i);

	    ret = check_bases(self, base, attr);
	    if (ret)
		return ret;
	}
    }
    return NULL;
}
static PyObject *
pygobject_getattro(PyGObject *self, PyObject *attro)
{
    char *attr;
    PyObject *ret;

    attr = PyString_AsString(attro);

    ret = Py_FindAttrString((PyObject *)self, attr);
    if (ret)
	return ret;
    PyErr_Clear();
    ret = check_bases(self, (PyExtensionClass *)self->ob_type, attr);
    if (ret)
	return ret;
    PyErr_SetString(PyExc_AttributeError, attr);
    return NULL;
}

static int
pygobject_setattr(PyGObject *self, char *attr, PyObject *value)
{
    PyDict_SetItemString(INSTANCE_DICT(self), attr, value);
    return 0;
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
    gchar buf[128];

    g_snprintf(buf, sizeof(buf), "<%s at 0x%lx>", G_OBJECT_TYPE_NAME(self->obj),
	       (long)self->obj);
    return PyString_FromString(buf);
}

/* ---------------- PyGObject methods ----------------- */
static destructor real_subclass_dealloc = NULL;
static void
pygobject_subclass_dealloc(PyGObject *self)
{
    GObject *obj = self->obj;

    if (obj) {
	/* save reference to python wrapper if there are still
	 * references to the GObject in such a way that it will be
	 * freed when the GObject is destroyed, so is the python
	 * wrapper, but if a python wrapper can be */
	if (obj->ref_count > 1) {
	    Py_INCREF(self); /* grab a reference on the wrapper */
	    self->hasref = TRUE;
	    g_object_set_qdata_full(obj, pygobject_ownedref_key,
				    self, pygobject_destroy_notify);
	    g_object_unref(obj);
	    return;
	}
	if (!self->hasref) /* don't unref the GObject if it owns us */
	    g_object_unref(obj);
    }
    if (real_subclass_dealloc)
	(* real_subclass_dealloc)((PyObject *)self);
}
/* more hackery to stop segfaults caused by multi deallocs on a subclass
 * (which happens quite regularly in pygobject) */
static PyObject *
pygobject__class_init__(PyObject *something, PyObject *args)
{
    PyExtensionClass *subclass;

    if (!PyArg_ParseTuple(args, "O:GObject.__class_init__", &subclass))
	return NULL;
    g_message("__class_init__ called for %s", subclass->tp_name);
    if ((subclass->class_flags & EXTENSIONCLASS_PYSUBCLASS_FLAG) &&
	subclass->tp_dealloc != (destructor)pygobject_subclass_dealloc) {
	real_subclass_dealloc = subclass->tp_dealloc;
	subclass->tp_dealloc = (destructor)pygobject_subclass_dealloc;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject__init__(PyGObject *self, PyObject *args)
{
    GType object_type = G_TYPE_OBJECT;

    if (!PyArg_ParseTuple(args, "|i:GObject.__init__", &object_type))
	return NULL;
    self->obj = g_object_new(object_type, NULL);
    if (!self->obj) {
	PyErr_SetString(PyExc_RuntimeError, "could not create object");
	return NULL;
    }
    pygobject_register_wrapper((PyObject *)self);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_get_param(PyGObject *self, PyObject *args)
{
    gchar *param_name;
    GParamSpec *pspec;
    GValue value;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "s:GObject.get_param", &param_name))
	return NULL;
    pspec = g_object_class_find_param_spec(G_OBJECT_GET_CLASS(self->obj),
					   param_name);
    if (!pspec) {
	PyErr_SetString(PyExc_TypeError,
			"the object does not support the given parameter");
	return NULL;
    }
    g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
    g_object_get_param(self->obj, param_name, &value);
    ret = pyg_value_as_pyobject(&value);
    g_value_unset(&value);
    return ret;
}

static PyObject *
pygobject_set_param(PyGObject *self, PyObject *args)
{
    gchar *param_name;
    GParamSpec *pspec;
    GValue value;
    PyObject *pvalue;

    if (!PyArg_ParseTuple(args, "sO:GObject.set_param", &param_name, &pvalue))
	return NULL;
    pspec = g_object_class_find_param_spec(G_OBJECT_GET_CLASS(self->obj),
					   param_name);
    if (!pspec) {
	PyErr_SetString(PyExc_TypeError,
			"the object does not support the given parameter");
	return NULL;
    }
    g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
    if (pyg_value_from_pyobject(&value, pvalue) < 0) {
	PyErr_SetString(PyExc_TypeError,
			"could not convert argument to correct param type");
	return NULL;
    }
    g_object_set_param(self->obj, param_name, &value);
    g_value_unset(&value);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_queue_param_changed(PyGObject *self, PyObject *args)
{
    char *param_name;

    if (!PyArg_ParseTuple(args, "s:GObject.queue_param_changed", &param_name))
	return NULL;
    g_object_queue_param_changed(self->obj, param_name);
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
    g_object_set_qdata_full(self->obj, quark, data, pygobject_destroy_notify);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_connect(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args;
    gchar *name;
    guint handlerid, sigid, len;

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
    sigid = g_signal_lookup(name, G_OBJECT_TYPE(self->obj));
    if (sigid == 0) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    extra_args = PySequence_GetSlice(args, 2, len);
    if (extra_args == NULL)
	return NULL;
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, 0,
			pyg_closure_new(callback, extra_args, NULL), FALSE);
    return PyInt_FromLong(handlerid);
}

static PyObject *
pygobject_connect_after(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args;
    gchar *name;
    guint handlerid, sigid, len;

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
    sigid = g_signal_lookup(name, G_OBJECT_TYPE(self->obj));
    if (sigid == 0) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    extra_args = PySequence_GetSlice(args, 2, len);
    if (extra_args == NULL)
	return NULL;
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, 0,
			pyg_closure_new(callback, extra_args, NULL), TRUE);
    return PyInt_FromLong(handlerid);
}

static PyObject *
pygobject_connect_object(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *object;
    gchar *name;
    guint handlerid, sigid, len;

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
    sigid = g_signal_lookup(name, G_OBJECT_TYPE(self->obj));
    if (sigid == 0) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    extra_args = PySequence_GetSlice(args, 3, len);
    if (extra_args == NULL)
	return NULL;
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, 0,
			pyg_closure_new(callback, extra_args, object), FALSE);
    return PyInt_FromLong(handlerid);
}

static PyObject *
pygobject_connect_object_after(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *object;
    gchar *name;
    guint handlerid, sigid, len;

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
    sigid = g_signal_lookup(name, G_OBJECT_TYPE(self->obj));
    if (sigid == 0) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    extra_args = PySequence_GetSlice(args, 3, len);
    if (extra_args == NULL)
	return NULL;
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, 0,
			pyg_closure_new(callback, extra_args, object), TRUE);
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
    signal_id = g_signal_lookup(name, G_OBJECT_TYPE(self->obj));
    if (signal_id == 0) {
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
	g_value_init(&params[i + 1], query.param_types[i]);
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
	g_value_init(&ret, query.return_type);
    g_signal_emitv(params, signal_id, 0, &ret);
    for (i = 0; i < query.n_params + 1; i++)
	g_value_unset(&params[i]);
    g_free(params);
    if (query.return_type != G_TYPE_NONE) {
	py_ret = pyg_value_as_pyobject(&ret);
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

    if (!PyArg_ParseTuple(args, "s:GObject.stop_emission", &signal))
	return NULL;
    signal_id = g_signal_lookup(signal, G_OBJECT_TYPE(self->obj));
    if (signal_id == 0) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    g_signal_stop_emission(self->obj, signal_id, 0);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pygobject_methods[] = {
    { "__class_init__", (PyCFunction)pygobject__class_init__, METH_VARARGS|METH_CLASS_METHOD },
    { "__init__", (PyCFunction)pygobject__init__, METH_VARARGS },
    { "get_param", (PyCFunction)pygobject_get_param, METH_VARARGS },
    { "set_param", (PyCFunction)pygobject_set_param, METH_VARARGS },
    { "queue_param_changed", (PyCFunction)pygobject_queue_param_changed, METH_VARARGS },
    { "get_data", (PyCFunction)pygobject_get_data, METH_VARARGS },
    { "set_data", (PyCFunction)pygobject_set_data, METH_VARARGS },
    { "connect", (PyCFunction)pygobject_connect, METH_VARARGS },
    { "signal_connect", (PyCFunction)pygobject_connect, METH_VARARGS },
    { "connect_after", (PyCFunction)pygobject_connect_after, METH_VARARGS },
    { "signal_connect_after", (PyCFunction)pygobject_connect_after, METH_VARARGS },
    { "connect_object", (PyCFunction)pygobject_connect_object, METH_VARARGS },
    { "signal_connect_object", (PyCFunction)pygobject_connect_object, METH_VARARGS },
    { "connect_object_after", (PyCFunction)pygobject_connect_object_after, METH_VARARGS },
    { "signal_connect_object_after", (PyCFunction)pygobject_connect_object_after, METH_VARARGS },
    { "disconnect", (PyCFunction)pygobject_disconnect, METH_VARARGS },
    { "handler_disconnect", (PyCFunction)pygobject_disconnect, METH_VARARGS },
    { "handler_block", (PyCFunction)pygobject_handler_block, METH_VARARGS },
    { "handler_unblock", (PyCFunction)pygobject_handler_unblock,METH_VARARGS },
    { "emit", (PyCFunction)pygobject_emit, METH_VARARGS },
    { "stop_emission", (PyCFunction)pygobject_stop_emission, METH_VARARGS },
    { "emit_stop_by_name", (PyCFunction)pygobject_stop_emission,METH_VARARGS },
    { NULL, NULL, 0 }
};

static PyExtensionClass PyGObject_Type = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "GObject",				/* tp_name */
    sizeof(PyGObject),			/* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)pygobject_dealloc,	/* tp_dealloc */
    (printfunc)0,			/* tp_print */
    (getattrfunc)0,			/* tp_getattr */
    (setattrfunc)pygobject_setattr,	/* tp_setattr */
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
    /* Space for future expansion */
    0L, 0L,
    NULL, /* Documentation string */
    METHOD_CHAIN(pygobject_methods),
    EXTENSIONCLASS_INSTDICT_FLAG,
};

/* ---------------- gobject module functions -------------------- */

static PyMethodDef pygobject_functions[] = {
    { NULL, NULL, 0 }
};


/* ----------------- gobject module initialisation -------------- */

static struct _PyGObject_Functions functions = {
  pygobject_register_class,
  pygobject_register_wrapper,
  pygobject_lookup_class,
  pygobject_new,
  pyg_enum_get_value,
  pyg_flags_get_value,
  pyg_boxed_register,
  pyg_value_from_pyobject,
  pyg_value_as_pyobject,
};

DL_EXPORT(void)
initgobject(void)
{
    PyObject *m, *d;

    m = Py_InitModule("gobject", pygobject_functions);
    d = PyModule_GetDict(m);

    g_type_init();
    pygobject_register_class(d, "GObject", &PyGObject_Type, NULL);

    boxed_marshalers = g_hash_table_new(g_direct_hash, g_direct_equal);

    pygobject_wrapper_key = g_quark_from_static_string("py-gobject-wrapper");
    pygobject_ownedref_key = g_quark_from_static_string("py-gobject-ownedref");

    /* for addon libraries ... */
    PyDict_SetItemString(d, "_PyGObject_API",
			 PyCObject_FromVoidPtr(&functions, NULL));

    if (PyErr_Occurred()) {
	PyErr_Print();
	Py_FatalError("can't initialise module gobject");
    }
}

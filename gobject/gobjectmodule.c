/* -*- Mode: C; c-basic-offset: 4 -*- */
#define _INSIDE_PYGOBJECT_
#include "pygobject.h"

static PyObject *gerror_exc = NULL;

static GQuark pygobject_class_key = 0;
static GQuark pygobject_wrapper_key = 0;
static GQuark pygobject_ownedref_key = 0;

static GList *pygobject_exception_notifiers = NULL;

staticforward PyTypeObject PyGObject_Type;
static void pygobject_dealloc(PyGObject *self);
static int  pygobject_traverse(PyGObject *self, visitproc visit, void *arg);

static int  pyg_fatal_exceptions_notify(void);

static void
object_free(PyObject *op)
{
    PyObject_FREE(op);
}

static void
object_gc_free(PyObject *op)
{
    PyObject_GC_Del(op);
}

/* -------------- __gtype__ objects ---------------------------- */

typedef struct {
    PyObject_HEAD
    GType type;
} PyGTypeWrapper;

static int
pyg_type_wrapper_compare(PyGTypeWrapper *self, PyGTypeWrapper *v)
{
    if (self->type == v->type) return 0;
    if (self->type > v->type) return -1;
    return 1;
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

    g_snprintf(buf, sizeof(buf), "<GType %s (%p)>",
	       name?name:"invalid", self->type);
    return PyString_FromString(buf);
}

static void
pyg_type_wrapper_dealloc(PyGTypeWrapper *self)
{
    PyMem_DEL(self);
}

PyTypeObject PyGTypeWrapper_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "gobject.GType",
    sizeof(PyGTypeWrapper),
    0,
    (destructor)pyg_type_wrapper_dealloc,
    (printfunc)0,
    (getattrfunc)0,
    (setattrfunc)0,
    (cmpfunc)pyg_type_wrapper_compare,
    (reprfunc)pyg_type_wrapper_repr,
    0,
    0,
    0,
    (hashfunc)pyg_type_wrapper_hash,
    (ternaryfunc)0,
    (reprfunc)0,
    0L,0L,0L,0L,
    NULL
};

static PyObject *
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

/* -------------- GParamSpec objects ---------------------------- */

typedef struct {
    PyObject_HEAD
    GParamSpec *pspec;
} PyGParamSpec;

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
    PyMem_DEL(self);
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

static PyObject *
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


/* -------------- class <-> wrapper manipulation --------------- */

static void
pygobject_destroy_notify(gpointer user_data)
{
    PyObject *obj = (PyObject *)user_data;

    /* PyGTK_BLOCK_THREADS */
    Py_DECREF(obj);
    /* PyGTK_UNBLOCK_THREADS */
}

static void
pygobject_register_class(PyObject *dict, const gchar *type_name,
			 GType gtype, PyTypeObject *type,
			 PyObject *bases)
{
    PyObject *o;
    const char *class_name, *s;

    class_name = type->tp_name;
    s = strrchr(class_name, '.');
    if (s != NULL)
	class_name = s + 1;

    type->ob_type = &PyType_Type;
    if (bases) {
	type->tp_bases = bases;
	type->tp_base = (PyTypeObject *)PyTuple_GetItem(bases, 0);
    }

    type->tp_dealloc  = (destructor)pygobject_dealloc;
    type->tp_traverse = (traverseproc)pygobject_traverse;
    type->tp_flags |= Py_TPFLAGS_HAVE_GC;
    type->tp_weaklistoffset = offsetof(PyGObject, weakreflist);
    type->tp_dictoffset = offsetof(PyGObject, inst_dict);

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

    PyDict_SetItemString(dict, (char *)class_name, (PyObject *)type);
}

void
pygobject_register_wrapper(PyObject *self)
{
    GObject *obj = ((PyGObject *)self)->obj;

    /* g_object_ref(obj); -- not needed because no floating reference */
    g_object_set_qdata(obj, pygobject_wrapper_key, self);
}

static PyTypeObject *
pygobject_lookup_class(GType gtype)
{
    PyTypeObject *type;

    /* find the python type for this object.  If not found, use parent. */
    while (gtype != G_TYPE_INVALID &&
	   (type = g_type_get_qdata(gtype, pygobject_class_key)) == NULL)
        gtype = g_type_parent(gtype);
    g_assert(type != NULL);
    return type;
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
	} else
	    Py_INCREF(self);
	return (PyObject *)self;
    }

    tp = pygobject_lookup_class(G_TYPE_FROM_INSTANCE(obj));
    self = PyObject_GC_New(PyGObject, tp);

    if (self == NULL)
	return NULL;
    self->obj = g_object_ref(obj);
    self->hasref = FALSE;
    self->inst_dict = NULL;
    self->weakreflist = NULL;
    /* save wrapper pointer so we can access it later */
    g_object_set_qdata(obj, pygobject_wrapper_key, self);

    PyObject_GC_Track((PyObject *)self);

    return (PyObject *)self;
}

/* ---------------- GBoxed functions -------------------- */

static GType PY_TYPE_OBJECT = 0;

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

    Py_DECREF(object);
}

static void
pyg_boxed_dealloc(PyGBoxed *self)
{
    if (self->free_on_dealloc && self->boxed)
	g_boxed_free(self->gtype, self->boxed);

    self->ob_type->tp_free((PyObject *)self);
}

static int
pyg_boxed_compare(PyGBoxed *self, PyGBoxed *v)
{
    if (self->boxed == v->boxed) return 0;
    if (self->boxed > v->boxed)  return -1;
    return 1;
}

static long
pyg_boxed_hash(PyGBoxed *self)
{
    return (long)self->boxed;
}

static PyObject *
pyg_boxed_repr(PyGBoxed *self)
{
    gchar buf[128];

    g_snprintf(buf, sizeof(buf), "<%s at 0x%lx>", g_type_name(self->gtype),
	       (long)self->boxed);
    return PyString_FromString(buf);
}

static int
pyg_boxed_init(PyGBoxed *self, PyObject *args, PyObject *kwargs)
{
    gchar buf[512];

    if (!PyArg_ParseTuple(args, ":GBoxed.__init__"))
	return -1;

    self->boxed = NULL;
    self->gtype = 0;
    self->free_on_dealloc = FALSE;

    g_snprintf(buf, sizeof(buf), "%s can not be constructed", self->ob_type->tp_name);
    PyErr_SetString(PyExc_NotImplementedError, buf);
    return -1;
}

static PyTypeObject PyGBoxed_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gobject.GBoxed",                   /* tp_name */
    sizeof(PyGBoxed),                   /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pyg_boxed_dealloc,      /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)pyg_boxed_compare,         /* tp_compare */
    (reprfunc)pyg_boxed_repr,           /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)pyg_boxed_hash,           /* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
    (reprfunc)0,                        /* tp_str */
    (getattrofunc)0,			/* tp_getattro */
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
    (initproc)pyg_boxed_init,		/* tp_init */
    PyType_GenericAlloc,		/* tp_alloc */
    PyType_GenericNew,			/* tp_new */
    object_free,			/* tp_free */
    (inquiry)0,				/* tp_is_gc */
    (PyObject *)0,			/* tp_bases */
};

static GHashTable *boxed_types = NULL;

static void
pyg_register_boxed(PyObject *dict, const gchar *class_name,
		   GType boxed_type, PyTypeObject *type)
{
    PyObject *o;

    g_return_if_fail(dict != NULL);
    g_return_if_fail(class_name != NULL);
    g_return_if_fail(boxed_type != 0);

    if (!boxed_types)
	boxed_types = g_hash_table_new(g_direct_hash, g_direct_equal);

    if (!type->tp_dealloc)  type->tp_dealloc  = (destructor)pyg_boxed_dealloc;

    type->ob_type = &PyType_Type;
    type->tp_base = &PyGBoxed_Type;

    if (PyType_Ready(type) < 0) {
	g_warning("could not get type `%s' ready", type->tp_name);
	return;
    }

    PyDict_SetItemString(type->tp_dict, "__gtype__",
			 o=pyg_type_wrapper_new(boxed_type));
    Py_DECREF(o);

    g_hash_table_insert(boxed_types, GUINT_TO_POINTER(boxed_type), type);

    PyDict_SetItemString(dict, (char *)class_name, (PyObject *)type);
}

static PyObject *
pyg_boxed_new(GType boxed_type, gpointer boxed, gboolean copy_boxed,
	      gboolean own_ref)
{
    PyGBoxed *self;
    PyTypeObject *tp;

    g_return_val_if_fail(boxed_type != 0, NULL);
    g_return_val_if_fail(!copy_boxed || (copy_boxed && own_ref), NULL);

    if (!boxed) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    tp = g_hash_table_lookup(boxed_types, GUINT_TO_POINTER(boxed_type));
    if (!tp)
	tp = (PyTypeObject *)&PyGBoxed_Type; /* fallback */
    self = PyObject_NEW(PyGBoxed, tp);

    if (self == NULL)
	return NULL;

    if (copy_boxed)
	boxed = g_boxed_copy(boxed_type, boxed);
    self->boxed = boxed;
    self->gtype = boxed_type;
    self->free_on_dealloc = own_ref;

    return (PyObject *)self;
}

/* -------------- GValue marshalling ------------------ */

static gint
pyg_enum_get_value(GType enum_type, PyObject *obj, gint *val)
{
    GEnumClass *eclass = NULL;
    gint res = -1;

    g_return_val_if_fail(val != NULL, -1);
    if (!obj) {
	*val = 0;
	res = 0;
    } else if (PyInt_Check(obj)) {
	*val = PyInt_AsLong(obj);
	res = 0;
    } else if (PyString_Check(obj)) {
	GEnumValue *info;
	char *str = PyString_AsString(obj);
	
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

static gint
pyg_flags_get_value(GType flag_type, PyObject *obj, gint *val)
{
    GFlagsClass *fclass = NULL;
    gint res = -1;

    g_return_val_if_fail(val != NULL, -1);
    if (!obj) {
	*val = 0;
	res = 0;
    } else if (PyInt_Check(obj)) {
	*val = PyInt_AsLong(obj);
	res = 0;
    } else if (PyString_Check(obj)) {
	GFlagsValue *info;
	char *str = PyString_AsString(obj);

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
	g_type_class_unref(fclass);
    } else {
	PyErr_SetString(PyExc_TypeError,
			"flag values must be strings, ints or tuples");
	res = -1;
    }
    return res;
}

static GType
pyg_type_from_object(PyObject *obj)
{
    PyObject *gtype;
    GType type;

    /* NULL check */
    if (!obj) {
	PyErr_SetString(PyExc_TypeError, "can't get type from NULL object");
	return 0;
    }

    if (obj->ob_type == &PyGTypeWrapper_Type) {
	return ((PyGTypeWrapper *)obj)->type;
    }

    /* handle strings */
    if (PyString_Check(obj)) {
	type = g_type_from_name(PyString_AsString(obj));
	if (type == 0)
	    PyErr_SetString(PyExc_TypeError, "could not find named typecode");
	return type;
    }

    /* finally, look for a __gtype__ attribute on the object */
    gtype = PyObject_GetAttrString(obj, "__gtype__");
    if (gtype) {
	if (gtype->ob_type == &PyGTypeWrapper_Type) {
	    type = ((PyGTypeWrapper *)gtype)->type;
	    Py_DECREF(gtype);
	    return type;
	}
	Py_DECREF(gtype);
    }

    PyErr_Clear();
    PyErr_SetString(PyExc_TypeError, "could not get typecode from object");
    return 0;
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
pyg_register_boxed_custom(GType boxed_type,
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

    switch (G_TYPE_FUNDAMENTAL(G_VALUE_TYPE(value))) {
    case G_TYPE_CHAR:
	if ((tmp = PyObject_Str(obj)))
	    g_value_set_char(value, PyString_AsString(tmp)[0]);
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
	break;
    case G_TYPE_UCHAR:
	if ((tmp = PyObject_Str(obj)))
	    g_value_set_char(value, PyString_AsString(tmp)[0]);
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
	break;
    case G_TYPE_BOOLEAN:
	g_value_set_boolean(value, PyObject_IsTrue(obj));
	break;
    case G_TYPE_INT:
	if ((tmp = PyNumber_Int(obj)))
	    g_value_set_int(value, PyInt_AsLong(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
	break;
    case G_TYPE_UINT:
	if ((tmp = PyNumber_Int(obj)))
	    g_value_set_uint(value, PyInt_AsLong(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
	break;
    case G_TYPE_LONG:
	if ((tmp = PyNumber_Int(obj)))
	    g_value_set_long(value, PyInt_AsLong(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
	break;
    case G_TYPE_ULONG:
	if ((tmp = PyNumber_Int(obj)))
	    g_value_set_ulong(value, PyInt_AsLong(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
	break;
    case G_TYPE_FLOAT:
	if ((tmp = PyNumber_Float(obj)))
	    g_value_set_float(value, PyFloat_AsDouble(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
	break;
    case G_TYPE_DOUBLE:
	if ((tmp = PyNumber_Float(obj)))
	    g_value_set_double(value, PyFloat_AsDouble(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
	break;
    case G_TYPE_STRING:
	if ((tmp = PyObject_Str(obj)))
	    g_value_set_string(value, PyString_AsString(tmp));
	else {
	    PyErr_Clear();
	    return -1;
	}
	Py_DECREF(tmp);
	break;
    case G_TYPE_OBJECT:
	{
	    PyTypeObject *type =pygobject_lookup_class(G_VALUE_TYPE(value));
	    if (!PyObject_TypeCheck(obj, type)) {
		return -1;
	    }
	    g_value_set_object(value, pygobject_get(obj));
	}
	break;
    case G_TYPE_ENUM:
	{
	    gint val = 0;
	    if (pyg_enum_get_value(G_VALUE_TYPE(value), obj, &val) < 0)
		return -1;
	    g_value_set_enum(value, val);
	}
	break;
    case G_TYPE_FLAGS:
	{
	    guint val = 0;
	    if (pyg_flags_get_value(G_VALUE_TYPE(value), obj, &val) < 0)
		return -1;
	    g_value_set_flags(value, val);
	}
	break;
    case G_TYPE_BOXED:
	{
	    PyGBoxedMarshal *bm;

	    if (G_VALUE_HOLDS(value, PY_TYPE_OBJECT)) {
		g_value_set_boxed(value, obj);
	    } else if (PyObject_TypeCheck(obj, &PyGBoxed_Type) &&
		       G_VALUE_HOLDS(value, ((PyGBoxed *)obj)->gtype)) {
		g_value_set_boxed(value, pyg_boxed_get(obj, gpointer));
	    } else if ((bm = pyg_boxed_lookup(G_VALUE_TYPE(value))) != NULL) {
		return bm->tovalue(value, obj);
	    } else if (PyCObject_Check(obj)) {
		g_value_set_boxed(value, PyCObject_AsVoidPtr(obj));
	    } else
		return -1;
	}
	break;
    case G_TYPE_POINTER:
	if (PyCObject_Check(obj))
	    g_value_set_pointer(value, PyCObject_AsVoidPtr(obj));
	else
	    return -1;
	break;
    default:
	break;
    }
    return 0;
}

static PyObject *
pyg_value_as_pyobject(const GValue *value)
{
    gchar buf[128];

    switch (G_TYPE_FUNDAMENTAL(G_VALUE_TYPE(value))) {
    case G_TYPE_CHAR:
	{
	    gint8 val = g_value_get_char(value);
	    return PyString_FromStringAndSize((char *)&val, 1);
	}
    case G_TYPE_UCHAR:
	{
	    guint8 val = g_value_get_uchar(value);
	    return PyString_FromStringAndSize((char *)&val, 1);
	}
    case G_TYPE_BOOLEAN:
	return PyInt_FromLong(g_value_get_boolean(value));
    case G_TYPE_INT:
	return PyInt_FromLong(g_value_get_int(value));
    case G_TYPE_UINT:
	return PyInt_FromLong(g_value_get_uint(value));
    case G_TYPE_LONG:
	return PyInt_FromLong(g_value_get_long(value));
    case G_TYPE_ULONG:
	return PyInt_FromLong(g_value_get_ulong(value));
    case G_TYPE_FLOAT:
	return PyFloat_FromDouble(g_value_get_float(value));
    case G_TYPE_DOUBLE:
	return PyFloat_FromDouble(g_value_get_double(value));
    case G_TYPE_STRING:
	{
	    const gchar *str = g_value_get_string(value);

	    if (str)
		return PyString_FromString(str);
	    Py_INCREF(Py_None);
	    return Py_None;
	}
    case G_TYPE_OBJECT:
	return pygobject_new(g_value_get_object(value));
    case G_TYPE_ENUM:
	return PyInt_FromLong(g_value_get_enum(value));
    case G_TYPE_FLAGS:
	return PyInt_FromLong(g_value_get_flags(value));
    case G_TYPE_BOXED:
	{
	    PyGBoxedMarshal *bm;

	    if (G_VALUE_HOLDS(value, PY_TYPE_OBJECT))
		return (PyObject *)g_value_dup_boxed(value);

	    bm = pyg_boxed_lookup(G_VALUE_TYPE(value));
	    if (bm)
		return bm->fromvalue(value);
	    else
		return pyg_boxed_new(G_VALUE_TYPE(value),
				     g_value_get_boxed(value), TRUE, TRUE);
	}
    case G_TYPE_POINTER:
	return PyCObject_FromVoidPtr(g_value_get_pointer(value), NULL);
    default:
	break;
    }
    g_snprintf(buf, sizeof(buf), "unknown type %s",
	       g_type_name(G_VALUE_TYPE(value)));
    PyErr_SetString(PyExc_TypeError, buf);
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
	PyErr_Print();
	PyErr_Clear();
	return;
    }
    if (return_value)
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
    g_closure_add_finalize_notifier(closure, NULL, pyg_closure_destroy);
    g_closure_set_marshal(closure, pyg_closure_marshal);
    Py_INCREF(callback);
    ((PyGClosure *)closure)->callback = callback;
    if (extra_args) {
	if (!PyTuple_Check(extra_args)) {
	    PyObject *tmp = PyTuple_New(1);
	    PySequence_SetItem(tmp, 0, extra_args);
	    extra_args = tmp;
	} else {
            Py_INCREF(extra_args);
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
    GObject *object;
    PyObject *object_wrapper;
    GSignalInvocationHint *hint = (GSignalInvocationHint *)invocation_hint;
    gchar *method_name, *tmp;
    PyObject *method;
    PyObject *params, *ret;
    guint i;

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
	return;
    }
    Py_DECREF(object_wrapper);

    /* construct Python tuple for the parameter values */
    params = PyTuple_New(n_param_values - 1);
    for (i = 1; i < n_param_values; i++) {
	PyObject *item = pyg_value_as_pyobject(&param_values[i]);

	/* error condition */
	if (!item) {
	    Py_DECREF(params);
	    /* XXXX - clean up if threading was used */
	    return;
	}
	PyTuple_SetItem(params, i - 1, item);
    }

    ret = PyObject_CallObject(method, params);
    if (ret == NULL) {
	PyErr_Print();
	PyErr_Clear();
	/* XXXX - clean up if threading was used */
	Py_DECREF(method);
	return;
    }
    Py_DECREF(method);
    if (return_value)
	pyg_value_from_pyobject(return_value, ret);
    Py_DECREF(ret);
    /* XXXX - clean up if threading was used */
}

static GClosure *
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

/* -------------- PyGObject behaviour ----------------- */

static void
pygobject_dealloc(PyGObject *self)
{
    GObject *obj = self->obj;

    /* save reference to python wrapper if there are still
     * references to the GObject in such a way that it will be
     * freed when the GObject is destroyed, so is the python
     * wrapper, but if a python wrapper can be */
    if (obj && obj->ref_count > 1) {
	Py_INCREF(self); /* grab a reference on the wrapper */
	self->hasref = TRUE;
	g_object_set_qdata_full(obj, pygobject_ownedref_key,
				self, pygobject_destroy_notify);
	g_object_unref(obj);

	/* we ref the type, so subtype_dealloc() doesn't kill off our
         * instance's type. */
	if (self->ob_type->tp_flags & Py_TPFLAGS_HEAPTYPE)
	    Py_INCREF(self->ob_type);
	
#ifdef Py_TRACE_REFS
	/* if we're tracing refs, set up the reflist again, as it was just
	 * torn down */
        _Py_NewReference((PyObject *) self);
#endif

	return;
    }
    if (obj && !self->hasref) /* don't unref the GObject if it owns us */
	g_object_unref(obj);

    PyObject_ClearWeakRefs((PyObject *)self);

    PyObject_GC_UnTrack((PyObject *)self);

    if (self->inst_dict)
	Py_DECREF(self->inst_dict);
    self->inst_dict = NULL;

    /* the following causes problems with subclassed types */
    /*self->ob_type->tp_free((PyObject *)self); */
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
    if (self->inst_dict)
	return visit(self->inst_dict, arg);
    return 0;
}

/* ---------------- PyGObject methods ----------------- */

static int
pygobject_init(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    GType object_type;

    if (!PyArg_ParseTuple(args, ":GObject.__init__", &object_type))
	return -1;

    object_type = pyg_type_from_object((PyObject *)self);
    if (!object_type)
	return -1;

    self->obj = g_object_new(object_type, NULL);
    if (!self->obj) {
	PyErr_SetString(PyExc_RuntimeError, "could not create object");
	return -1;
    }
    pygobject_register_wrapper((PyObject *)self);

    return 0;
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
    g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
    g_object_get_property(self->obj, param_name, &value);
    ret = pyg_value_as_pyobject(&value);
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
    g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
    if (pyg_value_from_pyobject(&value, pvalue) < 0) {
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
    GQuark detail = 0;

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
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, detail,
			pyg_closure_new(callback, extra_args, NULL), FALSE);
    return PyInt_FromLong(handlerid);
}

static PyObject *
pygobject_connect_after(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args;
    gchar *name;
    guint handlerid, sigid, len;
    GQuark detail;

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
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, detail,
			pyg_closure_new(callback, extra_args, NULL), TRUE);
    return PyInt_FromLong(handlerid);
}

static PyObject *
pygobject_connect_object(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *object;
    gchar *name;
    guint handlerid, sigid, len;
    GQuark detail;

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
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, detail,
			pyg_closure_new(callback, extra_args, object), FALSE);
    return PyInt_FromLong(handlerid);
}

static PyObject *
pygobject_connect_object_after(PyGObject *self, PyObject *args)
{
    PyObject *first, *callback, *extra_args, *object;
    gchar *name;
    guint handlerid, sigid, len;
    GQuark detail;

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
    handlerid = g_signal_connect_closure_by_id(self->obj, sigid, detail,
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
    if (query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE != G_TYPE_NONE) {
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
    guint signal_id, i, len;
    PyObject *first, *py_ret;
    gchar *name;
    GSignalQuery query;
    GValue *params, ret = { 0, };

    len = PyTuple_Size(args);
    if (len < 1) {
	PyErr_SetString(PyExc_TypeError,"GObject.chain_from_overridden needs at least one arg");
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
    g_signal_chain_from_overridden(params, signal_id, &ret);
    for (i = 0; i < query.n_params + 1; i++)
	g_value_unset(&params[i]);
    g_free(params);
    if (query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE != G_TYPE_NONE) {
	py_ret = pyg_value_as_pyobject(&ret);
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
    { "handler_block", (PyCFunction)pygobject_handler_block, METH_VARARGS },
    { "handler_unblock", (PyCFunction)pygobject_handler_unblock,METH_VARARGS },
    { "emit", (PyCFunction)pygobject_emit, METH_VARARGS },
    { "stop_emission", (PyCFunction)pygobject_stop_emission, METH_VARARGS },
    { "emit_stop_by_name", (PyCFunction)pygobject_stop_emission,METH_VARARGS },
    { "chain_from_overridden", (PyCFunction)pygobject_chain_from_overridden,METH_VARARGS },
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

static PyGetSetDef pygobject_getsets[] = {
    { "__dict__", (getter)pygobject_get_dict, (setter)0 },
    { NULL, 0, 0 }
};

static PyTypeObject PyGObject_Type = {
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
    (inquiry)0,				/* tp_clear */
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
    PyType_GenericAlloc,		/* tp_alloc */
    PyType_GenericNew,			/* tp_new */
    object_gc_free,			/* tp_free */
    (inquiry)0,				/* tp_is_gc */
    (PyObject *)0,			/* tp_bases */
};

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

static PyTypeObject PyGInterface_Type = {
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
    PyType_GenericAlloc,		/* tp_alloc */
    PyType_GenericNew,			/* tp_new */
    object_free,			/* tp_free */
    (inquiry)0,				/* tp_is_gc */
    (PyObject *)0,			/* tp_bases */
};

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

    PyDict_SetItemString(dict, (char *)class_name, (PyObject *)type);
}


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
pyg_object_class_init(GObjectClass *class, PyObject *py_class)
{
    g_message("GType class init for `%s' with python class %p",
	      G_OBJECT_CLASS_NAME(class), py_class);
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
    GObjectClass *oclass;
    int pos = 0;
    PyObject *key, *value;

    oclass = g_type_class_ref(instance_type);
    while (PyDict_Next(signals, &pos, &key, &value)) {
	const gchar *signal_name;
	gboolean retval = TRUE;

	if (!PyString_Check(key)) {
	    PyErr_SetString(PyExc_TypeError,
			    "__gsignals__ keys must be strings");
	    g_type_class_unref(oclass);
	    return FALSE;
	}
	signal_name = PyString_AsString (key);

	if (value == Py_None ||
	    (PyString_Check(value) &&
	     !strcmp(PyString_AsString(value), "override"))) {
	    retval = override_signal(instance_type, signal_name);
	} else {
	    retval = create_signal(instance_type, signal_name, value);
	}

	if (!retval) {
	    g_type_class_unref(oclass);
	    return FALSE;
	}
    }
    g_type_class_unref(oclass);
    return TRUE;
}

static PyObject *
pyg_type_register(PyObject *self, PyObject *args)
{
    PyObject *gtype, *module, *gsignals;
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
    g_type_set_qdata(instance_type, pygobject_class_key, class);

    /* set new value of __gtype__ on class */
    gtype = pyg_type_wrapper_new(instance_type);
    PyDict_SetItemString(class->tp_dict, "__gtype__", gtype);
    Py_DECREF(gtype);

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
	Py_DECREF(gsignals);
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
    PyObject *value;
    PyObject *key;
    int pos=0, num_params=0, i;
    GParameter *params;

    if (!PyArg_ParseTuple (args, "O:gobject.new", &pytype)) {
	return NULL;
    }

    if ((type = pyg_type_from_object (pytype)) == 0)
	return NULL;
    
    if ((class = g_type_class_ref (type)) == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"could not get a reference to type class");
	return NULL;
    }

    params = g_new0(GParameter, PyDict_Size(kwargs));
    
    while (kwargs && PyDict_Next (kwargs, &pos, &key, &value)) {
	GParamSpec *pspec;
	gchar *key_str = g_strdup(PyString_AsString (key));
	pspec = g_object_class_find_property (class, key_str);
	if (!pspec) {
	    gchar buf[512];

	    g_snprintf(buf, sizeof(buf),
		       "gobject `%s' doesn't support property `%s'",
		       g_type_name(type), key_str);
	    PyErr_SetString(PyExc_TypeError, buf);
	    goto cleanup;
	}
	g_value_init(&params[num_params].value,
		     G_PARAM_SPEC_VALUE_TYPE(pspec));
	if (pyg_value_from_pyobject(&params[num_params].value, value)) {
	    gchar buf[512];

	    g_snprintf(buf, sizeof(buf),
		       "could not convert value for property `%s'", key_str);
	    PyErr_SetString(PyExc_TypeError, buf);
	    goto cleanup;
	}
	params[num_params].name = key_str;
	num_params++;
    }

    obj = g_object_newv(type, num_params, params);
    if (!obj)
	PyErr_SetString (PyExc_RuntimeError, "could not create object");
	   
 cleanup:
    for (i = 0; i < num_params; i++) {
	g_free((gchar *) params[i].name);
	g_value_unset(&params[i].value);
    }
    g_free(params);
    g_type_class_unref(class);
    
    if (obj)
	return pygobject_new ((GObject *)obj);
    return NULL;
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
    { NULL, NULL, 0 }
};


/* ----------------- Constant extraction ------------------------ */

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

static void
pyg_enum_add_constants(PyObject *module, GType enum_type,
		       const gchar *strip_prefix)
{
    GEnumClass *eclass;
    guint i;

    /* a more useful warning */
    if (!G_TYPE_IS_ENUM(enum_type)) {
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

static void
pyg_flags_add_constants(PyObject *module, GType flags_type,
			const gchar *strip_prefix)
{
    GFlagsClass *fclass;
    guint i;

    /* a more useful warning */
    if (!G_TYPE_IS_FLAGS(flags_type)) {
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

static gboolean
pyg_error_check(GError **error)
{
    g_return_val_if_fail(error != NULL, FALSE);

    if (*error != NULL) {
	PyObject *exc_instance;
	PyObject *d;

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
	g_clear_error(error);
	return TRUE;
    }
    return FALSE;
}

/* ----------------- gobject module initialisation -------------- */

static struct _PyGObject_Functions functions = {
  pygobject_register_class,
  pygobject_register_wrapper,
  pygobject_lookup_class,
  pygobject_new,
  pyg_closure_new,
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

  pyg_enum_add_constants,
  pyg_flags_add_constants,
  
  pyg_constant_strip_prefix,

  pyg_error_check,
};

DL_EXPORT(void)
initgobject(void)
{
    PyObject *m, *d, *o, *tuple;

    PyGTypeWrapper_Type.ob_type = &PyType_Type;
    PyGParamSpec_Type.ob_type = &PyType_Type;

    m = Py_InitModule("gobject", pygobject_functions);
    d = PyModule_GetDict(m);

    g_type_init();

    pygobject_class_key = g_quark_from_static_string("PyGObject::class");
    pygobject_wrapper_key = g_quark_from_static_string("PyGObject::wrapper");
    pygobject_ownedref_key = g_quark_from_static_string("PyGObject::ownedref");

    PY_TYPE_OBJECT = g_boxed_type_register_static("PyObject",
						  pyobject_copy,
						  pyobject_free);

    gerror_exc = PyErr_NewException("gobject.GError", PyExc_RuntimeError,NULL);
    PyDict_SetItemString(d, "GError", gerror_exc);

    pygobject_register_class(d, "GObject", G_TYPE_OBJECT,
			     &PyGObject_Type, NULL);

    PyGInterface_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&PyGInterface_Type))
	return;
    PyDict_SetItemString(d, "GInterface", (PyObject *)&PyGInterface_Type);
    PyDict_SetItemString(PyGInterface_Type.tp_dict, "__gtype__",
			 o=pyg_type_wrapper_new(G_TYPE_INTERFACE));
    Py_DECREF(o);

    PyGBoxed_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&PyGBoxed_Type))
	return;
    PyDict_SetItemString(d, "GBoxed", (PyObject *)&PyGBoxed_Type);
    PyDict_SetItemString(PyGBoxed_Type.tp_dict, "__gtype__",
			 o=pyg_type_wrapper_new(G_TYPE_BOXED));
    Py_DECREF(o);

    boxed_marshalers = g_hash_table_new(g_direct_hash, g_direct_equal);

    tuple = Py_BuildValue ("(iii)", glib_major_version, glib_minor_version,
			   glib_micro_version);
    PyDict_SetItemString(d, "glib_version", tuple);    
    Py_DECREF(tuple);

    /* for addon libraries ... */
    PyDict_SetItemString(d, "_PyGObject_API",
			 PyCObject_FromVoidPtr(&functions, NULL));

    /* some constants */
    PyModule_AddIntConstant(m, "SIGNAL_RUN_FIRST", G_SIGNAL_RUN_FIRST);
    PyModule_AddIntConstant(m, "SIGNAL_RUN_LAST", G_SIGNAL_RUN_LAST);
    PyModule_AddIntConstant(m, "SIGNAL_RUN_CLEANUP", G_SIGNAL_RUN_CLEANUP);
    PyModule_AddIntConstant(m, "SIGNAL_NO_RECURSE", G_SIGNAL_NO_RECURSE);
    PyModule_AddIntConstant(m, "SIGNAL_DETAILED", G_SIGNAL_DETAILED);
    PyModule_AddIntConstant(m, "SIGNAL_ACTION", G_SIGNAL_ACTION);
    PyModule_AddIntConstant(m, "SIGNAL_NO_HOOKS", G_SIGNAL_NO_HOOKS);

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

    if (PyErr_Occurred()) {
	PyErr_Print();
	Py_FatalError("can't initialise module gobject");
    }
}

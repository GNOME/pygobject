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

    g_snprintf(buf, sizeof(buf), "<GType %s>",
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
    "GType",
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
			 GType type, PyExtensionClass *ec,
			 PyObject *bases)
{
    PyObject *o;
    const char *class_name;

    if (!class_hash)
	class_hash = g_hash_table_new(g_str_hash, g_str_equal);

    class_name = ec->tp_name;
    /* set standard pyobject class functions if they aren't already set */
    if (!ec->tp_dealloc)  ec->tp_dealloc  = (destructor)pygobject_dealloc;
    if (!ec->tp_getattro) ec->tp_getattro = (getattrofunc)pygobject_getattro;
    if (!ec->tp_setattr)  ec->tp_setattr  = (setattrfunc)pygobject_setattr;
    if (!ec->tp_compare)  ec->tp_compare  = (cmpfunc)pygobject_compare;
    if (!ec->tp_repr)     ec->tp_repr     = (reprfunc)pygobject_repr;
    if (!ec->tp_hash)     ec->tp_hash     = (hashfunc)pygobject_hash;

    if (bases) {
        PyExtensionClass_ExportSubclass(dict, (char *)class_name,
					*ec, bases);
    } else {
        PyExtensionClass_Export(dict, (char *)class_name, *ec);
    }

    if (type) {
	o = pyg_type_wrapper_new(type);
	PyDict_SetItemString(ec->class_dictionary, "__gtype__", o);
	Py_DECREF(o);
    }
    g_hash_table_insert(class_hash, g_strdup(type_name), ec);
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
    PyMem_DEL(self);
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

static PyObject *
pyg_boxed_getattro(PyGBoxed *self, PyObject *attro)
{
    char *attr;
    PyObject *ret;

    attr = PyString_AsString(attro);

    ret = Py_FindAttrString((PyObject *)self, attr);
    if (ret)
	return ret;
    PyErr_Clear();

    if (self->ob_type->tp_getattr)
	return (* self->ob_type->tp_getattr)((PyObject *)self, attr);

    PyErr_SetString(PyExc_AttributeError, attr);
    return NULL;
}

static PyObject *
pyg_boxed__class_init__(PyObject *self, PyObject *args)
{
    PyExtensionClass *subclass;

    if (!PyArg_ParseTuple(args, "O:GBoxed.__class_init__", &subclass))
	return NULL;

    g_message("subclassing GBoxed types is bad m'kay");
    PyErr_SetString(PyExc_TypeError, "attempt to subclass a boxed type");
    return NULL;
}

static PyObject *
pyg_boxed_init(PyGBoxed *self, PyObject *args)
{
    gchar buf[512];

    if (!PyArg_ParseTuple(args, ":GBoxed.__init__"))
	return NULL;

    self->boxed = NULL;
    self->gtype = 0;
    self->free_on_dealloc = FALSE;

    g_snprintf(buf, sizeof(buf), "%s can not be constructed", self->ob_type->tp_name);
    PyErr_SetString(PyExc_NotImplementedError, buf);
    return NULL;
}

static PyMethodDef pyg_boxed_methods[] = {
    {"__class_init__",pyg_boxed__class_init__, METH_VARARGS|METH_CLASS_METHOD},
    {"__init__",  (PyCFunction)pyg_boxed_init, METH_VARARGS},
    {NULL,NULL,0}
};

static PyExtensionClass PyGBoxed_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "GBoxed",                          /* tp_name */
    sizeof(PyGBoxed),                  /* tp_basicsize */
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
    (getattrofunc)pyg_boxed_getattro,   /* tp_getattro */
    (setattrofunc)0,                    /* tp_setattro */
    /* Space for future expansion */
    0L, 0L,
    NULL, /* Documentation string */
    METHOD_CHAIN(pyg_boxed_methods),
    0,
};

static GHashTable *boxed_types = NULL;

static void
pyg_register_boxed(PyObject *dict, const gchar *class_name,
		   GType boxed_type, PyExtensionClass *ec)
{
    PyObject *o;

    g_return_if_fail(dict != NULL);
    g_return_if_fail(class_name != NULL);
    g_return_if_fail(boxed_type != 0);
    g_return_if_fail(ec != NULL);

    if (!boxed_types)
	boxed_types = g_hash_table_new(g_direct_hash, g_direct_equal);

    if (!ec->tp_dealloc)  ec->tp_dealloc  = (destructor)pyg_boxed_dealloc;
    if (!ec->tp_compare)  ec->tp_compare  = (cmpfunc)pyg_boxed_compare;
    if (!ec->tp_hash)     ec->tp_hash     = (hashfunc)pyg_boxed_hash;
    if (!ec->tp_repr)     ec->tp_repr     = (reprfunc)pyg_boxed_repr;
    if (!ec->tp_getattro) ec->tp_getattro = (getattrofunc)pyg_boxed_getattro;

    PyExtensionClass_ExportSubclassSingle(dict, (char *)class_name, *ec,
					  PyGBoxed_Type);
    PyDict_SetItemString(ec->class_dictionary, "__gtype__",
			 o=pyg_type_wrapper_new(boxed_type));
    Py_DECREF(o);
    g_hash_table_insert(boxed_types, GUINT_TO_POINTER(boxed_type), ec);
}

static PyObject *
pyg_boxed_new(GType boxed_type, gpointer boxed, gboolean copy_boxed,
	      gboolean own_ref)
{
    PyGBoxed *self;
    PyTypeObject *tp;

    g_return_if_fail(boxed_type != 0);
    g_return_if_fail(!copy_boxed || (copy_boxed && own_ref));

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
	    PyExtensionClass *ec =pygobject_lookup_class(G_VALUE_TYPE(value));
	    if (!ExtensionClassSubclassInstance_Check(obj, ec)) {
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
	    } else if (ExtensionClassSubclassInstance_Check(obj, &PyGBoxed_Type) &&
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
	return PyString_FromString(g_value_get_string(value));
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
	/* XXXX - do fatal exceptions thing here */
	PyErr_Print();
	PyErr_Clear();
	/* XXXX - clean up if threading was used */
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
	/* XXXX - do fatal exceptions thing here */
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
    if (PyExtensionClass_Check(class) && class->bases) {
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
    GTypeInfo type_info = {
	0,     /* class_size */
	(GBaseInitFunc) 0,
	(GBaseFinalizeFunc) 0,
	(GClassInitFunc) 0,
	(GClassFinalizeFunc) 0,
	NULL,  /* class_data */

	0,     /* instance_size */
	0,     /* n_preallocs */
	(GInstanceInitFunc) 0
    };

    if (!PyArg_ParseTuple(args, "O:GObject.__class_init__", &subclass))
	return NULL;

    g_message("__class_init__ called for %s", subclass->tp_name);

    /* make sure ExtensionClass doesn't screw up our dealloc hack */
    if ((subclass->class_flags & EXTENSIONCLASS_PYSUBCLASS_FLAG) &&
	subclass->tp_dealloc != (destructor)pygobject_subclass_dealloc) {
	real_subclass_dealloc = subclass->tp_dealloc;
	subclass->tp_dealloc = (destructor)pygobject_subclass_dealloc;
    }

    /* put code in here to create a new GType for this subclass, using
     * __module__.__name__ as the name for the type.  Then we can add
     * the code needed for adding signals to the subclass.  The actual
     * implementation will have to wait for a g_type_query function */

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject__init__(PyGObject *self, PyObject *args)
{
    GType object_type;

    if (!PyArg_ParseTuple(args, ":GObject.__init__", &object_type))
	return NULL;

    object_type = pyg_type_from_object((PyObject *)self);
    if (!object_type)
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
    g_signal_emitv(params, signal_id, detail, &ret);
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

static PyMethodDef pygobject_methods[] = {
    { "__class_init__", (PyCFunction)pygobject__class_init__, METH_VARARGS|METH_CLASS_METHOD },
    { "__init__", (PyCFunction)pygobject__init__, METH_VARARGS },
    { "__gobject_init__", (PyCFunction)pygobject__init__, METH_VARARGS },
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

/* ---------------- GInterface functions -------------------- */

static PyObject *
pyg_interface__class_init__(PyObject *self, PyObject *args)
{
    PyExtensionClass *subclass;

    if (!PyArg_ParseTuple(args, "O:GInterface.__class_init__", &subclass))
	return NULL;

    g_message("subclassing GInterface types is bad m'kay");
    PyErr_SetString(PyExc_TypeError, "attempt to subclass an interface");
    return NULL;
}

static PyObject *
pyg_interface_init(PyObject *self, PyObject *args)
{
    gchar buf[512];

    if (!PyArg_ParseTuple(args, ":GInterface.__init__"))
	return NULL;

    g_snprintf(buf, sizeof(buf), "%s can not be constructed", self->ob_type->tp_name);
    PyErr_SetString(PyExc_NotImplementedError, buf);
    return NULL;
}

static PyMethodDef pyg_interface_methods[] = {
    {"__class_init__", pyg_interface__class_init__,
     METH_VARARGS|METH_CLASS_METHOD},
    {"__init__",       pyg_interface_init, METH_VARARGS},
    {NULL,NULL,0}
};

static PyExtensionClass PyGInterface_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "GInterface",                       /* tp_name */
    sizeof(PyPureMixinObject),          /* tp_basicsize */
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
    /* Space for future expansion */
    0L, 0L,
    NULL, /* Documentation string */
    METHOD_CHAIN(pyg_interface_methods),
    0,
};

static void
pyg_register_interface(PyObject *dict, const gchar *class_name,
		       GType type, PyExtensionClass *ec)
{
    PyObject *o;

    PyExtensionClass_ExportSubclassSingle(dict, (char *)class_name,
					  *ec, PyGInterface_Type);

    if (type) {
	o = pyg_type_wrapper_new(type);
	PyDict_SetItemString(ec->class_dictionary, "__gtype__", o);
	Py_DECREF(o);
    }
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

static PyObject *
pyg_type_register(PyObject *self, PyObject *args)
{
    PyObject *class, *gtype, *module;
    GType parent_type, instance_type;
    gchar *type_name = NULL;
    gint i;
    GTypeQuery query;
    GTypeInfo type_info = {
	0,    /* class_size */

	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,

	(GClassInitFunc) NULL,
	(GClassFinalizeFunc) NULL,
	NULL, /* class_data */

	0,    /* instance_size */
	0,    /* n_preallocs */
	(GInstanceInitFunc) NULL
    };

    if (!PyArg_ParseTuple(args, "O:gobject.type_register", &class))
	return NULL;
    if (!ExtensionClassSubclass_Check(class, &PyGObject_Type)) {
	PyErr_SetString(PyExc_TypeError,"argument must be a GObject subclass");
	return NULL;
    }

    /* find the GType of the parent */
    parent_type = pyg_type_from_object(class);
    if (!parent_type) {
	return NULL;
    }

    /* make name for new widget */
    module = PyObject_GetAttrString(class, "__module__");
    if (module && PyString_Check(module)) {
	type_name = g_strconcat(PyString_AsString(module), ".",
				((PyExtensionClass *)class)->tp_name, NULL);
    } else {
	if (module)
	    Py_DECREF(module);
	else
	    PyErr_Clear();
	type_name = g_strdup(((PyExtensionClass *)class)->tp_name);
    }
    /* convert '.' in type name to '+', which isn't banned (grumble) */
    for (i = 0; type_name[i] != '\0'; i++)
	if (type_name[i] == '.')
	    type_name[i] = '+';

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

    /* set new value of __gtype__ on class */
    gtype = pyg_type_wrapper_new(instance_type);
    PyObject_SetAttrString(class, "__gtype__", gtype);
    Py_DECREF(gtype);

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

    if (!PyArg_ParseTuple(args, "O:gobject.object_class_list_properties",
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
	PyTuple_SetItem(list, i, PyString_FromString(specs[i]->name));
    }
    g_free(specs);
    g_type_class_unref(class);

    return list;
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
    { "object_class_list_properties", pyg_object_class_list_properties, METH_VARARGS },
    { NULL, NULL, 0 }
};


/* ----------------- Constant extraction ------------------------ */

static void
pyg_enum_add_constants(PyObject *module, GType enum_type,
		       const gchar *strip_prefix)
{
    GEnumClass *eclass;
    guint i, j;
    gint prefix_len;

    /* a more useful warning */
    if (!G_TYPE_IS_ENUM(enum_type)) {
	g_warning("`%s' is not an enum type", g_type_name(enum_type));
	return;
    }
    g_return_if_fail (strip_prefix != NULL);

    prefix_len = strlen(strip_prefix);
    eclass = G_ENUM_CLASS(g_type_class_ref(enum_type));

    for (i = 0; i < eclass->n_values; i++) {
	gchar *name = eclass->values[i].value_name;
	gint value = eclass->values[i].value;

	/* strip off prefix from value name, while keeping it a valid
         * identifier */
	for (j = prefix_len; j >= 0; j--) {
	    if (g_ascii_isalpha(name[j]) || name[j] == '_') {
		name = &name[j];
		break;
	    }
	}
	PyModule_AddIntConstant(module, name, (long) value);
    }

    g_type_class_unref(eclass);
}

static void
pyg_flags_add_constants(PyObject *module, GType flags_type,
			const gchar *strip_prefix)
{
    GFlagsClass *fclass;
    guint i, j;
    gint prefix_len;

    /* a more useful warning */
    if (!G_TYPE_IS_FLAGS(flags_type)) {
	g_warning("`%s' is not an flags type", g_type_name(flags_type));
	return;
    }
    g_return_if_fail (strip_prefix != NULL);

    prefix_len = strlen(strip_prefix);
    fclass = G_FLAGS_CLASS(g_type_class_ref(flags_type));

    for (i = 0; i < fclass->n_values; i++) {
	gchar *name = fclass->values[i].value_name;
	guint value = fclass->values[i].value;

	/* strip off prefix from value name, while keeping it a valid
         * identifier */
	for (j = prefix_len; j >= 0; j--) {
	    if (g_ascii_isalpha(name[j]) || name[j] == '_') {
		name = &name[j];
		break;
	    }
	}
	PyModule_AddIntConstant(module, name, (long) value);
    }

    g_type_class_unref(fclass);
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
};

DL_EXPORT(void)
initgobject(void)
{
    PyObject *m, *d, *o;

    PyGTypeWrapper_Type.ob_type = &PyType_Type;

    m = Py_InitModule("gobject", pygobject_functions);
    d = PyModule_GetDict(m);

    g_type_init();

    PY_TYPE_OBJECT = g_boxed_type_register_static("PyObject",
						  pyobject_copy,
						  pyobject_free);

    pygobject_register_class(d, "GObject", G_TYPE_OBJECT,
			     &PyGObject_Type, NULL);

    PyExtensionClass_Export(d, "GInterface", PyGInterface_Type);
    PyDict_SetItemString(PyGInterface_Type.class_dictionary, "__gtype__",
			 o=pyg_type_wrapper_new(G_TYPE_INTERFACE));
    Py_DECREF(o);

    PyExtensionClass_Export(d, "GBoxed", PyGBoxed_Type);
    PyDict_SetItemString(PyGBoxed_Type.class_dictionary, "__gtype__",
			 o=pyg_type_wrapper_new(G_TYPE_BOXED));
    Py_DECREF(o);

    boxed_marshalers = g_hash_table_new(g_direct_hash, g_direct_equal);

    pygobject_wrapper_key = g_quark_from_static_string("py-gobject-wrapper");
    pygobject_ownedref_key = g_quark_from_static_string("py-gobject-ownedref");

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

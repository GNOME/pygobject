/* -*- Mode: C; c-basic-offset: 4 -*- */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "pygobject-private.h"

static PyObject *gerror_exc = NULL;

static GQuark pygobject_class_key = 0;
static GQuark pygobject_wrapper_key = 0;
static GQuark pygobject_ownedref_key = 0;

static GList *pygobject_exception_notifiers = NULL;

staticforward PyTypeObject PyGObject_Type;
static void pygobject_dealloc(PyGObject *self);
static int  pygobject_traverse(PyGObject *self, visitproc visit, void *arg);

static int  pyg_fatal_exceptions_notify(void);


/* -------------- GDK threading hooks ---------------------------- */

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

static void
object_gc_free(PyObject *op)
{
    PyObject_GC_Del(op);
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
pyg_destroy_notify(gpointer user_data)
{
    PyObject *obj = (PyObject *)user_data;

    pyg_block_threads();
    Py_DECREF(obj);
    pyg_unblock_threads();
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

PyTypeObject *
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

PyObject *
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

    Py_DECREF(object);
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
				self, pyg_destroy_notify);
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
    GSignalInvocationHint *ihint;
    guint signal_id, i, len;
    PyObject *first, *py_ret;
    const gchar *name;
    GSignalQuery query;
    GValue *params, ret = { 0, };

    ihint = g_signal_get_invocation_hint(self->obj);
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
    g_signal_chain_from_overridden(params, &ret);
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
pyg_object_set_property (GObject *object, guint property_id,
			 const GValue *value, GParamSpec *pspec)
{
    PyObject *object_wrapper, *retval;
    PyObject *py_pspec, *py_value;

    object_wrapper = pygobject_new(object);
    g_return_if_fail(object_wrapper != NULL);

    py_pspec = pyg_param_spec_new(pspec);
    py_value = pyg_value_as_pyobject (value);
    retval = PyObject_CallMethod(object_wrapper, "do_set_property",
				 "OO", py_pspec, py_value);
    if (retval) {
	Py_DECREF(retval);
    } else {
	PyErr_Print();
	PyErr_Clear();
    }
    Py_DECREF(py_pspec);
    Py_DECREF(py_value);
}

static void
pyg_object_get_property (GObject *object, guint property_id,
			 GValue *value, GParamSpec *pspec)
{
    PyObject *object_wrapper, *retval;
    PyObject *py_pspec;

    object_wrapper = pygobject_new(object);
    g_return_if_fail(object_wrapper != NULL);

    py_pspec = pyg_param_spec_new(pspec);
    retval = PyObject_CallMethod(object_wrapper, "do_get_property",
				 "O", py_pspec);
    if (retval == NULL || pyg_value_from_pyobject(value, retval) < 0) {
	PyErr_Print();
	PyErr_Clear();
    }
    Py_XDECREF(retval);
    Py_DECREF(py_pspec);
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
	Py_DECREF(gproperties);
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

static struct _PyGObject_Functions pygobject_api_functions = {
  pygobject_register_class,
  pygobject_register_wrapper,
  pygobject_lookup_class,
  pygobject_new,

  pyg_closure_new,
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

  pyg_enum_add_constants,
  pyg_flags_add_constants,
  
  pyg_constant_strip_prefix,

  pyg_error_check,

  pyg_set_thread_block_funcs,
  (PyGThreadBlockFunc)0, /* block_threads */
  (PyGThreadBlockFunc)0, /* unblock_threads */
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

    tuple = Py_BuildValue ("(iii)", glib_major_version, glib_minor_version,
			   glib_micro_version);
    PyDict_SetItemString(d, "glib_version", tuple);    
    Py_DECREF(tuple);

    /* for addon libraries ... */
    PyDict_SetItemString(d, "_PyGObject_API",
			 PyCObject_FromVoidPtr(&pygobject_api_functions,NULL));

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

    if (PyErr_Occurred()) {
	PyErr_Print();
	Py_FatalError("can't initialise module gobject");
    }
}

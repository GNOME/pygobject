/* -*- mode: C; c-basic-indent: 4 -*- */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "pygobject-private.h"

static void
pyg_boxed_dealloc(PyGBoxed *self)
{
    if (self->free_on_dealloc && self->boxed) {
        pyg_unblock_threads();
	g_boxed_free(self->gtype, self->boxed);
	pyg_block_threads();
    }

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

static void
pyg_boxed_free(PyObject *op)
{
  PyObject_FREE(op);
}

static PyObject *
pyg_boxed_copy(PyGBoxed *self)
{
    return pyg_boxed_new (self->gtype, self->boxed, TRUE, TRUE);
}



static PyMethodDef pygboxed_methods[] = {
    { "copy", (PyCFunction) pyg_boxed_copy, METH_NOARGS },
    { NULL, NULL, 0 }
};



PyTypeObject PyGBoxed_Type = {
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
    pygboxed_methods,                   /* tp_methods */
    0,					/* tp_members */
    0,					/* tp_getset */
    (PyTypeObject *)0,			/* tp_base */
    (PyObject *)0,			/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    (initproc)pyg_boxed_init,		/* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
    pyg_boxed_free,			/* tp_free */
    (inquiry)0,				/* tp_is_gc */
    (PyObject *)0,			/* tp_bases */
};

static GQuark boxed_type_id = 0;

void
pyg_register_boxed(PyObject *dict, const gchar *class_name,
		   GType boxed_type, PyTypeObject *type)
{
    PyObject *o;

    g_return_if_fail(dict != NULL);
    g_return_if_fail(class_name != NULL);
    g_return_if_fail(boxed_type != 0);

    if (!boxed_type_id)
      boxed_type_id = g_quark_from_static_string("PyGBoxed::class");

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

    g_type_set_qdata(boxed_type, boxed_type_id, type);

    PyDict_SetItemString(dict, (char *)class_name, (PyObject *)type);
}

PyObject *
pyg_boxed_new(GType boxed_type, gpointer boxed, gboolean copy_boxed,
	      gboolean own_ref)
{
    PyGBoxed *self;
    PyTypeObject *tp;

    g_return_val_if_fail(boxed_type != 0, NULL);
    g_return_val_if_fail(!copy_boxed || (copy_boxed && own_ref), NULL);

    pyg_block_threads();

    if (!boxed) {
	Py_INCREF(Py_None);
	pyg_unblock_threads();
	return Py_None;
    }

    tp = g_type_get_qdata(boxed_type, boxed_type_id);
    if (!tp)
	tp = (PyTypeObject *)&PyGBoxed_Type; /* fallback */
    self = PyObject_NEW(PyGBoxed, tp);

    if (self == NULL) {
        pyg_unblock_threads();
        return NULL;
    }

    if (copy_boxed)
	boxed = g_boxed_copy(boxed_type, boxed);
    self->boxed = boxed;
    self->gtype = boxed_type;
    self->free_on_dealloc = own_ref;

    pyg_unblock_threads();

    return (PyObject *)self;
}

/* ------------------ G_TYPE_POINTER derivatives ------------------ */

static void
pyg_pointer_dealloc(PyGPointer *self)
{
    self->ob_type->tp_free((PyObject *)self);
}

static int
pyg_pointer_compare(PyGPointer *self, PyGPointer *v)
{
    if (self->pointer == v->pointer) return 0;
    if (self->pointer > v->pointer)  return -1;
    return 1;
}

static long
pyg_pointer_hash(PyGPointer *self)
{
    return (long)self->pointer;
}

static PyObject *
pyg_pointer_repr(PyGPointer *self)
{
    gchar buf[128];

    g_snprintf(buf, sizeof(buf), "<%s at 0x%lx>", g_type_name(self->gtype),
	       (long)self->pointer);
    return PyString_FromString(buf);
}

static int
pyg_pointer_init(PyGPointer *self, PyObject *args, PyObject *kwargs)
{
    gchar buf[512];

    if (!PyArg_ParseTuple(args, ":GPointer.__init__"))
	return -1;

    self->pointer = NULL;
    self->gtype = 0;

    g_snprintf(buf, sizeof(buf), "%s can not be constructed", self->ob_type->tp_name);
    PyErr_SetString(PyExc_NotImplementedError, buf);
    return -1;
}

static void
pyg_pointer_free(PyObject *op)
{
  PyObject_FREE(op);
}

PyTypeObject PyGPointer_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "gobject.GPointer",                 /* tp_name */
    sizeof(PyGPointer),                 /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pyg_pointer_dealloc,    /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)pyg_pointer_compare,       /* tp_compare */
    (reprfunc)pyg_pointer_repr,         /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)pyg_pointer_hash,         /* tp_hash */
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
    (initproc)pyg_pointer_init,		/* tp_init */
    (allocfunc)0,			/* tp_alloc */
    (newfunc)0,				/* tp_new */
    pyg_pointer_free,			/* tp_free */
    (inquiry)0,				/* tp_is_gc */
    (PyObject *)0,			/* tp_bases */
};

static GQuark pointer_type_id = 0;

void
pyg_register_pointer(PyObject *dict, const gchar *class_name,
		     GType pointer_type, PyTypeObject *type)
{
    PyObject *o;

    g_return_if_fail(dict != NULL);
    g_return_if_fail(class_name != NULL);
    g_return_if_fail(pointer_type != 0);

    if (!pointer_type_id)
      pointer_type_id = g_quark_from_static_string("PyGPointer::class");

    if (!type->tp_dealloc) type->tp_dealloc = (destructor)pyg_pointer_dealloc;

    type->ob_type = &PyType_Type;
    type->tp_base = &PyGPointer_Type;

    if (PyType_Ready(type) < 0) {
	g_warning("could not get type `%s' ready", type->tp_name);
	return;
    }

    PyDict_SetItemString(type->tp_dict, "__gtype__",
			 o=pyg_type_wrapper_new(pointer_type));
    Py_DECREF(o);

    g_type_set_qdata(pointer_type, pointer_type_id, type);

    PyDict_SetItemString(dict, (char *)class_name, (PyObject *)type);
}

PyObject *
pyg_pointer_new(GType pointer_type, gpointer pointer)
{
    PyGPointer *self;
    PyTypeObject *tp;

    g_return_val_if_fail(pointer_type != 0, NULL);

    pyg_block_threads();

    if (!pointer) {
	Py_INCREF(Py_None);
	pyg_unblock_threads();
	return Py_None;
    }

    tp = g_type_get_qdata(pointer_type, pointer_type_id);
    if (!tp)
	tp = (PyTypeObject *)&PyGPointer_Type; /* fallback */
    self = PyObject_NEW(PyGPointer, tp);

    pyg_unblock_threads();

    if (self == NULL)
	return NULL;

    self->pointer = pointer;
    self->gtype = pointer_type;

    return (PyObject *)self;
}

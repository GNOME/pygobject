#include "pygobject.h"
#include <gobject/gmarshal.h>

#include "test-thread.h"
#include "test-unknown.h"

static PyTypeObject *_PyGObject_Type;
#define PyGObject_Type (*_PyGObject_Type)

static PyObject * _wrap_TestInterface__do_iface_method(PyObject *cls,
						       PyObject *args,
						       PyObject *kwargs);

GType
test_type_get_type(void)
{
    static GType gtype = 0;
    GType parent_type;
    
    if (gtype == 0)
    {
        GTypeInfo *type_info;
        GTypeQuery query;
	
	parent_type = g_type_from_name("PyGObject");
	if (parent_type == 0)
	     g_error("could not get PyGObject from testmodule");

	type_info = (GTypeInfo *)g_new0(GTypeInfo, 1);
	
        g_type_query(parent_type, &query);
        type_info->class_size = query.class_size;
        type_info->instance_size = query.instance_size;
	
        gtype = g_type_register_static(parent_type,
				       "TestType", type_info, 0);
	if (!gtype)
	     g_error("Could not register TestType");
    }
    
    return gtype;
}

#define TYPE_TEST (test_type_get_type())

static PyObject *
_wrap_get_test_thread (PyObject * self)
{
  GObject *obj;

  test_thread_get_type();
  g_assert (g_type_is_a (TEST_TYPE_THREAD, G_TYPE_OBJECT));
  obj = g_object_new (TEST_TYPE_THREAD, NULL);
  g_assert (obj != NULL);
  
  return pygobject_new(obj);
}

static PyObject *
_wrap_get_unknown (PyObject * self)
{
  GObject *obj;
  obj = g_object_new (TEST_TYPE_UNKNOWN, NULL);
  return pygobject_new(obj);
}

static PyObject *
_wrap_create_test_type (PyObject * self)
{
    GObject *obj;
    PyObject *rv;
    obj = g_object_new(TYPE_TEST, NULL);
    rv = pygobject_new(obj);
    g_object_unref(obj);
    return rv;
}

static PyObject *
_wrap_test_g_object_new (PyObject * self)
{
    GObject *obj;
    PyObject *rv;

    obj = g_object_new(g_type_from_name("PyGObject"), NULL);
    rv = PyInt_FromLong(obj->ref_count); /* should be == 2 at this point */
    g_object_unref(obj);
    return rv;
}

static PyMethodDef testhelper_methods[] = {
    { "get_test_thread", (PyCFunction)_wrap_get_test_thread, METH_NOARGS },
    { "get_unknown", (PyCFunction)_wrap_get_unknown, METH_NOARGS },
    { "create_test_type", (PyCFunction)_wrap_create_test_type, METH_NOARGS },
    { "test_g_object_new", (PyCFunction)_wrap_test_g_object_new, METH_NOARGS },
    { NULL, NULL }
};

/* TestUnknown */
static PyObject *
_wrap_test_interface_iface_method(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,":", kwlist))
        return NULL;
    
    test_interface_iface_method(TEST_INTERFACE(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static const PyMethodDef _PyTestInterface_methods[] = {
    { "iface_method", (PyCFunction)_wrap_test_interface_iface_method, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "do_iface_method", (PyCFunction)_wrap_TestInterface__do_iface_method, METH_VARARGS|METH_KEYWORDS|METH_CLASS,
      NULL },
    { NULL, NULL, 0, NULL }
};

/* TestInterface */
PyTypeObject PyTestInterface_Type = {
    PyObject_HEAD_INIT(NULL)
    0,					/* ob_size */
    "test.Interface",			/* tp_name */
    sizeof(PyObject),	        /* tp_basicsize */
    0,					/* tp_itemsize */
    /* methods */
    (destructor)0,	/* tp_dealloc */
    (printfunc)0,			/* tp_print */
    (getattrfunc)0,	/* tp_getattr */
    (setattrfunc)0,	/* tp_setattr */
    (cmpfunc)0,		/* tp_compare */
    (reprfunc)0,		/* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,		/* tp_hash */
    (ternaryfunc)0,		/* tp_call */
    (reprfunc)0,		/* tp_str */
    (getattrofunc)0,			/* tp_getattro */
    (setattrofunc)0,			/* tp_setattro */
    (PyBufferProcs*)0,	/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    0,             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyTestInterface_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    0,                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */


    
};

static PyObject *
_wrap_TestInterface__do_iface_method(PyObject *cls, PyObject *args, PyObject *kwargs)
{
  TestInterfaceIface *iface;
  static char *kwlist[] = { "self", NULL };
  PyGObject *self;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!:TestInterface.iface_method", kwlist, &PyTestInterface_Type, &self))
    return NULL;
  
  iface = g_type_interface_peek(g_type_class_peek(pyg_type_from_object(cls)),
				TEST_TYPE_INTERFACE);
  if (iface->iface_method)
    iface->iface_method(TEST_INTERFACE(self->obj));
  else {
    PyErr_SetString(PyExc_NotImplementedError,
		    "interface method TestInterface.iface_method not implemented");
    return NULL;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyTypeObject PyTestUnknown_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "testhelper.Unknown",            /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)0, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)0,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};


static void
_wrap_TestInterface__proxy_do_iface_method(TestInterface *self)
{
    PyGILState_STATE __py_state;
    PyObject *py_self;
    PyObject *py_retval;
    PyObject *py_args;
    PyObject *py_method;
    
    __py_state = pyg_gil_state_ensure();
    py_self = pygobject_new((GObject *) self);
    if (!py_self) {
        if (PyErr_Occurred())
            PyErr_Print();
        pyg_gil_state_release(__py_state);
        return;
    }
    py_args = PyTuple_New(0);
    py_method = PyObject_GetAttrString(py_self, "do_iface_method");
    if (!py_method) {
        if (PyErr_Occurred())
            PyErr_Print();
        Py_DECREF(py_args);
        Py_DECREF(py_self);
        pyg_gil_state_release(__py_state);
        return;
    }
    py_retval = PyObject_CallObject(py_method, py_args);
    if (!py_retval) {
        if (PyErr_Occurred())
            PyErr_Print();
        Py_DECREF(py_method);
        Py_DECREF(py_args);
        Py_DECREF(py_self);
        pyg_gil_state_release(__py_state);
        return;
    }
    if (py_retval != Py_None) {
        if (PyErr_Occurred())
            PyErr_Print();
        PyErr_SetString(PyExc_TypeError, "retval should be None");
        Py_DECREF(py_retval);
        Py_DECREF(py_method);
        Py_DECREF(py_args);
        Py_DECREF(py_self);
        pyg_gil_state_release(__py_state);
        return;
    }
    
    Py_DECREF(py_retval);
    Py_DECREF(py_method);
    Py_DECREF(py_args);
    Py_DECREF(py_self);
    pyg_gil_state_release(__py_state);
}

static void
__TestInterface__interface_init(TestInterfaceIface *iface,
				PyTypeObject *pytype)
{
    TestInterfaceIface *parent_iface = g_type_interface_peek_parent(iface);
    PyObject *py_method;

    py_method = pytype ? PyObject_GetAttrString((PyObject *) pytype,
						"do_iface_method") : NULL;

    if (py_method && !PyObject_TypeCheck(py_method, &PyCFunction_Type)) {
        iface->iface_method = _wrap_TestInterface__proxy_do_iface_method;
    } else {
        PyErr_Clear();
        if (parent_iface) {
            iface->iface_method = parent_iface->iface_method;
        }
	Py_XDECREF(py_method);
    }
}

static const GInterfaceInfo __TestInterface__iinfo = {
    (GInterfaceInitFunc) __TestInterface__interface_init,
    NULL,
    NULL
};

void 
inittesthelper ()
{
  PyObject *m, *d;
  PyObject *module;
  
  init_pygobject();
  g_thread_init(NULL);
  m = Py_InitModule ("testhelper", testhelper_methods);

  d = PyModule_GetDict(m);

  if ((module = PyImport_ImportModule("gobject")) != NULL) {
    PyObject *moddict = PyModule_GetDict(module);
    
    _PyGObject_Type = (PyTypeObject *)PyDict_GetItemString(moddict, "GObject");
    if (_PyGObject_Type == NULL) {
      PyErr_SetString(PyExc_ImportError,
		      "cannot import name GObject from gobject");
      return ;
    }
  } else {
    PyErr_SetString(PyExc_ImportError,
		    "could not import gobject");
    return ;
  }

  /* TestInterface */
  pyg_register_interface(d, "Interface", TEST_TYPE_INTERFACE,
			 &PyTestInterface_Type);
  pyg_register_interface_info(TEST_TYPE_INTERFACE, &__TestInterface__iinfo);


  /* TestUnknown */
  pygobject_register_class(d, "Unknown", TEST_TYPE_UNKNOWN,
			   &PyTestUnknown_Type,
			   Py_BuildValue("(O)",
					 &PyGObject_Type,
					 &PyTestInterface_Type));
  pyg_set_object_has_new_constructor(TEST_TYPE_UNKNOWN);
  //pyg_register_class_init(TEST_TYPE_UNKNOWN, __GtkUIManager_class_init);

}


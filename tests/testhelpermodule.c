#include "pygobject.h"
#include <gobject/gmarshal.h>

#include "test-thread.h"
#include "test-unknown.h"


static GType
py_label_get_type(void)
{
    static GType gtype = 0;
    if (gtype == 0) {
        PyObject *module;
        if ((module = PyImport_ImportModule("testmodule")) != NULL) {
            PyObject *moddict = PyModule_GetDict(module);
            PyObject *py_label_type = PyDict_GetItemString(moddict, "PyLabel");
            if (py_label_type != NULL)
                gtype = pyg_type_from_object(py_label_type);
        }
    }
    if (gtype == 0)
        g_warning("could not get PyLabel from testmodule");
    return gtype;
}

GType
test_type_get_type(void)
{
    static GType gtype = 0;

    if (gtype == 0)
    {
        GTypeQuery q;
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
        g_type_query(py_label_get_type(), &q);
        type_info.class_size = q.class_size;
        type_info.instance_size = q.instance_size;
        gtype = g_type_register_static(py_label_get_type(), "TestType", &type_info, 0);
    }
    return gtype;
}

#define TYPE_TEST (test_type_get_type())

static PyObject *
_wrap_get_tp_basicsize (PyObject * self, PyObject * args)
{
  PyObject *item = PyTuple_GetItem(args, 0);
  return PyInt_FromLong(((PyTypeObject*)item)->tp_basicsize);
}

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

    obj = g_object_new(py_label_get_type(), NULL);
    rv = PyInt_FromLong(obj->ref_count); /* should be == 2 at this point */
    g_object_unref(obj);
    return rv;
}

static PyMethodDef testhelper_methods[] = {
    { "get_tp_basicsize", _wrap_get_tp_basicsize, METH_VARARGS },
    { "get_test_thread", (PyCFunction)_wrap_get_test_thread, METH_NOARGS },
    { "get_unknown", (PyCFunction)_wrap_get_unknown, METH_NOARGS },
    { "create_test_type", (PyCFunction)_wrap_create_test_type, METH_NOARGS },
    { "test_g_object_new", (PyCFunction)_wrap_test_g_object_new, METH_NOARGS },
    { NULL, NULL }
};

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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL, 				/* Documentation string */
    (traverseproc)0,	/* tp_traverse */
    (inquiry)0,		/* tp_clear */
    (richcmpfunc)0,	/* tp_richcompare */
    0,             /* tp_weaklistoffset */
    (getiterfunc)0,		/* tp_iter */
    (iternextfunc)0,	/* tp_iternext */
    0,			/* tp_methods */
    0,					/* tp_members */
    0,		       	/* tp_getset */
    NULL,				/* tp_base */
    NULL,				/* tp_dict */
    (descrgetfunc)0,	/* tp_descr_get */
    (descrsetfunc)0,	/* tp_descr_set */
    0,                 /* tp_dictoffset */
    (initproc)0,		/* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};

void 
inittesthelper ()
{
  PyObject *m, *d;
  
  init_pygobject();
  g_thread_init(NULL);
  m = Py_InitModule ("testhelper", testhelper_methods);

  d = PyModule_GetDict(m);
  
  pyg_register_interface(d, "Interface", TEST_TYPE_INTERFACE, &PyTestInterface_Type);

}


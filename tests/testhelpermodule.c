#include "pygobject.h"
#include <gobject/gmarshal.h>

#include "test-thread.h"
#include "test-unknown.h"


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

static PyMethodDef testhelper_methods[] = {
    { "get_tp_basicsize", _wrap_get_tp_basicsize, METH_VARARGS },
    { "get_test_thread", (PyCFunction)_wrap_get_test_thread, METH_NOARGS },
    { "get_unknown", (PyCFunction)_wrap_get_unknown, METH_NOARGS },
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


#include "pygobject.h"
#include <gobject/gmarshal.h>

#include "test-thread.h"
#include "test-unknown.h"

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
};

void 
inittesthelper ()
{
  PyObject *m, *d;
  
  init_pygobject();
  g_thread_init(NULL);
  m = Py_InitModule ("testhelper", testhelper_methods);

  d = PyModule_GetDict(m);
  
  pyg_register_interface(d, "Interface", TEST_TYPE_INTERFACE,
			 &PyTestInterface_Type);

}


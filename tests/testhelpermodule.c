#include "pygobject.h"

PyObject *
_wrap_get_tp_basicsize (PyObject * self,
	   PyObject * args)
{
  PyObject *item;
  item = PyTuple_GetItem(args, 0);

  return PyInt_FromLong(((PyTypeObject*)item)->tp_basicsize);
}

static PyMethodDef testhelper_methods[] = {
    { "get_tp_basicsize", _wrap_get_tp_basicsize, METH_VARARGS },
    { NULL, NULL }
};

void 
inittesthelper ()
{
    Py_InitModule ("testhelper", testhelper_methods);
}


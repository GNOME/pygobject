  /* Introspection Method/function object implementation */

#include "Python.h"
#include <girepository.h>
#include "pygobject-private.h"

    /* ------------------------------- */
    /* Introspected function or method */
    /* ------------------------------- */

typedef struct {
    PyObject_HEAD
    GIFunctionInfo *info;
    PyObject       *m_self; /* can be NULL */
} PyGIFunctionObject;


PyObject *
pyg_ifunction_new(GIFunctionInfo *info, PyObject *self)
{
    PyGIFunctionObject *op;

    op = PyObject_GC_New(PyGIFunctionObject, &PyGIFunction_Type);
    if (op == NULL)
        return NULL;
    op->info = info;
    Py_XINCREF(self);
    op->m_self = self;
    _PyObject_GC_TRACK(op);

    return (PyObject *) op;
}


  /* Most of the code in this function written by Johan Dahlin (stolen from pybank) */
PyObject *
pyg_ifunction_call(PyGIFunctionObject *self, PyObject *args, PyObject *kwargs)
{
    GArgument *in_args;
    GArgument *out_args;
    GArgument return_arg;
    int n_in_args;
    int n_out_args;
    int i;
    PyObject *py_arg;
    GIDirection direction;
    GError *error = NULL;
    GIArgInfo *arg_info;
    PyObject *retval;

    if (kwargs != NULL) {
        PyErr_SetString(PyExc_TypeError, "keyword arguments not yet supported in introspected functions");
        return NULL;
    }
    g_assert(PyTuple_Check(args));
    
    n_in_args = 0;
    n_out_args = 0;
    for (i = 0; i < g_callable_info_get_n_args((GICallableInfo*)self->info); i++) {
	arg_info = g_callable_info_get_arg((GICallableInfo*)self->info, i);
	direction = g_arg_info_get_direction(arg_info);
	if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT)
	    n_in_args++;
	if (direction == GI_DIRECTION_OUT || direction == GI_DIRECTION_INOUT)
	    n_out_args++;
	g_base_info_unref((GIBaseInfo*)arg_info);
    }

    g_message("n_in_args: %i", n_in_args);

    if (PyTuple_GET_SIZE(args) + (self->m_self? 1 : 0) != n_in_args) {
        PyErr_Format(PyExc_TypeError, "invalid number of arguments; expected %i, got %i",
                     n_in_args, PyTuple_GET_SIZE(args) + (self->m_self? 1 : 0));
        return NULL;
    }

    in_args = g_new0(GArgument, n_in_args);
    out_args = g_new0(GArgument, n_out_args);
    for (i = 0; i < g_callable_info_get_n_args((GICallableInfo*)self->info); i++) {
	arg_info = g_callable_info_get_arg((GICallableInfo*)self->info, i);
	direction = g_arg_info_get_direction(arg_info);
	if (direction == GI_DIRECTION_IN || direction == GI_DIRECTION_INOUT) {
            if (self->m_self) {
                if (i == 0)
                    py_arg = self->m_self;
                else
                    py_arg = PyTuple_GetItem(args, i - 1);
            } else
                    py_arg = PyTuple_GetItem(args, i);
            
            if (pyg_argument_from_pyobject(py_arg, arg_info, &in_args[i])) {
                  /* TODO: better error string and cleanup memory */
                PyErr_SetString(PyExc_TypeError, "error converting argument");
                return NULL;
            }
	}
	
	g_base_info_unref((GIBaseInfo*)arg_info);
    }

    if (g_function_info_invoke((GIFunctionInfo*)self->info, 
			       in_args, n_in_args,
			       out_args, n_out_args,
			       &return_arg,
			       &error))
    {
	GITypeInfo *return_info;
	GITypeTag type_tag;

	return_info = g_callable_info_get_return_type
	    ((GICallableInfo*)self->info);

	type_tag = g_type_info_get_tag((GITypeInfo*)return_info);

	if (n_out_args == 1) {
	    retval = pyg_argument_to_pyobject(&return_arg, return_info);
	} else {
	    PyObject *tuple;
	    PyObject *item;
	    int j = 0;
	    int last = 0;
	    int n_args;
	    int start;

	    
	    if (n_out_args > 0) {
		if (g_type_info_get_tag((GITypeInfo*)return_info) != GI_TYPE_TAG_VOID) {
		    item = pyg_argument_to_pyobject(&return_arg, return_info);
		    tuple = PyTuple_New(n_out_args+1);
		    PyTuple_SetItem(tuple, 0, item);
		    start = 1;
		} else {
		    tuple = PyTuple_New(n_out_args);
		    start = 0;
		}
		    
		n_args = g_callable_info_get_n_args((GICallableInfo*)self->info);
		for (i = 0; i < n_out_args; i++) {
		    
		    if (last >= n_args)
			break;
			    
		    for (j = last; j < n_args; j++) {
			arg_info = g_callable_info_get_arg((GICallableInfo*)self->info, j);
			direction = g_arg_info_get_direction(arg_info);
			if (direction != GI_DIRECTION_IN) {
			    GITypeInfo *type_info;
			    type_info = g_arg_info_get_type(arg_info);
			    item = pyg_argument_to_pyobject(&out_args[i], type_info);
                            if (item == NULL) {
                                  /* TODO: better error string and cleanup memory */
                                PyErr_SetString(PyExc_TypeError, "error converting argument");
                                return NULL;
                            }
			    PyTuple_SetItem(tuple, i + start, item);
			    last = j;
			    g_base_info_unref((GIBaseInfo*)type_info);
			}
			g_base_info_unref((GIBaseInfo*)arg_info);
		    }
		}
		retval = tuple;
	    } else {
		retval = pyg_argument_to_pyobject(&return_arg, return_info);
	    }
	}
	g_base_info_unref((GIBaseInfo*)return_info);
    } else {
	PyErr_Format(PyExc_RuntimeError, "Introspection invocation error: %s\n", error->message);
        return NULL;
    }

    return retval;
}

static void
meth_dealloc(PyGIFunctionObject *m)
{
    _PyObject_GC_UNTRACK(m);
    Py_CLEAR(m->m_self);
      /* FIXME: free info? yes or no? */
}

static PyObject *
meth_get__name__(PyGIFunctionObject *m, void *closure)
{
    return PyString_FromString(g_function_info_get_symbol(m->info));
}

static int
meth_traverse(PyGIFunctionObject *m, visitproc visit, void *arg)
{
    int err;
    if (m->m_self != NULL) {
        err = visit(m->m_self, arg);
        if (err)
            return err;
    }
    return 0;
}

static PyObject *
meth_get__self__(PyGIFunctionObject *m, void *closure)
{
    PyObject *self;

    self = m->m_self;
    if (self == NULL)
        self = Py_None;
    Py_INCREF(self);
    return self;
}

static PyGetSetDef meth_getsets [] = {
    {"__name__", (getter)meth_get__name__, NULL, NULL}, 
    {"__self__", (getter)meth_get__self__, NULL, NULL}, 
    {0}
};


static PyObject *
meth_repr(PyGIFunctionObject *m)
{
    g_message("g_function_info_get_symbol(m->info): %s", g_function_info_get_symbol(m->info));
    if (m->m_self == NULL)
        return PyString_FromFormat("<introspected function %s>", 
                                   g_function_info_get_symbol(m->info)?:"(? ? ? )");
    return PyString_FromFormat("<introspected method %s of %s object at %p>", 
                               g_function_info_get_symbol(m->info)?:"(? ? ? )",
                               m->m_self->ob_type->tp_name, 
                               m->m_self);
}

static int
meth_compare(PyGIFunctionObject *a, PyGIFunctionObject *b)
{
    if (a->m_self != b->m_self)
        return (a->m_self < b->m_self) ? -1 : 1;
    if (a->info == b->info)
        return 0;
    if (strcmp(g_function_info_get_symbol(a->info), g_function_info_get_symbol(b->info)) < 0)
        return -1;
    else
        return 1;
}

static long
meth_hash(PyGIFunctionObject *a)
{
    long x, y;
    if (a->m_self == NULL)
        x = 0;
    else {
        x = PyObject_Hash(a->m_self);
        if (x == -1)
            return -1;
    }
    y = _Py_HashPointer((void*)(a->info));
    if (y == -1)
        return -1;
    x ^= y;
    if (x == -1)
        x = -2;
    return x;
}


PyTypeObject PyGIFunction_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0, 
    "introspection_function_or_method", 
    sizeof(PyGIFunctionObject), 
    0, 
    (destructor)meth_dealloc, 		/* tp_dealloc */
    0,					/* tp_print */
    0,					/* tp_getattr */
    0,					/* tp_setattr */
    (cmpfunc)meth_compare,		/* tp_compare */
    (reprfunc)meth_repr,		/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    (hashfunc)meth_hash,		/* tp_hash */
    (ternaryfunc)pyg_ifunction_call,	/* tp_call */
    0,					/* tp_str */
    PyObject_GenericGetAttr,		/* tp_getattro */
    0,					/* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,					/* tp_doc */
    (traverseproc)meth_traverse,	/* tp_traverse */
    0,					/* tp_clear */
    0,					/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    0,					/* tp_iter */
    0,					/* tp_iternext */
    0,					/* tp_methods */
    0,				        /* tp_members */
    meth_getsets,			/* tp_getset */
    0,					/* tp_base */
    0,					/* tp_dict */ 
};


    /* ------------------------------ */
    /* Introspected method descriptor */
    /* ------------------------------ */

typedef struct {
    PyObject_HEAD
    GIFunctionInfo *info;
} PyGIFunctionDescrObject;

PyObject *
pyg_ifunction_descr_new(GIFunctionInfo *info)
{
    PyGIFunctionDescrObject *op;

    op = PyObject_New(PyGIFunctionDescrObject, &PyGIFunctionDescr_Type);
    if (op == NULL)
        return NULL;
    op->info = info;

    return (PyObject *) op;
}

static PyObject *
pyg_ifunction_descr_descr_get(PyGIFunctionDescrObject *descr, PyObject *obj, PyObject *type)
{
    return pyg_ifunction_new(descr->info, obj);
}

static void
pyg_ifunction_descr_dealloc(PyGIFunctionDescrObject *descr)
{
      /* FIXME: free info? yes or no? */
}

static long
pyg_ifunction_descr_hash(PyGIFunctionDescrObject *a)
{
    long x;
    x = _Py_HashPointer((void*)(a->info));
    if (x == -1)
        x = -2;
    return x;
}

PyTypeObject PyGIFunctionDescr_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0, 
    "introspection_function_or_method_descriptor", 
    sizeof(PyGIFunctionDescrObject), 
    0, 
    (destructor)pyg_ifunction_descr_dealloc, /* tp_dealloc */
    0,					/* tp_print */
    0,					/* tp_getattr */
    0,					/* tp_setattr */
    0,					/* tp_compare */
    0,					/* tp_repr */
    0,					/* tp_as_number */
    0,					/* tp_as_sequence */
    0,					/* tp_as_mapping */
    (hashfunc)pyg_ifunction_descr_hash,	/* tp_hash */
    0,					/* tp_call */
    0,					/* tp_str */
    PyObject_GenericGetAttr,		/* tp_getattro */
    0,					/* tp_setattro */
    0,					/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT, 		/* tp_flags */
    0,					/* tp_doc */
    0,					/* tp_traverse */
    0,					/* tp_clear */
    0,					/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    0,					/* tp_iter */
    0,					/* tp_iternext */
    0,					/* tp_methods */
    0,				        /* tp_members */
    0,					/* tp_getset */
    0,					/* tp_base */
    0,					/* tp_dict */ 
    (descrgetfunc)pyg_ifunction_descr_descr_get, /* tp_descr_get */
    0,					/* tp_descr_set */
};

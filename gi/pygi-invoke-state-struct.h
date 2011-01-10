#ifndef __PYGI_INVOKE_STATE_STRUCT_H__
#define __PYGI_INVOKE_STATE_STRUCT_H__

#include <Python.h>

#include <girepository.h>

G_BEGIN_DECLS

typedef struct _PyGIInvokeState
{
    PyObject *py_in_args;
    gint n_py_in_args;

    GIArgument **args;
    GIArgument *in_args;
    GIArgument *out_args;

    GIArgument return_arg;

    GError *error;
} PyGIInvokeState;

G_END_DECLS

#endif

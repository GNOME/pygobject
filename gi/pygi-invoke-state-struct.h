#ifndef __PYGI_INVOKE_STATE_STRUCT_H__
#define __PYGI_INVOKE_STATE_STRUCT_H__

#include <Python.h>

#include <girepository.h>

G_BEGIN_DECLS

typedef struct _PyGIInvokeState
{
    PyObject *py_in_args;
    gssize n_py_in_args;

    GIArgument **args;
    GIArgument *in_args;

    /* Out args and out values
     * In order to pass a parameter and get something back out in C
     * we need to pass a pointer to the value, e.g.
     *    int *out_integer;
     *
     * so while out_args == out_integer, out_value == *out_integer
     * or in other words out_args = &out_values
     *
     * We do all of our processing on out_values but we pass out_args to 
     * the actual function.
     */
    GIArgument *out_args;
    GIArgument *out_values;

    GIArgument return_arg;

    GError *error;
} PyGIInvokeState;

G_END_DECLS

#endif

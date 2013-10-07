#ifndef __PYGI_INVOKE_STATE_STRUCT_H__
#define __PYGI_INVOKE_STATE_STRUCT_H__

#include <Python.h>

#include <girepository.h>

G_BEGIN_DECLS

typedef struct _PyGIInvokeState
{
    PyObject *py_in_args;
    gssize n_py_in_args;
    gssize current_arg;

    GType implementor_gtype;

    GIArgument **args;
    GIArgument *in_args;

    /* Array of pointers allocated to the same length as args which holds from_py
     * marshaler cleanup data.
     */
    gpointer *args_cleanup_data;

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

    gboolean failed;

    gpointer user_data;
} PyGIInvokeState;

G_END_DECLS

#endif

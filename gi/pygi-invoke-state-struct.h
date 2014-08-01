#ifndef __PYGI_INVOKE_STATE_STRUCT_H__
#define __PYGI_INVOKE_STATE_STRUCT_H__

#include <Python.h>

#include <girepository.h>

G_BEGIN_DECLS

typedef struct _PyGIInvokeState
{
    PyObject *py_in_args;
    gssize n_py_in_args;

    /* Number of arguments the ffi wrapped C function takes. Used as the exact
     * count for argument related arrays held in this struct.
     */
    gssize n_args;

    /* List of arguments passed to ffi. Elements can point directly to values held in
     * arg_values for "in/from Python" or indirectly via arg_pointers for
     * "out/inout/to Python". In the latter case, the arg_pointers[x]->v_pointer
     * member points to memory for the value storage.
     */
    GIArgument **args;

    /* Holds memory for the C value of arguments marshaled "to" or "from" Python. */
    GIArgument *arg_values;

    /* Holds pointers to values in arg_values or a caller allocated chunk of
     * memory via arg_pointers[x].v_pointer.
     */
    GIArgument *arg_pointers;

    /* Array of pointers allocated to the same length as args which holds from_py
     * marshaler cleanup data.
     */
    gpointer *args_cleanup_data;

    /* Memory to receive the result of the C ffi function call. */
    GIArgument return_arg;

    /* A GError exception which is indirectly bound into the last position of
     * the "args" array if the callable caches "throws" member is set.
     */
    GError *error;

    gboolean failed;

    gpointer user_data;

    /* Function pointer to call with ffi. */
    gpointer function_ptr;

} PyGIInvokeState;

G_END_DECLS

#endif

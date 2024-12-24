#ifndef __PYGI_INVOKE_STATE_STRUCT_H__
#define __PYGI_INVOKE_STATE_STRUCT_H__

#include <girepository/girepository.h>
#include <pythoncapi_compat.h>

G_BEGIN_DECLS

typedef struct _PyGIInvokeArgState {
    /* Holds memory for the C value of arguments marshaled "to" or "from" Python. */
    GIArgument arg_value;

    /* Holds pointers to values in arg_values or a caller allocated chunk of
     * memory via arg_pointer.v_pointer.
     */
    GIArgument arg_pointer;

    /* Holds from_py marshaler cleanup data. */
    gpointer arg_cleanup_data;

    /* Holds to_py marshaler cleanup data. */
    gpointer to_py_arg_cleanup_data;
} PyGIInvokeArgState;


typedef struct _PyGIInvokeState {
    PyObject *py_in_args;
    gssize n_py_in_args;

    /* Number of arguments the ffi wrapped C function takes. Used as the exact
     * count for argument related arrays held in this struct.
     */
    gssize n_args;

    /* List of arguments passed to ffi. Elements can point directly to values held in
     * arg_values for "in/from Python" or indirectly via arg_pointers for
     * "out/inout/to Python". In the latter case, the args[x].arg_pointer.v_pointer
     * member points to memory for the value storage.
     */
    GIArgument **ffi_args;

    /* Array of size n_args containing per argument state */
    PyGIInvokeArgState *args;

    /* Memory to receive the result of the C ffi function call. */
    GIArgument return_arg;
    gpointer to_py_return_arg_cleanup_data;

    /* A GError exception which is indirectly bound into the last position of
     * the "args" array if the callable caches "throws" member is set.
     */
    GError *error;

    gboolean failed;

    /* An awaitable to return for an async function that was called with
     * default arguments.
     */
    PyObject *py_async;

    gpointer user_data;

    /* Function pointer to call with ffi. */
    gpointer function_ptr;

} PyGIInvokeState;

G_END_DECLS

#endif

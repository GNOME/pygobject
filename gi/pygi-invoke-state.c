#include "pygi-invoke-state.h"

/* We keep track of the topmost state invocation.
 * This allows us to collect all to be freed data in the outermost
 * array of cleanup data.
 */
static GPrivate pygi_invoke_state_private;

/* To reduce calls to g_slice_*() we (1) allocate all the memory depended on
 * the argument count in one go and (2) keep one version per argument count
 * around for faster reuse.
 *
 * NB. This should be removed if we want to support threadless-python.
 */

#define PyGI_INVOKE_ARG_STATE_SIZE(n)                                         \
    (n * (sizeof (PyGIInvokeArgState) + sizeof (GIArgument *)))
#define PyGI_INVOKE_ARG_STATE_N_MAX 10
static gpointer free_arg_state[PyGI_INVOKE_ARG_STATE_N_MAX];

static void
do_clean_up_data (PyGIInvokeStateCleanup *cleanup)
{
    cleanup->notifier (cleanup->data);
}

/**
 * pygi_invoke_state_init:
 * Sets PyGIInvokeState.args and PyGIInvokeState.ffi_args.
 * On error returns FALSE and sets an exception.
 */
gboolean
pygi_invoke_state_init (PyGIInvokeState *state)
{
    PyGIInvokeState *outer_state = g_private_get (&pygi_invoke_state_private);
    gpointer mem;

    if (state->n_args < PyGI_INVOKE_ARG_STATE_N_MAX
        && (mem = free_arg_state[state->n_args]) != NULL) {
        free_arg_state[state->n_args] = NULL;
        memset (mem, 0, PyGI_INVOKE_ARG_STATE_SIZE (state->n_args));
    } else {
        mem = g_malloc0 (PyGI_INVOKE_ARG_STATE_SIZE (state->n_args));
    }

    if (mem == NULL && state->n_args != 0) {
        PyErr_NoMemory ();
        return FALSE;
    }

    if (mem != NULL) {
        state->args = mem;
        state->ffi_args =
            (gpointer)((gchar *)mem
                       + state->n_args * sizeof (PyGIInvokeArgState));
    }

    if (outer_state != NULL) {
        state->cleanup_data = g_array_ref (outer_state->cleanup_data);
    } else {
        g_private_set (&pygi_invoke_state_private, state);

        state->cleanup_data =
            g_array_new (FALSE, TRUE, sizeof (PyGIInvokeStateCleanup));
        g_array_set_clear_func (state->cleanup_data,
                                (GDestroyNotify)do_clean_up_data);
    }
    return TRUE;
}

/**
 * pygi_invoke_state_free:
 * Frees PyGIInvokeState.args and PyGIInvokeState.ffi_args
 */
void
pygi_invoke_state_free (PyGIInvokeState *state)
{
    if (g_private_get (&pygi_invoke_state_private) == state)
        g_private_set (&pygi_invoke_state_private, NULL);

    if (state->cleanup_data) {
        g_clear_pointer (&state->cleanup_data, g_array_unref);
    }

    Py_CLEAR (state->py_in_args);
    Py_CLEAR (state->py_async);

    if (state->n_args < PyGI_INVOKE_ARG_STATE_N_MAX
        && free_arg_state[state->n_args] == NULL) {
        free_arg_state[state->n_args] = state->args;
        return;
    }

    g_free (state->args);
}

void
pygi_invoke_state_add_cleanup_data (PyGIInvokeState *state,
                                    GDestroyNotify notifier, gpointer data)
{
    PyGIInvokeStateCleanup cleanup = { notifier, data };
    g_array_append_val (state->cleanup_data, cleanup);
}

/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2014 Simon Feltman <sfeltman@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "pygi-async.h"
#include "pygi-ccallback.h"
#include "pygi-closure.h"
#include "pygi-cache-private.h"

extern PyObject *_PyGIDefaultArgPlaceholder;

/* _pygi_destroy_notify_dummy:
 *
 * Dummy method used in the occasion when a method has a GDestroyNotify
 * argument without user data.
 */
static void
_pygi_destroy_notify_dummy (gpointer data)
{
}

static gboolean
_pygi_marshal_from_py_interface_callback (PyGIInvokeState *state,
                                          PyGICallableCache *callable_cache,
                                          PyGIArgCache *arg_cache,
                                          PyObject *py_arg, GIArgument *arg,
                                          PyGIMarshalCleanupData *cleanup_data)
{
    GICallableInfo *callable_info;
    PyGICClosure *closure;
    PyGIArgCache *user_data_cache = NULL;
    PyGIArgCache *destroy_cache = NULL;
    PyGICallbackCache *callback_cache;
    PyObject *py_user_data = NULL;

    callback_cache = (PyGICallbackCache *)arg_cache;

    if (py_arg == _PyGIDefaultArgPlaceholder) {
        /* We need to have an async to "marshal" instead in this case. */
        if (!state->py_async) return FALSE;

        if (callback_cache->user_data_index <= 0) return FALSE;

        user_data_cache = _pygi_callable_cache_get_arg (
            callable_cache, callback_cache->user_data_index);

        Py_INCREF (state->py_async);
        arg->v_pointer = pygi_async_finish_cb;
        state->args[user_data_cache->c_arg_index].arg_value.v_pointer =
            state->py_async;

        return TRUE;
    }

    if (callback_cache->has_user_data && callback_cache->user_data_index > 0) {
        user_data_cache = _pygi_callable_cache_get_arg (
            callable_cache, callback_cache->user_data_index);
        if (user_data_cache->py_arg_index < state->n_py_in_args) {
            /* py_user_data is a borrowed reference. */
            py_user_data = PyTuple_GetItem (state->py_in_args,
                                            user_data_cache->py_arg_index);
            if (!py_user_data) return FALSE;
            /* NULL out user_data if it was not supplied and the default arg placeholder
             * was used instead.
             */
            if (py_user_data == _PyGIDefaultArgPlaceholder) {
                py_user_data = NULL;
            } else if (callable_cache->user_data_varargs_arg == NULL) {
                /* For non-variable length user data, place the user data in a
                 * single item tuple which is concatenated to the callbacks arguments.
                 * This allows callback input arg marshaling to always expect a
                 * tuple for user data. Note the
                 */
                py_user_data = Py_BuildValue ("(O)", py_user_data, NULL);
            } else {
                /* increment the ref borrowed from PyTuple_GetItem above */
                Py_INCREF (py_user_data);
            }
        }
    }

    if (Py_IsNone (py_arg)) {
        return TRUE;
    }

    if (!PyCallable_Check (py_arg)) {
        PyErr_Format (PyExc_TypeError,
                      "Callback needs to be a function or method not %s",
                      Py_TYPE (py_arg)->tp_name);

        return FALSE;
    }

    callable_info = (GICallableInfo *)callback_cache->interface_info;

    closure = _pygi_make_native_closure (
        callable_info, callback_cache->closure_cache, callback_cache->scope,
        py_arg, py_user_data);

    if (closure->closure != NULL)
        arg->v_pointer = gi_callable_info_get_closure_native_address (
            callable_info, closure->closure);
    else
        arg->v_pointer = NULL;

    /* always decref the user data as _pygi_make_native_closure adds its own ref */
    Py_XDECREF (py_user_data);

    /* The PyGICClosure instance is used as user data passed into the C function.
     * The return trip to python will marshal this back and pull the python user data out.
     */
    if (user_data_cache != NULL) {
        state->args[user_data_cache->c_arg_index].arg_value.v_pointer =
            closure;
    }

    /* Setup a GDestroyNotify callback if this method supports it along with
     * a user data field. The user data field is a requirement in order
     * free resources and ref counts associated with this arguments closure.
     * In case a user data field is not available, show a warning giving
     * explicit information and setup a dummy notification to avoid a crash
     * later on in _pygi_destroy_notify_callback_closure.
     */
    if (callback_cache->has_destroy_notify
        && callback_cache->destroy_notify_index > 0) {
        destroy_cache = _pygi_callable_cache_get_arg (
            callable_cache, callback_cache->destroy_notify_index);
    }

    if (destroy_cache) {
        if (user_data_cache != NULL) {
            state->args[destroy_cache->c_arg_index].arg_value.v_pointer =
                _pygi_invoke_closure_free;
        } else {
            char *full_name =
                pygi_callable_cache_get_full_name (callable_cache);
            gchar *msg = g_strdup_printf (
                "Callables passed to %s will leak references because "
                "the method does not support a user_data argument. "
                "See: https://bugzilla.gnome.org/show_bug.cgi?id=685598",
                full_name);
            g_free (full_name);
            if (PyErr_WarnEx (PyExc_RuntimeWarning, msg, 2)) {
                g_free (msg);
                _pygi_invoke_closure_free (closure);
                return FALSE;
            }
            g_free (msg);
            state->args[destroy_cache->c_arg_index].arg_value.v_pointer =
                _pygi_destroy_notify_dummy;
        }
    }

    /* Use the PyGIClosure as data passed to cleanup for GI_SCOPE_TYPE_CALL. */
    if (callback_cache->scope == GI_SCOPE_TYPE_CALL) {
        cleanup_data->data = closure;
        cleanup_data->destroy = (GDestroyNotify)_pygi_invoke_closure_free;
    }
    return TRUE;
}

static PyObject *
_pygi_marshal_to_py_interface_callback (
    PyGIInvokeState *state, PyGICallableCache *callable_cache,
    PyGIArgCache *arg_cache, GIArgument *arg,
    PyGIMarshalCleanupData *arg_cleanup_data)
{
    PyGICallbackCache *callback_cache = (PyGICallbackCache *)arg_cache;
    gpointer user_data = NULL;
    GDestroyNotify destroy_notify = NULL;

    if (callback_cache->has_user_data)
        user_data =
            state->args[callback_cache->user_data_index].arg_value.v_pointer;

    if (callback_cache->has_destroy_notify)
        destroy_notify = state->args[callback_cache->destroy_notify_index]
                             .arg_value.v_pointer;

    return _pygi_ccallback_new (
        arg->v_pointer, user_data, callback_cache->scope,
        GI_CALLABLE_INFO (callback_cache->interface_info), destroy_notify);
}

static void
_pygi_marshal_cleanup_from_py_interface_callback (PyGIInvokeState *state,
                                                  PyGIMarshalCleanupData data)
{
    pygi_marshal_cleanup_data_destroy (&data);
}

static void
_callback_cache_free_func (PyGICallbackCache *cache)
{
    if (cache != NULL) {
        if (cache->interface_info != NULL)
            gi_base_info_unref ((GIBaseInfo *)cache->interface_info);

        if (cache->closure_cache != NULL) {
            pygi_callable_cache_free (
                (PyGICallableCache *)cache->closure_cache);
            cache->closure_cache = NULL;
        }

        g_slice_free (PyGICallbackCache, cache);
    }
}

PyGIArgCache *
pygi_arg_callback_new_from_info (GITypeInfo *type_info,
                                 GIArgInfo *arg_info, /* may be null */
                                 GITransfer transfer, PyGIDirection direction,
                                 GICallbackInfo *iface_info,
                                 PyGICallableCache *callable_cache)
{
    PyGICallbackCache *callback_cache;
    PyGIArgCache *arg_cache;

    callback_cache = g_slice_new0 (PyGICallbackCache);
    if (callback_cache == NULL) return NULL;

    arg_cache = (PyGIArgCache *)callback_cache;
    unsigned int child_offset = 0;

    pygi_arg_base_setup ((PyGIArgCache *)arg_cache, type_info, arg_info,
                         transfer, direction);

    if (callable_cache != NULL) child_offset = callable_cache->args_offset;

    ((PyGIArgCache *)arg_cache)->destroy_notify =
        (GDestroyNotify)_callback_cache_free_func;

    callback_cache->has_user_data =
        arg_info != NULL
        && gi_arg_info_get_closure_index (arg_info,
                                          &callback_cache->user_data_index);
    if (callback_cache->has_user_data)
        callback_cache->user_data_index += child_offset;

    callback_cache->has_destroy_notify =
        arg_info != NULL
        && gi_arg_info_get_destroy_index (
            arg_info, &callback_cache->destroy_notify_index);
    if (callback_cache->has_destroy_notify)
        callback_cache->destroy_notify_index += child_offset;

    if (callback_cache->has_user_data) {
        PyGIArgCache *user_data_arg_cache = pygi_arg_cache_alloc ();
        user_data_arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD_WITH_PYARG;
        user_data_arg_cache->direction = direction;
        _pygi_callable_cache_set_arg (callable_cache,
                                      callback_cache->user_data_index,
                                      user_data_arg_cache);
    }

    if (callback_cache->has_destroy_notify) {
        PyGIArgCache *destroy_arg_cache = pygi_arg_cache_alloc ();
        destroy_arg_cache->meta_type = PYGI_META_ARG_TYPE_CHILD;
        destroy_arg_cache->direction = direction;
        _pygi_callable_cache_set_arg (callable_cache,
                                      callback_cache->destroy_notify_index,
                                      destroy_arg_cache);
    }

    callback_cache->scope = arg_info != NULL ? gi_arg_info_get_scope (arg_info)
                                             : GI_SCOPE_TYPE_INVALID;
    gi_base_info_ref (iface_info);
    callback_cache->interface_info = GI_BASE_INFO (iface_info);

    if (direction & PYGI_DIRECTION_FROM_PYTHON) {
        callback_cache->closure_cache = pygi_closure_cache_new (
            GI_CALLABLE_INFO (callback_cache->interface_info));
        arg_cache->from_py_marshaller =
            _pygi_marshal_from_py_interface_callback;
        arg_cache->from_py_cleanup =
            _pygi_marshal_cleanup_from_py_interface_callback;

        if (callback_cache->scope == GI_SCOPE_TYPE_ASYNC)
            callback_cache->arg_cache.async_context =
                PYGI_ASYNC_CONTEXT_CALLBACK;
    }

    if (direction & PYGI_DIRECTION_TO_PYTHON) {
        callback_cache->arg_cache.to_py_marshaller =
            _pygi_marshal_to_py_interface_callback;
    }

    return (PyGIArgCache *)callback_cache;
}

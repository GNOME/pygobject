/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2015 Christoph Reiter <reiter.christoph@gmail.com>
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

#include <Python.h>
#include <structmember.h>
#include <glib.h>
#include "gimodule.h"
#include "pygobject-object.h"
#include "pygboxed.h"
#include "pygi-async.h"
#include "pygi-util.h"
#include "pygi-info.h"
#include "pygi-invoke.h"


static PyObject *asyncio_InvalidStateError;
static PyObject *asyncio_get_running_loop;
#if defined(PYPY_VERSION)
static PyObject *contextvars_copy_context;
#endif
static PyObject *cancellable_info;

/* This is never instantiated. */
PYGI_DEFINE_TYPE ("gi._gi.Async", PyGIAsync_Type, PyGIAsync)

/**
 * Async.__repr__() implementation.
 * Takes the _Async.__repr_format format string and applies the finish function
 * info to it.
 */
static PyObject*
async_repr(PyGIAsync *self) {
    PyObject *string;
    char *func_descr;

    func_descr = _pygi_g_base_info_get_fullname (self->finish_func->base.info);

    string = PyUnicode_FromFormat ("%s(finish_func=%s, done=%s)",
                                   Py_TYPE(self)->tp_name,
                                   func_descr,
                                   (self->result || self->exception) ? "True" : "False");

    g_free (func_descr);

    return string;
}

/**
 * async_cancel:
 *
 * Cancel the asynchronous operation.
 */
static PyObject *
async_cancel(PyGIAsync *self) {

    return PyObject_CallMethod (self->cancellable, "cancel", NULL);
}

static PyObject *
async_done(PyGIAsync *self) {

    return PyBool_FromLong (self->result || self->exception);
}

static PyObject *
async_result(PyGIAsync *self) {

    if (!self->result && !self->exception) {
        PyErr_SetString(asyncio_InvalidStateError, "Async task is still running!");
        return NULL;
    }

    self->log_tb = FALSE;

    if (self->result) {
        Py_INCREF (self->result);
        return self->result;
    } else {
        PyErr_SetObject(PyExceptionInstance_Class(self->exception), self->exception);
        return NULL;
    }
}

static PyObject *
async_exception(PyGIAsync *self) {

    PyObject *res;

    if (!self->result && !self->exception) {
        PyErr_SetString(asyncio_InvalidStateError, "Async task is still running!");
        return NULL;
    }

    if (self->exception)
        res =self->exception;
    else
        res = Py_None;

    self->log_tb = FALSE;

    Py_INCREF (res);
    return res;
}

static void
call_cb (PyGIAsync *self, PyGIAsyncCallback *cb)
{
    PyObject *func;
    PyObject *args;
    PyObject *res;

    if (cb->context) {
        func = PyObject_GetAttrString (cb->context, "run");
        args = Py_BuildValue ("(OO)", cb->func, self);
    } else {
        func = cb->func;
        Py_INCREF(func);
        args = Py_BuildValue ("(O)", self);
    }

    res = PyObject_Call (func, args, NULL);

    Py_CLEAR (args);
    Py_CLEAR (func);

    if (!res) {
        /* Should we clear the error here? */
        PyErr_Print();
        PyErr_Clear();
    } else {
        Py_DECREF(res);
    }
}

static void
call_callbacks (PyGIAsync *self)
{
    GArray *callbacks = self->callbacks;
    guint i;

    if (callbacks == NULL)
        return;

    /*
     * A new callback might be added from a callback
     * (which will only be called in the next main context iteration)
     */
    self->callbacks = NULL;

    for (i = 0; i < callbacks->len; i++) {
        PyGIAsyncCallback *cb = &g_array_index (callbacks, PyGIAsyncCallback, i);
        call_cb (self, cb);

        Py_DECREF(cb->func);
        Py_DECREF(cb->context);
    }

    g_array_free(callbacks, TRUE);
}

static gboolean
idle_call_done_callbacks (void *data)
{
    PyGILState_STATE py_state;
    PyGIAsync *self = data;

    py_state = PyGILState_Ensure ();

    call_callbacks (self);

    PyGILState_Release (py_state);

    return G_SOURCE_REMOVE;
}

static PyObject*
async_add_done_callback (PyGIAsync *self, PyObject *args, PyObject *kwargs)
{
    PyGIAsyncCallback callback = { NULL };

    static char * kwlist[] = {"", "context", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|$O:add_done_callback", kwlist,
                                     &callback.func, &callback.context))
        return NULL;

    Py_INCREF(callback.func);
    if (callback.context == NULL)
#ifndef PYPY_VERSION
        callback.context = PyContext_CopyCurrent ();
#else
        callback.context = PyObject_CallObject (contextvars_copy_context, NULL);
#endif
    else
        Py_INCREF(callback.context);

    /* If callbacks is NULL then we need a (new) idle handler to process them. */
    if (!self->callbacks && (self->result || self->exception)) {
        GSource *idle_source;

        Py_INCREF(self);

        idle_source = g_idle_source_new ();
        g_source_set_name (idle_source, "gi.Async idle");
        g_source_attach (idle_source, self->ctx);
        g_source_set_callback(idle_source, idle_call_done_callbacks, self, pyg_destroy_notify);
    }

    if (!self->callbacks)
        self->callbacks = g_array_new (TRUE, TRUE, sizeof (PyGIAsyncCallback));

    g_array_append_val (self->callbacks, callback);

    Py_RETURN_NONE;
}

static PyObject*
async_remove_done_callback (PyGIAsync *self, PyObject *fn)
{
    guint i = 0;
    gssize removed = 0;

    while (self->callbacks && i < self->callbacks->len) {
        PyGIAsyncCallback *cb = &g_array_index (self->callbacks, PyGIAsyncCallback, i);

        if (PyObject_RichCompareBool (cb->func, fn, Py_EQ) == 1) {
            Py_DECREF(cb->func);
            Py_DECREF(cb->context);

            removed += 1;
            g_array_remove_index (self->callbacks, i);
        } else {
            i += 1;
        }
    }

    return PyLong_FromSsize_t(removed);
}

static int
async_init(PyGIAsync *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "finish_func", "cancellable", NULL };
    static gboolean warned_loop_wrong_context = FALSE;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|O!$:gi._gi.Async.__init__", kwlist,
                                     &PyGICallableInfo_Type, &self->finish_func,
                                     &PyGObject_Type, &self->cancellable))
        return -1;

    Py_INCREF(self->finish_func);

    /* We need to pull in Gio.Cancellable at some point, but we delay it
     * until really needed to avoid having a dependency.
     */
    if (G_UNLIKELY (!cancellable_info)) {
        PyObject *gio;

        gio = PyImport_ImportModule("gi.repository.Gio");
        if (gio == NULL)
            return -1;

        cancellable_info = PyObject_GetAttrString(gio, "Cancellable");
        Py_DECREF(gio);

        if (!cancellable_info)
            return -1;
    }

    if (self->cancellable) {
        int res;

        Py_INCREF (self->cancellable);

        res = PyObject_IsInstance (self->cancellable, cancellable_info);
        if (res == -1)
            return -1;

        if (res == 0) {
            PyErr_SetString (PyExc_TypeError, "cancellable argument needs to be of type Gio.Cancellable");
            return -1;
        }
    } else {
        self->cancellable = PyObject_CallObject (cancellable_info, NULL);
    }

    /* We use g_main_context_ref_thread_default() here, as that is what GTask
     * does. We then only allow creating an awaitable if either there is no
     * python EventLoop, or, if there is, it has to be running and iterating
     * the same context.
     *
     * The point of not requiring a mainloop is to allow custom mini-mainloops
     * that are not real python EventLoop implementations.
     *
     * NOTE: This is a bit backward. Instead, it would make more sense to just
     * fetch the current EventLoop object for the ref'ed GMainContext.
     * Python will then do the rest and ensure it is not awaited from the wrong
     * EventLoop.
     */
    self->ctx = g_main_context_ref_thread_default ();
    if (!self->ctx)
        return -1;

    /* asyncio.get_running_loop raising an exception is bad, just error out */
    self->loop = PyObject_CallObject (asyncio_get_running_loop, NULL);
    if (!self->loop)
        return -1;

    if (self->loop && Py_IsNone(self->loop))
        Py_CLEAR(self->loop);

    if (self->loop) {
        PyObject *context = NULL;

        /* Duck-type the running loop. It should have a _context attribute. */
        context = PyObject_GetAttrString (self->loop, "_context");
        if (context == NULL) {
            Py_XDECREF (self->loop);
            self->loop = NULL;

            return -1;
        }

        /*
         * Pointing to the same main context
         *
         * One *could* possibly define a good behaviour here. However, if the
         * loop does not match the current main context, then we always need to
         * call the completion callback from an idle handler.
         */
        if (!pyg_boxed_check (context, G_TYPE_MAIN_CONTEXT) ||
            pyg_boxed_get_ptr (context) != self->ctx) {

            Py_XDECREF (context);
            Py_XDECREF (self->loop);
            self->loop = NULL;

            if (!warned_loop_wrong_context) {
                warned_loop_wrong_context = TRUE;
                PyErr_SetString (PyExc_AssertionError,
                                 "Currently running EventLoop is iterating a different GMainContext. This should probably not happen.");
            }
            return -1;
        }

        Py_XDECREF (context);
    }

    return 0;
}

static PyMethodDef async_methods[] = {
    {"cancel", (PyCFunction)async_cancel, METH_NOARGS},
    {"done", (PyCFunction)async_done, METH_NOARGS},
    {"result", (PyCFunction)async_result, METH_NOARGS},
    {"exception", (PyCFunction)async_exception, METH_NOARGS},
    {"add_done_callback", (PyCFunction)async_add_done_callback, METH_VARARGS | METH_KEYWORDS},
    {"remove_done_callback", (PyCFunction)async_remove_done_callback, METH_O},
    {NULL, NULL, 0},
};

static PyObject *
async_await (PyGIAsync *self) {
    /* We return ourselves as iterator. This is legal in principle, but we
     * don't stop iterating after one item and just continue indefinately,
     * meaning that certain errors cannot be caught!
     */

    if (!self->result && !self->exception)
        self->_asyncio_future_blocking = TRUE;

    Py_INCREF (self);
    return (PyObject *) self;
}

static PyObject *
async_iternext (PyGIAsync *self) {
    /* Return ourselves if iteration needs to continue. */
    if (!self->result && !self->exception) {
        Py_INCREF (self);
        return (PyObject *) self;
    }

    if (self->exception) {
        PyErr_SetObject(PyExceptionInstance_Class(self->exception), self->exception);
        return NULL;
    } else {
        PyObject *e;

        e = PyObject_CallFunctionObjArgs(PyExc_StopIteration, self->result, NULL);
        if (e == NULL)
            return NULL;

        PyErr_SetObject (PyExc_StopIteration, e);
        Py_DECREF (e);
        return NULL;
    }
}

static PyAsyncMethods async_async_methods = {
    .am_await = (unaryfunc) async_await,
};

static void
async_finalize(PyGIAsync *self)
{
    if (self->log_tb) {
        PyObject *error_type, *error_value, *error_traceback;
        PyObject *context = NULL;
        PyObject *message = NULL;
        PyObject *call_exception_handler = NULL;
        PyObject *res = NULL;

        assert(self->exception != NULL);
        self->log_tb = 0;

        /* Save the current exception, if any. */
        PyErr_Fetch(&error_type, &error_value, &error_traceback);

        context = PyDict_New();
        if (!context)
            goto finally;

        message = PyUnicode_FromFormat(
            "%s exception was never retrieved", Py_TYPE(self)->tp_name);
        if (!message)
            goto finally;

        /*
         * If we do not have an EventLoop, then we cannot report it properly.
         * So, nest it inside an AssertionError and report it to sys.excepthook.
         */
        if (!self->loop) {
            PyObject *wrapper;

            wrapper = PyObject_CallFunctionObjArgs(PyExc_AssertionError, message, NULL);
            PyException_SetCause(wrapper, Py_NewRef(self->exception));

            PyErr_Restore(Py_NewRef(PyExc_AssertionError), wrapper, NULL);
            PyErr_PrintEx(0);

            goto finally;
        }

        if (PyDict_SetItemString(context, "message", message) < 0 ||
            PyDict_SetItemString(context, "exception", self->exception) < 0 ||
            PyDict_SetItemString(context, "future", (PyObject*) self) < 0)
            goto finally;

        call_exception_handler = PyObject_GetAttrString(self->loop, "call_exception_handler");
        if (!call_exception_handler)
            goto finally;

        res = PyObject_CallFunction(call_exception_handler, "(O)", context);
        if (res == NULL)
            PyErr_WriteUnraisable(context);

finally:
        Py_CLEAR (res);
        Py_CLEAR (context);
        Py_CLEAR (message);
        Py_CLEAR (call_exception_handler);

        /* Restore the saved exception. */
        PyErr_Restore(error_type, error_value, error_traceback);
    }

    Py_CLEAR(self->loop);
    Py_CLEAR(self->finish_func);
    if (self->cancellable)
        Py_CLEAR(self->cancellable);
    if (self->result)
        Py_CLEAR(self->result);
    if (self->exception)
        Py_CLEAR(self->exception);

    g_main_context_unref (self->ctx);

    /* Precaution, cannot happen */
    if (self->callbacks)
        g_array_free (self->callbacks, TRUE);
}

static void
async_dealloc(PyGIAsync *self)
{
#ifndef PYPY_VERSION
    /* The finalizer might resurrect the object */
    if (PyObject_CallFinalizerFromDealloc((PyObject *)self) < 0)
        return;
#endif

    Py_TYPE(self)->tp_free((PyObject *)self);
}

void
pygi_async_finish_cb (GObject *source_object, gpointer res, PyGIAsync *self)
{
    PyGILState_STATE py_state;
    PyObject *source_pyobj, *res_pyobj, *args;
    PyObject *ret;

    py_state = PyGILState_Ensure ();

    /* We might still be called at shutdown time. */
    if (!Py_IsInitialized()) {
        PyGILState_Release (py_state);
        return;
    }

    res_pyobj = pygobject_new_full (res, FALSE, NULL);
    if (source_object) {
        source_pyobj = pygobject_new_full (source_object, FALSE, NULL);
        args = Py_BuildValue ("(OO)", source_pyobj, res_pyobj);
    } else {
        source_pyobj = NULL;
        args = Py_BuildValue ("(O)", res_pyobj);
    }
    ret = _wrap_g_callable_info_invoke ((PyGIBaseInfo *) self->finish_func, args, NULL);
    Py_XDECREF (res_pyobj);
    Py_XDECREF (source_pyobj);
    Py_XDECREF (args);

    if (PyErr_Occurred ()) {
        PyObject *exc = NULL, *value = NULL, *traceback = NULL;

        /* NOTE: cPython >=3.12 has PyErr_{Get,Set}RaisedException */
        PyErr_Fetch (&exc, &value, &traceback);
        PyErr_NormalizeException(&exc, &value, &traceback);

        self->exception = value;
        self->log_tb = TRUE;

        Py_XDECREF (exc);
        Py_XDECREF (traceback);
        Py_XDECREF (ret);
    } else {
        self->result = ret;
    }

    /*
     * NOTE: This assumes that the async API is well-behaved.
     * Any GTask using API will be, but theoretically there may be API that
     * does not guarante that we are called in a later iteration of the main
     * context we were stared from.
     */
    call_callbacks (self);

    Py_DECREF (self);
    PyGILState_Release (py_state);
}

/**
 * pygi_async_new:
 * @finish_func: A #GIFunctionInfo to wrap that is used to finish.
 * @cancellable: A #PyObject containging a #GCancellable, or None
 *
 * Return a new async instance.
 *
 * Returns: An instance of gi.Async or %NULL on error.
 */
PyObject *
pygi_async_new(PyObject *finish_func, PyObject *cancellable) {

    PyObject *res;
    PyObject *args;

    res = PyGIAsync_Type.tp_alloc (&PyGIAsync_Type, 0);

    if (res) {
        if (cancellable && cancellable != Py_None)
            args = Py_BuildValue ("(OO)", finish_func, cancellable);
        else
            args = Py_BuildValue ("(O)", finish_func);

        if (PyGIAsync_Type.tp_init (res, args, NULL) < 0) {
            Py_DECREF (args);
            Py_DECREF (res);

            /* Dump error from initializer */
            PyErr_Print ();

            return NULL;
        }

        Py_DECREF (args);
    }

    return res;
}

static struct PyMemberDef async_members[] = {
    {
        "_asyncio_future_blocking",
        T_BOOL,
        offsetof(PyGIAsync, _asyncio_future_blocking),
        0,
        NULL
    },
    {
        "_loop",
        T_OBJECT,
        offsetof(PyGIAsync, loop),
        READONLY,
        NULL
    },
    {
        "_finish_func",
        T_OBJECT,
        offsetof(PyGIAsync, finish_func),
        READONLY,
        NULL
    },
    {
        "cancellable",
        T_OBJECT,
        offsetof(PyGIAsync, cancellable),
        READONLY,
        "The Gio.Cancellable associated with the task."
    },
    { NULL, }
};


/**
 * pygi_async_register_types:
 * @module: A Python modules to which Async gets added to.
 *
 * Initializes the Async class and adds it to the passed @module.
 *
 * Returns: -1 on error, 0 on success.
 */
int pygi_async_register_types(PyObject *module) {
    PyObject *asyncio = NULL;
#ifdef PYPY_VERSION
    PyObject *contextvars = NULL;
#endif

#ifndef PYPY_VERSION
    PyGIAsync_Type.tp_finalize = (destructor)async_finalize;
#else
    PyGIAsync_Type.tp_del = (destructor)async_finalize;
#endif
    PyGIAsync_Type.tp_dealloc = (destructor)async_dealloc;
    PyGIAsync_Type.tp_repr = (reprfunc)async_repr;
    PyGIAsync_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_FINALIZE;
    PyGIAsync_Type.tp_methods = async_methods;
    PyGIAsync_Type.tp_members = async_members;
    PyGIAsync_Type.tp_as_async = &async_async_methods;
    PyGIAsync_Type.tp_iternext = (iternextfunc) &async_iternext;
    PyGIAsync_Type.tp_init = (initproc)async_init;
    PyGIAsync_Type.tp_new = PyType_GenericNew;

    if (PyType_Ready (&PyGIAsync_Type) < 0)
        return -1;

    Py_INCREF (&PyGIAsync_Type);
    if (PyModule_AddObject (module, "Async",
                            (PyObject *)&PyGIAsync_Type) < 0) {
        Py_DECREF (&PyGIAsync_Type);
        return -1;
    }

    asyncio = PyImport_ImportModule("asyncio");
    if (asyncio == NULL) {
        goto fail;
    }
    asyncio_InvalidStateError = PyObject_GetAttrString(asyncio, "InvalidStateError");
    if (asyncio_InvalidStateError == NULL)
        goto fail;

    asyncio_get_running_loop = PyObject_GetAttrString(asyncio, "_get_running_loop");
    if (asyncio_get_running_loop == NULL)
        goto fail;

#if defined(PYPY_VERSION)
    contextvars = PyImport_ImportModule("contextvars");
    if (asyncio == NULL) {
        goto fail;
    }

    contextvars_copy_context = PyObject_GetAttrString(contextvars, "copy_context");
    if (contextvars_copy_context == NULL)
        goto fail;
#endif

    /* Only initialized when really needed! */
    cancellable_info = NULL;

    Py_CLEAR(asyncio);
    return 0;

fail:
    Py_CLEAR(asyncio);
    return -1;
}

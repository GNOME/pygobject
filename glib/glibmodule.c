/* -*- Mode: C; c-set-style: python; c-basic-offset: 4  -*-
 * pyglib - Python bindings for GLib toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *               2004-2008  Johan Dahlin
 *
 *   glibmodule.c: wrapper for the glib library.
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Python.h>
#include <glib.h>
#include "pyglib.h"

#include "pygspawn.h"
#include "pyglib-private.h"

#define PYGLIB_MAJOR_VERSION PYGOBJECT_MAJOR_VERSION
#define PYGLIB_MINOR_VERSION PYGOBJECT_MINOR_VERSION
#define PYGLIB_MICRO_VERSION PYGOBJECT_MICRO_VERSION

/**
 * pyg_destroy_notify:
 * @user_data: a PyObject pointer.
 *
 * A function that can be used as a GDestroyNotify callback that will
 * call Py_DECREF on the data.
 */
void
pyg_destroy_notify(gpointer user_data)
{
    PyObject *obj = (PyObject *)user_data;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();
    Py_DECREF(obj);
    pyglib_gil_state_release(state);
}



/* ---------------- glib module functions -------------------- */

static gint
get_handler_priority(gint *priority, PyObject *kwargs)
{
    Py_ssize_t len, pos;
    PyObject *key, *val;

    /* no keyword args? leave as default */
    if (kwargs == NULL)	return 0;

    len = PyDict_Size(kwargs);
    if (len == 0) return 0;

    if (len != 1) {
	PyErr_SetString(PyExc_TypeError,
			"expecting at most one keyword argument");
	return -1;
    }
    pos = 0;
    PyDict_Next(kwargs, &pos, &key, &val);
    if (!PyString_Check(key)) {
	PyErr_SetString(PyExc_TypeError,
			"keyword argument name is not a string");
	return -1;
    }

    if (strcmp(PyString_AsString(key), "priority") != 0) {
	PyErr_SetString(PyExc_TypeError,
			"only 'priority' keyword argument accepted");
	return -1;
    }

    *priority = PyInt_AsLong(val);
    if (PyErr_Occurred()) {
	PyErr_Clear();
	PyErr_SetString(PyExc_ValueError, "could not get priority value");
	return -1;
    }
    return 0;
}

gboolean
pyg_handler_marshal(gpointer user_data)
{
    PyObject *tuple, *ret;
    gboolean res;
    PyGILState_STATE state;

    g_return_val_if_fail(user_data != NULL, FALSE);

    state = pyglib_gil_state_ensure();

    tuple = (PyObject *)user_data;
    ret = PyObject_CallObject(PyTuple_GetItem(tuple, 0),
			      PyTuple_GetItem(tuple, 1));
    if (!ret) {
	PyErr_Print();
	res = FALSE;
    } else {
	res = PyObject_IsTrue(ret);
	Py_DECREF(ret);
    }
    
    pyglib_gil_state_release(state);

    return res;
}

static PyObject *
pyg_idle_add(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *first, *callback, *cbargs = NULL, *data;
    gint len, priority = G_PRIORITY_DEFAULT_IDLE;
    guint handler_id;

    len = PyTuple_Size(args);
    if (len < 1) {
	PyErr_SetString(PyExc_TypeError,
			"idle_add requires at least 1 argument");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 1);
    if (!PyArg_ParseTuple(first, "O:idle_add", &callback)) {
	Py_DECREF(first);
        return NULL;
    }
    Py_DECREF(first);
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "first argument not callable");
        return NULL;
    }
    if (get_handler_priority(&priority, kwargs) < 0)
	return NULL;

    cbargs = PySequence_GetSlice(args, 1, len);
    if (cbargs == NULL)
	return NULL;

    data = Py_BuildValue("(ON)", callback, cbargs);
    if (data == NULL)
	return NULL;
    handler_id = g_idle_add_full(priority, pyg_handler_marshal, data,
				 pyg_destroy_notify);
    return PyInt_FromLong(handler_id);
}


static PyObject *
pyg_timeout_add(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *first, *callback, *cbargs = NULL, *data;
    gint len, priority = G_PRIORITY_DEFAULT;
    guint interval, handler_id;

    len = PyTuple_Size(args);
    if (len < 2) {
	PyErr_SetString(PyExc_TypeError,
			"timeout_add requires at least 2 args");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 2);
    if (!PyArg_ParseTuple(first, "IO:timeout_add", &interval, &callback)) {
	Py_DECREF(first);
        return NULL;
    }
    Py_DECREF(first);
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "second argument not callable");
        return NULL;
    }
    if (get_handler_priority(&priority, kwargs) < 0)
	return NULL;

    cbargs = PySequence_GetSlice(args, 2, len);
    if (cbargs == NULL)
	return NULL;

    data = Py_BuildValue("(ON)", callback, cbargs);
    if (data == NULL)
	return NULL;
    handler_id = g_timeout_add_full(priority, interval,
				    pyg_handler_marshal, data,
				    pyg_destroy_notify);
    return PyInt_FromLong(handler_id);
}

static PyObject *
pyg_timeout_add_seconds(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *first, *callback, *cbargs = NULL, *data;
    gint len, priority = G_PRIORITY_DEFAULT;
    guint interval, handler_id;

    len = PyTuple_Size(args);
    if (len < 2) {
	PyErr_SetString(PyExc_TypeError,
			"timeout_add_seconds requires at least 2 args");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 2);
    if (!PyArg_ParseTuple(first, "IO:timeout_add_seconds", &interval, &callback)) {
	Py_DECREF(first);
        return NULL;
    }
    Py_DECREF(first);
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "second argument not callable");
        return NULL;
    }
    if (get_handler_priority(&priority, kwargs) < 0)
	return NULL;

    cbargs = PySequence_GetSlice(args, 2, len);
    if (cbargs == NULL)
	return NULL;

    data = Py_BuildValue("(ON)", callback, cbargs);
    if (data == NULL)
	return NULL;
    handler_id = g_timeout_add_seconds_full(priority, interval,
                                            pyg_handler_marshal, data,
                                            pyg_destroy_notify);
    return PyInt_FromLong(handler_id);
}

static gboolean
iowatch_marshal(GIOChannel *source,
		GIOCondition condition,
		gpointer user_data)
{
    PyGILState_STATE state;
    PyObject *tuple, *func, *firstargs, *args, *ret;
    gboolean res;

    g_return_val_if_fail(user_data != NULL, FALSE);

    state = pyglib_gil_state_ensure();

    tuple = (PyObject *)user_data;
    func = PyTuple_GetItem(tuple, 0);

    /* arg vector is (fd, condtion, *args) */
    firstargs = Py_BuildValue("(Oi)", PyTuple_GetItem(tuple, 1), condition);
    args = PySequence_Concat(firstargs, PyTuple_GetItem(tuple, 2));
    Py_DECREF(firstargs);

    ret = PyObject_CallObject(func, args);
    Py_DECREF(args);
    if (!ret) {
	PyErr_Print();
	res = FALSE;
    } else {
        if (ret == Py_None) {
            if (PyErr_Warn(PyExc_Warning, "glib.io_add_watch callback returned None;"
                           " should return True/False")) {
                PyErr_Print();
            }
        }
	res = PyObject_IsTrue(ret);
	Py_DECREF(ret);
    }

    pyglib_gil_state_release(state);

    return res;
}

static PyObject *
pyg_io_add_watch(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *first, *pyfd, *callback, *cbargs = NULL, *data;
    gint fd, priority = G_PRIORITY_DEFAULT, condition;
    Py_ssize_t len;
    GIOChannel *iochannel;
    guint handler_id;

    len = PyTuple_Size(args);
    if (len < 3) {
	PyErr_SetString(PyExc_TypeError,
			"io_add_watch requires at least 3 args");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 3);
    if (!PyArg_ParseTuple(first, "OiO:io_add_watch", &pyfd, &condition,
			  &callback)) {
	Py_DECREF(first);
        return NULL;
    }
    Py_DECREF(first);
    fd = PyObject_AsFileDescriptor(pyfd);
    if (fd < 0) {
	return NULL;
    }
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "third argument not callable");
        return NULL;
    }
    if (get_handler_priority(&priority, kwargs) < 0)
	return NULL;

    cbargs = PySequence_GetSlice(args, 3, len);
    if (cbargs == NULL)
      return NULL;
    data = Py_BuildValue("(OON)", callback, pyfd, cbargs);
    if (data == NULL)
      return NULL;
    iochannel = g_io_channel_unix_new(fd);
    handler_id = g_io_add_watch_full(iochannel, priority, condition,
				     iowatch_marshal, data,
				     (GDestroyNotify)pyg_destroy_notify);
    g_io_channel_unref(iochannel);
    
    return PyInt_FromLong(handler_id);
}

static PyObject *
pyg_source_remove(PyObject *self, PyObject *args)
{
    guint tag;

    if (!PyArg_ParseTuple(args, "i:source_remove", &tag))
	return NULL;

    return PyBool_FromLong(g_source_remove(tag));
}

#ifdef FIXME
static PyObject *
pyg_main_context_default(PyObject *unused)
{
    return pyg_main_context_new(g_main_context_default());
}
#endif

struct _PyGChildData {
    PyObject *func;
    PyObject *data;
};

static void
child_watch_func(GPid pid, gint status, gpointer data)
{
    struct _PyGChildData *child_data = (struct _PyGChildData *) data;
    PyObject *retval;
    PyGILState_STATE gil;

    gil = pyglib_gil_state_ensure();
    if (child_data->data)
        retval = PyObject_CallFunction(child_data->func, "iiO", pid, status,
                                       child_data->data);
    else
        retval = PyObject_CallFunction(child_data->func, "ii", pid, status);

    if (retval)
	Py_DECREF(retval);
    else
	PyErr_Print();

    pyglib_gil_state_release(gil);
}

static void
child_watch_dnotify(gpointer data)
{
    struct _PyGChildData *child_data = (struct _PyGChildData *) data;
    Py_DECREF(child_data->func);
    Py_XDECREF(child_data->data);
    g_free(child_data);
}


static PyObject *
pyg_child_watch_add(PyObject *unused, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "pid", "function", "data", "priority", NULL };
    guint id;
    gint priority = G_PRIORITY_DEFAULT;
    int pid;
    PyObject *func, *user_data = NULL;
    struct _PyGChildData *child_data;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "iO|Oi:glib.child_watch_add", kwlist,
                                     &pid, &func, &user_data, &priority))
        return NULL;
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                        "glib.child_watch_add: second argument must be callable");
        return NULL;
    }

    child_data = g_new(struct _PyGChildData, 1);
    child_data->func = func;
    child_data->data = user_data;
    Py_INCREF(child_data->func);
    if (child_data->data)
        Py_INCREF(child_data->data);
    id = g_child_watch_add_full(priority, pid, child_watch_func,
                                child_data, child_watch_dnotify);
    return PyInt_FromLong(id);
}

static PyObject *
pyg_markup_escape_text(PyObject *unused, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "text", NULL };
    char *text_in, *text_out;
    Py_ssize_t text_size;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s#:glib.markup_escape_text", kwlist,
                                     &text_in, &text_size))
        return NULL;

    text_out = g_markup_escape_text(text_in, text_size);
    retval = PyString_FromString(text_out);
    g_free(text_out);
    return retval;
}

static PyObject *
pyg_get_current_time(PyObject *unused)
{
    GTimeVal timeval;
    double ret;

    g_get_current_time(&timeval);
    ret = (double)timeval.tv_sec + (double)timeval.tv_usec * 0.000001;
    return PyFloat_FromDouble(ret);
}

static PyObject *
pyg_main_depth(PyObject *unused)
{
    return PyInt_FromLong(g_main_depth());
}

static PyObject *
pyg_filename_display_name(PyObject *self, PyObject *args)
{
    PyObject *py_display_name;
    char *filename, *display_name;
    
    if (!PyArg_ParseTuple(args, "s:glib.filename_display_name",
			  &filename))
	return NULL;

    display_name = g_filename_display_name(filename);
    py_display_name = PyUnicode_DecodeUTF8(display_name, strlen(display_name), NULL);
    g_free(display_name);
    return py_display_name;
}

static PyObject *
pyg_filename_display_basename(PyObject *self, PyObject *args)
{
    PyObject *py_display_basename;
    char *filename, *display_basename;
    
    if (!PyArg_ParseTuple(args, "s:glib.filename_display_basename",
			  &filename))
	return NULL;

    display_basename = g_filename_display_basename(filename);
    py_display_basename = PyUnicode_DecodeUTF8(display_basename, strlen(display_basename), NULL);
    g_free(display_basename);
    return py_display_basename;
}

static PyObject *
pyg_filename_from_utf8(PyObject *self, PyObject *args)
{
    char *filename, *utf8string;
    Py_ssize_t utf8string_len;
    gsize bytes_written;
    GError *error = NULL;
    PyObject *py_filename;
    
    if (!PyArg_ParseTuple(args, "s#:glib.filename_from_utf8",
			  &utf8string, &utf8string_len))
	return NULL;

    filename = g_filename_from_utf8(utf8string, utf8string_len, NULL, &bytes_written, &error);
    if (pyglib_error_check(&error)) {
        g_free(filename);
        return NULL;
    }
    py_filename = PyString_FromStringAndSize(filename, bytes_written);
    g_free(filename);
    return py_filename;
}


PyObject*
pyg_get_application_name(PyObject *self)
{
    const char *name;

    name = g_get_application_name();
    if (!name) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    return PyString_FromString(name);
}

PyObject*
pyg_set_application_name(PyObject *self, PyObject *args)
{
    char *s;

    if (!PyArg_ParseTuple(args, "s:glib.set_application_name", &s))
        return NULL;
    g_set_application_name(s);
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject*
pyg_get_prgname(PyObject *self)
{
    char *name;

    name = g_get_prgname();
    if (!name) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    return PyString_FromString(name);
}

PyObject*
pyg_set_prgname(PyObject *self, PyObject *args)
{
    char *s;

    if (!PyArg_ParseTuple(args, "s:glib.set_prgname", &s))
        return NULL;
    g_set_prgname(s);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef pyglib_functions[] = {
    { "spawn_async",
      (PyCFunction)pyglib_spawn_async, METH_VARARGS|METH_KEYWORDS },

    { "idle_add",
      (PyCFunction)pyg_idle_add, METH_VARARGS|METH_KEYWORDS },
    { "timeout_add",
      (PyCFunction)pyg_timeout_add, METH_VARARGS|METH_KEYWORDS },
    { "timeout_add_seconds",
      (PyCFunction)pyg_timeout_add_seconds, METH_VARARGS|METH_KEYWORDS },
    { "io_add_watch",
      (PyCFunction)pyg_io_add_watch, METH_VARARGS|METH_KEYWORDS },
    { "source_remove",
      pyg_source_remove, METH_VARARGS },
    { "child_watch_add",
      (PyCFunction)pyg_child_watch_add, METH_VARARGS|METH_KEYWORDS },
    { "markup_escape_text",
      (PyCFunction)pyg_markup_escape_text, METH_VARARGS|METH_KEYWORDS },
    { "get_current_time",
      (PyCFunction)pyg_get_current_time, METH_NOARGS },
    { "filename_display_name",
      (PyCFunction)pyg_filename_display_name, METH_VARARGS },
    { "filename_display_basename",
      (PyCFunction)pyg_filename_display_basename, METH_VARARGS },
    { "filename_from_utf8",
      (PyCFunction)pyg_filename_from_utf8, METH_VARARGS },
    { "get_application_name",
      (PyCFunction)pyg_get_application_name, METH_NOARGS },
    { "set_application_name",
      (PyCFunction)pyg_set_application_name, METH_VARARGS },
    { "get_prgname",
      (PyCFunction)pyg_get_prgname, METH_NOARGS },
    { "set_prgname",
      (PyCFunction)pyg_set_prgname, METH_VARARGS },
    { "main_depth",
      (PyCFunction)pyg_main_depth, METH_NOARGS },
#if 0
    { "main_context_default",
      (PyCFunction)pyg_main_context_default, METH_NOARGS },
#endif
    { NULL, NULL, 0 }
};

/* ----------------- glib module initialisation -------------- */

struct _PyGLib_Functions pyglib_api_functions = {
    FALSE, /* threads_enabled */
    NULL  /* gerror_exception */
};

static void
pyg_register_api(PyObject *d)
{
    PyObject *o;

    /* for addon libraries ... */
    PyDict_SetItemString(d, "_PyGLib_API",
			 o=PyCObject_FromVoidPtr(&pyglib_api_functions,NULL));
    Py_DECREF(o);
    
    pyglib_init_internal(o);
}

static void
pyg_register_error(PyObject *d)
{
    PyObject *dict;
    PyObject *gerror_class;
    dict = PyDict_New();
    /* This is a hack to work around the deprecation warning of
     * BaseException.message in Python 2.6+.
     * GError has also an "message" attribute.
     */
    PyDict_SetItemString(dict, "message", Py_None);
    gerror_class = PyErr_NewException("glib.GError", PyExc_RuntimeError, dict);
    Py_DECREF(dict);

    PyDict_SetItemString(d, "GError", gerror_class);
    pyglib_api_functions.gerror_exception = gerror_class;
}

static void
pyg_register_version_tuples(PyObject *d)
{
    PyObject *o;

    /* glib version */
    o = Py_BuildValue("(iii)", glib_major_version, glib_minor_version,
		      glib_micro_version);
    PyDict_SetItemString(d, "glib_version", o);
    Py_DECREF(o);

    /* pyglib version */
    o = Py_BuildValue("(iii)",
		      PYGLIB_MAJOR_VERSION,
		      PYGLIB_MINOR_VERSION,
		      PYGLIB_MICRO_VERSION);
    PyDict_SetItemString(d, "pyglib_version", o);
    Py_DECREF(o);
}

DL_EXPORT(void)
init_glib(void)
{
    PyObject *m, *d;

    m = Py_InitModule("glib._glib", pyglib_functions);
    d = PyModule_GetDict(m);

    pyg_register_api(d);
    pyg_register_error(d);
    pyg_register_version_tuples(d);
    pyg_spawn_register_types(d);
}

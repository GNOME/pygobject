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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Python.h>
#include <glib.h>
#include "pyglib.h"
#include "pyglib-private.h"
#include "pygoptioncontext.h"
#include "pygoptiongroup.h"
#include "pygsource.h"
#include "pygspawn.h"

#define PYGLIB_MAJOR_VERSION PYGOBJECT_MAJOR_VERSION
#define PYGLIB_MINOR_VERSION PYGOBJECT_MINOR_VERSION
#define PYGLIB_MICRO_VERSION PYGOBJECT_MICRO_VERSION


/* ---------------- glib module functions -------------------- */

static PyObject *
pyglib_threads_init(PyObject *unused, PyObject *args, PyObject *kwargs)
{
    if (!pyglib_enable_threads())
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pyglib_filename_from_utf8(PyObject *self, PyObject *args)
{
    char *filename, *utf8string;
    Py_ssize_t utf8string_len;
    gsize bytes_written;
    GError *error = NULL;
    PyObject *py_filename;
    
    if (!PyArg_ParseTuple(args, "s#:glib.filename_from_utf8",
			  &utf8string, &utf8string_len))
	return NULL;

    filename = g_filename_from_utf8(utf8string, utf8string_len,
				    NULL, &bytes_written, &error);
    if (pyglib_error_check(&error)) {
        g_free(filename);
        return NULL;
    }
    py_filename = PYGLIB_PyUnicode_FromStringAndSize(filename, bytes_written);
    g_free(filename);
    return py_filename;
}

static PyMethodDef _glib_functions[] = {
    { "threads_init",
      (PyCFunction) pyglib_threads_init, METH_NOARGS,
      "threads_init()\n"
      "Initialize GLib for use from multiple threads. If you also use GTK+\n"
      "itself (i.e. GUI, not just PyGObject), use gtk.gdk.threads_init()\n"
      "instead." },
    { "spawn_async",
      (PyCFunction)pyglib_spawn_async, METH_VARARGS|METH_KEYWORDS,
      "spawn_async(argv, envp=None, working_directory=None,\n"
      "            flags=0, child_setup=None, user_data=None,\n"
      "            standard_input=None, standard_output=None,\n"
      "            standard_error=None) -> (pid, stdin, stdout, stderr)\n"
      "Execute a child program asynchronously within a glib.MainLoop()\n"
      "See the reference manual for a complete reference." },
    { "filename_from_utf8",
      (PyCFunction)pyglib_filename_from_utf8, METH_VARARGS },
    { NULL, NULL, 0 }
};

/* ----------------- glib module initialisation -------------- */

static struct _PyGLib_Functions pyglib_api = {
    FALSE, /* threads_enabled */
    NULL,  /* gerror_exception */
    NULL,  /* block_threads */
    NULL,  /* unblock_threads */
    NULL,  /* pyg_main_context_new */
    pyg_option_context_new,
    pyg_option_group_new,
};

static void
pyglib_register_api(PyObject *d)
{
    PyObject *o;

    /* for addon libraries ... */
    PyDict_SetItemString(d, "_PyGLib_API",
			 o=PYGLIB_CPointer_WrapPointer(&pyglib_api,"gi._glib._PyGLib_API"));
    Py_DECREF(o);
    
    pyglib_init_internal(o);
}

static void
pyglib_register_error(PyObject *d)
{
    PyObject *dict;
    PyObject *gerror_class;
    dict = PyDict_New();
    /* This is a hack to work around the deprecation warning of
     * BaseException.message in Python 2.6+.
     * GError has also an "message" attribute.
     */
    PyDict_SetItemString(dict, "message", Py_None);
    gerror_class = PyErr_NewException("gi._glib.GError", PyExc_RuntimeError, dict);
    Py_DECREF(dict);

    PyDict_SetItemString(d, "GError", gerror_class);
    pyglib_api.gerror_exception = gerror_class;
}

static void
pyglib_register_constants(PyObject *m)
{
    PyModule_AddIntConstant(m, "OPTION_FLAG_HIDDEN",
			    G_OPTION_FLAG_HIDDEN);
    PyModule_AddIntConstant(m, "OPTION_FLAG_IN_MAIN",
			    G_OPTION_FLAG_IN_MAIN);
    PyModule_AddIntConstant(m, "OPTION_FLAG_REVERSE",
			    G_OPTION_FLAG_REVERSE);
    PyModule_AddIntConstant(m, "OPTION_FLAG_NO_ARG",
			    G_OPTION_FLAG_NO_ARG);
    PyModule_AddIntConstant(m, "OPTION_FLAG_FILENAME",
			    G_OPTION_FLAG_FILENAME);
    PyModule_AddIntConstant(m, "OPTION_FLAG_OPTIONAL_ARG",
			    G_OPTION_FLAG_OPTIONAL_ARG);
    PyModule_AddIntConstant(m, "OPTION_FLAG_NOALIAS",
			    G_OPTION_FLAG_NOALIAS); 

    PyModule_AddIntConstant(m, "OPTION_ERROR_UNKNOWN_OPTION",
			    G_OPTION_ERROR_UNKNOWN_OPTION);
    PyModule_AddIntConstant(m, "OPTION_ERROR_BAD_VALUE",
			    G_OPTION_ERROR_BAD_VALUE);
    PyModule_AddIntConstant(m, "OPTION_ERROR_FAILED",
			    G_OPTION_ERROR_FAILED);
 
    PyModule_AddStringConstant(m, "OPTION_REMAINING",
			       G_OPTION_REMAINING);
    PyModule_AddStringConstant(m, "OPTION_ERROR",
			       (char*) g_quark_to_string(G_OPTION_ERROR));
}

PYGLIB_MODULE_START(_glib, "_glib")
{
    PyObject *d = PyModule_GetDict(module);

    pyglib_register_constants(module);
    pyglib_register_api(d);
    pyglib_register_error(d);
    pyglib_source_register_types(d);
    pyglib_spawn_register_types(d);
    pyglib_option_context_register_types(d);
    pyglib_option_group_register_types(d);
}
PYGLIB_MODULE_END

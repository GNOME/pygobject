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
#include "pygspawn.h"

/* ---------------- glib module functions -------------------- */

static PyMethodDef _glib_functions[] = {
    { "spawn_async",
      (PyCFunction)pyglib_spawn_async, METH_VARARGS|METH_KEYWORDS,
      "spawn_async(argv, envp=None, working_directory=None,\n"
      "            flags=0, child_setup=None, user_data=None,\n"
      "            standard_input=None, standard_output=None,\n"
      "            standard_error=None) -> (pid, stdin, stdout, stderr)\n"
      "Execute a child program asynchronously within a glib.MainLoop()\n"
      "See the reference manual for a complete reference." },
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

PYGLIB_MODULE_START(_glib, "_glib")
{
    PyObject *d = PyModule_GetDict(module);

    pyglib_register_api(d);
    pyglib_register_error(d);
    pyglib_spawn_register_types(d);
    pyglib_option_context_register_types(d);
    pyglib_option_group_register_types(d);
}
PYGLIB_MODULE_END

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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
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
      "\n"
      "Execute a child program asynchronously within a glib.MainLoop()\n"
      "See the reference manual for a complete reference.\n" },
    { NULL, NULL, 0 }
};

PYGLIB_MODULE_START(_glib, "_glib")
{
    PyObject *d = PyModule_GetDict(module);

    pyglib_spawn_register_types(d);
    pyglib_option_context_register_types(d);
    pyglib_option_group_register_types(d);
}
PYGLIB_MODULE_END

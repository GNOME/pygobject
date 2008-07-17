/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygobject - Python bindings for GObject
 * Copyright (C) 2008  Johan Dahlin
 *
 *   giomodule.c: module wrapping the GIO library
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
#  include "config.h"
#endif
#include <Python.h>
#include "pygobject.h"

#include <gio/gio.h>

#define PYGIO_MAJOR_VERSION PYGOBJECT_MAJOR_VERSION
#define PYGIO_MINOR_VERSION PYGOBJECT_MINOR_VERSION
#define PYGIO_MICRO_VERSION PYGOBJECT_MICRO_VERSION

/* include any extra headers needed here */

void pygio_register_classes(PyObject *d);
void pygio_add_constants(PyObject *module, const gchar *strip_prefix);

extern PyMethodDef pygio_functions[];

DL_EXPORT(void)
init_gio(void)
{
    PyObject *m, *d;
    PyObject *tuple;
    
    /* perform any initialisation required by the library here */

    m = Py_InitModule("gio._gio", pygio_functions);
    d = PyModule_GetDict(m);

    init_pygobject();

    pygio_register_classes(d);
    pygio_add_constants(m, "G_IO_");

    PyModule_AddStringConstant(m, "ERROR", g_quark_to_string(G_IO_ERROR));

    /* pygio version */
    tuple = Py_BuildValue ("(iii)",
			   PYGIO_MAJOR_VERSION,
			   PYGIO_MINOR_VERSION,
			   PYGIO_MICRO_VERSION);
    PyDict_SetItemString(d, "pygio_version", tuple); 
    Py_DECREF(tuple);
}


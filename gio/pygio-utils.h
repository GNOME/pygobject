/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#ifndef __PYGIO_UTILS_H__
#define __PYGIO_UTILS_H__

#define NO_IMPORT_PYGOBJECT
#include <Python.h>
#include <pygobject.h>
#include <gio/gio.h>

extern PyTypeObject PyGCancellable_Type;
extern PyTypeObject PyGAppLaunchContext_Type;
extern PyTypeObject PyGFile_Type;

gboolean pygio_check_cancellable(PyGObject *pycancellable,
				 GCancellable **cancellable);

gboolean pygio_check_launch_context(PyGObject *pycontext,
				    GAppLaunchContext **context);

GList* pygio_pylist_to_gfile_glist(PyObject *pycontext);

GList* pygio_pylist_to_uri_glist(PyObject *pycontext);

PyObject* strv_to_pylist (char **strv);

gboolean pylist_to_strv (PyObject *list, char ***strvp);

#endif /* __PYGIO_UTILS_H__ */

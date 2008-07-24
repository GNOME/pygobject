/* -*- Mode: C; c-basic-offset: 4 -*-
 * pyglib - Python bindings for GLib toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *               2004-2008  Johan Dahlin
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

#ifndef __PYGLIB_PRIVATE_H__
#define __PYGLIB_PRIVATE_H__

#include <Python.h>
#include <glib.h>

G_BEGIN_DECLS

/* Compilation on Python 2.4 */
#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#endif

struct _PyGLib_Functions {
    gboolean threads_enabled;
    PyObject *gerror_exception;
    PyGLibThreadBlockFunc block_threads;
    PyGLibThreadBlockFunc unblock_threads;
};

#define PYGLIB_REGISTER_TYPE(d, type, name)	\
    type.ob_type = &PyType_Type; \
    if (!type.tp_alloc) \
	type.tp_alloc = PyType_GenericAlloc; \
    if (!type.tp_new) \
	type.tp_new = PyType_GenericNew; \
    if (PyType_Ready(&type)) \
	return; \
    PyDict_SetItemString(d, name, (PyObject *)&type);

gboolean _pyglib_handler_marshal(gpointer user_data);
void _pyglib_destroy_notify(gpointer user_data);

G_END_DECLS

#endif /* __PYGLIB_PRIVATE_H__ */



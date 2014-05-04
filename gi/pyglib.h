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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PYGLIB_H__
#define __PYGLIB_H__

#include <Python.h>

#include <glib.h>

G_BEGIN_DECLS

typedef void (*PyGLibThreadsEnabledFunc) (void);
typedef void (*PyGLibThreadBlockFunc) (void);

#ifdef DISABLE_THREADING
#    define pyglib_gil_state_ensure()        PyGILState_LOCKED
#    define pyglib_gil_state_release(state)  state
#else
#    define pyglib_gil_state_ensure          PyGILState_Ensure
#    define pyglib_gil_state_release         PyGILState_Release
#endif

GOptionGroup * pyglib_option_group_transfer_group(PyObject *self);

/* Private: for gobject <-> glib interaction only. */
PyObject* _pyglib_generic_ptr_richcompare(void* a, void *b, int op);
PyObject* _pyglib_generic_long_richcompare(long a, long b, int op);


#define PYGLIB_REGISTER_TYPE(d, type, name)	        \
    if (!type.tp_alloc)                                 \
	type.tp_alloc = PyType_GenericAlloc;            \
    if (!type.tp_new)                                   \
	type.tp_new = PyType_GenericNew;                \
    if (PyType_Ready(&type))                            \
	return;                                         \
    PyDict_SetItemString(d, name, (PyObject *)&type);


G_END_DECLS

#endif /* __PYGLIB_H__ */


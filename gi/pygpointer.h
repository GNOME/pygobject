/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
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

#ifndef __PYGOBJECT_POINTER_H__
#define __PYGOBJECT_POINTER_H__

#include <pythoncapi_compat.h>

extern GQuark pygpointer_class_key;

extern PyTypeObject PyGPointer_Type;

void pyg_register_pointer (PyObject *dict, const gchar *class_name,
                           GType pointer_type, PyTypeObject *type);
PyObject *pyg_pointer_new (GType pointer_type, gpointer pointer);

int pygi_pointer_register_types (PyObject *d);

#endif /* __PYGOBJECT_POINTER_H__ */

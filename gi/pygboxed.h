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

#ifndef __PYGOBJECT_BOXED_H__
#define __PYGOBJECT_BOXED_H__

extern GQuark pygboxed_type_key;

extern PyTypeObject PyGBoxed_Type;

void pygi_register_gboxed (PyObject *dict, const gchar *class_name,
                           GType boxed_type, PyTypeObject *type);
PyObject *pygi_gboxed_new (GType boxed_type, gpointer boxed,
                           gboolean copy_boxed, gboolean own_ref);

int pygi_gboxed_register_types (PyObject *d);

#endif /* __PYGOBJECT_BOXED_H__ */

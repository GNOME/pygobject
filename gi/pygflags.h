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

#ifndef __PYGOBJECT_FLAGS_H__ 
#define __PYGOBJECT_FLAGS_H__

extern GQuark pygflags_class_key;

typedef struct {
    PYGLIB_PyLongObject parent;
    int zero_pad; /* must always be 0 */
    GType gtype;
} PyGFlags;

extern PyTypeObject PyGFlags_Type;

#define PyGFlags_Check(x) (PyObject_IsInstance((PyObject *)x, (PyObject *)&PyGFlags_Type) && g_type_is_a(((PyGFlags*)x)->gtype, G_TYPE_FLAGS))

extern PyObject * pyg_flags_add        (PyObject *   module,
                                        const char * type_name,
                                        const char * strip_prefix,
                                        GType        gtype);
extern PyObject * pyg_flags_from_gtype (GType        gtype,
                                        guint        value);

gint pyg_flags_get_value (GType flag_type, PyObject *obj, guint *val);

void pygobject_flags_register_types(PyObject *d);

#endif /* __PYGOBJECT_FLAGS_H__ */

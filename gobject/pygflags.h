/* -*- Mode: C; c-basic-offset: 4 -*-
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 * Copyright (C) 2004       Johan Dahlin
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

#ifndef __PYGFLAGS_H__
#define __PYGFLAGS_H__

#include <Python.h>
#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    PyIntObject parent;
    GType gtype;
} PyGFlags;

PyTypeObject PyGFlags_Type;

#define PyGFlags_Check(x) (g_type_is_a(((PyGFlags*)x)->gtype, G_TYPE_FLAGS))
			   
PyObject * pyg_flags_add        (PyObject *   module,
				 const char * typename,
				 const char * strip_prefix,
				 GType        gtype);
PyObject * pyg_flags_from_gtype (GType        gtype,
				 int          value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PYGFLAGS_H__ */

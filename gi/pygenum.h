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

#ifndef __PYGOBJECT_ENUM_H__
#define __PYGOBJECT_ENUM_H__

#include <glib-object.h>
#include <girepository/girepository.h>
#include <pythoncapi_compat.h>

extern GQuark pygenum_class_key;

extern PyTypeObject *PyGEnum_Type;

PyObject *pyg_enum_add (PyObject *module, const char *type_name,
                        const char *strip_prefix, GType gtype);

PyObject *pyg_enum_add_full (PyObject *module, const char *typename,
                             GType gtype, GIEnumInfo *info);

gboolean pyg_enum_register (PyTypeObject *enum_class, char *type_name);

PyObject *pyg_enum_val_new (PyObject *pyclass, int value);

PyObject *pyg_enum_from_gtype (GType gtype, int value);

int pyg_enum_check_type (PyObject *obj, GType expected_type);

gint pyg_enum_get_value (GType enum_type, PyObject *obj, gint *val);

int pygi_enum_register_types (PyObject *mod);

#endif /* __PYGOBJECT_ENUM_H__ */

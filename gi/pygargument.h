/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
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

#ifndef __PYG_ARGUMENT_H__
#define __PYG_ARGUMENT_H__

#include <Python.h>

#include <girepository.h>

G_BEGIN_DECLS

gint pygi_gi_type_info_check_py_object(GITypeInfo *type_info,
                                       PyObject *object);

GArgument pyg_argument_from_pyobject(PyObject *object,
				     GITypeInfo *info);
PyObject*  pyg_argument_to_pyobject(GArgument *arg,
				    GITypeInfo *info);

PyObject* pyg_array_to_pyobject(gpointer items, gsize length, GITypeInfo *info);
gpointer pyg_array_from_pyobject(PyObject *object, GITypeInfo *type_info, gsize *length);

G_END_DECLS

#endif /* __PYG_ARGUMENT_H__ */

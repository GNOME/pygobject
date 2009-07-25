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

gsize pygi_gi_type_tag_get_size(GITypeTag type_tag);

gint pygi_gi_type_info_check_py_object(GITypeInfo *type_info,
                                       gboolean may_be_null,
                                       PyObject *object);

GArgument pygi_g_argument_from_py_object(PyObject *object,
                                         GITypeInfo *type_info,
                                         GITransfer transfer);
PyObject * pygi_g_argument_to_py_object(GArgument *arg,
                                        GITypeInfo *type_info);

void pygi_g_argument_clean(GArgument *arg,
                           GITypeInfo *type_info,
                           GITransfer transfer,
                           GIDirection direction);

G_END_DECLS

#endif /* __PYG_ARGUMENT_H__ */

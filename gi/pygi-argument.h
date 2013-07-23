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

#ifndef __PYGI_ARGUMENT_H__
#define __PYGI_ARGUMENT_H__

#include <Python.h>

#include <girepository.h>

G_BEGIN_DECLS


/* Private */
gpointer _pygi_arg_to_hash_pointer (const GIArgument *arg,
                                    GITypeTag         type_tag);

void _pygi_hash_pointer_to_arg (GIArgument *arg,
                                GITypeTag   type_tag);

gint _pygi_g_type_interface_check_object (GIBaseInfo *info,
                                          PyObject   *object);

gint _pygi_g_type_info_check_object (GITypeInfo *type_info,
                                     PyObject   *object,
                                     gboolean   allow_none);

gint _pygi_g_registered_type_info_check_object (GIRegisteredTypeInfo *info,
                                                gboolean              is_instance,
                                                PyObject             *object);


GArray* _pygi_argument_to_array (GIArgument  *arg,
                                 GIArgument  *args[],
                                 const GValue *args_values,
                                 GICallableInfo *callable_info,
                                 GITypeInfo  *type_info,
                                 gboolean    *out_free_array);

GIArgument _pygi_argument_from_object (PyObject   *object,
                                      GITypeInfo *type_info,
                                      GITransfer  transfer);

PyObject* _pygi_argument_to_object (GIArgument  *arg,
                                    GITypeInfo *type_info,
                                    GITransfer  transfer);

GIArgument _pygi_argument_from_g_value(const GValue *value,
                                       GITypeInfo *type_info);

void _pygi_argument_release (GIArgument   *arg,
                             GITypeInfo  *type_info,
                             GITransfer   transfer,
                             GIDirection  direction);

void _pygi_argument_init (void);

G_END_DECLS

#endif /* __PYGI_ARGUMENT_H__ */

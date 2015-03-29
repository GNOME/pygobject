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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PYGI_ARGUMENT_H__
#define __PYGI_ARGUMENT_H__

#include <Python.h>

#include <girepository.h>

G_BEGIN_DECLS


/* Private */
typedef gssize (*PyGIArgArrayLengthPolicy) (gsize item_index,
                                            void *user_data1,
                                            void *user_data2);

gssize _pygi_argument_array_length_marshal (gsize length_arg_index,
                                            void *user_data1,
                                            void *user_data2);

gpointer _pygi_arg_to_hash_pointer (const GIArgument *arg,
                                    GITypeTag         type_tag);

void _pygi_hash_pointer_to_arg (GIArgument *arg,
                                GITypeTag   type_tag);

GArray* _pygi_argument_to_array (GIArgument  *arg,
                                 PyGIArgArrayLengthPolicy array_length_policy,
                                 void        *user_data1,
                                 void        *user_data2,
                                 GITypeInfo  *type_info,
                                 gboolean    *out_free_array);

GIArgument _pygi_argument_from_object (PyObject   *object,
                                      GITypeInfo *type_info,
                                      GITransfer  transfer);

PyObject* _pygi_argument_to_object (GIArgument  *arg,
                                    GITypeInfo *type_info,
                                    GITransfer  transfer);

void _pygi_argument_release (GIArgument   *arg,
                             GITypeInfo  *type_info,
                             GITransfer   transfer,
                             GIDirection  direction);

gboolean pygi_argument_to_gssize (GIArgument *arg_in,
                                  GITypeTag  type_tag,
                                  gssize *gssize_out);

G_END_DECLS

#endif /* __PYGI_ARGUMENT_H__ */

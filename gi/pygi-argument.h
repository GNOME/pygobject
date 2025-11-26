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

#include <girepository/girepository.h>
#include <pythoncapi_compat.h>

G_BEGIN_DECLS

/* GIArgument is initialized based on the assumption that
 * it's the same size as a gint64 (long long).
 */
G_STATIC_ASSERT (sizeof (GIArgument) == sizeof (gint64));

#define PYGI_ARG_INIT { .v_int64 = 0 }

/* Private */
typedef gboolean (*PyGIArgArrayLengthPolicy) (gsize item_index,
                                              void *user_data1,
                                              void *user_data2,
                                              gsize *array_len);

gpointer _pygi_arg_to_hash_pointer (const GIArgument arg,
                                    GITypeInfo *type_info);

void _pygi_hash_pointer_to_arg_in_place (GIArgument *arg,
                                         GITypeInfo *type_info);

GArray *pygi_argument_to_array (GIArgument arg,
                                PyGIArgArrayLengthPolicy array_length_policy,
                                void *user_data1, void *user_data2,
                                GITypeInfo *type_info,
                                gboolean *out_free_array);

GIArgument pygi_argument_from_object (PyObject *object, GITypeInfo *type_info,
                                      GITransfer transfer);

PyObject *pygi_argument_to_object (GIArgument arg, GITypeInfo *type_info,
                                   GITransfer transfer);

void _pygi_argument_release (GIArgument *arg, GITypeInfo *type_info,
                             GITransfer transfer, GIDirection direction);

gboolean pygi_argument_to_gsize (GIArgument arg, GITypeTag type_tag,
                                 gsize *gsize_out);

GIArgument pygi_argument_interface_from_py (PyObject *object,
                                            GITypeInfo *type_info,
                                            GITransfer transfer);

PyObject *pygi_argument_interface_to_py (GIArgument arg, GITypeInfo *type_info,
                                         GITransfer transfer);

GIArgument pygi_argument_list_from_py (PyObject *object, GITypeInfo *type_info,
                                       GITransfer transfer);

GIArgument pygi_argument_hash_table_from_py (PyObject *object,
                                             GITypeInfo *type_info,
                                             GITransfer transfer);

PyObject *pygi_argument_hash_table_to_py (GIArgument arg,
                                          GITypeInfo *type_info,
                                          GITransfer transfer);

G_END_DECLS

#endif /* __PYGI_ARGUMENT_H__ */

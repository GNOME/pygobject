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

#include "pygi-invoke-state-struct.h"

G_BEGIN_DECLS

/* GIArgument is initialized based on the assumption that
 * it's the same size as a gint64 (long long).
 */
G_STATIC_ASSERT (sizeof (GIArgument) == sizeof (gint64));

#define PYGI_ARG_INIT { .v_int64 = 0 }

/**
 * Pass PyGIArgumentFromPyCleanupData as zero-initialized struct to pygi_argument_from_py.
 * Some cleanup related state will be stored in this struct. After processing the argument,
 * cleanup can be performed by calling pygi_argument_from_py_cleanup.
 */
typedef struct {
    PyObject *object;
    gpointer cache;
    gpointer cleanup_data;
    PyGIInvokeState state;
} PyGIArgumentFromPyCleanupData;

GIArgument pygi_argument_from_py (GITypeInfo *type_info, PyObject *object,
                                  GITransfer transfer,
                                  PyGIArgumentFromPyCleanupData *arg_cleanup);

/* Invoke pygi_argument_from_py_cleanup after you're done handling the argument aquired by pygi_argument_from_py. */
void pygi_argument_from_py_cleanup (
    PyGIArgumentFromPyCleanupData *arg_cleanup);

PyObject *pygi_argument_to_py (GITypeInfo *type_info, GIArgument value);

/* -- The functions below should be considered deprecated -- */

/* Private */
typedef gboolean (*PyGIArgArrayLengthPolicy) (gsize item_index,
                                              void *user_data1,
                                              void *user_data2,
                                              gsize *array_len);

gpointer _pygi_arg_to_hash_pointer (const GIArgument arg,
                                    GITypeInfo *type_info);

void _pygi_hash_pointer_to_arg_in_place (GIArgument *arg,
                                         GITypeInfo *type_info);

GArray *_pygi_argument_to_array (GIArgument arg,
                                 PyGIArgArrayLengthPolicy array_length_policy,
                                 void *user_data1, void *user_data2,
                                 GITypeInfo *type_info,
                                 gboolean *out_free_array);

GIArgument _pygi_argument_from_object (PyObject *object, GITypeInfo *type_info,
                                       GITransfer transfer);

PyObject *_pygi_argument_to_object (GIArgument arg, GITypeInfo *type_info,
                                    GITransfer transfer);

void _pygi_argument_release (GIArgument *arg, GITypeInfo *type_info,
                             GITransfer transfer, GIDirection direction);

gboolean pygi_argument_to_gsize (GIArgument arg, GITypeTag type_tag,
                                 gsize *gsize_out);

G_END_DECLS

#endif /* __PYGI_ARGUMENT_H__ */

/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2014 Simon Feltman <sfeltman@gnome.org>
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

#ifndef __PYGI_STRUCT_MARSHAL_H__
#define __PYGI_STRUCT_MARSHAL_H__

#include <girepository.h>
#include "pygi-cache.h"

G_BEGIN_DECLS

PyGIArgCache *pygi_arg_struct_new_from_info  (GITypeInfo      *type_info,
                                              GIArgInfo       *arg_info,   /* may be null */
                                              GITransfer       transfer,
                                              PyGIDirection    direction,
                                              GIInterfaceInfo *iface_info);


gboolean pygi_arg_gvalue_from_py_marshal     (PyObject        *py_arg, /*in*/
                                              GIArgument      *arg,    /*out*/
                                              GITransfer       transfer,
                                              gboolean         is_allocated);

gboolean pygi_arg_struct_from_py_marshal     (PyObject        *py_arg,
                                              GIArgument      *arg,
                                              const gchar     *arg_name,
                                              GIBaseInfo      *interface_info,
                                              GType            g_type,
                                              PyObject        *py_type,
                                              GITransfer       transfer,
                                              gboolean         is_allocated,
                                              gboolean         is_foreign,
                                              gboolean         is_pointer);

PyObject *pygi_arg_struct_to_py_marshal      (GIArgument      *arg,
                                              GIInterfaceInfo *interface_info,
                                              GType            g_type,
                                              PyObject        *py_type,
                                              GITransfer       transfer,
                                              gboolean         is_allocated,
                                              gboolean         is_foreign);

/* Needed for hack in pygi-arg-garray.c */
void pygi_arg_gvalue_from_py_cleanup         (PyGIInvokeState *state,
                                              PyGIArgCache    *arg_cache,
                                              PyObject        *py_arg,
                                              gpointer         data,
                                              gboolean         was_processed);

G_END_DECLS

#endif /*__PYGI_STRUCT_MARSHAL_H__*/

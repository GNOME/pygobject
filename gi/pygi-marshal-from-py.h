/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>, Red Hat, Inc.
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

#ifndef __PYGI_MARSHAL_from_py_PY_H__
#define __PYGI_MARSHAL_from_py_PY_H__

#include <Python.h>

#include <girepository.h>

#include "pygi-private.h"

G_BEGIN_DECLS

gboolean _pygi_marshal_from_py_ssize_t     (PyGIArgCache      *arg_cache,
                                            Py_ssize_t         size,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_void        (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg,
                                            gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_array       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg,
                                            gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_glist       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg,
                                            gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_gslist      (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg,
                                            gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_ghash       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg,
                                            gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_gerror      (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg,
                                            gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_interface_callback (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg,
                                                   gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_interface_enum     (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg,
                                                   gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_interface_flags    (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg,
                                                   gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_interface_struct_cache_adapter   (PyGIInvokeState   *state,
                                                                 PyGICallableCache *callable_cache,
                                                                 PyGIArgCache      *arg_cache,
                                                                 PyObject          *py_arg,
                                                                 GIArgument        *arg,
                                                                 gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_interface_boxed    (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg,
                                                   gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_interface_object   (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg,
                                                   gpointer          *cleanup_data);
gboolean _pygi_marshal_from_py_interface_union    (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg,
                                                   gpointer          *cleanup_data);

/* Simplified marshalers shared between vfunc/closure and direct function calls. */
gboolean _pygi_marshal_from_py_basic_type (PyObject   *object,   /* in */
                                           GIArgument *arg,      /* out */
                                           GITypeTag   type_tag,
                                           GITransfer  transfer,
                                           gpointer   *cleanup_data);
gboolean _pygi_marshal_from_py_basic_type_cache_adapter  (PyGIInvokeState   *state,
                                                          PyGICallableCache *callable_cache,
                                                          PyGIArgCache      *arg_cache,
                                                          PyObject          *py_arg,
                                                          GIArgument        *arg,
                                                          gpointer          *cleanup_data);

gboolean _pygi_marshal_from_py_gobject (PyObject *py_arg, /*in*/
                                        GIArgument *arg,  /*out*/
                                        GITransfer transfer);
gboolean _pygi_marshal_from_py_gobject_out_arg (PyObject *py_arg, /*in*/
                                                GIArgument *arg,  /*out*/
                                                GITransfer transfer);

gboolean _pygi_marshal_from_py_gvalue (PyObject *py_arg, /*in*/
                                       GIArgument *arg,  /*out*/
                                       GITransfer transfer,
                                       gboolean is_allocated);

gboolean _pygi_marshal_from_py_gclosure(PyObject *py_arg, /*in*/
                                        GIArgument *arg); /*out*/

gboolean _pygi_marshal_from_py_interface_struct (PyObject *py_arg,
                                                 GIArgument *arg,
                                                 const gchar *arg_name,
                                                 GIBaseInfo *interface_info,
                                                 GType g_type,
                                                 PyObject *py_type,
                                                 GITransfer transfer,
                                                 gboolean is_allocated,
                                                 gboolean is_foreign,
                                                 gboolean is_pointer);

G_END_DECLS

#endif /* __PYGI_MARSHAL_from_py_PY__ */

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
gboolean _pygi_marshal_from_py_interface_union    (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg,
                                                   gpointer          *cleanup_data);

G_END_DECLS

#endif /* __PYGI_MARSHAL_from_py_PY__ */

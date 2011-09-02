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

gboolean _pygi_marshal_from_py_void        (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_boolean     (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_int8        (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_uint8       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_int16       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_uint16      (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_int32       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_uint32      (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_int64       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_uint64      (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_float       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_double      (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_unichar     (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_gtype       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_utf8        (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_filename    (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_array       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_glist       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_gslist      (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_ghash       (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_gerror      (PyGIInvokeState   *state,
                                            PyGICallableCache *callable_cache,
                                            PyGIArgCache      *arg_cache,
                                            PyObject          *py_arg,
                                            GIArgument        *arg);
gboolean _pygi_marshal_from_py_interface_callback (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg);
gboolean _pygi_marshal_from_py_interface_enum     (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg);
gboolean _pygi_marshal_from_py_interface_flags    (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg);
gboolean _pygi_marshal_from_py_interface_struct   (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg);
gboolean _pygi_marshal_from_py_interface_interface(PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg);
gboolean _pygi_marshal_from_py_interface_boxed    (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg);
gboolean _pygi_marshal_from_py_interface_object   (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg);
gboolean _pygi_marshal_from_py_interface_union    (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg);
gboolean _pygi_marshal_from_py_interface_instance (PyGIInvokeState   *state,
                                                   PyGICallableCache *callable_cache,
                                                   PyGIArgCache      *arg_cache,
                                                   PyObject          *py_arg,
                                                   GIArgument        *arg);

G_END_DECLS

#endif /* __PYGI_MARSHAL_from_py_PY__ */

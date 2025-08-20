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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PYGI_MARSHAL_CLEANUP_H__
#define __PYGI_MARSHAL_CLEANUP_H__

#include "pygi-struct.h"
#include "pygi-invoke-state-struct.h"
#include "pygi-cache.h"

G_BEGIN_DECLS

void pygi_marshal_cleanup_args_from_py_marshal_success (
    PyGIInvokeState *state, PyGICallableCache *cache);
void pygi_marshal_cleanup_args_from_py_parameter_fail (
    PyGIInvokeState *state, PyGICallableCache *cache, gssize failed_arg_index);

void pygi_marshal_cleanup_args_to_py_marshal_success (
    PyGIInvokeState *state, PyGICallableCache *cache);
void pygi_marshal_cleanup_args_return_fail (PyGIInvokeState *state,
                                            PyGICallableCache *cache);
void pygi_marshal_cleanup_args_to_py_parameter_fail (
    PyGIInvokeState *state, PyGICallableCache *cache,
    gssize failed_to_py_arg_index);
G_END_DECLS

#endif /* __PYGI_MARSHAL_CLEANUP_H__ */

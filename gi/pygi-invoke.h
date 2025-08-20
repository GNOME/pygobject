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

#ifndef __PYGI_INVOKE_H__
#define __PYGI_INVOKE_H__

#include <girepository/girepository.h>
#include <pythoncapi_compat.h>

#include "pygi-info.h"
#include "pygi-invoke-state-struct.h"

G_BEGIN_DECLS

PyObject *pygi_invoke_c_callable (PyGIFunctionCache *function_cache,
                                  PyGIInvokeState *state,
                                  PyObject *const *py_args, size_t py_nargsf,
                                  PyObject *py_kwnames);
PyObject *pygi_callable_info_invoke (PyGICallableInfo *self,
                                     PyObject *const *py_args,
                                     size_t py_nargsf, PyObject *kwnames);

gboolean _pygi_invoke_arg_state_init (PyGIInvokeState *state);

void _pygi_invoke_arg_state_free (PyGIInvokeState *state);

G_END_DECLS

#endif /* __PYGI_INVOKE_H__ */

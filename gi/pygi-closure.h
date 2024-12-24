/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
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

#ifndef __PYGI_CLOSURE_H__
#define __PYGI_CLOSURE_H__


/* Workaround for FFI_GO_CLOSURES not being defined on macOS
 * See also: https://github.com/openjdk/jdk17u-dev/pull/741
 */
#ifndef FFI_GO_CLOSURES
#define FFI_GO_CLOSURES 0
#endif

#include <ffi.h>
#include <pythoncapi_compat.h>
#include <girepository/girepository.h>

#include "pygi-cache.h"

G_BEGIN_DECLS


/* Private */

typedef struct _PyGICClosure {
    GICallableInfo *info;
    PyObject *function;

    ffi_closure *closure;
    ffi_cif cif;

    GIScopeType scope;

    PyObject *user_data;

    PyGIClosureCache *cache;
} PyGICClosure;

void _pygi_closure_handle (ffi_cif *cif, void *result, void **args,
                           void *userdata);

void _pygi_invoke_closure_free (PyGICClosure *invoke_closure);

PyGICClosure *_pygi_make_native_closure (GICallableInfo *info,
                                         PyGIClosureCache *cache,
                                         GIScopeType scope, PyObject *function,
                                         PyObject *user_data);

PyGIArgCache *pygi_arg_callback_new_from_info (
    GITypeInfo *type_info, GIArgInfo *arg_info, /* may be null */
    GITransfer transfer, PyGIDirection direction, GICallbackInfo *iface_info,
    PyGICallableCache *callable_cache);

G_END_DECLS

#endif /* __PYGI_CLOSURE_H__ */

/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
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

#ifndef __PYGI_ARG_BASICTYPE_H__
#define __PYGI_ARG_BASICTYPE_H__

#include <girepository.h>
#include "pygi-cache.h"

G_BEGIN_DECLS

gboolean _pygi_marshal_from_py_basic_type               (PyObject      *object,     /* in */
                                                         GIArgument    *arg,        /* out */
                                                         GITypeTag      type_tag,
                                                         GITransfer     transfer,
                                                         gpointer      *cleanup_data);
gboolean _pygi_marshal_from_py_basic_type_cache_adapter (PyGIInvokeState   *state,
                                                         PyGICallableCache *callable_cache,
                                                         PyGIArgCache      *arg_cache,
                                                         PyObject          *py_arg,
                                                         GIArgument        *arg,
                                                         gpointer          *cleanup_data);

PyObject *_pygi_marshal_to_py_basic_type               (GIArgument    *arg,        /* in */
                                                        GITypeTag      type_tag,
                                                        GITransfer     transfer);
PyObject *_pygi_marshal_to_py_basic_type_cache_adapter (PyGIInvokeState   *state,
                                                        PyGICallableCache *callable_cache,
                                                        PyGIArgCache      *arg_cache,
                                                        GIArgument        *arg);

PyGIArgCache *pygi_arg_basic_type_new_from_info        (GITypeInfo    *type_info,
                                                        GIArgInfo     *arg_info,   /* may be null */
                                                        GITransfer     transfer,
                                                        PyGIDirection  direction);
G_END_DECLS

#endif /*__PYGI_ARG_BASICTYPE_H__*/

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

#include "pygi-type.h"
#include "pygenum.h"
#include "pygflags.h"
#include "pygi-argument.h"
#include "pygi-cache-private.h"

static gboolean
_pygi_marshal_from_py_interface_enum (PyGIInvokeState *state,
                                      PyGICallableCache *callable_cache,
                                      PyGIArgCache *arg_cache,
                                      PyObject *py_arg, GIArgument *arg,
                                      gpointer *cleanup_data)
{
    *arg = pygi_argument_interface_from_py (py_arg, arg_cache->type_info,
                                            arg_cache->transfer);
    return !PyErr_Occurred ();
}

static PyObject *
_pygi_marshal_to_py_interface_enum (PyGIInvokeState *state,
                                    PyGICallableCache *callable_cache,
                                    PyGIArgCache *arg_cache, GIArgument *arg,
                                    gpointer *cleanup_data)
{
    return pygi_argument_interface_to_py (*arg, arg_cache->type_info,
                                          arg_cache->transfer);
}

PyGIArgCache *
pygi_arg_enum_new_from_info (GITypeInfo *type_info, GIArgInfo *arg_info,
                             GITransfer transfer, PyGIDirection direction,
                             GIEnumInfo *iface_info)
{
    PyGIArgCache *arg_cache = NULL;

    arg_cache = pygi_arg_interface_new_from_info (
        type_info, arg_info, transfer, direction,
        GI_REGISTERED_TYPE_INFO (iface_info));
    if (arg_cache == NULL) return NULL;

    if (direction & PYGI_DIRECTION_FROM_PYTHON)
        arg_cache->from_py_marshaller = _pygi_marshal_from_py_interface_enum;

    if (direction & PYGI_DIRECTION_TO_PYTHON)
        arg_cache->to_py_marshaller = _pygi_marshal_to_py_interface_enum;

    return arg_cache;
}

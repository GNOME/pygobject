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

#ifndef __PYGI_OBJECT_H__
#define __PYGI_OBJECT_H__

#include <girepository.h>
#include "pygi-cache.h"

G_BEGIN_DECLS

gboolean
pygi_arg_gobject_out_arg_from_py     (PyObject          *py_arg,     /* in */
                                      GIArgument        *arg,        /* out */
                                      GITransfer         transfer);

PyObject *
pygi_arg_object_to_py_called_from_c  (GIArgument        *arg,
                                      GITransfer         transfer);

PyGIArgCache *
pygi_arg_gobject_new_from_info       (GITypeInfo        *type_info,
                                      GIArgInfo         *arg_info,   /* may be null */
                                      GITransfer         transfer,
                                      PyGIDirection      direction,
                                      GIInterfaceInfo   *iface_info,
                                      PyGICallableCache *callable_cache);

G_END_DECLS

#endif /*__PYGI_OBJECT_H__*/

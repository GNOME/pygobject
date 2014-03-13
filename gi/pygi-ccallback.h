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

#ifndef __PYGI_CCLOSURE_H__
#define __PYGI_CCLOSURE_H__

#include <Python.h>

G_BEGIN_DECLS

extern PyTypeObject PyGICCallback_Type;

PyObject * _pygi_ccallback_new (GCallback       callback,
                                gpointer        user_data,
                                GIScopeType     scope,
                                GIFunctionInfo *info,
                                GDestroyNotify  destroy_notify);

void _pygi_ccallback_register_types (PyObject *m);

G_END_DECLS

#endif /* __PYGI_CCLOSURE_H__ */

/* -*- Mode: C; c-basic-offset: 4 -*-
 * pyglib - Python bindings for GLib toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *               2004-2008  Johan Dahlin
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

#ifndef __PYGLIB_PRIVATE_H__
#define __PYGLIB_PRIVATE_H__

#include <Python.h>
#include <glib.h>

#include <pyglib.h>
#include <pyglib-python-compat.h>

G_BEGIN_DECLS

gboolean _pyglib_handler_marshal(gpointer user_data);
void _pyglib_destroy_notify(gpointer user_data);

extern PyObject *pyglib__glib_module_create (void);

G_END_DECLS

#endif /* __PYGLIB_PRIVATE_H__ */



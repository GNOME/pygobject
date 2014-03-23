/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2009 Simon van der Linden <svdlinden@src.gnome.org>
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

#ifndef __PYGI_TYPE_H__
#define __PYGI_TYPE_H__

#include <Python.h>
#include <girepository.h>

G_BEGIN_DECLS

/* Public */

PyObject *pygi_type_import_by_g_type (GType g_type);


/* Private */

PyObject *_pygi_type_import_by_name (const char *namespace_, const char *name);

PyObject *_pygi_type_import_by_gi_info (GIBaseInfo *info);

PyObject *_pygi_type_get_from_g_type (GType g_type);

G_END_DECLS

#endif /* __PYGI_TYPE_H__ */

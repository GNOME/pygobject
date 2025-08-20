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

#ifndef __PYGI_REPOSITORY_H__
#define __PYGI_REPOSITORY_H__

#include <girepository/girepository.h>
#include <pythoncapi_compat.h>

G_BEGIN_DECLS

typedef struct {
    PyObject_HEAD
    GIRepository *repository;
} PyGIRepository;

/* Private */

extern PyTypeObject PyGIRepository_Type;

extern PyObject *PyGIRepositoryError;

GIRepository *pygi_repository_get_default (void);

int pygi_repository_register_types (PyObject *m);

G_END_DECLS

#endif /* __PYGI_REPOSITORY_H__ */

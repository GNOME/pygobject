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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#ifndef __PYGI_H__
#define __PYGI_H__

#include <config.h>
#include <pygobject.h>

#if ENABLE_INTROSPECTION

#include <girepository.h>

typedef struct {
    PyObject_HEAD
    GIRepository *repository;
} PyGIRepository;

typedef struct {
    PyObject_HEAD
    GIBaseInfo *info;
    PyObject *inst_weakreflist;
} PyGIBaseInfo;

typedef struct {
    PyGPointer base;
    gboolean free_on_dealloc;
} PyGIStruct;

typedef struct {
    PyGBoxed base;
    gboolean slice_allocated;
    gsize size;
} PyGIBoxed;


struct PyGI_API {
    PyObject* (*type_import_by_g_type) (GType g_type);
};

static struct PyGI_API *PyGI_API = NULL;

static int
_pygi_import (void)
{
    if (PyGI_API != NULL) {
        return 1;
    }

    PyGI_API = (struct PyGI_API*) PyCObject_Import("gi", "_API");
    if (PyGI_API == NULL) {
        return -1;
    }

    return 0;
}

static inline PyObject *
pygi_type_import_by_g_type (GType g_type)
{
   if (_pygi_import() < 0) {
       return NULL;
   }
   return PyGI_API->type_import_by_g_type(g_type);
}

#else /* ENABLE_INTROSPECTION */

static inline PyObject *
pygi_type_import_by_g_type (GType g_type)
{
    return NULL;
}

#endif /* ENABLE_INTROSPECTION */

#endif /* __PYGI_H__ */

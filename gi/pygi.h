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

#ifndef __PYGI_H__
#define __PYGI_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <pygobject-private.h>

#include <girepository.h>
#include "pygi-cache.h"

extern PyObject *PyGIDeprecationWarning;
extern PyObject *_PyGIDefaultArgPlaceholder;

typedef struct {
    PyObject_HEAD
    GIRepository *repository;
} PyGIRepository;

typedef struct {
    PyObject_HEAD
    GIBaseInfo *info;
    PyObject *inst_weakreflist;
    PyGICallableCache *cache;
} PyGIBaseInfo;

typedef struct {
    PyGIBaseInfo base;

    /* Reference the unbound version of this struct.
     * We use this for the actual call to invoke because it manages the cache.
     */
    struct PyGICallableInfo *py_unbound_info;

    /* Holds bound argument for instance, class, and vfunc methods. */
    PyObject *py_bound_arg;

} PyGICallableInfo;

typedef struct {
    PyGPointer base;
    gboolean free_on_dealloc;
} PyGIStruct;

typedef struct {
    PyGBoxed base;
    gboolean slice_allocated;
    gsize size;
} PyGIBoxed;

typedef struct {
    PyObject_HEAD
    GCallback callback;
    GIFunctionInfo *info;
    gpointer user_data;
    GIScopeType scope;
    GDestroyNotify destroy_notify_func;
    PyGICCallbackCache *cache;
} PyGICCallback;


#endif /* __PYGI_H__ */

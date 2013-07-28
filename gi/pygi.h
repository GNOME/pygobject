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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>

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
    PyGICallableCache *cache;
} PyGICCallback;

typedef PyObject * (*PyGIArgOverrideToGIArgumentFunc) (PyObject        *value,
                                                       GIInterfaceInfo *interface_info,
                                                       GITransfer       transfer,
                                                       GIArgument      *arg);
typedef PyObject * (*PyGIArgOverrideFromGIArgumentFunc) (GIInterfaceInfo *interface_info,
                                                         gpointer         data);
typedef PyObject * (*PyGIArgOverrideReleaseFunc) (GITypeInfo *type_info,
                                                  gpointer  struct_);

struct PyGI_API {
    PyObject* (*type_import_by_g_type) (GType g_type);
    PyObject* (*get_property_value) (PyGObject *instance,
                                     GParamSpec *pspec);
    gint (*set_property_value) (PyGObject *instance,
                                GParamSpec *pspec,
                                PyObject *value);
    GClosure * (*signal_closure_new) (PyGObject *instance,
                                      GType g_type,
                                      const gchar *sig_name,
                                      PyObject *callback,
                                      PyObject *extra_args,
                                      PyObject *swap_data);
    void (*register_foreign_struct) (const char* namespace_,
                                     const char* name,
                                     PyGIArgOverrideToGIArgumentFunc to_func,
                                     PyGIArgOverrideFromGIArgumentFunc from_func,
                                     PyGIArgOverrideReleaseFunc release_func);
};

static struct PyGI_API *PyGI_API = NULL;

static int
_pygi_import (void)
{
    if (PyGI_API != NULL) {
        return 1;
    }
    PyGI_API = (struct PyGI_API*) PyCapsule_Import("gi._API", FALSE);
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

static inline PyObject *
pygi_get_property_value (PyGObject *instance,
                         GParamSpec *pspec)
{
    if (_pygi_import() < 0) {
        return NULL;
    }
    return PyGI_API->get_property_value(instance, pspec);
}

static inline gint
pygi_set_property_value (PyGObject *instance,
                         GParamSpec *pspec,
                         PyObject *value)
{
    if (_pygi_import() < 0) {
        return -1;
    }
    return PyGI_API->set_property_value(instance, pspec, value);
}

static inline GClosure *
pygi_signal_closure_new (PyGObject *instance,
                         GType g_type,
                         const gchar *sig_name,
                         PyObject *callback,
                         PyObject *extra_args,
                         PyObject *swap_data)
{
    if (_pygi_import() < 0) {
        return NULL;
    }
    return PyGI_API->signal_closure_new(instance, g_type, sig_name, callback, extra_args, swap_data);
}

static inline PyObject *
pygi_register_foreign_struct (const char* namespace_,
                              const char* name,
                              PyGIArgOverrideToGIArgumentFunc to_func,
                              PyGIArgOverrideFromGIArgumentFunc from_func,
                              PyGIArgOverrideReleaseFunc release_func)
{
    if (_pygi_import() < 0) {
        return NULL;
    }
    PyGI_API->register_foreign_struct(namespace_,
                                      name,
                                      to_func,
                                      from_func,
                                      release_func);
    Py_RETURN_NONE;
}

#endif /* __PYGI_H__ */

/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>, Red Hat, Inc.
 *
 *   pygi-boxed-closure.c: wrapper to handle GClosure box types with C callbacks.
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

#include "pygi-private.h"
#include "pygobject-private.h"

#include <girepository.h>
#include <pyglib-python-compat.h>


static PyObject *
_ccallback_call(PyGICCallback *self, PyObject *args, PyObject *kwargs)
{
    PyObject *result;

    if (self->cache == NULL) {
        self->cache = (PyGICCallbackCache *)pygi_ccallback_cache_new (self->info,
                                                                      self->callback);
        if (self->cache == NULL)
            return NULL;
    }

    result = pygi_ccallback_cache_invoke (self->cache,
                                          args,
                                          kwargs,
                                          self->user_data);
    return result;
}

PYGLIB_DEFINE_TYPE("gi.CCallback", PyGICCallback_Type, PyGICCallback);

PyObject *
_pygi_ccallback_new (GCallback callback,
                     gpointer user_data,
                     GIScopeType scope,
                     GIFunctionInfo *info,
                     GDestroyNotify destroy_notify)
{
    PyGICCallback *self;

    if (!callback) {
        Py_RETURN_NONE;
    }

    self = (PyGICCallback *) PyGICCallback_Type.tp_alloc (&PyGICCallback_Type, 0);
    if (self == NULL) {
        return NULL;
    }

    self->callback = (GCallback) callback;
    self->user_data = user_data;
    self->scope = scope;
    self->destroy_notify_func = destroy_notify;
    self->info = g_base_info_ref( (GIBaseInfo *) info);

    return (PyObject *) self;
}

static void
_ccallback_dealloc (PyGICCallback *self)
{
    g_base_info_unref ( (GIBaseInfo *)self->info);

    if (self->cache != NULL) {
        pygi_callable_cache_free ( (PyGICallableCache *)self->cache);
    }

    Py_TYPE (self)->tp_free ((PyObject *)self);
}

void
_pygi_ccallback_register_types (PyObject *m)
{
    Py_TYPE(&PyGICCallback_Type) = &PyType_Type;
    PyGICCallback_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);
    PyGICCallback_Type.tp_dealloc = (destructor) _ccallback_dealloc;
    PyGICCallback_Type.tp_call = (ternaryfunc) _ccallback_call;


    if (PyType_Ready (&PyGICCallback_Type))
        return;
    if (PyModule_AddObject (m, "CCallback", (PyObject *) &PyGICCallback_Type))
        return;
}

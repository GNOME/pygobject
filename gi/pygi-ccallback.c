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

#include <girepository/girepository.h>

#include "pygi-ccallback.h"
#include "pygi-util.h"

static PyObject *
_ccallback_vectorcall (PyGICCallback *self, PyObject *const *args,
                       size_t nargsf, PyObject *kwnames)
{
    PyObject *result;

    if (self->cache == NULL) {
        self->cache = (PyGICCallbackCache *)pygi_ccallback_cache_new (
            GI_CALLABLE_INFO (self->info), self->callback);
        if (self->cache == NULL) return NULL;
    }

    result = pygi_ccallback_cache_invoke (self->cache, args, nargsf, kwnames,
                                          self->user_data);
    return result;
}

PYGI_DEFINE_TYPE ("gi.CCallback", PyGICCallback_Type, PyGICCallback);

PyObject *
_pygi_ccallback_new (GCallback callback, gpointer user_data, GIScopeType scope,
                     GICallableInfo *info, GDestroyNotify destroy_notify)
{
    PyGICCallback *self;

    if (!callback) {
        Py_RETURN_NONE;
    }

    self =
        (PyGICCallback *)PyGICCallback_Type.tp_alloc (&PyGICCallback_Type, 0);
    if (self == NULL) {
        return NULL;
    }

    self->callback = (GCallback)callback;
    self->user_data = user_data;
    self->scope = scope;
    self->destroy_notify_func = destroy_notify;
    self->info = GI_CALLABLE_INFO (gi_base_info_ref (info));
    self->vectorcall = (vectorcallfunc)_ccallback_vectorcall;

    return (PyObject *)self;
}

static void
_ccallback_dealloc (PyGICCallback *self)
{
    gi_base_info_unref ((GIBaseInfo *)self->info);

    if (self->cache != NULL) {
        pygi_callable_cache_free ((PyGICallableCache *)self->cache);
    }

    Py_TYPE (self)->tp_free ((PyObject *)self);
}

/**
 * Returns 0 on success, or -1 and sets an exception.
 */
int
pygi_ccallback_register_types (PyObject *m)
{
    Py_SET_TYPE (&PyGICCallback_Type, &PyType_Type);
    PyGICCallback_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
                                   | Py_TPFLAGS_HAVE_VECTORCALL);
    PyGICCallback_Type.tp_dealloc = (destructor)_ccallback_dealloc;
    PyGICCallback_Type.tp_call = PyVectorcall_Call;
    PyGICCallback_Type.tp_vectorcall_offset =
        offsetof (PyGICCallback, vectorcall);


    if (PyType_Ready (&PyGICCallback_Type) < 0) return -1;
    Py_INCREF ((PyObject *)&PyGICCallback_Type);
    if (PyModule_AddObject (m, "CCallback", (PyObject *)&PyGICCallback_Type)
        < 0) {
        Py_DECREF ((PyObject *)&PyGICCallback_Type);
        return -1;
    }

    return 0;
}

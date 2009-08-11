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

#include <Python.h>

#include <girepository.h>

G_BEGIN_DECLS

typedef struct {
    PyObject_HEAD
    GIRepository *repository;
} PyGIRepository;

typedef struct {
    PyObject_HEAD
    GIBaseInfo *info;
    PyObject *inst_weakreflist;
} PyGIBaseInfo;

#define PyGIBaseInfo_GET_GI_INFO(object) g_base_info_ref(((PyGIBaseInfo *)object)->info)


struct PyGI_API {
    /* Misc */
    PyObject* (*type_find_by_gi_info) (GIBaseInfo *info);

    /* Boxed */
    PyObject* (*boxed_new) (PyTypeObject *type,
                            gpointer      pointer,
                            gboolean      own_pointer);
};


#ifndef __PYGI_PRIVATE_H__

static struct PyGI_API *PyGI_API = NULL;

/* Misc */
#define pygi_type_find_by_gi_info (PyGI_API->type_find_by_gi_info)

/* Boxed */
#define pygi_boxed_new (PyGI_API->boxed_new)


static int
pygi_import (void)
{
    PyObject *module;
    PyObject *api;

    if (PyGI_API != NULL) {
        return 1;
    }

    module = PyImport_ImportModule("gi");
    if (module == NULL) {
        return -1;
    }

    api = PyObject_GetAttrString(module, "_API");
    if (api == NULL) {
        Py_DECREF(module);
        return -1;
    }
    if (!PyCObject_Check(api)) {
        Py_DECREF(module);
        Py_DECREF(api);
        PyErr_Format(PyExc_TypeError, "gi._API must be cobject, not %s",
                api->ob_type->tp_name);
        return -1;
    }

    PyGI_API = (struct PyGI_API *)PyCObject_AsVoidPtr(api);

    Py_DECREF(api);

    return 0;
}

#endif

G_END_DECLS

#endif /* __PYGI_H__ */

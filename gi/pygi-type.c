/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2009 Simon van der Linden <svdlinden@src.gnome.org>
 *
 *   pygi-type.c: helpers to lookup Python wrappers from GType and GIBaseInfo.
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

#include <pyglib-python-compat.h>


PyObject *
_pygi_type_import_by_name (const char *namespace_,
                           const char *name)
{
    gchar *module_name;
    PyObject *py_module;
    PyObject *py_object;

    module_name = g_strconcat ("gi.repository.", namespace_, NULL);

    py_module = PyImport_ImportModule (module_name);

    g_free (module_name);

    if (py_module == NULL) {
        return NULL;
    }

    py_object = PyObject_GetAttrString (py_module, name);

    Py_DECREF (py_module);

    return py_object;
}

PyObject *
pygi_type_import_by_g_type (GType g_type)
{
    GIRepository *repository;
    GIBaseInfo *info;
    PyObject *type;

    repository = g_irepository_get_default();

    info = g_irepository_find_by_gtype (repository, g_type);
    if (info == NULL) {
        return NULL;
    }

    type = _pygi_type_import_by_gi_info (info);
    g_base_info_unref (info);

    return type;
}

PyObject *
_pygi_type_import_by_gi_info (GIBaseInfo *info)
{
    return _pygi_type_import_by_name (g_base_info_get_namespace (info),
                                      g_base_info_get_name (info));
}

PyObject *
_pygi_type_get_from_g_type (GType g_type)
{
    PyObject *py_g_type;
    PyObject *py_type;

    py_g_type = pyg_type_wrapper_new (g_type);
    if (py_g_type == NULL) {
        return NULL;
    }

    py_type = PyObject_GetAttrString (py_g_type, "pytype");
    if (py_type == Py_None) {
        py_type = pygi_type_import_by_g_type (g_type);
    }

    Py_DECREF (py_g_type);

    return py_type;
}


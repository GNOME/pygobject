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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
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
pygi_type_import_by_g_type_real (GType g_type)
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
        py_type = pygi_type_import_by_g_type_real (g_type);
    }

    Py_DECREF (py_g_type);

    return py_type;
}

/* _pygi_get_py_type_hint
 *
 * This gives a hint to what python type might be used as
 * a particular gi type.
 */
PyObject *
_pygi_get_py_type_hint(GITypeTag type_tag)
{
    PyObject *type = Py_None;

    switch (type_tag) {
        case GI_TYPE_TAG_BOOLEAN:
            type = (PyObject *) &PyBool_Type;
            break;

        case GI_TYPE_TAG_INT8:
        case GI_TYPE_TAG_UINT8:
        case GI_TYPE_TAG_INT16:
        case GI_TYPE_TAG_UINT16:
        case GI_TYPE_TAG_INT32:
        case GI_TYPE_TAG_UINT32:
        case GI_TYPE_TAG_INT64:
        case GI_TYPE_TAG_UINT64:
            type = (PyObject *) &PYGLIB_PyLong_Type;
            break;

        case GI_TYPE_TAG_FLOAT:
        case GI_TYPE_TAG_DOUBLE:
            type = (PyObject *) &PyFloat_Type;
            break;

        case GI_TYPE_TAG_GLIST:
        case GI_TYPE_TAG_GSLIST:
        case GI_TYPE_TAG_ARRAY:
            type = (PyObject *) &PyList_Type;
            break;

        case GI_TYPE_TAG_GHASH:
            type = (PyObject *) &PyDict_Type;
            break;

        case GI_TYPE_TAG_UTF8:
        case GI_TYPE_TAG_FILENAME:
        case GI_TYPE_TAG_UNICHAR:
            type = (PyObject *) &PYGLIB_PyUnicode_Type;
            break;

        case GI_TYPE_TAG_INTERFACE:
        case GI_TYPE_TAG_GTYPE:
        case GI_TYPE_TAG_ERROR:
        case GI_TYPE_TAG_VOID:
            break;
    }

    Py_INCREF(type);
    return type;
}


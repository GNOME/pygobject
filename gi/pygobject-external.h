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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#ifndef __PYGOBJECT_EXTERN_H__
#define __PYGOBJECT_EXTERN_H__

#include <Python.h>

G_BEGIN_DECLS

static PyTypeObject *_PyGObject_Type;
static PyTypeObject *_PyGTypeWrapper_Type;

#define PyGObject_Type (*_PyGObject_Type)
#define PyGTypeWrapper_Type (*_PyGTypeWrapper_Type)

__attribute__ ( (unused))
static int
_pygobject_import (void)
{
    static gboolean imported = FALSE;
    PyObject *from_list;
    PyObject *module;
    int retval = 0;

    if (imported) {
        return 1;
    }

    from_list = Py_BuildValue ("(ss)", "GObject", "GTypeWrapper");
    if (from_list == NULL) {
        return -1;
    }

    module = PyImport_ImportModuleEx ("gobject", NULL, NULL, from_list);

    Py_DECREF (from_list);

    if (module == NULL) {
        return -1;
    }

    _PyGObject_Type = (PyTypeObject *) PyObject_GetAttrString (module, "GObject");
    if (_PyGObject_Type == NULL) {
        retval = -1;
        goto out;
    }

    _PyGTypeWrapper_Type = (PyTypeObject *) PyObject_GetAttrString (module, "GType");
    if (_PyGTypeWrapper_Type == NULL) {
        retval = -1;
        goto out;
    }

    imported = TRUE;

out:
    Py_DECREF (module);

    return retval;
}

G_END_DECLS

#endif /* __PYGOBJECT_EXTERN_H__ */

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

#ifndef __PYGI_FOREIGN_API_H__
#define __PYGI_FOREIGN_API_H__

#include <girepository/girepository.h>
#include <pygobject.h>

typedef PyObject *(*PyGIArgOverrideToGIArgumentFunc) (
    PyObject *value, GIRegisteredTypeInfo *interface_info, GITransfer transfer,
    GIArgument *arg);
typedef PyObject *(*PyGIArgOverrideFromGIArgumentFunc) (
    GIRegisteredTypeInfo *interface_info, GITransfer transfer, gpointer data);
typedef PyObject *(*PyGIArgOverrideReleaseFunc) (GIBaseInfo *base_info,
                                                 gpointer struct_);


struct PyGI_API {
    void (*register_foreign_struct) (
        const char *namespace_, const char *name,
        PyGIArgOverrideToGIArgumentFunc to_func,
        PyGIArgOverrideFromGIArgumentFunc from_func,
        PyGIArgOverrideReleaseFunc release_func);
};


#ifndef _INSIDE_PYGOBJECT_

static struct PyGI_API *PyGI_API = NULL;

static int
_pygi_import (void)
{
    if (PyGI_API != NULL) {
        return 1;
    }
    PyGI_API = (struct PyGI_API *)PyCapsule_Import ("gi._API", FALSE);
    if (PyGI_API == NULL) {
        return -1;
    }

    return 0;
}


static inline PyObject *
pygi_register_foreign_struct (const char *namespace_, const char *name,
                              PyGIArgOverrideToGIArgumentFunc to_func,
                              PyGIArgOverrideFromGIArgumentFunc from_func,
                              PyGIArgOverrideReleaseFunc release_func)
{
    if (_pygi_import () < 0) {
        return NULL;
    }
    PyGI_API->register_foreign_struct (namespace_, name, to_func, from_func,
                                       release_func);
    Py_RETURN_NONE;
}

#endif /* _INSIDE_PYGOBJECT_ */

#endif /* __PYGI_FOREIGN_API_H__ */

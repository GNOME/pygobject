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

#ifndef __PYGI_FOREIGN_TYPES_H__
#define __PYGI_FOREIGN_TYPES_H__

#include <girepository/girepository.h>
#include <pygobject-types.h>

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

#endif /* __PYGI_FOREIGN_TYPES_H__ */

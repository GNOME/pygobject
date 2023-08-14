/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2009 Simon van der Linden <svdlinden@src.gnome.org>
 * Copyright (C) 2010 Tomeu Vizoso <tomeu.vizoso@collabora.co.uk>
 * Copyright (C) 2012 Bastian Winkler <buz@netbuz.org>
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

#ifndef __PYGI_FUNDAMENTAL_H__
#define __PYGI_FUNDAMENTAL_H__

#include <Python.h>
#include <girepository.h>

#include "pygobject-internal.h"
#include "pygpointer.h"
#include "pygi-type.h"

G_BEGIN_DECLS

extern PyTypeObject PyGIFundamental_Type;

typedef struct {
    PyGPointer base;
    GIObjectInfoRefFunction ref_func;
    GIObjectInfoUnrefFunction unref_func;
} PyGIFundamental;


PyObject* pygi_fundamental_new   (gpointer      pointer);

void      pygi_fundamental_ref   (PyGIFundamental *self);
void      pygi_fundamental_unref (PyGIFundamental *self);

int pygi_fundamental_register_types (PyObject *m);

#define pygi_check_fundamental(info_type,info) \
  ((info_type) == GI_INFO_TYPE_OBJECT && \
   g_object_info_get_fundamental ((GIObjectInfo *)(info)))

#endif /* __PYGI_FUNDAMENTAL_H__ */
